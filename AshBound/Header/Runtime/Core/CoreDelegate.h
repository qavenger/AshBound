#pragma once

#include <cstdint>
#include <vector>

#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/DisplayManager.h"
#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

struct WindowSizeChangedInfo
{
	PlatformWindowHandle window = {};
	int width = 0;
	int height = 0;
};

struct WindowMovedInfo
{
	PlatformWindowHandle window = {};
	int x = 0;
	int y = 0;
};

struct WindowDisplayChangedInfo
{
	PlatformWindowHandle window = {};
	PlatformMonitorHandle previousMonitor = {};
	PlatformMonitorHandle currentMonitor = {};
};

struct DisplayConfigurationChangedInfo
{
	PlatformWindowHandle window = {};
	int width = 0;
	int height = 0;
	int bitsPerPixel = 0;
};

struct DisplayDevicesChangedInfo
{
	PlatformWindowHandle window = {};
	unsigned int event = 0;
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
};

struct DisplaySettingsChangedInfo
{
	PlatformWindowHandle window = {};
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
};

struct HdrStateChangedInfo
{
	PlatformWindowHandle window = {};
	bool hdrSupported = false;
	bool hdrActive = false;
};

struct DisplayCacheRefreshedInfo
{
	std::vector<DisplayInfo> displays;
};

class CoreDelegate
{
public:
	using WindowSizeChangedCallback = Delegate<void(const WindowSizeChangedInfo&)>;
	using WindowMovedCallback = Delegate<void(const WindowMovedInfo&)>;
	using WindowDisplayChangedCallback = Delegate<void(const WindowDisplayChangedInfo&)>;
	using DisplayConfigurationChangedCallback = Delegate<void(const DisplayConfigurationChangedInfo&)>;
	using DisplayDevicesChangedCallback = Delegate<void(const DisplayDevicesChangedInfo&)>;
	using DisplaySettingsChangedCallback = Delegate<void(const DisplaySettingsChangedInfo&)>;
	using HdrStateChangedCallback = Delegate<void(const HdrStateChangedInfo&)>;
	using DisplayCacheRefreshedCallback = Delegate<void(const DisplayCacheRefreshedInfo&)>;

	/// callback signature: void(const WindowSizeChangedInfo& info)
	static DelegateHandle AddWindowSizeChangedCallback(WindowSizeChangedCallback callback);
	static bool RemoveWindowSizeChangedCallback(const DelegateHandle& handle);
	static void ClearWindowSizeChangedCallbacks();

	/// callback signature: void(const WindowMovedInfo& info)
	static DelegateHandle AddWindowMovedCallback(WindowMovedCallback callback);
	static bool RemoveWindowMovedCallback(const DelegateHandle& handle);
	static void ClearWindowMovedCallbacks();

	/// callback signature: void(const WindowDisplayChangedInfo& info)
	static DelegateHandle AddWindowDisplayChangedCallback(WindowDisplayChangedCallback callback);
	static bool RemoveWindowDisplayChangedCallback(const DelegateHandle& handle);
	static void ClearWindowDisplayChangedCallbacks();

	/// callback signature: void(const DisplayConfigurationChangedInfo& info)
	static DelegateHandle AddDisplayConfigurationChangedCallback(DisplayConfigurationChangedCallback callback);
	static bool RemoveDisplayConfigurationChangedCallback(const DelegateHandle& handle);
	static void ClearDisplayConfigurationChangedCallbacks();

	/// callback signature: void(const DisplayDevicesChangedInfo& info)
	static DelegateHandle AddDisplayDevicesChangedCallback(DisplayDevicesChangedCallback callback);
	static bool RemoveDisplayDevicesChangedCallback(const DelegateHandle& handle);
	static void ClearDisplayDevicesChangedCallbacks();

	/// callback signature: void(const DisplaySettingsChangedInfo& info)
	static DelegateHandle AddDisplaySettingsChangedCallback(DisplaySettingsChangedCallback callback);
	static bool RemoveDisplaySettingsChangedCallback(const DelegateHandle& handle);
	static void ClearDisplaySettingsChangedCallbacks();

	/// callback signature: void(const HdrStateChangedInfo& info)
	static DelegateHandle AddHdrStateChangedCallback(HdrStateChangedCallback callback);
	static bool RemoveHdrStateChangedCallback(const DelegateHandle& handle);
	static void ClearHdrStateChangedCallbacks();

	/// callback signature: void(const DisplayCacheRefreshedInfo& info)
	static DelegateHandle AddDisplayCacheRefreshedCallback(DisplayCacheRefreshedCallback callback);
	static bool RemoveDisplayCacheRefreshedCallback(const DelegateHandle& handle);
	static void ClearDisplayCacheRefreshedCallbacks();

	static void BroadcastWindowSizeChanged(const WindowSizeChangedInfo& info);
	static void BroadcastWindowMoved(const WindowMovedInfo& info);
	static void BroadcastWindowDisplayChanged(const WindowDisplayChangedInfo& info);
	static void BroadcastDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info);
	static void BroadcastDisplayDevicesChanged(const DisplayDevicesChangedInfo& info);
	static void BroadcastDisplaySettingsChanged(const DisplaySettingsChangedInfo& info);
	static void BroadcastHdrStateChanged(const HdrStateChangedInfo& info);
	static void BroadcastDisplayCacheRefreshed(const DisplayCacheRefreshedInfo& info);
};
