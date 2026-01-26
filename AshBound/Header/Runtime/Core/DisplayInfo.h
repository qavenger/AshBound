#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Runtime/Core/Platform/PlatformMonitorHandle.h"

struct DisplayInfo
{
	struct DisplayMode
	{
		int width = 0;
		int height = 0;
		int refreshRate = 0;
	};

	PlatformMonitorHandlePtr monitor;
	std::wstring deviceName;
	std::wstring friendlyName;
	RECT monitorRect = {};
	RECT workRect = {};
	bool isPrimary = false;
	UINT dpiX = 0;
	UINT dpiY = 0;
	bool hdrSupported = false;
	bool hdrActive = false;
	bool hdr10PlusSupported = false;
	bool dolbyVisionSupported = false;
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
	float sdrWhiteLevelNits = 0.0f;
	std::vector<DisplayMode> supportedModes;
};
