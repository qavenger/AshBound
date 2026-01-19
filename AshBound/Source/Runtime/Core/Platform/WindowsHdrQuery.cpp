#include "Runtime/Core/Platform/WindowsHdrQuery.h"

#include <cmath>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace
{
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

bool WindowsHdrQuery::TryQuery(const WindowsMonitorHandle& monitor, WindowsHdrInfo& outInfo)
{
	outInfo = {};

	DXGI_OUTPUT_DESC1 desc1 = {};
	if (!TryGetOutputDesc1ForMonitor(monitor.handle, desc1))
	{
		return false;
	}

	outInfo.active = desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
	outInfo.supported = outInfo.active || desc1.MaxLuminance > 0.0f;
	outInfo.maxLuminance = desc1.MaxLuminance;
	if (desc1.MinLuminance > 0.0f)
	{
		outInfo.minLuminanceLog10 = static_cast<float>(std::log10(desc1.MinLuminance));
	}
	return true;
}
