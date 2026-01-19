#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

struct DisplayInfo
{
	struct DisplayMode
	{
		int width = 0;
		int height = 0;
		int refreshRate = 0;
	};

	PlatformMonitorHandle monitor = {};
	std::wstring deviceName;
	std::wstring friendlyName;
	RECT monitorRect = {};
	RECT workRect = {};
	bool isPrimary = false;
	UINT dpiX = 0;
	UINT dpiY = 0;
	bool hdrSupported = false;
	bool hdrActive = false;
	bool isActive = false;
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
	std::vector<DisplayMode> supportedModes;
};

class DisplayManager
{
public:
	static void Initialize(PlatformWindowHandle referenceWindow);
	static void Refresh();

	static const std::vector<DisplayInfo>& GetDisplays();
	static const DisplayInfo* FindDisplayByMonitor(PlatformMonitorHandle monitor);
	static bool HasMonitor(PlatformMonitorHandle monitor);
	static bool IsMonitorActive(PlatformMonitorHandle monitor);
	static PlatformMonitorHandle GetPrimaryMonitor();
	static PlatformMonitorHandle GetFirstActiveMonitor();

	static bool TryGetDisplayInfoFromMonitor(PlatformMonitorHandle monitor, DisplayInfo& outInfo);
	static bool TryGetDisplayInfoFromWindow(PlatformWindowHandle window, DisplayInfo& outInfo);
};
