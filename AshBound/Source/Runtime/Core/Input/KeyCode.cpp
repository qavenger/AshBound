#include "Runtime/Core/Input/KeyCode.h"

#include <cwctype>
#include <unordered_map>

namespace
{
	struct NameEntry
	{
		KeyCode key;
		const wchar_t* name;
	};

	const NameEntry kKeyNames[] = {
		{ KeyCode::MouseLeft, L"mouseleft" },
		{ KeyCode::MouseRight, L"mouseright" },
		{ KeyCode::MouseMiddle, L"mousemiddle" },
		{ KeyCode::MouseX1, L"mousex1" },
		{ KeyCode::MouseX2, L"mousex2" },
		{ KeyCode::Backspace, L"backspace" },
		{ KeyCode::Tab, L"tab" },
		{ KeyCode::Enter, L"enter" },
		{ KeyCode::Shift, L"shift" },
		{ KeyCode::Control, L"ctrl" },
		{ KeyCode::Alt, L"alt" },
		{ KeyCode::Pause, L"pause" },
		{ KeyCode::CapsLock, L"capslock" },
		{ KeyCode::Escape, L"esc" },
		{ KeyCode::Space, L"space" },
		{ KeyCode::PageUp, L"pageup" },
		{ KeyCode::PageDown, L"pagedown" },
		{ KeyCode::End, L"end" },
		{ KeyCode::Home, L"home" },
		{ KeyCode::Left, L"left" },
		{ KeyCode::Up, L"up" },
		{ KeyCode::Right, L"right" },
		{ KeyCode::Down, L"down" },
		{ KeyCode::Insert, L"insert" },
		{ KeyCode::Delete, L"delete" },
		{ KeyCode::LeftWin, L"lwin" },
		{ KeyCode::RightWin, L"rwin" },
		{ KeyCode::Apps, L"apps" },
		{ KeyCode::NumPad0, L"numpad0" },
		{ KeyCode::NumPad1, L"numpad1" },
		{ KeyCode::NumPad2, L"numpad2" },
		{ KeyCode::NumPad3, L"numpad3" },
		{ KeyCode::NumPad4, L"numpad4" },
		{ KeyCode::NumPad5, L"numpad5" },
		{ KeyCode::NumPad6, L"numpad6" },
		{ KeyCode::NumPad7, L"numpad7" },
		{ KeyCode::NumPad8, L"numpad8" },
		{ KeyCode::NumPad9, L"numpad9" },
		{ KeyCode::Multiply, L"multiply" },
		{ KeyCode::Add, L"add" },
		{ KeyCode::Separator, L"separator" },
		{ KeyCode::Subtract, L"subtract" },
		{ KeyCode::Decimal, L"decimal" },
		{ KeyCode::Divide, L"divide" },
		{ KeyCode::F1, L"f1" },
		{ KeyCode::F2, L"f2" },
		{ KeyCode::F3, L"f3" },
		{ KeyCode::F4, L"f4" },
		{ KeyCode::F5, L"f5" },
		{ KeyCode::F6, L"f6" },
		{ KeyCode::F7, L"f7" },
		{ KeyCode::F8, L"f8" },
		{ KeyCode::F9, L"f9" },
		{ KeyCode::F10, L"f10" },
		{ KeyCode::F11, L"f11" },
		{ KeyCode::F12, L"f12" },
		{ KeyCode::NumLock, L"numlock" },
		{ KeyCode::ScrollLock, L"scrolllock" },
		{ KeyCode::LeftShift, L"lshift" },
		{ KeyCode::RightShift, L"rshift" },
		{ KeyCode::LeftControl, L"lctrl" },
		{ KeyCode::RightControl, L"rctrl" },
		{ KeyCode::LeftAlt, L"lalt" },
		{ KeyCode::RightAlt, L"ralt" },
		{ KeyCode::OemSemicolon, L"semicolon" },
		{ KeyCode::OemPlus, L"equals" },
		{ KeyCode::OemComma, L"comma" },
		{ KeyCode::OemMinus, L"minus" },
		{ KeyCode::OemPeriod, L"period" },
		{ KeyCode::OemSlash, L"slash" },
		{ KeyCode::OemGrave, L"grave" },
		{ KeyCode::OemLeftBracket, L"lbracket" },
		{ KeyCode::OemBackslash, L"backslash" },
		{ KeyCode::OemRightBracket, L"rbracket" },
		{ KeyCode::OemApostrophe, L"apostrophe" }
	};

	std::wstring ToLower(const std::wstring& value)
	{
		std::wstring result = value;
		for (wchar_t& ch : result)
		{
			ch = static_cast<wchar_t>(std::towlower(ch));
		}
		return result;
	}

	bool TryParseNumeric(const std::wstring& value, std::uint32_t& out)
	{
		if (value.rfind(L"0x", 0) == 0 && value.size() > 2)
		{
			out = static_cast<std::uint32_t>(wcstoul(value.c_str(), nullptr, 16));
			return true;
		}

		bool allDigits = !value.empty();
		for (wchar_t ch : value)
		{
			if (ch < L'0' || ch > L'9')
			{
				allDigits = false;
				break;
			}
		}
		if (allDigits)
		{
			out = static_cast<std::uint32_t>(wcstoul(value.c_str(), nullptr, 10));
			return true;
		}
		return false;
	}
}

KeyCode ToKeyCode(std::uint32_t vk)
{
	return static_cast<KeyCode>(vk);
}

std::uint32_t ToVK(KeyCode key)
{
	return static_cast<std::uint32_t>(key);
}

const wchar_t* KeyCodeToName(KeyCode key)
{
	for (const auto& entry : kKeyNames)
	{
		if (entry.key == key)
		{
			return entry.name;
		}
	}

	if (key >= KeyCode::A && key <= KeyCode::Z)
	{
		static wchar_t buffer[2] = {};
		buffer[0] = static_cast<wchar_t>(L'A' + (ToVK(key) - ToVK(KeyCode::A)));
		buffer[1] = L'\0';
		return buffer;
	}
	if (key >= KeyCode::D0 && key <= KeyCode::D9)
	{
		static wchar_t buffer[2] = {};
		buffer[0] = static_cast<wchar_t>(L'0' + (ToVK(key) - ToVK(KeyCode::D0)));
		buffer[1] = L'\0';
		return buffer;
	}
	return L"unknown";
}

bool TryParseKeyCodeName(const std::wstring& name, KeyCode& out)
{
	const std::wstring value = ToLower(name);
	if (value.empty())
	{
		return false;
	}

	if (value.size() == 1)
	{
		const wchar_t ch = value[0];
		if (ch >= L'a' && ch <= L'z')
		{
			out = static_cast<KeyCode>(L'A' + (ch - L'a'));
			return true;
		}
		if (ch >= L'0' && ch <= L'9')
		{
			out = static_cast<KeyCode>(L'0' + (ch - L'0'));
			return true;
		}
	}

	for (const auto& entry : kKeyNames)
	{
		if (value == entry.name)
		{
			out = entry.key;
			return true;
		}
	}

	if (value.rfind(L"vk_", 0) == 0)
	{
		KeyCode vkParsed;
		if (TryParseKeyCodeName(value.substr(3), vkParsed))
		{
			out = vkParsed;
			return true;
		}
	}

	std::uint32_t numeric = 0;
	if (TryParseNumeric(value, numeric))
	{
		out = ToKeyCode(numeric);
		return true;
	}

	return false;
}
