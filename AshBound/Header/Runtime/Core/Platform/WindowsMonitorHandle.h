#pragma once

#include <Windows.h>

#include "Runtime/Core/Platform/PlatformMonitorHandle.h"

struct WindowsMonitorHandle
{
	HMONITOR handle = nullptr;

	bool IsValid() const { return handle != nullptr; }

	PlatformMonitorHandle ToPlatformHandle() const
	{
		return PlatformMonitorHandle{ reinterpret_cast<void*>(handle) };
	}

	static WindowsMonitorHandle FromPlatformHandle(const PlatformMonitorHandle& handle)
	{
		return WindowsMonitorHandle{ reinterpret_cast<HMONITOR>(handle.handle) };
	}
};
