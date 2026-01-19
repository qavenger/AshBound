#pragma once

#include <unordered_map>
#include <vector>

#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/DisplayManager.h"

class ViewportSubsystem;

class DisplaySubsystem
{
public:
	void Initialize(ViewportSubsystem* viewportSubsystem, PlatformWindowHandle referenceWindow);
	void Shutdown();

	void RefreshDisplays();
	const std::vector<DisplayInfo>& GetDisplays() const;

private:
	void RegisterCallbacks();
	void UnregisterCallbacks();

	void HandleDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info);
	void HandleDisplayDevicesChanged(const DisplayDevicesChangedInfo& info);
	void HandleDisplaySettingsChanged(const DisplaySettingsChangedInfo& info);
	void HandleWindowDisplayChanged(const WindowDisplayChangedInfo& info);
	void HandleWindowMoved(const WindowMovedInfo& info);
	void HandleWindowSizeChanged(const WindowSizeChangedInfo& info);

	void UpdateHdrState(PlatformWindowHandle window);
	void UpdateHdrStatesForAllViewports();
	void CleanupHdrStates(const std::vector<PlatformWindowHandle>& activeWindows);
	void BroadcastDisplayCacheRefreshed();
	void RefreshActiveDisplays();
	const DisplayInfo* FindActiveDisplayByMonitor(PlatformMonitorHandle monitor) const;
	const DisplayInfo* FindActiveDisplayByWindow(PlatformWindowHandle window) const;
	void UpdateViewportDisplaySnapshot(PlatformWindowHandle window);
	void UpdateViewportDisplaySnapshotsForAll();
	void SnapWindowToNearestDisplay(PlatformWindowHandle window);

	struct HdrStateCache
	{
		bool hasState = false;
		bool supported = false;
		bool active = false;
	};

	ViewportSubsystem* m_viewportSubsystem = nullptr;
	bool m_initialized = false;
	std::vector<DisplayInfo> m_activeDisplays;
	std::unordered_map<PlatformWindowHandle, HdrStateCache> m_hdrStates;

	DelegateHandle m_displayConfigurationHandle = DelegateHandle::Invalid();
	DelegateHandle m_displayDevicesHandle = DelegateHandle::Invalid();
	DelegateHandle m_displaySettingsHandle = DelegateHandle::Invalid();
	DelegateHandle m_windowDisplayChangedHandle = DelegateHandle::Invalid();
	DelegateHandle m_windowMovedHandle = DelegateHandle::Invalid();
	DelegateHandle m_windowSizeChangedHandle = DelegateHandle::Invalid();
};
