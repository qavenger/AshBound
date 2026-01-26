#include "Runtime/Core/Platform/WindowsHdrQuery.h"
#include "Runtime/Core/Log.h"

#include <Windows.h>
#include <cmath>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

namespace
{
	bool TryGetDisplayConfigTarget(const HMONITOR monitor, DISPLAYCONFIG_PATH_INFO& outPath)
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
			LOG(LogTemp, Warning, L"GetDisplayConfigBufferSizes failed with code %ld.", sizeResult);
			return false;
		}

		std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
		const LONG queryResult = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
		if (queryResult != ERROR_SUCCESS)
		{
			LOG(LogTemp, Warning, L"QueryDisplayConfig (paths) failed with code %ld.", queryResult);
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
				LOG(LogTemp, Warning, L"DisplayConfigGetDeviceInfo failed with code %ld for source path.", deviceInfoResult);
				continue;
			}

			if (wcscmp(sourceName.viewGdiDeviceName, monitorInfo.szDevice) == 0)
			{
				outPath = path;
				return true;
			}
		}

		LOG(LogTemp, Warning, L"No display config path matched monitor device %s.", monitorInfo.szDevice);
		return false;
	}

	bool TryGetAdvancedColorInfo(const DISPLAYCONFIG_PATH_INFO& path, DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO& outInfo)
	{
		outInfo = {};
		outInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
		outInfo.header.size = sizeof(outInfo);
		outInfo.header.adapterId = path.targetInfo.adapterId;
		outInfo.header.id = path.targetInfo.id;
		return DisplayConfigGetDeviceInfo(&outInfo.header) == ERROR_SUCCESS;
	}

	bool TryGetSdrWhiteLevel(const DISPLAYCONFIG_PATH_INFO& path, float& outSdrWhiteLevelNits)
	{
		outSdrWhiteLevelNits = 0.0f;
		DISPLAYCONFIG_SDR_WHITE_LEVEL sdrWhite = {};
		sdrWhite.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
		sdrWhite.header.size = sizeof(sdrWhite);
		sdrWhite.header.adapterId = path.targetInfo.adapterId;
		sdrWhite.header.id = path.targetInfo.id;
		if (DisplayConfigGetDeviceInfo(&sdrWhite.header) != ERROR_SUCCESS)
		{
			return false;
		}
		outSdrWhiteLevelNits = (static_cast<float>(sdrWhite.SDRWhiteLevel) / 1000.0f) * 80.0f;
		return true;
	}

	bool TryGetOutputDesc1ForMonitor(HMONITOR monitor, DXGI_OUTPUT_DESC1& outDesc)
	{
		using Microsoft::WRL::ComPtr;

		if (!monitor)
		{
			return false;
		}

		ComPtr<IDXGIFactory1> factory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
		{
			return false;
		}

		for (UINT adapterIndex = 0;; ++adapterIndex)
		{
			ComPtr<IDXGIAdapter1> adapter;
			if (factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			for (UINT outputIndex = 0;; ++outputIndex)
			{
				ComPtr<IDXGIOutput> output;
				if (adapter->EnumOutputs(outputIndex, &output) == DXGI_ERROR_NOT_FOUND)
				{
					break;
				}

				DXGI_OUTPUT_DESC desc = {};
				if (FAILED(output->GetDesc(&desc)))
				{
					continue;
				}

				if (desc.Monitor != monitor)
				{
					continue;
				}

				ComPtr<IDXGIOutput6> output6;
				if (FAILED(output.As(&output6)))
				{
					return false;
				}

				return SUCCEEDED(output6->GetDesc1(&outDesc));
			}
		}

		return false;
	}
}

bool WindowsHdrQuery::TryQuery(HMONITOR monitor, WindowsHdrInfo& outInfo)
{
	outInfo = {};

	DISPLAYCONFIG_PATH_INFO path = {};
	if (!TryGetDisplayConfigTarget(monitor, path))
	{
		LOG(LogTemp, Warning, L"Failed to resolve display config path for monitor %p.", monitor);
		return false;
	}

	DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO advancedInfo = {};
	if (TryGetAdvancedColorInfo(path, advancedInfo))
	{
		outInfo.supported = advancedInfo.advancedColorSupported != 0;
		const bool advancedEnabled = advancedInfo.advancedColorEnabled != 0;
		const bool wideColorEnforced = advancedInfo.wideColorEnforced != 0;
		outInfo.active = advancedEnabled && !wideColorEnforced;
		outInfo.activeKnown = true;
	}
	else
	{
		LOG(LogTemp, Warning, L"Failed to query advanced color info for monitor %p.", monitor);
	}

	TryGetSdrWhiteLevel(path, outInfo.sdrWhiteLevelNits);

	DXGI_OUTPUT_DESC1 desc1 = {};
	if (TryGetOutputDesc1ForMonitor(monitor, desc1))
	{
		outInfo.maxLuminance = desc1.MaxLuminance;
		if (desc1.MinLuminance > 0.0f)
		{
			outInfo.minLuminanceLog10 = static_cast<float>(std::log10(desc1.MinLuminance));
		}
	}
	return true;
}
