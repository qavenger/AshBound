#pragma once

#include "Runtime/Rendering/RHI/RHIEnum.h"

#include <dxgi1_6.h>

namespace DX12Format
{
	inline DXGI_FORMAT ToDxgiFormat(RHIEnum::Format format)
	{
		switch (format)
		{
		case RHIEnum::Format::R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case RHIEnum::Format::R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case RHIEnum::Format::RGBA8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case RHIEnum::Format::BGRA8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case RHIEnum::Format::R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case RHIEnum::Format::RGBA16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case RHIEnum::Format::R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case RHIEnum::Format::R16_FLOAT:
			return DXGI_FORMAT_R16_FLOAT;
		case RHIEnum::Format::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case RHIEnum::Format::D24S8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case RHIEnum::Format::D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		case RHIEnum::Format::Unknown:
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}
}
