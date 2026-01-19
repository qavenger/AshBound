#include "Runtime/Core/Input.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/Platform/WindowsWindowHandle.h"

#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

#include <array>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
	DEFINE_LOG_CATEGORY(LogInput, Info)

	HWND g_windowHandle = nullptr;
	std::array<bool, 256> g_keyDown = {};
	std::array<bool, 256> g_keyPrev = {};
	std::array<bool, 256> g_keyPressed = {};
	std::array<float, 256> g_keyAnalog = {};
	std::array<bool, 256> g_keyAnalogOverride = {};
	Input::AnalogProvider g_analogProvider = nullptr;
	POINT g_mousePos = {};
	POINT g_mousePrev = {};
	POINT g_mouseDelta = {};

	struct Axis2DBinding
	{
		KeyCode positiveX = KeyCode::None;
		KeyCode negativeX = KeyCode::None;
		KeyCode positiveY = KeyCode::None;
		KeyCode negativeY = KeyCode::None;
		bool invertY = false;
		float deadzone = 0.1f;
	};

	struct Axis1DBinding
	{
		KeyCode positive = KeyCode::None;
		KeyCode negative = KeyCode::None;
		float deadzone = 0.05f;
	};

	Axis2DBinding g_leftStick;
	Axis2DBinding g_rightStick;
	Axis1DBinding g_leftTrigger;
	Axis1DBinding g_rightTrigger;
	std::array<KeyCode, static_cast<size_t>(GamepadButton::Count)> g_buttonBindings = {};
	GamepadState g_gamepadState = {};

	enum class ActionBindingType
	{
		Key,
		GamepadButton
	};

	struct ActionBinding
	{
		ActionBindingType type = ActionBindingType::Key;
		KeyCode keyCode = KeyCode::None;
		GamepadButton gamepadButton = GamepadButton::A;
	};

	struct ActionState
	{
		bool down = false;
		bool prev = false;
	};

	struct AxisBinding
	{
		KeyCode positiveKey = KeyCode::None;
		KeyCode negativeKey = KeyCode::None;
		float scale = 1.0f;
		float deadzone = 0.05f;
		bool invert = false;
		bool useGamepad1D = false;
		bool useGamepad2D = false;
		GamepadAxis1D gamepadAxis1D = GamepadAxis1D::LeftTrigger;
		GamepadAxis2D gamepadAxis2D = GamepadAxis2D::LeftStick;
		Axis2DComponent axis2DComponent = Axis2DComponent::X;
	};

	struct AxisState
	{
		float value = 0.0f;
		float prev = 0.0f;
	};

	struct ActionCallbackSet
	{
		std::vector<Input::ActionCallback> pressed;
		std::vector<Input::ActionCallback> released;
		std::vector<Input::ActionCallback> held;
	};

	std::unordered_map<std::wstring, std::vector<ActionBinding>> g_actionBindings;
	std::unordered_map<std::wstring, ActionCallbackSet> g_actionCallbacks;
	std::unordered_map<std::wstring, ActionState> g_actionStates;

	std::unordered_map<std::wstring, AxisBinding> g_axisBindings;
	std::unordered_map<std::wstring, std::vector<Input::AxisCallback>> g_axisCallbacks;
	std::unordered_map<std::wstring, AxisState> g_axisStates;

	enum class EnhancedMappingType
	{
		Key,
		MouseButton,
		GamepadButton,
		KeyAxis1D,
		KeyAxis2D,
		GamepadAxis1D,
		GamepadStick,
		MouseDelta
	};

	struct EnhancedMapping
	{
		EnhancedMappingType type = EnhancedMappingType::Key;
		KeyCode keyCode = KeyCode::None;
		GamepadButton gamepadButton = GamepadButton::A;
		GamepadAxis1D gamepadAxis1D = GamepadAxis1D::LeftTrigger;
		GamepadAxis2D gamepadAxis2D = GamepadAxis2D::LeftStick;
		Axis2DComponent axis2DComponent = Axis2DComponent::X;
		KeyCode positiveKey = KeyCode::None;
		KeyCode negativeKey = KeyCode::None;
		KeyCode upKey = KeyCode::None;
		KeyCode downKey = KeyCode::None;
		KeyCode leftKey = KeyCode::None;
		KeyCode rightKey = KeyCode::None;
	};

	enum class EnhancedModifierType
	{
		Deadzone,
		Scale,
		Invert,
		InvertX,
		InvertY,
		Clamp
	};

	struct EnhancedModifier
	{
		EnhancedModifierType type = EnhancedModifierType::Deadzone;
		float a = 0.0f;
		float b = 0.0f;
	};

	struct EnhancedTrigger
	{
		EnhancedTriggerType type = EnhancedTriggerType::Always;
		float threshold = 0.0f;
		float timeSeconds = 0.0f;
	};

	struct EnhancedActionDef
	{
		std::wstring name;
		EnhancedActionType type = EnhancedActionType::Bool;
		std::vector<EnhancedMapping> mappings;
		std::vector<EnhancedModifier> modifiers;
		std::vector<EnhancedTrigger> triggers;
		int priority = 0;
	};

	struct KeyCodeHash
	{
		size_t operator()(KeyCode key) const noexcept
		{
			return static_cast<size_t>(key);
		}
	};

	struct KeyTriggerState
	{
		double lastPressedTime = 0.0;
		double lastReleasedTime = 0.0;
		double lastPulseTime = 0.0;
		double lastClickTime = 0.0;
		bool clickPending = false;
	};

	struct EnhancedActionState
	{
		bool active = false;
		bool prevActive = false;
		double lastPressedTime = 0.0;
		double lastReleasedTime = 0.0;
		double lastPulseTime = 0.0;
		double lastClickTime = 0.0;
		bool clickPending = false;
		EnhancedTriggerType lastTriggeredType = EnhancedTriggerType::Count;
		KeyCode lastKeyCode = KeyCode::None;
		std::unordered_map<KeyCode, KeyTriggerState, KeyCodeHash> keyStates;
		float value1D = 0.0f;
		Axis2DValue value2D;
	};

	std::unordered_map<std::wstring, uint32_t> g_enhancedActionIds;
	std::vector<EnhancedActionDef> g_enhancedActions;
	std::vector<EnhancedActionState> g_enhancedActionStates;
	struct EnhancedCallbackSet
	{
		std::vector<Input::ActionCallback> callbacks[static_cast<size_t>(EnhancedTriggerType::Count)];
	};
	struct EnhancedTriggerEventCallbackSet
	{
		struct Entry
		{
			DelegateHandle id = DelegateHandle::Invalid();
			Input::ActionCallback callback;
		};
		std::vector<Entry> callbacks[static_cast<size_t>(EnhancedTriggerEvent::Count)];
	};
	struct EnhancedTriggerEventInfoCallbackSet
	{
		struct Entry
		{
			DelegateHandle id = DelegateHandle::Invalid();
			Input::TriggerEventCallback callback;
		};
		std::vector<Entry> callbacks[static_cast<size_t>(EnhancedTriggerEvent::Count)];
	};
	std::vector<EnhancedCallbackSet> g_enhancedCallbacks;
	std::vector<EnhancedTriggerEventCallbackSet> g_enhancedTriggerEventCallbacks;
	std::vector<EnhancedTriggerEventInfoCallbackSet> g_enhancedTriggerEventInfoCallbacks;
	std::vector<uint32_t> g_sortedEnhancedActionIds;
	Input::TimeProvider g_enhancedTimeProvider;
	std::unordered_map<std::wstring, KeyCode> g_keyAliasMap;
	std::wstring g_enhancedConfigPath;
 

	// XInput
	constexpr uint32_t kMaxXInputControllers = 4;
	std::array<bool, kMaxXInputControllers> g_xinputConnected = {};
	std::array<bool, kMaxXInputControllers> g_xinputPrevConnected = {};
	std::array<GamepadState, kMaxXInputControllers> g_xinputState = {};
	std::array<GamepadState, kMaxXInputControllers> g_xinputPrevState = {};
	bool g_preferXInput = true;
	InputSource g_activeSource = InputSource::Keyboard;
	constexpr float kXInputDeadzone = 0.24f;
	constexpr float kXInputTriggerDeadzone = 0.1f;

	KeyCode MouseButtonToKeyCode(MouseButton button)
	{
		switch (button)
		{
		case MouseButton::Left:
			return KeyCode::MouseLeft;
		case MouseButton::Right:
			return KeyCode::MouseRight;
		case MouseButton::Middle:
			return KeyCode::MouseMiddle;
		case MouseButton::X1:
			return KeyCode::MouseX1;
		case MouseButton::X2:
			return KeyCode::MouseX2;
		default:
			return KeyCode::MouseLeft;
		}
	}

	void UpdateMousePosition()
	{
		POINT pos = {};
		if (GetCursorPos(&pos) && g_windowHandle)
		{
			ScreenToClient(g_windowHandle, &pos);
		}
		g_mousePos = pos;
	}

float GetAnalogKey(KeyCode key)
	{
	const uint32_t index = ToVK(key);
	if (index >= g_keyAnalog.size())
		{
			return 0.0f;
		}
	return g_keyAnalog[index];
	}

	size_t TriggerIndex(EnhancedTriggerType trigger)
	{
		return static_cast<size_t>(trigger);
	}

	uint32_t GetOrCreateActionId(const std::wstring& name)
	{
		const auto iter = g_enhancedActionIds.find(name);
		if (iter != g_enhancedActionIds.end())
		{
			return iter->second;
		}

		const uint32_t id = static_cast<uint32_t>(g_enhancedActions.size());
		g_enhancedActionIds[name] = id;
		EnhancedActionDef def;
		def.name = name;
		g_enhancedActions.push_back(def);
		g_enhancedActionStates.push_back(EnhancedActionState{});
		g_enhancedCallbacks.push_back(EnhancedCallbackSet{});
		g_enhancedTriggerEventCallbacks.push_back(EnhancedTriggerEventCallbackSet{});
		g_enhancedTriggerEventInfoCallbacks.push_back(EnhancedTriggerEventInfoCallbackSet{});
		return id;
	}

	bool TryGetActionId(const std::wstring& name, uint32_t& outId)
	{
		const auto iter = g_enhancedActionIds.find(name);
		if (iter == g_enhancedActionIds.end())
		{
			return false;
		}
		outId = iter->second;
		return true;
	}

	float ApplyDeadzone(float value, float deadzone)
	{
		if (std::abs(value) < deadzone)
		{
			return 0.0f;
		}
		return std::clamp(value, -1.0f, 1.0f);
	}

	std::wstring Trim(const std::wstring& value)
	{
		const size_t first = value.find_first_not_of(L" \t\r\n");
		if (first == std::wstring::npos)
		{
			return L"";
		}
		const size_t last = value.find_last_not_of(L" \t\r\n");
		return value.substr(first, last - first + 1);
	}

	std::wstring ToLower(const std::wstring& value)
	{
		std::wstring result = value;
		for (wchar_t& ch : result)
		{
			ch = static_cast<wchar_t>(towlower(ch));
		}
		return result;
	}

	std::vector<std::wstring> Split(const std::wstring& value, wchar_t delimiter)
	{
		std::vector<std::wstring> parts;
		std::wstring current;
		for (wchar_t ch : value)
		{
			if (ch == delimiter)
			{
				parts.push_back(Trim(current));
				current.clear();
			}
			else
			{
				current.push_back(ch);
			}
		}
		parts.push_back(Trim(current));
		return parts;
	}

	bool TryParseFloat(const std::wstring& value, float& outValue)
	{
		wchar_t* endPtr = nullptr;
		outValue = static_cast<float>(wcstod(value.c_str(), &endPtr));
		return endPtr && endPtr != value.c_str();
	}

bool TryParseKeyCode(const std::wstring& token, KeyCode& outKey)
{
	const std::wstring value = ToLower(Trim(token));
	if (value.empty())
	{
		return false;
	}

	const auto aliasIter = g_keyAliasMap.find(value);
	if (aliasIter != g_keyAliasMap.end())
	{
		outKey = aliasIter->second;
		return true;
	}

	return TryParseKeyCodeName(value, outKey);
}

	bool TryParseGamepadButton(const std::wstring& token, GamepadButton& outButton)
	{
		const std::wstring value = ToLower(Trim(token));
		if (value == L"a")
		{
			outButton = GamepadButton::A;
			return true;
		}
		if (value == L"b")
		{
			outButton = GamepadButton::B;
			return true;
		}
		if (value == L"x")
		{
			outButton = GamepadButton::X;
			return true;
		}
		if (value == L"y")
		{
			outButton = GamepadButton::Y;
			return true;
		}
		if (value == L"lb")
		{
			outButton = GamepadButton::LB;
			return true;
		}
		if (value == L"rb")
		{
			outButton = GamepadButton::RB;
			return true;
		}
		if (value == L"back")
		{
			outButton = GamepadButton::Back;
			return true;
		}
		if (value == L"start")
		{
			outButton = GamepadButton::Start;
			return true;
		}
		if (value == L"leftstick")
		{
			outButton = GamepadButton::LeftStick;
			return true;
		}
		if (value == L"rightstick")
		{
			outButton = GamepadButton::RightStick;
			return true;
		}
		if (value == L"dpadup")
		{
			outButton = GamepadButton::DpadUp;
			return true;
		}
		if (value == L"dpaddown")
		{
			outButton = GamepadButton::DpadDown;
			return true;
		}
		if (value == L"dpadleft")
		{
			outButton = GamepadButton::DpadLeft;
			return true;
		}
		if (value == L"dpadright")
		{
			outButton = GamepadButton::DpadRight;
			return true;
		}
		return false;
	}

	bool TryParseGamepadAxis1D(const std::wstring& token, GamepadAxis1D& outAxis)
	{
		const std::wstring value = ToLower(Trim(token));
		if (value == L"lefttrigger")
		{
			outAxis = GamepadAxis1D::LeftTrigger;
			return true;
		}
		if (value == L"righttrigger")
		{
			outAxis = GamepadAxis1D::RightTrigger;
			return true;
		}
		return false;
	}

	bool TryParseGamepadStick(const std::wstring& token, GamepadAxis2D& outAxis)
	{
		const std::wstring value = ToLower(Trim(token));
		if (value == L"left")
		{
			outAxis = GamepadAxis2D::LeftStick;
			return true;
		}
		if (value == L"right")
		{
			outAxis = GamepadAxis2D::RightStick;
			return true;
		}
		return false;
	}

	bool TryParseActionType(const std::wstring& token, EnhancedActionType& outType)
	{
		const std::wstring value = ToLower(Trim(token));
		if (value == L"bool")
		{
			outType = EnhancedActionType::Bool;
			return true;
		}
		if (value == L"axis1d")
		{
			outType = EnhancedActionType::Axis1D;
			return true;
		}
		if (value == L"axis2d")
		{
			outType = EnhancedActionType::Axis2D;
			return true;
		}
		return false;
	}

	bool TryParseTriggerType(const std::wstring& token, EnhancedTriggerType& outType)
	{
		const std::wstring value = ToLower(Trim(token));
		if (value == L"always")
		{
			outType = EnhancedTriggerType::Always;
			return true;
		}
		if (value == L"pressed")
		{
			outType = EnhancedTriggerType::Pressed;
			return true;
		}
		if (value == L"released")
		{
			outType = EnhancedTriggerType::Released;
			return true;
		}
		if (value == L"hold")
		{
			outType = EnhancedTriggerType::Hold;
			return true;
		}
		if (value == L"tap")
		{
		outType = EnhancedTriggerType::Click;
		return true;
	}
	if (value == L"click")
	{
		outType = EnhancedTriggerType::Click;
		return true;
	}
	if (value == L"doubleclick")
	{
		outType = EnhancedTriggerType::DoubleClick;
			return true;
		}
	if (value == L"pulse")
	{
		outType = EnhancedTriggerType::Pulse;
		return true;
	}
		return false;
	}

	EnhancedModifier ParseModifierToken(const std::wstring& token)
	{
		EnhancedModifier modifier;
		std::wstring name = token;
		std::wstring params;
		const size_t openPos = token.find(L'(');
		const size_t closePos = token.find(L')');
		if (openPos != std::wstring::npos && closePos != std::wstring::npos && closePos > openPos)
		{
			name = token.substr(0, openPos);
			params = token.substr(openPos + 1, closePos - openPos - 1);
		}

		const std::wstring lname = ToLower(Trim(name));
		if (lname == L"deadzone")
		{
			modifier.type = EnhancedModifierType::Deadzone;
			TryParseFloat(Trim(params), modifier.a);
		}
		else if (lname == L"scale")
		{
			modifier.type = EnhancedModifierType::Scale;
			const auto parts = Split(params, L',');
			if (!parts.empty())
			{
				TryParseFloat(parts[0], modifier.a);
			}
			if (parts.size() > 1)
			{
				TryParseFloat(parts[1], modifier.b);
			}
		}
		else if (lname == L"invertx")
		{
			modifier.type = EnhancedModifierType::InvertX;
		}
		else if (lname == L"inverty")
		{
			modifier.type = EnhancedModifierType::InvertY;
		}
		else if (lname == L"invert")
		{
			modifier.type = EnhancedModifierType::Invert;
		}
		else if (lname == L"clamp")
		{
			modifier.type = EnhancedModifierType::Clamp;
			const auto parts = Split(params, L',');
			if (!parts.empty())
			{
				TryParseFloat(parts[0], modifier.a);
			}
			if (parts.size() > 1)
			{
				TryParseFloat(parts[1], modifier.b);
			}
		}
		return modifier;
	}

	EnhancedTrigger ParseTriggerToken(const std::wstring& token)
	{
		EnhancedTrigger trigger;
		std::wstring name = token;
		std::wstring params;
		const size_t openPos = token.find(L'(');
		const size_t closePos = token.find(L')');
		if (openPos != std::wstring::npos && closePos != std::wstring::npos && closePos > openPos)
		{
			name = token.substr(0, openPos);
			params = token.substr(openPos + 1, closePos - openPos - 1);
		}

		if (!TryParseTriggerType(name, trigger.type))
		{
			trigger.type = EnhancedTriggerType::Always;
		}

	if (trigger.type == EnhancedTriggerType::Hold ||
		trigger.type == EnhancedTriggerType::Click ||
		trigger.type == EnhancedTriggerType::DoubleClick ||
		trigger.type == EnhancedTriggerType::Pulse)
		{
			TryParseFloat(Trim(params), trigger.timeSeconds);
		}
		return trigger;
	}

	bool TryParseMapping(const std::wstring& token, EnhancedMapping& outMapping)
	{
		std::wstring typePart;
		std::wstring argsPart;
		const size_t colonPos = token.find(L':');
		if (colonPos == std::wstring::npos)
		{
			typePart = Trim(token);
		}
		else
		{
			typePart = Trim(token.substr(0, colonPos));
			argsPart = Trim(token.substr(colonPos + 1));
		}

		const std::wstring typeName = ToLower(typePart);
		if (typeName == L"key")
		{
			outMapping.type = EnhancedMappingType::Key;
			return TryParseKeyCode(argsPart, outMapping.keyCode);
		}
		if (typeName == L"mousebutton")
		{
			outMapping.type = EnhancedMappingType::MouseButton;
			const std::wstring keyName = L"mouse" + ToLower(argsPart);
			return TryParseKeyCode(keyName, outMapping.keyCode);
		}
		if (typeName == L"gamepadbutton")
		{
			outMapping.type = EnhancedMappingType::GamepadButton;
			return TryParseGamepadButton(argsPart, outMapping.gamepadButton);
		}
		if (typeName == L"keyaxis1d")
		{
			outMapping.type = EnhancedMappingType::KeyAxis1D;
			const auto parts = Split(argsPart, L',');
			if (parts.size() < 2)
			{
				return false;
			}
			return TryParseKeyCode(parts[0], outMapping.positiveKey) && TryParseKeyCode(parts[1], outMapping.negativeKey);
		}
		if (typeName == L"keyaxis2d")
		{
			outMapping.type = EnhancedMappingType::KeyAxis2D;
			const auto parts = Split(argsPart, L',');
			if (parts.size() < 4)
			{
				return false;
			}
			return TryParseKeyCode(parts[0], outMapping.upKey) &&
				TryParseKeyCode(parts[1], outMapping.downKey) &&
				TryParseKeyCode(parts[2], outMapping.leftKey) &&
				TryParseKeyCode(parts[3], outMapping.rightKey);
		}
		if (typeName == L"gamepadaxis1d")
		{
			outMapping.type = EnhancedMappingType::GamepadAxis1D;
			return TryParseGamepadAxis1D(argsPart, outMapping.gamepadAxis1D);
		}
		if (typeName == L"gamepadstick")
		{
			outMapping.type = EnhancedMappingType::GamepadStick;
			return TryParseGamepadStick(argsPart, outMapping.gamepadAxis2D);
		}
		if (typeName == L"mousedelta")
		{
			outMapping.type = EnhancedMappingType::MouseDelta;
			return true;
		}
		return false;
	}

	float SampleGamepadAxis1D(GamepadAxis1D axis)
	{
		return (axis == GamepadAxis1D::LeftTrigger) ? g_gamepadState.leftTrigger : g_gamepadState.rightTrigger;
	}

	float SampleGamepadAxis2D(GamepadAxis2D axis, Axis2DComponent component)
	{
		if (axis == GamepadAxis2D::LeftStick)
		{
			return (component == Axis2DComponent::X) ? g_gamepadState.leftX : g_gamepadState.leftY;
		}
		return (component == Axis2DComponent::X) ? g_gamepadState.rightX : g_gamepadState.rightY;
	}

	Axis2DValue ApplyModifiers2D(Axis2DValue value, const std::vector<EnhancedModifier>& modifiers)
	{
		for (const EnhancedModifier& modifier : modifiers)
		{
			switch (modifier.type)
			{
			case EnhancedModifierType::Deadzone:
			{
				const float dx = value.x;
				const float dy = value.y;
				const float len = std::sqrt(dx * dx + dy * dy);
				const float dz = modifier.a;
				if (len < dz)
				{
					value.x = 0.0f;
					value.y = 0.0f;
				}
				else if (len > 0.0f)
				{
					const float scale = (len - dz) / (1.0f - dz);
					value.x = (dx / len) * scale;
					value.y = (dy / len) * scale;
				}
				break;
			}
			case EnhancedModifierType::Scale:
				value.x *= modifier.a;
				value.y *= (modifier.b != 0.0f) ? modifier.b : modifier.a;
				break;
			case EnhancedModifierType::Invert:
				value.x = -value.x;
				value.y = -value.y;
				break;
			case EnhancedModifierType::InvertX:
				value.x = -value.x;
				break;
			case EnhancedModifierType::InvertY:
				value.y = -value.y;
				break;
			case EnhancedModifierType::Clamp:
				value.x = std::clamp(value.x, modifier.a, modifier.b);
				value.y = std::clamp(value.y, modifier.a, modifier.b);
				break;
			default:
				break;
			}
		}
		value.x = std::clamp(value.x, -1.0f, 1.0f);
		value.y = std::clamp(value.y, -1.0f, 1.0f);
		return value;
	}

	float ApplyModifiers1D(float value, const std::vector<EnhancedModifier>& modifiers)
	{
		for (const EnhancedModifier& modifier : modifiers)
		{
			switch (modifier.type)
			{
			case EnhancedModifierType::Deadzone:
				value = ApplyDeadzone(value, modifier.a);
				break;
			case EnhancedModifierType::Scale:
				value *= modifier.a;
				break;
			case EnhancedModifierType::Invert:
				value = -value;
				break;
			case EnhancedModifierType::Clamp:
				value = std::clamp(value, modifier.a, modifier.b);
				break;
			default:
				break;
			}
		}
		return std::clamp(value, -1.0f, 1.0f);
	}

	float NormalizeThumbstick(SHORT value, float deadzone)
	{
		float normalized = static_cast<float>(value) / 32767.0f;
		if (std::abs(normalized) < deadzone)
		{
			return 0.0f;
		}
		float sign = (normalized > 0.0f) ? 1.0f : -1.0f;
		return sign * (std::abs(normalized) - deadzone) / (1.0f - deadzone);
	}

	float NormalizeTrigger(BYTE value, float deadzone)
	{
		float normalized = static_cast<float>(value) / 255.0f;
		if (normalized < deadzone)
		{
			return 0.0f;
		}
		return (normalized - deadzone) / (1.0f - deadzone);
	}

	void UpdateXInputControllers()
	{
		g_xinputPrevConnected = g_xinputConnected;
		g_xinputPrevState = g_xinputState;

		for (uint32_t i = 0; i < kMaxXInputControllers; ++i)
		{
			XINPUT_STATE state = {};
			DWORD result = XInputGetState(i, &state);

			if (result == ERROR_SUCCESS)
			{
				if (!g_xinputPrevConnected[i])
				{
					LOG(LogInput, Info, L"XInput: Controller %u connected.", i);
				}
				g_xinputConnected[i] = true;

				GamepadState& gs = g_xinputState[i];
				gs.leftX = NormalizeThumbstick(state.Gamepad.sThumbLX, kXInputDeadzone);
				gs.leftY = NormalizeThumbstick(state.Gamepad.sThumbLY, kXInputDeadzone);
				gs.rightX = NormalizeThumbstick(state.Gamepad.sThumbRX, kXInputDeadzone);
				gs.rightY = NormalizeThumbstick(state.Gamepad.sThumbRY, kXInputDeadzone);
				gs.leftTrigger = NormalizeTrigger(state.Gamepad.bLeftTrigger, kXInputTriggerDeadzone);
				gs.rightTrigger = NormalizeTrigger(state.Gamepad.bRightTrigger, kXInputTriggerDeadzone);

				gs.buttons[static_cast<size_t>(GamepadButton::A)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::B)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::X)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::Y)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::LB)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::RB)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::Back)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::Start)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::LeftStick)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::RightStick)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::DpadUp)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::DpadDown)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::DpadLeft)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
				gs.buttons[static_cast<size_t>(GamepadButton::DpadRight)] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
			}
			else
			{
				if (g_xinputPrevConnected[i])
				{
					LOG(LogInput, Info, L"XInput: Controller %u disconnected.", i);
				}
				g_xinputConnected[i] = false;
				g_xinputState[i] = GamepadState{};
			}
		}
	}

	void UpdateGamepadState()
	{
		g_gamepadState.leftX = ApplyDeadzone(GetAnalogKey(g_leftStick.positiveX) - GetAnalogKey(g_leftStick.negativeX), g_leftStick.deadzone);
		g_gamepadState.leftY = ApplyDeadzone(GetAnalogKey(g_leftStick.positiveY) - GetAnalogKey(g_leftStick.negativeY), g_leftStick.deadzone);
		if (g_leftStick.invertY)
		{
			g_gamepadState.leftY = -g_gamepadState.leftY;
		}

		g_gamepadState.rightX = ApplyDeadzone(GetAnalogKey(g_rightStick.positiveX) - GetAnalogKey(g_rightStick.negativeX), g_rightStick.deadzone);
		g_gamepadState.rightY = ApplyDeadzone(GetAnalogKey(g_rightStick.positiveY) - GetAnalogKey(g_rightStick.negativeY), g_rightStick.deadzone);
		if (g_rightStick.invertY)
		{
			g_gamepadState.rightY = -g_gamepadState.rightY;
		}

		g_gamepadState.leftTrigger = ApplyDeadzone(GetAnalogKey(g_leftTrigger.positive) - GetAnalogKey(g_leftTrigger.negative), g_leftTrigger.deadzone);
		g_gamepadState.leftTrigger = std::clamp(g_gamepadState.leftTrigger, 0.0f, 1.0f);
		g_gamepadState.rightTrigger = ApplyDeadzone(GetAnalogKey(g_rightTrigger.positive) - GetAnalogKey(g_rightTrigger.negative), g_rightTrigger.deadzone);
		g_gamepadState.rightTrigger = std::clamp(g_gamepadState.rightTrigger, 0.0f, 1.0f);

		for (size_t i = 0; i < g_buttonBindings.size(); ++i)
		{
			const KeyCode key = g_buttonBindings[i];
			g_gamepadState.buttons[i] = GetAnalogKey(key) > 0.5f;
		}
	}

	void DispatchActionEvents()
	{
		for (auto& entry : g_actionBindings)
		{
			const std::wstring& name = entry.first;
			const std::vector<ActionBinding>& bindings = entry.second;

			bool isDown = false;
			for (const ActionBinding& binding : bindings)
			{
				if (binding.type == ActionBindingType::Key)
				{
					const uint32_t keyIndex = ToVK(binding.keyCode);
					if (keyIndex < g_keyDown.size() && g_keyDown[keyIndex])
					{
						isDown = true;
						break;
					}
				}
				else
				{
					const size_t index = static_cast<size_t>(binding.gamepadButton);
					if (index < static_cast<size_t>(GamepadButton::Count) && g_gamepadState.buttons[index])
					{
						isDown = true;
						break;
					}
				}
			}

			ActionState& state = g_actionStates[name];
			state.prev = state.down;
			state.down = isDown;

			const bool pressed = state.down && !state.prev;
			const bool released = !state.down && state.prev;
			const bool held = state.down;

			const auto callbackIter = g_actionCallbacks.find(name);
			if (callbackIter == g_actionCallbacks.end())
			{
				continue;
			}

			const ActionCallbackSet& callbacks = callbackIter->second;
			if (pressed)
			{
				for (const auto& callback : callbacks.pressed)
				{
					if (callback)
					{
						callback();
					}
				}
			}
			if (released)
			{
				for (const auto& callback : callbacks.released)
				{
					if (callback)
					{
						callback();
					}
				}
			}
			if (held)
			{
				for (const auto& callback : callbacks.held)
				{
					if (callback)
					{
						callback();
					}
				}
			}
		}
	}

	void DispatchAxisEvents()
	{
		for (const auto& entry : g_axisBindings)
		{
			const std::wstring& name = entry.first;
			const AxisBinding& binding = entry.second;

			float value = 0.0f;
			if (binding.positiveKey != KeyCode::None || binding.negativeKey != KeyCode::None)
			{
				value += GetAnalogKey(binding.positiveKey) - GetAnalogKey(binding.negativeKey);
			}

			if (binding.useGamepad1D)
			{
				value += SampleGamepadAxis1D(binding.gamepadAxis1D);
			}

			if (binding.useGamepad2D)
			{
				value += SampleGamepadAxis2D(binding.gamepadAxis2D, binding.axis2DComponent);
			}

			if (binding.invert)
			{
				value = -value;
			}

			value = ApplyDeadzone(value * binding.scale, binding.deadzone);

			AxisState& state = g_axisStates[name];
			state.prev = state.value;
			state.value = std::clamp(value, -1.0f, 1.0f);

			const auto callbackIter = g_axisCallbacks.find(name);
			if (callbackIter == g_axisCallbacks.end())
			{
				continue;
			}

			const float delta = std::abs(state.value - state.prev);
			if (delta < 0.001f)
			{
				continue;
			}

			for (const auto& callback : callbackIter->second)
			{
				if (callback)
				{
					callback(state.value);
				}
			}
		}
	}

	bool EvaluateTrigger(const EnhancedTrigger& trigger, const EnhancedActionState& state, bool isActive, bool wasActive, double nowSeconds)
	{
		switch (trigger.type)
		{
		case EnhancedTriggerType::Always:
			return isActive;
		case EnhancedTriggerType::Pressed:
			return isActive && !wasActive;
		case EnhancedTriggerType::Released:
			return !isActive && wasActive;
		case EnhancedTriggerType::Hold:
		{
			if (!isActive)
			{
				return false;
			}
			const float holdSeconds = trigger.timeSeconds;
			if (holdSeconds <= 0.0f)
			{
				return true;
			}
		const double elapsedSeconds = nowSeconds - state.lastPressedTime;
		return elapsedSeconds >= holdSeconds;
		}
		case EnhancedTriggerType::Click:
		{
			if (isActive || !wasActive)
			{
				return false;
			}
			const float clickSeconds = trigger.timeSeconds;
			const double elapsedSeconds = nowSeconds - state.lastPressedTime;
			return clickSeconds > 0.0f && elapsedSeconds <= clickSeconds;
		}
		case EnhancedTriggerType::DoubleClick:
		{
			return false;
		}
		case EnhancedTriggerType::Pulse:
		{
			if (!isActive)
			{
				return false;
			}
			const float pulseSeconds = trigger.timeSeconds;
			if (pulseSeconds <= 0.0f)
			{
				return false;
			}
			const double elapsed = nowSeconds - state.lastPulseTime;
			return elapsed >= pulseSeconds;
		}
		default:
			break;
		}
		return false;
	}

	void UpdateEnhancedInput()
	{
		const double nowSeconds = g_enhancedTimeProvider ? g_enhancedTimeProvider() : (GetTickCount64() / 1000.0);

		if (g_sortedEnhancedActionIds.empty() && !g_enhancedActions.empty())
		{
			g_sortedEnhancedActionIds.reserve(g_enhancedActions.size());
			for (uint32_t actionId = 0; actionId < g_enhancedActions.size(); ++actionId)
			{
				g_sortedEnhancedActionIds.push_back(actionId);
			}
		}

		for (const uint32_t actionId : g_sortedEnhancedActionIds)
		{
				if (actionId >= g_enhancedActions.size())
				{
					continue;
				}

				const EnhancedActionDef& def = g_enhancedActions[actionId];
				EnhancedActionState& state = g_enhancedActionStates[actionId];
				state.prevActive = state.active;
				state.lastKeyCode = KeyCode::None;

				float value1D = 0.0f;
				Axis2DValue value2D;
				std::vector<KeyCode> mappingKeys;

				for (const EnhancedMapping& mapping : def.mappings)
				{
					switch (mapping.type)
					{
					case EnhancedMappingType::Key:
					case EnhancedMappingType::MouseButton:
					{
						const uint32_t keyIndex = ToVK(mapping.keyCode);
						if (mapping.keyCode != KeyCode::None &&
							std::find(mappingKeys.begin(), mappingKeys.end(), mapping.keyCode) == mappingKeys.end())
						{
							mappingKeys.push_back(mapping.keyCode);
						}
						if (keyIndex < g_keyDown.size() && g_keyDown[keyIndex])
						{
							value1D = 1.0f;
							state.lastKeyCode = mapping.keyCode;
						}
					}
						break;
					case EnhancedMappingType::GamepadButton:
					{
						const size_t index = static_cast<size_t>(mapping.gamepadButton);
						if (index < static_cast<size_t>(GamepadButton::Count) && g_gamepadState.buttons[index])
						{
							value1D = 1.0f;
						}
						break;
					}
					case EnhancedMappingType::KeyAxis1D:
				{
					const float positive = GetAnalogKey(mapping.positiveKey);
					const float negative = GetAnalogKey(mapping.negativeKey);
					value1D += positive - negative;
					if (positive > 0.5f)
					{
						state.lastKeyCode = mapping.positiveKey;
					}
					else if (negative > 0.5f)
					{
						state.lastKeyCode = mapping.negativeKey;
					}
						break;
				}
					case EnhancedMappingType::GamepadAxis1D:
						value1D += SampleGamepadAxis1D(mapping.gamepadAxis1D);
						break;
					case EnhancedMappingType::KeyAxis2D:
				{
					const float right = GetAnalogKey(mapping.rightKey);
					const float left = GetAnalogKey(mapping.leftKey);
					const float up = GetAnalogKey(mapping.upKey);
					const float down = GetAnalogKey(mapping.downKey);
					value2D.x += right - left;
					value2D.y += up - down;
					if (up > 0.5f)
					{
						state.lastKeyCode = mapping.upKey;
					}
					else if (down > 0.5f)
					{
						state.lastKeyCode = mapping.downKey;
					}
					else if (right > 0.5f)
					{
						state.lastKeyCode = mapping.rightKey;
					}
					else if (left > 0.5f)
					{
						state.lastKeyCode = mapping.leftKey;
					}
						break;
				}
					case EnhancedMappingType::GamepadStick:
						value2D.x += SampleGamepadAxis2D(mapping.gamepadAxis2D, Axis2DComponent::X);
						value2D.y += SampleGamepadAxis2D(mapping.gamepadAxis2D, Axis2DComponent::Y);
						break;
					case EnhancedMappingType::MouseDelta:
						value2D.x += static_cast<float>(g_mouseDelta.x);
						value2D.y += static_cast<float>(g_mouseDelta.y);
						break;
					default:
						break;
					}
				}

				if (def.type == EnhancedActionType::Axis2D)
				{
					value2D = ApplyModifiers2D(value2D, def.modifiers);
					state.value2D = value2D;
					state.value1D = 0.0f;
					state.active = (std::abs(value2D.x) > 0.001f || std::abs(value2D.y) > 0.001f);
				}
				else
				{
					value1D = ApplyModifiers1D(value1D, def.modifiers);
					state.value1D = value1D;
					state.value2D = Axis2DValue{};
					state.active = (def.type == EnhancedActionType::Bool) ? (value1D > 0.5f) : (std::abs(value1D) > 0.001f);
				}

				if (state.active && !state.prevActive)
				{
					state.lastPressedTime = nowSeconds;
					state.lastPulseTime = nowSeconds;
				}
				if (!state.active && state.prevActive)
				{
					state.lastReleasedTime = nowSeconds;
				}

				double clickSeconds = 0.0;
				double doubleClickSeconds = 0.0;
				double holdSeconds = 0.0;
				double pulseSeconds = 0.0;
				bool wantsPressed = false;
				bool wantsReleased = false;
				bool wantsHold = false;
				bool wantsClick = false;
				bool wantsDoubleClick = false;
				bool wantsPulse = false;
				for (const EnhancedTrigger& trigger : def.triggers)
				{
					if (trigger.type == EnhancedTriggerType::Click)
					{
						clickSeconds = trigger.timeSeconds;
						wantsClick = true;
					}
					else if (trigger.type == EnhancedTriggerType::DoubleClick)
					{
						doubleClickSeconds = trigger.timeSeconds;
						wantsDoubleClick = true;
					}
					else if (trigger.type == EnhancedTriggerType::Hold)
					{
						holdSeconds = trigger.timeSeconds;
						wantsHold = true;
					}
					else if (trigger.type == EnhancedTriggerType::Pressed)
					{
						wantsPressed = true;
					}
					else if (trigger.type == EnhancedTriggerType::Released)
					{
						wantsReleased = true;
					}
					else if (trigger.type == EnhancedTriggerType::Pulse)
					{
						pulseSeconds = trigger.timeSeconds;
						wantsPulse = true;
					}
				}

				std::array<bool, static_cast<size_t>(EnhancedTriggerType::Count)> handledKeyTriggers = {};
				std::vector<std::pair<KeyCode, EnhancedTriggerType>> keyTriggerEvents;

				const bool hasKeyMappings = !mappingKeys.empty();
				const auto fireKeyTrigger = [&](EnhancedTriggerType type, KeyCode key)
				{
					const size_t triggerIndex = TriggerIndex(type);
					if (triggerIndex >= static_cast<size_t>(EnhancedTriggerType::Count))
					{
						return;
					}
					if (actionId < g_enhancedCallbacks.size())
					{
						for (const auto& callback : g_enhancedCallbacks[actionId].callbacks[triggerIndex])
						{
							if (callback)
							{
								callback();
							}
						}
					}
					handledKeyTriggers[triggerIndex] = true;
					state.lastTriggeredType = type;
					state.lastKeyCode = key;
					keyTriggerEvents.emplace_back(key, type);
				};

				if (hasKeyMappings)
				{
					for (const KeyCode key : mappingKeys)
					{
						const uint32_t keyIndex = ToVK(key);
						const bool isDown = keyIndex < g_keyDown.size() && g_keyDown[keyIndex];
						const bool wasDown = keyIndex < g_keyPrev.size() && g_keyPrev[keyIndex];
						const bool pressed = keyIndex < g_keyPressed.size() && g_keyPressed[keyIndex];
						const bool released = wasDown && !isDown;

						KeyTriggerState& keyState = state.keyStates[key];
						if (isDown && !wasDown)
						{
							keyState.lastPressedTime = nowSeconds;
							keyState.lastPulseTime = nowSeconds;
						}
						if (!isDown && wasDown)
						{
							keyState.lastReleasedTime = nowSeconds;
						}

						if (wantsPressed && (pressed || (isDown && !wasDown)))
						{
							fireKeyTrigger(EnhancedTriggerType::Pressed, key);
						}
						if (wantsReleased && released)
						{
							fireKeyTrigger(EnhancedTriggerType::Released, key);
						}
						if (wantsHold && isDown)
						{
							if (holdSeconds <= 0.0 || (nowSeconds - keyState.lastPressedTime) >= holdSeconds)
							{
								fireKeyTrigger(EnhancedTriggerType::Hold, key);
							}
						}
						if (wantsPulse && isDown && pulseSeconds > 0.0)
						{
							if ((nowSeconds - keyState.lastPulseTime) >= pulseSeconds)
							{
								fireKeyTrigger(EnhancedTriggerType::Pulse, key);
								keyState.lastPulseTime = nowSeconds;
							}
						}

						if (!isDown && wasDown)
						{
							const double heldDuration = nowSeconds - keyState.lastPressedTime;
							const bool qualifiesClick = (clickSeconds > 0.0) ? (heldDuration <= clickSeconds) : true;
							const bool hasDouble = (doubleClickSeconds > 0.0);

							if (hasDouble)
							{
								if (keyState.clickPending &&
									(nowSeconds - keyState.lastClickTime) <= doubleClickSeconds &&
									qualifiesClick)
								{
									fireKeyTrigger(EnhancedTriggerType::DoubleClick, key);
									keyState.clickPending = false;
								}
								else if (qualifiesClick)
								{
									keyState.clickPending = true;
									keyState.lastClickTime = nowSeconds;
								}
							}
							else if (qualifiesClick && clickSeconds > 0.0)
							{
								fireKeyTrigger(EnhancedTriggerType::Click, key);
							}

							if (holdSeconds > 0.0 && heldDuration < holdSeconds)
							{
								// Keep action-level cancel tracking intact.
							}
						}

						if (keyState.clickPending && doubleClickSeconds > 0.0 &&
							(nowSeconds - keyState.lastClickTime) > doubleClickSeconds)
						{
							if (clickSeconds > 0.0)
							{
								fireKeyTrigger(EnhancedTriggerType::Click, key);
							}
							keyState.clickPending = false;
						}
					}
				}

				bool didTrigger = false;
				bool didCancel = false;

				if (!hasKeyMappings && !state.active && state.prevActive)
				{
					const double heldDuration = nowSeconds - state.lastPressedTime;
					const bool qualifiesClick = (clickSeconds > 0.0) ? (heldDuration <= clickSeconds) : true;
					const bool hasDouble = (doubleClickSeconds > 0.0);

					if (hasDouble)
					{
						if (state.clickPending &&
							(nowSeconds - state.lastClickTime) <= doubleClickSeconds &&
							qualifiesClick)
						{
							const size_t triggerIndex = TriggerIndex(EnhancedTriggerType::DoubleClick);
							if (triggerIndex < static_cast<size_t>(EnhancedTriggerType::Count))
							{
								for (const auto& callback : g_enhancedCallbacks[actionId].callbacks[triggerIndex])
								{
									if (callback)
									{
										callback();
									}
								}
							}
							didTrigger = true;
							state.lastTriggeredType = EnhancedTriggerType::DoubleClick;
							state.clickPending = false;
						}
						else if (qualifiesClick)
						{
							state.clickPending = true;
							state.lastClickTime = nowSeconds;
						}
					}
					else if (qualifiesClick && clickSeconds > 0.0)
					{
						const size_t triggerIndex = TriggerIndex(EnhancedTriggerType::Click);
						if (triggerIndex < static_cast<size_t>(EnhancedTriggerType::Count))
						{
							for (const auto& callback : g_enhancedCallbacks[actionId].callbacks[triggerIndex])
							{
								if (callback)
								{
									callback();
								}
							}
						}
						didTrigger = true;
						state.lastTriggeredType = EnhancedTriggerType::Click;
					}

					if (holdSeconds > 0.0 && heldDuration < holdSeconds)
					{
						didCancel = true;
					}
				}

				if (!hasKeyMappings && state.clickPending && doubleClickSeconds > 0.0 &&
					(nowSeconds - state.lastClickTime) > doubleClickSeconds)
				{
					if (clickSeconds > 0.0)
					{
						const size_t triggerIndex = TriggerIndex(EnhancedTriggerType::Click);
						if (triggerIndex < static_cast<size_t>(EnhancedTriggerType::Count))
						{
							for (const auto& callback : g_enhancedCallbacks[actionId].callbacks[triggerIndex])
							{
								if (callback)
								{
									callback();
								}
							}
						}
						didTrigger = true;
						state.lastTriggeredType = EnhancedTriggerType::Click;
					}
					state.clickPending = false;
				}

				if (actionId >= g_enhancedCallbacks.size())
				{
					continue;
				}

				const auto& callbackSet = g_enhancedCallbacks[actionId];
				const std::vector<EnhancedTrigger>& triggers = def.triggers;
				for (const EnhancedTrigger& trigger : triggers)
				{
					if (trigger.type == EnhancedTriggerType::Click ||
						trigger.type == EnhancedTriggerType::DoubleClick)
					{
						continue;
					}
					const size_t triggerIndex = TriggerIndex(trigger.type);
					if (triggerIndex < static_cast<size_t>(EnhancedTriggerType::Count) &&
						hasKeyMappings && handledKeyTriggers[triggerIndex])
					{
						continue;
					}
					if (!EvaluateTrigger(trigger, state, state.active, state.prevActive, nowSeconds))
					{
						continue;
					}

					if (triggerIndex >= static_cast<size_t>(EnhancedTriggerType::Count))
					{
						continue;
					}
					for (const auto& callback : callbackSet.callbacks[triggerIndex])
					{
						if (callback)
						{
							callback();
						}
					}
					didTrigger = true;
					state.lastTriggeredType = trigger.type;

					if (trigger.type == EnhancedTriggerType::Pulse)
					{
						state.lastPulseTime = nowSeconds;
					}
				}

				if (actionId < g_enhancedTriggerEventCallbacks.size())
				{
					const auto& eventCallbacks = g_enhancedTriggerEventCallbacks[actionId];
					const auto emitEvent = [&](EnhancedTriggerEvent evt)
					{
						const size_t evtIndex = static_cast<size_t>(evt);
						if (evtIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
						{
							return;
						}
						for (const auto& entry : eventCallbacks.callbacks[evtIndex])
						{
							if (entry.callback)
							{
								entry.callback();
							}
						}
					};

					const bool didTriggerOverall = didTrigger || !keyTriggerEvents.empty();

					const auto emitEventInfo = [&](EnhancedTriggerEvent evt)
					{
						if (actionId >= g_enhancedTriggerEventInfoCallbacks.size())
						{
							return;
						}
						const size_t evtIndex = static_cast<size_t>(evt);
						if (evtIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
						{
							return;
						}

						EnhancedTriggerEventInfo info;
						info.actionName = def.name;
						info.actionType = def.type;
						info.triggerType = state.lastTriggeredType;
						info.triggerEvent = evt;
					info.keyCode = state.lastKeyCode;
						info.isActive = state.active;
						info.wasActive = state.prevActive;
						info.triggered = didTriggerOverall;
						info.canceled = didCancel;
						info.value1D = state.value1D;
						info.value2D = state.value2D;
						double held = nowSeconds - state.lastPressedTime;
						const auto keyStateIter = state.keyStates.find(info.keyCode);
						if (keyStateIter != state.keyStates.end())
						{
							held = nowSeconds - keyStateIter->second.lastPressedTime;
						}
						info.heldSeconds = state.active && held > 0.0 ? held : 0.0;
						info.timeSeconds = nowSeconds;

						const auto& infoCallbacks = g_enhancedTriggerEventInfoCallbacks[actionId];
						for (const auto& entry : infoCallbacks.callbacks[evtIndex])
						{
							if (entry.callback)
							{
								entry.callback(info);
							}
						}
					};

					if (!keyTriggerEvents.empty())
					{
						for (const auto& entry : keyTriggerEvents)
						{
							state.lastKeyCode = entry.first;
							state.lastTriggeredType = entry.second;
							emitEvent(EnhancedTriggerEvent::Triggered);
							emitEventInfo(EnhancedTriggerEvent::Triggered);
						}
					}

					if (state.active && !state.prevActive)
					{
						emitEvent(EnhancedTriggerEvent::Started);
						emitEventInfo(EnhancedTriggerEvent::Started);
					}
					if (state.active)
					{
						emitEvent(EnhancedTriggerEvent::Ongoing);
						emitEventInfo(EnhancedTriggerEvent::Ongoing);
					}
					if (didTrigger)
					{
						emitEvent(EnhancedTriggerEvent::Triggered);
						emitEventInfo(EnhancedTriggerEvent::Triggered);
					}
					if (!state.active && state.prevActive)
					{
						if (didCancel)
						{
							emitEvent(EnhancedTriggerEvent::Canceled);
							emitEventInfo(EnhancedTriggerEvent::Canceled);
						}
						else
						{
							emitEvent(EnhancedTriggerEvent::Completed);
							emitEventInfo(EnhancedTriggerEvent::Completed);
						}
					}
				}
			}
		}
	

	void SetupDefaultBindings()
	{
		g_leftStick = { KeyCode::D, KeyCode::A, KeyCode::W, KeyCode::S, true, 0.1f };
		g_rightStick = { KeyCode::Right, KeyCode::Left, KeyCode::Up, KeyCode::Down, true, 0.1f };
		g_leftTrigger = { KeyCode::Q, KeyCode::None, 0.05f };
		g_rightTrigger = { KeyCode::E, KeyCode::None, 0.05f };

		g_buttonBindings.fill(KeyCode::None);
		g_buttonBindings[static_cast<size_t>(GamepadButton::A)] = KeyCode::Space;
		g_buttonBindings[static_cast<size_t>(GamepadButton::B)] = KeyCode::LeftControl;
		g_buttonBindings[static_cast<size_t>(GamepadButton::X)] = KeyCode::R;
		g_buttonBindings[static_cast<size_t>(GamepadButton::Y)] = KeyCode::F;
		g_buttonBindings[static_cast<size_t>(GamepadButton::LB)] = KeyCode::LeftShift;
		g_buttonBindings[static_cast<size_t>(GamepadButton::RB)] = KeyCode::RightShift;
		g_buttonBindings[static_cast<size_t>(GamepadButton::Back)] = KeyCode::Tab;
		g_buttonBindings[static_cast<size_t>(GamepadButton::Start)] = KeyCode::Enter;
		g_buttonBindings[static_cast<size_t>(GamepadButton::LeftStick)] = KeyCode::LeftAlt;
		g_buttonBindings[static_cast<size_t>(GamepadButton::RightStick)] = KeyCode::RightAlt;
		g_buttonBindings[static_cast<size_t>(GamepadButton::DpadUp)] = KeyCode::Up;
		g_buttonBindings[static_cast<size_t>(GamepadButton::DpadDown)] = KeyCode::Down;
		g_buttonBindings[static_cast<size_t>(GamepadButton::DpadLeft)] = KeyCode::Left;
		g_buttonBindings[static_cast<size_t>(GamepadButton::DpadRight)] = KeyCode::Right;
	}
}

void Input::Init(PlatformWindowHandle windowHandle)
{
	g_windowHandle = WindowsWindowHandle::FromPlatformHandle(windowHandle).handle;
	g_keyDown.fill(false);
	g_keyPrev.fill(false);
	g_keyPressed.fill(false);
	g_keyAnalog.fill(0.0f);
	g_keyAnalogOverride.fill(false);
	UpdateMousePosition();
	g_mousePrev = g_mousePos;
	g_mouseDelta = { 0, 0 };
	SetupDefaultBindings();

	// XInput initialization
	g_xinputConnected.fill(false);
	g_xinputPrevConnected.fill(false);
	for (auto& state : g_xinputState)
	{
		state = GamepadState{};
	}
	g_preferXInput = true;
	g_activeSource = InputSource::Keyboard;

	// Detect connected XInput controllers
	UpdateXInputControllers();
	for (uint32_t i = 0; i < kMaxXInputControllers; ++i)
	{
		if (g_xinputConnected[i])
		{
			g_activeSource = InputSource::XInput;
			break;
		}
	}

	LOG(LogInput, Info, L"Input initialized. Active source: %s",
		g_activeSource == InputSource::XInput ? L"XInput" : L"Keyboard");
}

void Input::Update()
{
	g_keyPrev = g_keyDown;
	for (uint32_t key = 0; key < g_keyDown.size(); ++key)
	{
		const short state = GetAsyncKeyState(static_cast<int>(key));
		const bool downNow = (state & 0x8000) != 0;
		const bool pressedSinceLast = (state & 0x0001) != 0;
		g_keyDown[key] = downNow || pressedSinceLast;
		g_keyPressed[key] = pressedSinceLast;
	}

	if (g_analogProvider)
	{
		for (uint32_t key = 0; key < g_keyAnalog.size(); ++key)
		{
			float analogValue = 0.0f;
			if (g_analogProvider(ToKeyCode(key), analogValue))
			{
				g_keyAnalog[key] = std::clamp(analogValue, 0.0f, 1.0f);
				g_keyAnalogOverride[key] = true;
			}
		}
	}

	for (uint32_t key = 0; key < g_keyAnalog.size(); ++key)
	{
		if (!g_keyAnalogOverride[key])
		{
			g_keyAnalog[key] = g_keyDown[key] ? 1.0f : 0.0f;
		}
	}
	g_keyAnalogOverride.fill(false);

	g_mousePrev = g_mousePos;
	UpdateMousePosition();
	g_mouseDelta.x = g_mousePos.x - g_mousePrev.x;
	g_mouseDelta.y = g_mousePos.y - g_mousePrev.y;

	// Update XInput
	UpdateXInputControllers();

	// Update gamepad state (current source)
	UpdateGamepadState();

	// Prefer XInput state when available
	if (g_preferXInput)
	{
		for (uint32_t i = 0; i < kMaxXInputControllers; ++i)
		{
			if (g_xinputConnected[i])
			{
				g_gamepadState = g_xinputState[i];
				if (g_activeSource != InputSource::XInput)
				{
					g_activeSource = InputSource::XInput;
					LOG(LogInput, Info, L"Input source switched to XInput.");
				}
				break;
			}
		}
	}

	// No XInput controller, fall back to keyboard
	if (g_activeSource != InputSource::Keyboard)
	{
		g_activeSource = InputSource::Keyboard;
		LOG(LogInput, Info, L"Input source switched to Keyboard.");
	}

	DispatchActionEvents();
	DispatchAxisEvents();
	UpdateEnhancedInput();
}

bool Input::IsKeyDown(KeyCode keyCode)
{
	const uint32_t index = ToVK(keyCode);
	if (index >= g_keyDown.size())
	{
		return false;
	}
	return g_keyDown[index];
}

bool Input::WasKeyPressed(KeyCode keyCode)
{
	const uint32_t index = ToVK(keyCode);
	if (index >= g_keyDown.size())
	{
		return false;
	}
	return g_keyPressed[index] || (g_keyDown[index] && !g_keyPrev[index]);
}

bool Input::WasKeyReleased(KeyCode keyCode)
{
	const uint32_t index = ToVK(keyCode);
	if (index >= g_keyDown.size())
	{
		return false;
	}
	return !g_keyDown[index] && g_keyPrev[index];
}

bool Input::IsMouseButtonDown(MouseButton button)
{
	return IsKeyDown(MouseButtonToKeyCode(button));
}

bool Input::WasMouseButtonPressed(MouseButton button)
{
	return WasKeyPressed(MouseButtonToKeyCode(button));
}

bool Input::WasMouseButtonReleased(MouseButton button)
{
	return WasKeyReleased(MouseButtonToKeyCode(button));
}

POINT Input::GetMousePosition()
{
	return g_mousePos;
}

POINT Input::GetMouseDelta()
{
	return g_mouseDelta;
}

void Input::SetKeyAnalogValue(KeyCode keyCode, float value)
{
	const uint32_t index = ToVK(keyCode);
	if (index >= g_keyAnalog.size())
	{
		return;
	}
	g_keyAnalog[index] = std::clamp(value, 0.0f, 1.0f);
	g_keyAnalogOverride[index] = true;
}

float Input::GetKeyAnalogValue(KeyCode keyCode)
{
	const uint32_t index = ToVK(keyCode);
	if (index >= g_keyAnalog.size())
	{
		return 0.0f;
	}
	return g_keyAnalog[index];
}

void Input::SetAnalogProvider(AnalogProvider provider)
{
	g_analogProvider = provider;
}

void Input::Shutdown()
{
	g_windowHandle = nullptr;
	g_keyDown.fill(false);
	g_keyPrev.fill(false);
	g_keyPressed.fill(false);
	g_keyAnalog.fill(0.0f);
	g_keyAnalogOverride.fill(false);
}

void Input::BindGamepadButton(GamepadButton button, KeyCode keyCode)
{
	const size_t index = static_cast<size_t>(button);
	if (index >= g_buttonBindings.size())
	{
		return;
	}
	g_buttonBindings[index] = keyCode;
}

void Input::BindGamepadAxis2D(GamepadAxis2D axis,
	KeyCode positiveXKey,
	KeyCode negativeXKey,
	KeyCode positiveYKey,
	KeyCode negativeYKey,
	bool invertY)
{
	Axis2DBinding* binding = (axis == GamepadAxis2D::LeftStick) ? &g_leftStick : &g_rightStick;
	binding->positiveX = positiveXKey;
	binding->negativeX = negativeXKey;
	binding->positiveY = positiveYKey;
	binding->negativeY = negativeYKey;
	binding->invertY = invertY;
}

void Input::BindGamepadAxis1D(GamepadAxis1D axis, KeyCode positiveKey, KeyCode negativeKey)
{
	Axis1DBinding* binding = (axis == GamepadAxis1D::LeftTrigger) ? &g_leftTrigger : &g_rightTrigger;
	binding->positive = positiveKey;
	binding->negative = negativeKey;
}

GamepadState Input::GetGamepadState()
{
	return g_gamepadState;
}

bool Input::IsXInputConnected(uint32_t controllerIndex)
{
	if (controllerIndex >= kMaxXInputControllers)
	{
		return false;
	}
	return g_xinputConnected[controllerIndex];
}

GamepadState Input::GetXInputState(uint32_t controllerIndex)
{
	if (controllerIndex >= kMaxXInputControllers)
	{
		return GamepadState{};
	}
	return g_xinputState[controllerIndex];
}

InputSource Input::GetActiveInputSource()
{
	return g_activeSource;
}

void Input::SetPreferXInput(bool prefer)
{
	g_preferXInput = prefer;
}

void Input::BindAction(const std::wstring& name, KeyCode keyCode)
{
	ActionBinding binding;
	binding.type = ActionBindingType::Key;
	binding.keyCode = keyCode;
	g_actionBindings[name].push_back(binding);
}

void Input::BindAction(const std::wstring& name, GamepadButton button)
{
	ActionBinding binding;
	binding.type = ActionBindingType::GamepadButton;
	binding.gamepadButton = button;
	g_actionBindings[name].push_back(binding);
}

void Input::ClearActionBindings(const std::wstring& name)
{
	g_actionBindings.erase(name);
	g_actionStates.erase(name);
}

void Input::AddActionCallback(const std::wstring& name, ActionEvent eventType, ActionCallback callback)
{
	ActionCallbackSet& set = g_actionCallbacks[name];
	switch (eventType)
	{
	case ActionEvent::Pressed:
		set.pressed.push_back(std::move(callback));
		break;
	case ActionEvent::Released:
		set.released.push_back(std::move(callback));
		break;
	case ActionEvent::Hold:
		set.held.push_back(std::move(callback));
		break;
	default:
		break;
	}
}

void Input::BindAxisKey(const std::wstring& name, KeyCode positiveKey, KeyCode negativeKey,
	float scale, float deadzone, bool invert)
{
	AxisBinding& binding = g_axisBindings[name];
	binding.positiveKey = positiveKey;
	binding.negativeKey = negativeKey;
	binding.scale = scale;
	binding.deadzone = deadzone;
	binding.invert = invert;
}

void Input::BindAxisGamepad1D(const std::wstring& name, GamepadAxis1D axis,
	float scale, float deadzone, bool invert)
{
	AxisBinding& binding = g_axisBindings[name];
	binding.useGamepad1D = true;
	binding.gamepadAxis1D = axis;
	binding.scale = scale;
	binding.deadzone = deadzone;
	binding.invert = invert;
}

void Input::BindAxisGamepad2D(const std::wstring& name, GamepadAxis2D axis, Axis2DComponent component,
	float scale, float deadzone, bool invert)
{
	AxisBinding& binding = g_axisBindings[name];
	binding.useGamepad2D = true;
	binding.gamepadAxis2D = axis;
	binding.axis2DComponent = component;
	binding.scale = scale;
	binding.deadzone = deadzone;
	binding.invert = invert;
}

void Input::ClearAxisBinding(const std::wstring& name)
{
	g_axisBindings.erase(name);
	g_axisStates.erase(name);
}

void Input::AddAxisCallback(const std::wstring& name, AxisCallback callback)
{
	g_axisCallbacks[name].push_back(std::move(callback));
}

bool Input::LoadEnhancedInputConfig(const std::wstring& path)
{
	g_enhancedConfigPath = path;
	std::vector<std::filesystem::path> files;
	const std::filesystem::path root(path);
	if (std::filesystem::exists(root))
	{
		if (std::filesystem::is_directory(root))
		{
			for (const auto& entry : std::filesystem::directory_iterator(root))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}
				if (entry.path().extension() == L".ini")
				{
					files.push_back(entry.path());
				}
			}
			std::sort(files.begin(), files.end());
		}
		else
		{
			files.push_back(root);
		}
	}

	if (files.empty())
	{
		LOG(LogInput, Warning, L"EnhancedInput: No config files found at: %s", path.c_str());
		return false;
	}

	g_keyAliasMap.clear();
	g_enhancedActionIds.clear();
	g_enhancedActions.clear();
	g_enhancedActionStates.clear();
	g_enhancedCallbacks.clear();
	g_enhancedTriggerEventCallbacks.clear();
	g_enhancedTriggerEventInfoCallbacks.clear();
	g_sortedEnhancedActionIds.clear();

	auto parseKeyAliases = [&](const std::filesystem::path& filePath) -> bool
	{
		std::wifstream file(filePath);
		if (!file.is_open())
		{
			return false;
		}

		std::wstring currentSection;
		std::wstring currentActionName;
		uint32_t currentActionId = 0;
		bool hasCurrentAction = false;
		std::wstring line;
		while (std::getline(file, line))
		{
			const std::wstring trimmed = Trim(line);
			if (trimmed.empty() || trimmed[0] == L';' || trimmed[0] == L'#')
			{
				continue;
			}

			if (trimmed.front() == L'[' && trimmed.back() == L']')
			{
				currentSection = Trim(trimmed.substr(1, trimmed.size() - 2));
				if (currentSection == L"Action")
				{
					currentActionName.clear();
					currentActionId = 0;
					hasCurrentAction = false;
				}
				continue;
			}

			const std::wstring section = ToLower(currentSection);
			if (section.rfind(L"keyalias", 0) != 0)
			{
				continue;
			}

			const size_t eqPos = trimmed.find(L'=');
			if (eqPos == std::wstring::npos)
			{
				continue;
			}

			const std::wstring key = ToLower(Trim(trimmed.substr(0, eqPos)));
			const std::wstring value = Trim(trimmed.substr(eqPos + 1));
			KeyCode aliasKey = KeyCode::None;
			if (TryParseKeyCodeName(value, aliasKey))
			{
				g_keyAliasMap[key] = aliasKey;
			}
		}
		return true;
	};

	auto parseFile = [&](const std::filesystem::path& filePath) -> bool
	{
		std::wifstream file(filePath);
		if (!file.is_open())
		{
			LOG(LogInput, Warning, L"EnhancedInput: Failed to open config: %s", filePath.wstring().c_str());
			return false;
		}

		std::wstring currentSection;
		std::wstring currentActionName;
		uint32_t currentActionId = 0;
		bool hasCurrentAction = false;
		std::wstring line;
		while (std::getline(file, line))
		{
			const std::wstring trimmed = Trim(line);
			if (trimmed.empty() || trimmed[0] == L';' || trimmed[0] == L'#')
			{
				continue;
			}

			if (trimmed.front() == L'[' && trimmed.back() == L']')
			{
				currentSection = Trim(trimmed.substr(1, trimmed.size() - 2));
				if (currentSection == L"Action")
				{
					currentActionName.clear();
					currentActionId = 0;
					hasCurrentAction = false;
				}
				continue;
			}

			const size_t eqPos = trimmed.find(L'=');
			if (eqPos == std::wstring::npos)
			{
				continue;
			}

			const std::wstring key = Trim(trimmed.substr(0, eqPos));
			const std::wstring value = Trim(trimmed.substr(eqPos + 1));

			if (currentSection == L"EnhancedInput")
			{
				continue;
			}

			if (currentSection == L"Action")
			{
				const std::wstring lowerKey = ToLower(key);
				if (lowerKey == L"name")
				{
					currentActionName = value;
					currentActionId = GetOrCreateActionId(currentActionName);
					hasCurrentAction = true;
					EnhancedActionDef& action = g_enhancedActions[currentActionId];
					action.name = currentActionName;
					continue;
				}
				if (!hasCurrentAction)
				{
					continue;
				}
				EnhancedActionDef& action = g_enhancedActions[currentActionId];
				if (lowerKey == L"type")
				{
					TryParseActionType(value, action.type);
				}
				else if (lowerKey == L"mappings")
				{
					action.mappings.clear();
					const auto mappings = Split(value, L';');
					for (const auto& mappingToken : mappings)
					{
						if (mappingToken.empty())
						{
							continue;
						}
						EnhancedMapping mapping;
						if (TryParseMapping(mappingToken, mapping))
						{
							action.mappings.push_back(mapping);
						}
					}
				}
				else if (lowerKey == L"modifiers")
				{
					action.modifiers.clear();
					const auto modifiers = Split(value, L';');
					for (const auto& modifierToken : modifiers)
					{
						if (modifierToken.empty())
						{
							continue;
						}
						action.modifiers.push_back(ParseModifierToken(modifierToken));
					}
				}
				else if (lowerKey == L"triggers")
				{
					action.triggers.clear();
					const auto triggers = Split(value, L';');
					for (const auto& triggerToken : triggers)
					{
						if (triggerToken.empty())
						{
							continue;
						}
						action.triggers.push_back(ParseTriggerToken(triggerToken));
					}
				}
				else if (lowerKey == L"priority")
				{
					action.priority = static_cast<int>(wcstol(value.c_str(), nullptr, 10));
				}
				continue;
			}

			if (currentSection.rfind(L"Action.", 0) == 0)
			{
				const std::wstring actionName = currentSection.substr(7);
				const uint32_t actionId = GetOrCreateActionId(actionName);
				EnhancedActionDef& action = g_enhancedActions[actionId];
				action.name = actionName;

				const std::wstring lowerKey = ToLower(key);
				if (lowerKey == L"type")
				{
					TryParseActionType(value, action.type);
				}
				else if (lowerKey == L"mappings")
				{
					action.mappings.clear();
					const auto mappings = Split(value, L';');
					for (const auto& mappingToken : mappings)
					{
						if (mappingToken.empty())
						{
							continue;
						}
						EnhancedMapping mapping;
						if (TryParseMapping(mappingToken, mapping))
						{
							action.mappings.push_back(mapping);
						}
					}
				}
				else if (lowerKey == L"modifiers")
				{
					action.modifiers.clear();
					const auto modifiers = Split(value, L';');
					for (const auto& modifierToken : modifiers)
					{
						if (modifierToken.empty())
						{
							continue;
						}
						action.modifiers.push_back(ParseModifierToken(modifierToken));
					}
				}
				else if (lowerKey == L"triggers")
				{
					action.triggers.clear();
					const auto triggers = Split(value, L';');
					for (const auto& triggerToken : triggers)
					{
						if (triggerToken.empty())
						{
							continue;
						}
						action.triggers.push_back(ParseTriggerToken(triggerToken));
					}
				}
				else if (lowerKey == L"priority")
				{
					action.priority = static_cast<int>(wcstol(value.c_str(), nullptr, 10));
				}
				continue;
			}
		}
		return true;
	};

	for (const auto& filePath : files)
	{
		parseKeyAliases(filePath);
	}

	size_t loadedFiles = 0;
	for (const auto& filePath : files)
	{
		if (parseFile(filePath))
		{
			++loadedFiles;
		}
	}

	if (loadedFiles == 0)
	{
		return false;
	}

		for (auto& entry : g_enhancedActions)
	{
			if (entry.triggers.empty())
		{
			EnhancedTrigger trigger;
			trigger.type = EnhancedTriggerType::Always;
				entry.triggers.push_back(trigger);
		}
	}

	g_sortedEnhancedActionIds.clear();
	g_sortedEnhancedActionIds.reserve(g_enhancedActions.size());
	for (uint32_t actionId = 0; actionId < g_enhancedActions.size(); ++actionId)
	{
		g_sortedEnhancedActionIds.push_back(actionId);
	}
	std::stable_sort(g_sortedEnhancedActionIds.begin(), g_sortedEnhancedActionIds.end(),
		[](uint32_t a, uint32_t b)
		{
			return g_enhancedActions[a].priority > g_enhancedActions[b].priority;
		});

	LOG(LogInput, Info, L"EnhancedInput: Loaded %u config file(s), %u actions.",
		static_cast<unsigned>(loadedFiles),
		static_cast<unsigned>(g_enhancedActions.size()));
	return true;
}

bool Input::ReloadEnhancedInputConfig()
{
	if (g_enhancedConfigPath.empty())
	{
		return false;
	}
	return LoadEnhancedInputConfig(g_enhancedConfigPath);
}

void Input::SetEnhancedContextActive(const std::wstring& name, bool active)
{
	(void)name;
	(void)active;
}

Axis2DValue Input::GetEnhancedAxis2D(const std::wstring& name)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedActionStates.size())
	{
		return Axis2DValue{};
	}
	return g_enhancedActionStates[id].value2D;
}

float Input::GetEnhancedAxis1D(const std::wstring& name)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedActionStates.size())
	{
		return 0.0f;
	}
	return g_enhancedActionStates[id].value1D;
}

bool Input::IsEnhancedActionActive(const std::wstring& name)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedActionStates.size())
	{
		return false;
	}
	return g_enhancedActionStates[id].active;
}

double Input::GetEnhancedActionHeldSeconds(const std::wstring& name)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedActionStates.size())
	{
		return 0.0;
	}

	const EnhancedActionState& state = g_enhancedActionStates[id];
	if (!state.active)
	{
		return 0.0;
	}

	const double nowSeconds = g_enhancedTimeProvider ? g_enhancedTimeProvider() : (GetTickCount64() / 1000.0);
	const double elapsed = nowSeconds - state.lastPressedTime;
	return (elapsed > 0.0) ? elapsed : 0.0;
}

bool Input::HasEnhancedTrigger(const std::wstring& name, EnhancedTriggerType trigger)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedActions.size())
	{
		return false;
	}
	const auto& triggers = g_enhancedActions[id].triggers;
	for (const auto& entry : triggers)
	{
		if (entry.type == trigger)
		{
			return true;
		}
	}
	return false;
}

void Input::AddEnhancedActionCallback(const std::wstring& name, EnhancedTriggerType trigger, ActionCallback callback)
{
	const uint32_t id = GetOrCreateActionId(name);
	if (id >= g_enhancedCallbacks.size())
	{
		return;
	}
	const size_t triggerIndex = TriggerIndex(trigger);
	if (triggerIndex >= static_cast<size_t>(EnhancedTriggerType::Count))
	{
		return;
	}
	g_enhancedCallbacks[id].callbacks[triggerIndex].push_back(std::move(callback));
}

void Input::AddEnhancedTriggerEventCallback(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback)
{
	AddEnhancedTriggerEventCallbackHandle(name, triggerEvent, std::move(callback));
}

void Input::AddEnhancedTriggerEventCallbackEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback)
{
	AddEnhancedTriggerEventCallbackHandleEx(name, triggerEvent, std::move(callback));
}

EnhancedTriggerEventCallbackHandle Input::AddEnhancedTriggerEventCallbackHandle(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback)
{
	EnhancedTriggerEventCallbackHandle handle;
	if (!callback)
	{
		return handle;
	}
	const uint32_t id = GetOrCreateActionId(name);
	if (id >= g_enhancedTriggerEventCallbacks.size())
	{
		return handle;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return handle;
	}
	handle.actionId = id;
	handle.triggerEvent = triggerEvent;
	handle.id = callback.GetHandle();
	handle.isInfoCallback = false;
	g_enhancedTriggerEventCallbacks[id].callbacks[eventIndex].push_back({ handle.id, std::move(callback) });
	return handle;
}

EnhancedTriggerEventCallbackHandle Input::AddEnhancedTriggerEventCallbackHandleEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback)
{
	EnhancedTriggerEventCallbackHandle handle;
	if (!callback)
	{
		return handle;
	}
	const uint32_t id = GetOrCreateActionId(name);
	if (id >= g_enhancedTriggerEventInfoCallbacks.size())
	{
		return handle;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return handle;
	}
	handle.actionId = id;
	handle.triggerEvent = triggerEvent;
	handle.id = callback.GetHandle();
	handle.isInfoCallback = true;
	g_enhancedTriggerEventInfoCallbacks[id].callbacks[eventIndex].push_back({ handle.id, std::move(callback) });
	return handle;
}

void Input::RemoveEnhancedTriggerEventCallback(const EnhancedTriggerEventCallbackHandle& handle)
{
	if (!handle.IsValid())
	{
		return;
	}
	const size_t eventIndex = static_cast<size_t>(handle.triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return;
	}
	if (handle.isInfoCallback)
	{
		if (handle.actionId >= g_enhancedTriggerEventInfoCallbacks.size())
		{
			return;
		}
		auto& callbacks = g_enhancedTriggerEventInfoCallbacks[handle.actionId].callbacks[eventIndex];
		callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
			[&](const EnhancedTriggerEventInfoCallbackSet::Entry& entry)
			{
				return entry.id == handle.id;
			}),
			callbacks.end());
		return;
	}

	if (handle.actionId >= g_enhancedTriggerEventCallbacks.size())
	{
		return;
	}
	auto& callbacks = g_enhancedTriggerEventCallbacks[handle.actionId].callbacks[eventIndex];
	callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
		[&](const EnhancedTriggerEventCallbackSet::Entry& entry)
		{
			return entry.id == handle.id;
		}),
		callbacks.end());
}

bool Input::RemoveEnhancedTriggerEventCallback(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback)
{
	if (!callback)
	{
		return false;
	}
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedTriggerEventCallbacks.size())
	{
		return false;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return false;
	}
	auto& callbacks = g_enhancedTriggerEventCallbacks[id].callbacks[eventIndex];
	for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
	{
		if (it->callback == callback)
		{
			callbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool Input::RemoveEnhancedTriggerEventCallbackEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback)
{
	if (!callback)
	{
		return false;
	}
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedTriggerEventInfoCallbacks.size())
	{
		return false;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return false;
	}
	auto& callbacks = g_enhancedTriggerEventInfoCallbacks[id].callbacks[eventIndex];
	for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
	{
		if (it->callback == callback)
		{
			callbacks.erase(it);
			return true;
		}
	}
	return false;
}

void Input::ClearEnhancedTriggerEventCallbacks(const std::wstring& name, EnhancedTriggerEvent triggerEvent)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedTriggerEventCallbacks.size())
	{
		return;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return;
	}
	g_enhancedTriggerEventCallbacks[id].callbacks[eventIndex].clear();
}

void Input::ClearEnhancedTriggerEventCallbacksEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent)
{
	uint32_t id = 0;
	if (!TryGetActionId(name, id) || id >= g_enhancedTriggerEventInfoCallbacks.size())
	{
		return;
	}
	const size_t eventIndex = static_cast<size_t>(triggerEvent);
	if (eventIndex >= static_cast<size_t>(EnhancedTriggerEvent::Count))
	{
		return;
	}
	g_enhancedTriggerEventInfoCallbacks[id].callbacks[eventIndex].clear();
}
void Input::SetEnhancedTimeProvider(TimeProvider provider)
{
	g_enhancedTimeProvider = std::move(provider);
}
