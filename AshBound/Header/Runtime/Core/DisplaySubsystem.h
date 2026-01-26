#pragma once

#include <vector>

#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/DisplayInfo.h"
#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class DisplaySubsystem
{
public:
	void Initialize();
	void Shutdown();

	/// Refresh the display cache by enumerating all monitors.
	void RefreshDisplayCache();

	/// Get all cached display information.
	const std::vector<DisplayInfo>& GetDisplays() const;

	/// Get display info for the monitor containing the specified window.
	/// This iterates through cached displays and matches using MonitorFromWindow.
	/// Returns nullptr if no matching display is found.
	const DisplayInfo* GetDisplayInfoForWindow(PlatformWindowHandle window) const;

	/// Get display info by monitor handle from cache.
	const DisplayInfo* GetDisplayInfo(const PlatformMonitorHandlePtr& monitor) const;

	/// Get the primary monitor's display info.
	const DisplayInfo* GetPrimaryDisplayInfo() const;

	/// Request the system to enable HDR on the specified monitor.
	/// Returns true if the request was successful.
	bool TryEnableHdr(const PlatformMonitorHandlePtr& monitor);

	/// Request the system to disable HDR on the specified monitor.
	/// Returns true if the request was successful.
	bool TryDisableHdr(const PlatformMonitorHandlePtr& monitor);

	/// Refresh the display info associated with a window (single display update).
	/// Returns true if the refresh succeeded.
	bool RefreshDisplayForWindow(PlatformWindowHandle window, DisplayStateChangeType changeHint);

private:
	void RegisterCallbacks();
	void UnregisterCallbacks();
	static BOOL CALLBACK EnumMonitorCallback(HMONITOR monitor, HDC context, LPRECT rect, LPARAM data);

	void HandleDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info);
	void HandleDisplayDevicesChanged(const DisplayDevicesChangedInfo& info);
	void HandleDisplaySettingsChanged(const DisplaySettingsChangedInfo& info);

	void BroadcastDisplayCacheRefreshed();
	void BroadcastDisplayStateChanged(const PlatformMonitorHandlePtr& monitor, DisplayStateChangeType changeType);

	bool TryQueryDisplayInfo(HMONITOR nativeMonitor, DisplayInfo& outInfo) const;
	bool TryRefreshDisplayFromWindow(PlatformWindowHandle window, DisplayStateChangeType changeHint);
	void HandleUserHdrStateChange(DisplayInfo& display, bool oldHdrActive, bool newHdrActive);

	bool m_initialized = false;
	std::vector<DisplayInfo> m_displays;

	DelegateHandle m_displayConfigurationHandle = DelegateHandle::Invalid();
	DelegateHandle m_displayDevicesHandle = DelegateHandle::Invalid();
	DelegateHandle m_displaySettingsHandle = DelegateHandle::Invalid();
};
