#include "Runtime/Core/Platform/WindowsMonitorHandle.h"
#include "Runtime/Core/Log.h"

#include <vector>

DECLARE_LOG_CATEGORY_EXTERN(LogWindowsMonitor, Info)
DEFINE_LOG_CATEGORY(LogWindowsMonitor, Info)

namespace
{
	bool TryGetDisplayConfigTarget(HMONITOR monitor, DISPLAYCONFIG_PATH_INFO& outPath)
	{
		if (!monitor)
		{
			return false;
		}

		MONITORINFOEXW monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (!GetMonitorInfoW(monitor, &monitorInfo))
		{
			return false;
		}

		UINT32 pathCount = 0;
		UINT32 modeCount = 0;
		const LONG sizeResult = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
		if (sizeResult != ERROR_SUCCESS)
		{
			LOG(LogWindowsMonitor, Warning, L"GetDisplayConfigBufferSizes failed with code %ld.", sizeResult);
			return false;
		}

		std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
		const LONG queryResult = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
		if (queryResult != ERROR_SUCCESS)
		{
			LOG(LogWindowsMonitor, Warning, L"QueryDisplayConfig (paths) failed with code %ld.", queryResult);
			return false;
		}

		for (const auto& path : paths)
		{
			DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
			sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			sourceName.header.size = sizeof(sourceName);
			sourceName.header.adapterId = path.sourceInfo.adapterId;
			sourceName.header.id = path.sourceInfo.id;
			const LONG deviceInfoResult = DisplayConfigGetDeviceInfo(&sourceName.header);
			if (deviceInfoResult != ERROR_SUCCESS)
			{
				continue;
			}

			if (wcscmp(sourceName.viewGdiDeviceName, monitorInfo.szDevice) == 0)
			{
				outPath = path;
				return true;
			}
		}

		return false;
	}
}

WindowsMonitorHandle::WindowsMonitorHandle(HMONITOR handle)
	: m_handle(handle)
{
}

bool WindowsMonitorHandle::IsValid() const
{
	return m_handle != nullptr;
}

void* WindowsMonitorHandle::GetNativeHandle() const
{
	return reinterpret_cast<void*>(m_handle);
}

bool WindowsMonitorHandle::Equals(const PlatformMonitorHandle* other) const
{
	if (!other)
	{
		return false;
	}
	return GetNativeHandle() == other->GetNativeHandle();
}

size_t WindowsMonitorHandle::GetHash() const
{
	return std::hash<void*>{}(reinterpret_cast<void*>(m_handle));
}

PlatformMonitorHandlePtr WindowsMonitorHandle::Create(HMONITOR handle)
{
	return std::make_shared<WindowsMonitorHandle>(handle);
}

bool WindowsMonitorHandle::TrySetHdrEnabled(bool enable)
{
	if (!m_handle)
	{
		LOG(LogWindowsMonitor, Warning, L"TrySetHdrEnabled: Invalid monitor handle.");
		return false;
	}

	DISPLAYCONFIG_PATH_INFO path = {};
	if (!TryGetDisplayConfigTarget(m_handle, path))
	{
		LOG(LogWindowsMonitor, Warning, L"TrySetHdrEnabled: Failed to get display config target for monitor %p.", m_handle);
		return false;
	}

	// Use DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE to enable/disable HDR
	DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE setColorState = {};
	setColorState.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
	setColorState.header.size = sizeof(setColorState);
	setColorState.header.adapterId = path.targetInfo.adapterId;
	setColorState.header.id = path.targetInfo.id;
	setColorState.enableAdvancedColor = enable ? TRUE : FALSE;

	const LONG result = DisplayConfigSetDeviceInfo(&setColorState.header);
	if (result != ERROR_SUCCESS)
	{
		LOG(LogWindowsMonitor, Warning, L"DisplayConfigSetDeviceInfo failed with code %ld for monitor %p (enable=%s).",
			result, m_handle, enable ? L"true" : L"false");
		return false;
	}

	LOG(LogWindowsMonitor, Info, L"Successfully %s HDR on monitor %p.",
		enable ? L"enabled" : L"disabled", m_handle);
	return true;
}
