#pragma once

#include "Runtime/Core/Platform/WindowsMonitorHandle.h"

struct WindowsHdrInfo
{
	bool supported = false;
	bool active = false;
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
};

class WindowsHdrQuery
{
public:
	static bool TryQuery(const WindowsMonitorHandle& monitor, WindowsHdrInfo& outInfo);
};
