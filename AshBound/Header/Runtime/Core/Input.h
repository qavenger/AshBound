#pragma once

#include <cstdint>
#include <functional>
#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/Input/KeyCode.h"
#include <string>
#include <Windows.h>

#include "Runtime/Core/Platform/PlatformWindowHandle.h"

enum class InputSource
{
	Keyboard,
	XInput
};

enum class MouseButton
{
	Left,
	Right,
	Middle,
	X1,
	X2
};

enum class GamepadButton
{
	A,
	B,
	X,
	Y,
	LB,
	RB,
	Back,
	Start,
	LeftStick,
	RightStick,
	DpadUp,
	DpadDown,
	DpadLeft,
	DpadRight,
	Count
};

enum class GamepadAxis2D
{
	LeftStick,
	RightStick
};

enum class GamepadAxis1D
{
	LeftTrigger,
	RightTrigger
};

enum class ActionEvent
{
	Pressed,
	Released,
	Hold
};

enum class Axis2DComponent
{
	X,
	Y
};

enum class EnhancedActionType
{
	Bool,
	Axis1D,
	Axis2D
};

enum class EnhancedTriggerType
{
	Always,
	Pressed,
	Released,
	Hold,
	Click,
	DoubleClick,
	Pulse,
	Count
};

enum class EnhancedTriggerEvent
{
	Started,
	Ongoing,
	Triggered,
	Completed,
	Canceled,
	Count
};

struct Axis2DValue
{
	float x = 0.0f;
	float y = 0.0f;
};

struct EnhancedTriggerEventInfo
{
	std::wstring actionName;
	EnhancedActionType actionType = EnhancedActionType::Bool;
	EnhancedTriggerType triggerType = EnhancedTriggerType::Count;
	EnhancedTriggerEvent triggerEvent = EnhancedTriggerEvent::Started;
	KeyCode keyCode = KeyCode::None;
	bool isActive = false;
	bool wasActive = false;
	bool triggered = false;
	bool canceled = false;
	float value1D = 0.0f;
	Axis2DValue value2D;
	double heldSeconds = 0.0;
	double timeSeconds = 0.0;
};

struct EnhancedTriggerEventCallbackHandle
{
	uint32_t actionId = 0;
	EnhancedTriggerEvent triggerEvent = EnhancedTriggerEvent::Count;
	DelegateHandle id = DelegateHandle::Invalid();
	bool isInfoCallback = false;

	bool IsValid() const { return id.IsValid(); }
};

struct GamepadState
{
	float leftX = 0.0f;
	float leftY = 0.0f;
	float rightX = 0.0f;
	float rightY = 0.0f;
	float leftTrigger = 0.0f;
	float rightTrigger = 0.0f;
	bool buttons[static_cast<size_t>(GamepadButton::Count)] = {};
};

class Input
{
public:
	using AnalogProvider = bool(*)(KeyCode keyCode, float& outValue);
	using ActionCallback = Delegate<void()>;
	using AxisCallback = Delegate<void(float)>;
	using TimeProvider = Delegate<double()>;
	using TriggerEventCallback = Delegate<void(const EnhancedTriggerEventInfo&)>;

	static void Init(PlatformWindowHandle windowHandle);
	static void Update();

	static bool IsKeyDown(KeyCode keyCode);
	static bool WasKeyPressed(KeyCode keyCode);
	static bool WasKeyReleased(KeyCode keyCode);

	static bool IsMouseButtonDown(MouseButton button);
	static bool WasMouseButtonPressed(MouseButton button);
	static bool WasMouseButtonReleased(MouseButton button);

	static POINT GetMousePosition();
	static POINT GetMouseDelta();

	static void SetKeyAnalogValue(KeyCode keyCode, float value);
	static float GetKeyAnalogValue(KeyCode keyCode);
	static void SetAnalogProvider(AnalogProvider provider);
	static void Shutdown();

	static void BindGamepadButton(GamepadButton button, KeyCode keyCode);
	static void BindGamepadAxis2D(GamepadAxis2D axis,
		KeyCode positiveXKey,
		KeyCode negativeXKey,
		KeyCode positiveYKey,
		KeyCode negativeYKey,
		bool invertY = false);
	static void BindGamepadAxis1D(GamepadAxis1D axis, KeyCode positiveKey, KeyCode negativeKey = KeyCode::None);

	static GamepadState GetGamepadState();

	// XInput support
	static bool IsXInputConnected(uint32_t controllerIndex = 0);
	static GamepadState GetXInputState(uint32_t controllerIndex = 0);
	static InputSource GetActiveInputSource();
	static void SetPreferXInput(bool prefer);

	// Action/Axis mapping
	static void BindAction(const std::wstring& name, KeyCode keyCode);
	static void BindAction(const std::wstring& name, GamepadButton button);
	static void ClearActionBindings(const std::wstring& name);
	static void AddActionCallback(const std::wstring& name, ActionEvent eventType, ActionCallback callback);

	static void BindAxisKey(const std::wstring& name, KeyCode positiveKey, KeyCode negativeKey,
		float scale = 1.0f, float deadzone = 0.05f, bool invert = false);
	static void BindAxisGamepad1D(const std::wstring& name, GamepadAxis1D axis,
		float scale = 1.0f, float deadzone = 0.05f, bool invert = false);
	static void BindAxisGamepad2D(const std::wstring& name, GamepadAxis2D axis, Axis2DComponent component,
		float scale = 1.0f, float deadzone = 0.05f, bool invert = false);
	static void ClearAxisBinding(const std::wstring& name);
	static void AddAxisCallback(const std::wstring& name, AxisCallback callback);

	// Enhanced input (UE-like)
	static bool LoadEnhancedInputConfig(const std::wstring& path);
	static bool ReloadEnhancedInputConfig();
	static void SetEnhancedContextActive(const std::wstring& name, bool active);
	static Axis2DValue GetEnhancedAxis2D(const std::wstring& name);
	static float GetEnhancedAxis1D(const std::wstring& name);
	static bool IsEnhancedActionActive(const std::wstring& name);
	static double GetEnhancedActionHeldSeconds(const std::wstring& name);
	static bool HasEnhancedTrigger(const std::wstring& name, EnhancedTriggerType trigger);
	static void AddEnhancedActionCallback(const std::wstring& name, EnhancedTriggerType trigger, ActionCallback callback);
	static void AddEnhancedTriggerEventCallback(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback);
	static void AddEnhancedTriggerEventCallbackEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback);
	static EnhancedTriggerEventCallbackHandle AddEnhancedTriggerEventCallbackHandle(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback);
	static EnhancedTriggerEventCallbackHandle AddEnhancedTriggerEventCallbackHandleEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback);
	static void RemoveEnhancedTriggerEventCallback(const EnhancedTriggerEventCallbackHandle& handle);
	static bool RemoveEnhancedTriggerEventCallback(const std::wstring& name, EnhancedTriggerEvent triggerEvent, ActionCallback callback);
	static bool RemoveEnhancedTriggerEventCallbackEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent, TriggerEventCallback callback);
	static void ClearEnhancedTriggerEventCallbacks(const std::wstring& name, EnhancedTriggerEvent triggerEvent);
	static void ClearEnhancedTriggerEventCallbacksEx(const std::wstring& name, EnhancedTriggerEvent triggerEvent);
	static void SetEnhancedTimeProvider(TimeProvider provider);
};
