#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/ViewportSubsystem.h"
#include "Runtime/Core/Platform/WindowsWindowHandle.h"

#include <algorithm>

void DisplaySubsystem::Initialize(ViewportSubsystem* viewportSubsystem, PlatformWindowHandle referenceWindow)
{
	if (m_initialized)
	{
		return;
	}

	m_initialized = true;
	m_viewportSubsystem = viewportSubsystem;
	DisplayManager::Initialize(referenceWindow);
	RegisterCallbacks();
	RefreshDisplays();
	UpdateHdrStatesForAllViewports();
	UpdateViewportDisplaySnapshotsForAll();
}

void DisplaySubsystem::Shutdown()
{
	if (!m_initialized)
	{
		return;
	}

	UnregisterCallbacks();
	m_hdrStates.clear();
	m_viewportSubsystem = nullptr;
	m_initialized = false;
}

void DisplaySubsystem::RefreshDisplays()
{
	DisplayManager::Refresh();
	RefreshActiveDisplays();
	BroadcastDisplayCacheRefreshed();
}

const std::vector<DisplayInfo>& DisplaySubsystem::GetDisplays() const
{
	return m_activeDisplays;
}

void DisplaySubsystem::RegisterCallbacks()
{
	m_displayConfigurationHandle = CoreDelegate::AddDisplayConfigurationChangedCallback(
		CoreDelegate::DisplayConfigurationChangedCallback::Create(
			this, &DisplaySubsystem::HandleDisplayConfigurationChanged));
	m_displayDevicesHandle = CoreDelegate::AddDisplayDevicesChangedCallback(
		CoreDelegate::DisplayDevicesChangedCallback::Create(
			this, &DisplaySubsystem::HandleDisplayDevicesChanged));
	m_displaySettingsHandle = CoreDelegate::AddDisplaySettingsChangedCallback(
		CoreDelegate::DisplaySettingsChangedCallback::Create(
			this, &DisplaySubsystem::HandleDisplaySettingsChanged));
	m_windowDisplayChangedHandle = CoreDelegate::AddWindowDisplayChangedCallback(
		CoreDelegate::WindowDisplayChangedCallback::Create(
			this, &DisplaySubsystem::HandleWindowDisplayChanged));
	m_windowMovedHandle = CoreDelegate::AddWindowMovedCallback(
		CoreDelegate::WindowMovedCallback::Create(
			this, &DisplaySubsystem::HandleWindowMoved));
	m_windowSizeChangedHandle = CoreDelegate::AddWindowSizeChangedCallback(
		CoreDelegate::WindowSizeChangedCallback::Create(
			this, &DisplaySubsystem::HandleWindowSizeChanged));
}

void DisplaySubsystem::UnregisterCallbacks()
{
	if (m_displayConfigurationHandle.IsValid())
	{
		CoreDelegate::RemoveDisplayConfigurationChangedCallback(m_displayConfigurationHandle);
		m_displayConfigurationHandle = DelegateHandle::Invalid();
	}

	if (m_displayDevicesHandle.IsValid())
	{
		CoreDelegate::RemoveDisplayDevicesChangedCallback(m_displayDevicesHandle);
		m_displayDevicesHandle = DelegateHandle::Invalid();
	}

	if (m_displaySettingsHandle.IsValid())
	{
		CoreDelegate::RemoveDisplaySettingsChangedCallback(m_displaySettingsHandle);
		m_displaySettingsHandle = DelegateHandle::Invalid();
	}

	if (m_windowDisplayChangedHandle.IsValid())
	{
		CoreDelegate::RemoveWindowDisplayChangedCallback(m_windowDisplayChangedHandle);
		m_windowDisplayChangedHandle = DelegateHandle::Invalid();
	}
	if (m_windowMovedHandle.IsValid())
	{
		CoreDelegate::RemoveWindowMovedCallback(m_windowMovedHandle);
		m_windowMovedHandle = DelegateHandle::Invalid();
	}
	if (m_windowSizeChangedHandle.IsValid())
	{
		CoreDelegate::RemoveWindowSizeChangedCallback(m_windowSizeChangedHandle);
		m_windowSizeChangedHandle = DelegateHandle::Invalid();
	}
}

void DisplaySubsystem::HandleDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info)
{
	RefreshDisplays();
	UpdateViewportDisplaySnapshotsForAll();
	UpdateHdrStatesForAllViewports();
}

void DisplaySubsystem::HandleDisplayDevicesChanged(const DisplayDevicesChangedInfo& info)
{
	RefreshDisplays();
	UpdateViewportDisplaySnapshotsForAll();
	UpdateHdrStatesForAllViewports();
}

void DisplaySubsystem::HandleDisplaySettingsChanged(const DisplaySettingsChangedInfo& info)
{
	RefreshDisplays();
	UpdateViewportDisplaySnapshotsForAll();
	UpdateHdrStatesForAllViewports();
}

void DisplaySubsystem::HandleWindowDisplayChanged(const WindowDisplayChangedInfo& info)
{
	UpdateViewportDisplaySnapshot(info.window);
	UpdateHdrState(info.window);
}

void DisplaySubsystem::HandleWindowMoved(const WindowMovedInfo& info)
{
	SnapWindowToNearestDisplay(info.window);
}

void DisplaySubsystem::HandleWindowSizeChanged(const WindowSizeChangedInfo& info)
{
	SnapWindowToNearestDisplay(info.window);
}

void DisplaySubsystem::UpdateHdrState(PlatformWindowHandle window)
{
	const DisplayInfo* displayInfo = FindActiveDisplayByWindow(window);
	if (!displayInfo)
	{
		return;
	}

	HdrStateCache& cache = m_hdrStates[window];
	const bool supportedChanged = cache.supported != displayInfo->hdrSupported;
	const bool activeChanged = cache.active != displayInfo->hdrActive;
	if (!cache.hasState || activeChanged)
	{
		cache.hasState = true;
		cache.supported = displayInfo->hdrSupported;
		cache.active = displayInfo->hdrActive;
		CoreDelegate::BroadcastHdrStateChanged({ window, displayInfo->hdrSupported, displayInfo->hdrActive });
	}
	else if (supportedChanged)
	{
		cache.supported = displayInfo->hdrSupported;
	}
}

void DisplaySubsystem::UpdateHdrStatesForAllViewports()
{
	if (!m_viewportSubsystem)
	{
		return;
	}

	const auto windows = m_viewportSubsystem->GetViewportWindows();
	for (const auto& window : windows)
	{
		UpdateHdrState(window);
	}
	CleanupHdrStates(windows);
}

void DisplaySubsystem::RefreshActiveDisplays()
{
	m_activeDisplays.clear();
	const auto& displays = DisplayManager::GetDisplays();
	for (const auto& display : displays)
	{
		if (display.isActive)
		{
			m_activeDisplays.push_back(display);
		}
	}
}

void DisplaySubsystem::BroadcastDisplayCacheRefreshed()
{
	DisplayCacheRefreshedInfo info{};
	info.displays = m_activeDisplays;
	CoreDelegate::BroadcastDisplayCacheRefreshed(info);
}

const DisplayInfo* DisplaySubsystem::FindActiveDisplayByMonitor(PlatformMonitorHandle monitor) const
{
	for (const auto& display : m_activeDisplays)
	{
		if (display.monitor == monitor)
		{
			return &display;
		}
	}
	return nullptr;
}

const DisplayInfo* DisplaySubsystem::FindActiveDisplayByWindow(PlatformWindowHandle window) const
{
	DisplayInfo display{};
	if (!DisplayManager::TryGetDisplayInfoFromWindow(window, display))
	{
		return nullptr;
	}
	return FindActiveDisplayByMonitor(display.monitor);
}

void DisplaySubsystem::UpdateViewportDisplaySnapshot(PlatformWindowHandle window)
{
	if (!m_viewportSubsystem)
	{
		return;
	}

	const DisplayInfo* display = FindActiveDisplayByWindow(window);
	if (!display)
	{
		return;
	}

	m_viewportSubsystem->UpdateViewportDisplaySnapshot(window, *display);
}

void DisplaySubsystem::UpdateViewportDisplaySnapshotsForAll()
{
	if (!m_viewportSubsystem)
	{
		return;
	}

	const auto windows = m_viewportSubsystem->GetViewportWindows();
	for (const auto& window : windows)
	{
		UpdateViewportDisplaySnapshot(window);
	}
}

void DisplaySubsystem::SnapWindowToNearestDisplay(PlatformWindowHandle window)
{
	const HWND nativeWindow = WindowsWindowHandle::FromPlatformHandle(window).handle;
	if (!nativeWindow)
	{
		return;
	}
	if (m_activeDisplays.size() < 2)
	{
		UpdateViewportDisplaySnapshot(window);
		return;
	}

	RECT windowRect{};
	if (!GetWindowRect(nativeWindow, &windowRect))
	{
		return;
	}

	int overlappingCount = 0;
	for (const auto& display : m_activeDisplays)
	{
		RECT intersection{};
		if (IntersectRect(&intersection, &windowRect, &display.monitorRect))
		{
			const int width = intersection.right - intersection.left;
			const int height = intersection.bottom - intersection.top;
			if (width > 0 && height > 0)
			{
				overlappingCount++;
				if (overlappingCount >= 2)
				{
					break;
				}
			}
		}
	}

	if (overlappingCount < 2)
	{
		UpdateViewportDisplaySnapshot(window);
		return;
	}

	const int windowWidth = windowRect.right - windowRect.left;
	const int windowHeight = windowRect.bottom - windowRect.top;
	const int windowCenterX = windowRect.left + windowWidth / 2;
	const int windowCenterY = windowRect.top + windowHeight / 2;

	const DisplayInfo* nearestDisplay = nullptr;
	long long bestDistance = 0;
	for (const auto& display : m_activeDisplays)
	{
		const int centerX = (display.workRect.left + display.workRect.right) / 2;
		const int centerY = (display.workRect.top + display.workRect.bottom) / 2;
		const long long dx = static_cast<long long>(centerX - windowCenterX);
		const long long dy = static_cast<long long>(centerY - windowCenterY);
		const long long dist = dx * dx + dy * dy;
		if (!nearestDisplay || dist < bestDistance)
		{
			nearestDisplay = &display;
			bestDistance = dist;
		}
	}

	if (!nearestDisplay)
	{
		return;
	}

	const int targetCenterX = (nearestDisplay->workRect.left + nearestDisplay->workRect.right) / 2;
	const int targetCenterY = (nearestDisplay->workRect.top + nearestDisplay->workRect.bottom) / 2;
	const int targetLeft = targetCenterX - windowWidth / 2;
	const int targetTop = targetCenterY - windowHeight / 2;

	SetWindowPos(nativeWindow, nullptr, targetLeft, targetTop, 0, 0,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	if (m_viewportSubsystem)
	{
		m_viewportSubsystem->RefreshViewportMonitor(window);
	}
	UpdateViewportDisplaySnapshot(window);
}

void DisplaySubsystem::CleanupHdrStates(const std::vector<PlatformWindowHandle>& activeWindows)
{
	for (auto it = m_hdrStates.begin(); it != m_hdrStates.end();)
	{
		const PlatformWindowHandle window = it->first;
		const bool stillActive = std::find(activeWindows.begin(), activeWindows.end(), window) != activeWindows.end();
		if (!stillActive)
		{
			it = m_hdrStates.erase(it);
		}
		else
		{
			++it;
		}
	}
}
