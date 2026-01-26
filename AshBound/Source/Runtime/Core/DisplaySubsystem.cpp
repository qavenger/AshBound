#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/Platform/WindowsEdidQuery.h"
#include "Runtime/Core/Platform/WindowsHdrQuery.h"
#include "Runtime/Core/Platform/WindowsMonitorHandle.h"
#include "Runtime/Core/Platform/WindowsWindowHandle.h"

#include <algorithm>
#include <cmath>
#include <ShellScalingApi.h>

namespace
{
	void AppendUniqueMode(std::vector<DisplayInfo::DisplayMode>& modes, const DisplayInfo::DisplayMode& mode)
	{
		for (const auto& existing : modes)
		{
			if (existing.width == mode.width &&
				existing.height == mode.height &&
				existing.refreshRate == mode.refreshRate)
			{
				return;
			}
		}
		modes.push_back(mode);
	}

	bool TryGetMonitorDpi(HMONITOR monitor, UINT& outDpiX, UINT& outDpiY)
	{
		outDpiX = 0;
		outDpiY = 0;

		if (!monitor)
		{
			return false;
		}

		HMODULE shcore = LoadLibraryW(L"Shcore.dll");
		if (!shcore)
		{
			return false;
		}

		using GetDpiForMonitorProc = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
		auto proc = reinterpret_cast<GetDpiForMonitorProc>(GetProcAddress(shcore, "GetDpiForMonitor"));
		if (!proc)
		{
			FreeLibrary(shcore);
			return false;
		}

		const HRESULT result = proc(monitor, MDT_EFFECTIVE_DPI, &outDpiX, &outDpiY);
		FreeLibrary(shcore);
		return SUCCEEDED(result);
	}

	HMONITOR ToNativeMonitor(const PlatformMonitorHandlePtr& handle)
	{
		if (!handle)
		{
			return nullptr;
		}
		return reinterpret_cast<HMONITOR>(handle->GetNativeHandle());
	}

	HWND ToNativeWindow(const PlatformWindowHandle& handle)
	{
		return WindowsWindowHandle::FromPlatformHandle(handle).handle;
	}
}

void DisplaySubsystem::Initialize()
{
	if (m_initialized)
	{
		return;
	}

	m_initialized = true;
	RegisterCallbacks();
	RefreshDisplayCache();
}

void DisplaySubsystem::Shutdown()
{
	if (!m_initialized)
	{
		return;
	}

	// Restore HDR state for all monitors modified by the engine
	for (auto& display : m_displays)
	{
		if (display.monitor && display.monitor->IsHdrModifiedByEngine())
		{
			const bool initialState = display.monitor->GetInitialHdrActive();
			if (display.hdrActive != initialState)
			{
				LOG(LogTemp, Info, L"Restoring HDR state to %s for monitor %s.",
					initialState ? L"enabled" : L"disabled",
					display.deviceName.c_str());
				display.monitor->TrySetHdrEnabled(initialState);
			}
		}
	}

	UnregisterCallbacks();
	m_displays.clear();
	m_initialized = false;
}

void DisplaySubsystem::RefreshDisplayCache()
{
	// Store old displays to detect changes
	std::vector<DisplayInfo> oldDisplays = std::move(m_displays);
	m_displays.clear();

	EnumDisplayMonitors(nullptr, nullptr, &DisplaySubsystem::EnumMonitorCallback,
		reinterpret_cast<LPARAM>(this));

	// Detect changes and broadcast appropriate events
	for (auto& newDisplay : m_displays)
	{
		const auto oldIt = std::find_if(oldDisplays.begin(), oldDisplays.end(),
			[&newDisplay](const DisplayInfo& old) {
				return old.monitor && newDisplay.monitor && old.monitor->Equals(newDisplay.monitor.get());
			});

		if (oldIt == oldDisplays.end())
		{
			// New display added - record initial HDR state
			if (newDisplay.monitor)
			{
				newDisplay.monitor->m_initialHdrActive = newDisplay.hdrActive;
				newDisplay.monitor->m_initialHdrStateValid = true;
			}
			BroadcastDisplayStateChanged(newDisplay.monitor, DisplayStateChangeType::DeviceAdded);
		}
		else
		{
			// Preserve HDR state tracking from old display
			if (oldIt->monitor && newDisplay.monitor)
			{
				newDisplay.monitor->m_initialHdrActive = oldIt->monitor->m_initialHdrActive;
				newDisplay.monitor->m_initialHdrStateValid = oldIt->monitor->m_initialHdrStateValid;
				newDisplay.monitor->m_hdrModifiedByEngine = oldIt->monitor->m_hdrModifiedByEngine;

				// Handle user manual HDR state changes
				if (oldIt->hdrActive != newDisplay.hdrActive)
				{
					HandleUserHdrStateChange(newDisplay, oldIt->hdrActive, newDisplay.hdrActive);
				}
			}

			// Check for HDR state change
			if (oldIt->hdrActive != newDisplay.hdrActive)
			{
				BroadcastDisplayStateChanged(newDisplay.monitor, DisplayStateChangeType::HdrStateChanged);
			}
			// Check for resolution change
			if (oldIt->monitorRect.right - oldIt->monitorRect.left != newDisplay.monitorRect.right - newDisplay.monitorRect.left ||
				oldIt->monitorRect.bottom - oldIt->monitorRect.top != newDisplay.monitorRect.bottom - newDisplay.monitorRect.top)
			{
				BroadcastDisplayStateChanged(newDisplay.monitor, DisplayStateChangeType::ResolutionChanged);
			}
		}
	}

	// Check for removed displays
	for (const auto& oldDisplay : oldDisplays)
	{
		const auto newIt = std::find_if(m_displays.begin(), m_displays.end(),
			[&oldDisplay](const DisplayInfo& newDisp) {
				return newDisp.monitor && oldDisplay.monitor && newDisp.monitor->Equals(oldDisplay.monitor.get());
			});

		if (newIt == m_displays.end())
		{
			BroadcastDisplayStateChanged(oldDisplay.monitor, DisplayStateChangeType::DeviceRemoved);
		}
	}

	BroadcastDisplayCacheRefreshed();
}

const std::vector<DisplayInfo>& DisplaySubsystem::GetDisplays() const
{
	return m_displays;
}

const DisplayInfo* DisplaySubsystem::GetDisplayInfoForWindow(PlatformWindowHandle window) const
{
	if (!window.IsValid())
	{
		return nullptr;
	}

	const HWND nativeWindow = ToNativeWindow(window);
	const HMONITOR monitor = MonitorFromWindow(nativeWindow, MONITOR_DEFAULTTONEAREST);
	if (!monitor)
	{
		return nullptr;
	}

	// Iterate through cached displays and find the matching one
	for (const auto& display : m_displays)
	{
		if (display.monitor && ToNativeMonitor(display.monitor) == monitor)
		{
			return &display;
		}
	}

	return nullptr;
}

const DisplayInfo* DisplaySubsystem::GetDisplayInfo(const PlatformMonitorHandlePtr& monitor) const
{
	if (!monitor || !monitor->IsValid())
	{
		return nullptr;
	}

	for (const auto& display : m_displays)
	{
		if (display.monitor && display.monitor->Equals(monitor.get()))
		{
			return &display;
		}
	}

	return nullptr;
}

const DisplayInfo* DisplaySubsystem::GetPrimaryDisplayInfo() const
{
	for (const auto& display : m_displays)
	{
		if (display.isPrimary)
		{
			return &display;
		}
	}

	// Fallback: return first display if no primary found
	if (!m_displays.empty())
	{
		return &m_displays[0];
	}

	return nullptr;
}

bool DisplaySubsystem::TryEnableHdr(const PlatformMonitorHandlePtr& monitor)
{
	if (!monitor || !monitor->IsValid())
	{
		LOG(LogTemp, Warning, L"TryEnableHdr: Invalid monitor handle.");
		return false;
	}

	// Record initial state if not already recorded
	if (!monitor->m_initialHdrStateValid)
	{
		const DisplayInfo* info = GetDisplayInfo(monitor);
		if (info)
		{
			monitor->m_initialHdrActive = info->hdrActive;
			monitor->m_initialHdrStateValid = true;
		}
	}

	const bool success = monitor->TrySetHdrEnabled(true);
	if (success)
	{
		monitor->m_hdrModifiedByEngine = true;
		// Refresh display cache to pick up the new HDR state
		RefreshDisplayCache();
	}
	return success;
}

bool DisplaySubsystem::TryDisableHdr(const PlatformMonitorHandlePtr& monitor)
{
	if (!monitor || !monitor->IsValid())
	{
		LOG(LogTemp, Warning, L"TryDisableHdr: Invalid monitor handle.");
		return false;
	}

	const bool success = monitor->TrySetHdrEnabled(false);
	if (success)
	{
		// Refresh display cache to pick up the new HDR state
		RefreshDisplayCache();
	}
	return success;
}

bool DisplaySubsystem::RefreshDisplayForWindow(PlatformWindowHandle window, DisplayStateChangeType changeHint)
{
	if (!TryRefreshDisplayFromWindow(window, changeHint))
	{
		RefreshDisplayCache();
		return false;
	}
	return true;
}

BOOL CALLBACK DisplaySubsystem::EnumMonitorCallback(HMONITOR monitor, HDC, LPRECT, LPARAM data)
{
	auto* subsystem = reinterpret_cast<DisplaySubsystem*>(data);
	if (!subsystem)
	{
		return TRUE;
	}

	DisplayInfo info{};
	if (subsystem->TryQueryDisplayInfo(monitor, info))
	{
		subsystem->m_displays.push_back(std::move(info));
	}
	return TRUE;
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
}

void DisplaySubsystem::HandleDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info)
{
	if (!TryRefreshDisplayFromWindow(info.window, DisplayStateChangeType::ResolutionChanged))
	{
		RefreshDisplayCache();
	}
}

void DisplaySubsystem::HandleDisplayDevicesChanged(const DisplayDevicesChangedInfo& info)
{
	if (!TryRefreshDisplayFromWindow(info.window, DisplayStateChangeType::DeviceAdded | DisplayStateChangeType::DeviceRemoved))
	{
		RefreshDisplayCache();
	}
}

void DisplaySubsystem::HandleDisplaySettingsChanged(const DisplaySettingsChangedInfo& info)
{
	if (!TryRefreshDisplayFromWindow(info.window, DisplayStateChangeType::HdrStateChanged))
	{
		RefreshDisplayCache();
	}
}

void DisplaySubsystem::BroadcastDisplayCacheRefreshed()
{
	DisplayCacheRefreshedInfo info{};
	info.displays = m_displays;
	CoreDelegate::BroadcastDisplayCacheRefreshed(info);
}

void DisplaySubsystem::BroadcastDisplayStateChanged(const PlatformMonitorHandlePtr& monitor, DisplayStateChangeType changeType)
{
	DisplayStateChangedInfo info{};
	info.monitor = monitor;
	info.changeType = changeType;
	CoreDelegate::BroadcastDisplayStateChanged(info);
}

bool DisplaySubsystem::TryQueryDisplayInfo(HMONITOR nativeMonitor, DisplayInfo& outInfo) const
{
	outInfo = {};
	if (!nativeMonitor)
	{
		return false;
	}

	MONITORINFOEXW monitorInfo = {};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (!GetMonitorInfoW(nativeMonitor, &monitorInfo))
	{
		return false;
	}

	outInfo.monitor = WindowsMonitorHandle::Create(nativeMonitor);
	outInfo.deviceName = monitorInfo.szDevice;
	outInfo.monitorRect = monitorInfo.rcMonitor;
	outInfo.workRect = monitorInfo.rcWork;
	outInfo.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

	std::wstring deviceKey;
	std::wstring deviceId;
	DISPLAY_DEVICEW displayDevice = {};
	displayDevice.cb = sizeof(displayDevice);
	if (EnumDisplayDevicesW(monitorInfo.szDevice, 0, &displayDevice, 0))
	{
		outInfo.friendlyName = displayDevice.DeviceString;
		deviceKey = displayDevice.DeviceKey;
		deviceId = displayDevice.DeviceID;
	}

	for (DWORD modeIndex = 0;; ++modeIndex)
	{
		DEVMODEW devMode = {};
		devMode.dmSize = sizeof(devMode);
		if (!EnumDisplaySettingsW(monitorInfo.szDevice, modeIndex, &devMode))
		{
			break;
		}

		DisplayInfo::DisplayMode mode{};
		mode.width = static_cast<int>(devMode.dmPelsWidth);
		mode.height = static_cast<int>(devMode.dmPelsHeight);
		mode.refreshRate = static_cast<int>(devMode.dmDisplayFrequency);
		AppendUniqueMode(outInfo.supportedModes, mode);
	}

	UINT dpiX = 0;
	UINT dpiY = 0;
	if (TryGetMonitorDpi(nativeMonitor, dpiX, dpiY))
	{
		outInfo.dpiX = dpiX;
		outInfo.dpiY = dpiY;
	}

	EdidHdrInfo edidInfo{};
	if (WindowsEdidQuery::TryQuery(deviceKey, deviceId, edidInfo))
	{
		outInfo.hdrSupported = edidInfo.hdrSupported;
		outInfo.hdr10PlusSupported = edidInfo.hdr10PlusSupported;
		outInfo.dolbyVisionSupported = edidInfo.dolbyVisionSupported;
		if (edidInfo.maxLuminance > 0.0f)
		{
			outInfo.maxLuminance = edidInfo.maxLuminance;
		}
		if (edidInfo.minLuminance > 0.0f)
		{
			outInfo.minLuminanceLog10 = static_cast<float>(std::log10(edidInfo.minLuminance));
		}
	}

	WindowsHdrInfo hdrInfo{};
	if (WindowsHdrQuery::TryQuery(nativeMonitor, hdrInfo))
	{
		if (hdrInfo.activeKnown)
		{
			outInfo.hdrActive = hdrInfo.active;
			if (hdrInfo.active && !outInfo.hdrSupported)
			{
				LOG(LogTemp, Warning, L"HDR is active in Windows but EDID does not report HDR support for %s.",
					outInfo.deviceName.c_str());
			}
		}
		else
		{
			LOG(LogTemp, Warning, L"HDR active state unknown for display %s.", outInfo.deviceName.c_str());
		}
		outInfo.sdrWhiteLevelNits = hdrInfo.sdrWhiteLevelNits;
	}

	if (outInfo.maxLuminance <= 0.0f && outInfo.sdrWhiteLevelNits > 0.0f)
	{
		outInfo.maxLuminance = outInfo.sdrWhiteLevelNits;
	}
	if (outInfo.maxLuminance <= 0.0f)
	{
		LOG(LogTemp, Warning, L"Display %s has no luminance data; defaulting max luminance to 100 nits.",
			outInfo.deviceName.c_str());
		outInfo.maxLuminance = 100.0f;
	}

	return true;
}

bool DisplaySubsystem::TryRefreshDisplayFromWindow(PlatformWindowHandle window, DisplayStateChangeType changeHint)
{
	if (!window.IsValid())
	{
		return false;
	}

	const HWND nativeWindow = ToNativeWindow(window);
	const HMONITOR monitor = MonitorFromWindow(nativeWindow, MONITOR_DEFAULTTONEAREST);
	if (!monitor)
	{
		return false;
	}

	DisplayInfo updatedInfo{};
	if (!TryQueryDisplayInfo(monitor, updatedInfo))
	{
		return false;
	}

	const PlatformMonitorHandlePtr monitorHandle = updatedInfo.monitor;
	bool found = false;
	for (auto& display : m_displays)
	{
		if (display.monitor && monitorHandle && display.monitor->Equals(monitorHandle.get()))
		{
			// Preserve HDR state tracking
			if (display.monitor)
			{
				updatedInfo.monitor->m_initialHdrActive = display.monitor->m_initialHdrActive;
				updatedInfo.monitor->m_initialHdrStateValid = display.monitor->m_initialHdrStateValid;
				updatedInfo.monitor->m_hdrModifiedByEngine = display.monitor->m_hdrModifiedByEngine;

				// Handle user manual HDR state changes
				if (display.hdrActive != updatedInfo.hdrActive)
				{
					HandleUserHdrStateChange(updatedInfo, display.hdrActive, updatedInfo.hdrActive);
				}
			}
			display = std::move(updatedInfo);
			found = true;
			break;
		}
	}
	if (!found)
	{
		// New display - record initial HDR state
		if (updatedInfo.monitor)
		{
			updatedInfo.monitor->m_initialHdrActive = updatedInfo.hdrActive;
			updatedInfo.monitor->m_initialHdrStateValid = true;
		}
		m_displays.push_back(std::move(updatedInfo));
	}

	BroadcastDisplayStateChanged(monitorHandle, changeHint);
	BroadcastDisplayCacheRefreshed();
	return true;
}

void DisplaySubsystem::HandleUserHdrStateChange(DisplayInfo& display, bool oldHdrActive, bool newHdrActive)
{
	if (!display.monitor)
	{
		return;
	}

	// Scenario A: User manually enabled HDR (engine didn't modify it)
	if (!display.monitor->m_hdrModifiedByEngine && newHdrActive && !oldHdrActive)
	{
		// User turned on HDR themselves - update initial state to respect their intent
		LOG(LogTemp, Info, L"User manually enabled HDR on %s. Updating initial state.",
			display.deviceName.c_str());
		display.monitor->m_initialHdrActive = true;
	}
	// Scenario B: User manually disabled HDR (after engine enabled it)
	else if (display.monitor->m_hdrModifiedByEngine && !newHdrActive && oldHdrActive)
	{
		// User turned off HDR that the engine had enabled - respect their intent
		LOG(LogTemp, Info, L"User manually disabled HDR on %s. Clearing engine modification flag.",
			display.deviceName.c_str());
		display.monitor->m_hdrModifiedByEngine = false;
		display.monitor->m_initialHdrActive = false;

		// Broadcast a special notification for Viewport to degrade HdrPreference
		// This is handled by the HdrStateChanged event, Viewport should check and degrade
	}
}
