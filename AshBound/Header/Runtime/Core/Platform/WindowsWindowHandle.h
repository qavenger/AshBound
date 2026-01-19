#pragma once

#include <Windows.h>

#include "Runtime/Core/Platform/PlatformWindowHandle.h"

struct WindowsWindowHandle
{
	HWND handle = nullptr;

	bool IsValid() const { return handle != nullptr; }

	PlatformWindowHandle ToPlatformHandle() const
	{
		return PlatformWindowHandle{ reinterpret_cast<void*>(handle) };
	}

	static WindowsWindowHandle FromPlatformHandle(const PlatformWindowHandle& handle)
	{
		return WindowsWindowHandle{ reinterpret_cast<HWND>(handle.handle) };
	}
};
