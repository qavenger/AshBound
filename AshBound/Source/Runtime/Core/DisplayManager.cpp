#include "Runtime/Core/DisplayManager.h"

#include "Runtime/Core/Platform/WindowsHdrQuery.h"
#include "Runtime/Core/Platform/WindowsMonitorHandle.h"
#include "Runtime/Core/Platform/WindowsWindowHandle.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <ShellScalingApi.h>

namespace
{
	std::vector<DisplayInfo> s_displays;
	PlatformWindowHandle s_referenceWindow = {};
	bool s_initialized = false;

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

	HMONITOR ToNativeMonitor(const PlatformMonitorHandle& handle)
	{
		return WindowsMonitorHandle::FromPlatformHandle(handle).handle;
	}

	HWND ToNativeWindow(const PlatformWindowHandle& handle)
	{
		return WindowsWindowHandle::FromPlatformHandle(handle).handle;
	}

	bool IsDisplaySourceActive(const wchar_t* deviceName)
	{
		if (!deviceName)
		{
			return false;
		}

		UINT32 pathCount = 0;
		UINT32 modeCount = 0;
		if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS)
		{
			return false;
		}

		std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(),
			&modeCount, modes.data(), nullptr) != ERROR_SUCCESS)
		{
			return false;
		}

		for (UINT32 i = 0; i < pathCount; ++i)
		{
			DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
			sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			sourceName.header.size = sizeof(sourceName);
			sourceName.header.adapterId = paths[i].sourceInfo.adapterId;
			sourceName.header.id = paths[i].sourceInfo.id;
			if (DisplayConfigGetDeviceInfo(&sourceName.header) == ERROR_SUCCESS)
			{
				if (_wcsicmp(sourceName.viewGdiDeviceName, deviceName) == 0)
				{
					return true;
				}
			}
		}

		return false;
	}

	BOOL CALLBACK EnumMonitorCallback(HMONITOR monitor, HDC, LPRECT, LPARAM)
	{
		DisplayInfo info{};
		if (DisplayManager::TryGetDisplayInfoFromMonitor(WindowsMonitorHandle{ monitor }.ToPlatformHandle(), info))
		{
			s_displays.push_back(std::move(info));
		}
		return TRUE;
	}
}

void DisplayManager::Initialize(PlatformWindowHandle referenceWindow)
{
	if (!s_initialized)
	{
		s_initialized = true;
		s_referenceWindow = referenceWindow;
		Refresh();
		return;
	}

	if (!s_referenceWindow.IsValid() && referenceWindow.IsValid())
	{
		s_referenceWindow = referenceWindow;
	}
}

void DisplayManager::Refresh()
{
	s_displays.clear();
	EnumDisplayMonitors(nullptr, nullptr, EnumMonitorCallback, 0);

	if (s_referenceWindow.IsValid())
	{
		DisplayInfo windowInfo{};
		if (DisplayManager::TryGetDisplayInfoFromWindow(s_referenceWindow, windowInfo))
		{
			for (auto& info : s_displays)
			{
				if (info.monitor == windowInfo.monitor)
				{
					info.dpiX = windowInfo.dpiX;
					info.dpiY = windowInfo.dpiY;
					break;
				}
			}
		}
	}
}

const std::vector<DisplayInfo>& DisplayManager::GetDisplays()
{
	return s_displays;
}

const DisplayInfo* DisplayManager::FindDisplayByMonitor(PlatformMonitorHandle monitor)
{
	const auto it = std::find_if(s_displays.begin(), s_displays.end(),
		[monitor](const DisplayInfo& info)
		{
			return info.monitor == monitor;
		});
	return it != s_displays.end() ? &(*it) : nullptr;
}

bool DisplayManager::HasMonitor(PlatformMonitorHandle monitor)
{
	return FindDisplayByMonitor(monitor) != nullptr;
}

bool DisplayManager::IsMonitorActive(PlatformMonitorHandle monitor)
{
	const auto* info = FindDisplayByMonitor(monitor);
	return info ? info->isActive : false;
}

PlatformMonitorHandle DisplayManager::GetPrimaryMonitor()
{
	for (const auto& info : s_displays)
	{
		if (info.isPrimary)
		{
			return info.monitor;
		}
	}
	return WindowsMonitorHandle{ MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY) }.ToPlatformHandle();
}

PlatformMonitorHandle DisplayManager::GetFirstActiveMonitor()
{
	for (const auto& info : s_displays)
	{
		if (info.isActive)
		{
			return info.monitor;
		}
	}
	return GetPrimaryMonitor();
}

bool DisplayManager::TryGetDisplayInfoFromMonitor(PlatformMonitorHandle monitor, DisplayInfo& outInfo)
{
	outInfo = {};
	if (!monitor.IsValid())
	{
		return false;
	}

	const HMONITOR nativeMonitor = ToNativeMonitor(monitor);
	MONITORINFOEXW monitorInfo = {};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (!GetMonitorInfoW(nativeMonitor, &monitorInfo))
	{
		return false;
	}

	outInfo.monitor = monitor;
	outInfo.deviceName = monitorInfo.szDevice;
	outInfo.monitorRect = monitorInfo.rcMonitor;
	outInfo.workRect = monitorInfo.rcWork;
	outInfo.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
	outInfo.isActive = IsDisplaySourceActive(monitorInfo.szDevice);

	DISPLAY_DEVICEW displayDevice = {};
	displayDevice.cb = sizeof(displayDevice);
	if (EnumDisplayDevicesW(monitorInfo.szDevice, 0, &displayDevice, 0))
	{
		outInfo.friendlyName = displayDevice.DeviceString;
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

	WindowsHdrInfo hdrInfo{};
	if (WindowsHdrQuery::TryQuery(WindowsMonitorHandle{ nativeMonitor }, hdrInfo))
	{
		outInfo.hdrActive = hdrInfo.active;
		outInfo.hdrSupported = hdrInfo.supported;
		outInfo.maxLuminance = hdrInfo.maxLuminance;
		outInfo.minLuminanceLog10 = hdrInfo.minLuminanceLog10;
	}

	return true;
}

bool DisplayManager::TryGetDisplayInfoFromWindow(PlatformWindowHandle window, DisplayInfo& outInfo)
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

	if (!TryGetDisplayInfoFromMonitor(WindowsMonitorHandle{ monitor }.ToPlatformHandle(), outInfo))
	{
		return false;
	}

	const UINT dpi = GetDpiForWindow(nativeWindow);
	outInfo.dpiX = dpi;
	outInfo.dpiY = dpi;
	return true;
}
