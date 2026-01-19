#include "Runtime/Rendering/DX12/DX12Device.h"

#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstring>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace
{
	DXGI_FORMAT ToDxgiFormat(RHITextureFormat format)
	{
		switch (format)
		{
		case RHITextureFormat::RGBA8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case RHITextureFormat::BGRA8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case RHITextureFormat::R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case RHITextureFormat::R16_FLOAT:
			return DXGI_FORMAT_R16_FLOAT;
		case RHITextureFormat::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case RHITextureFormat::D24S8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		default:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	D3D12_RESOURCE_FLAGS GetTextureFlags(const RHITextureDesc& desc)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.renderTarget)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (desc.depthStencil)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (desc.unorderedAccess)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		return flags;
	}
}

bool DX12Device::Initialize(bool enableDebugLayer)
{
	if (m_device)
	{
		return true;
	}

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory;
	UINT factoryFlags = 0;
	if (enableDebugLayer)
	{
		factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory))))
	{
		return false;
	}

	for (UINT adapterIndex = 0;
		factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc = {};
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter.Reset();
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
		{
			break;
		}
		adapter.Reset();
	}

	if (!m_device)
	{
		if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
		{
			return false;
		}
	}

	return true;
}

void DX12Device::Shutdown()
{
	m_device.Reset();
}

ID3D12Device* DX12Device::GetDevice() const
{
	return m_device.Get();
}

void* DX12Device::GetNativeDevice() const
{
	return m_device.Get();
}

std::shared_ptr<IRHIBuffer> DX12Device::CreateBuffer(const RHIBufferDesc& desc, const void* initialData)
{
	if (!m_device || desc.sizeInBytes == 0)
	{
		return {};
	}

	const bool cpuWrite = desc.cpuAccess == RHICpuAccess::Write || desc.usage == RHIBufferUsage::Constant;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = cpuWrite ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = desc.sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	const D3D12_RESOURCE_STATES initialState = cpuWrite ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		initialState, nullptr, IID_PPV_ARGS(&resource))))
	{
		return {};
	}

	if (initialData && cpuWrite)
	{
		void* mapped = nullptr;
		D3D12_RANGE range = { 0, 0 };
		if (SUCCEEDED(resource->Map(0, &range, &mapped)))
		{
			memcpy(mapped, initialData, static_cast<size_t>(desc.sizeInBytes));
			resource->Unmap(0, nullptr);
		}
	}

	return std::make_shared<DX12Buffer>(desc, resource);
}

std::shared_ptr<IRHIBuffer> DX12Device::CreateConstantBuffer(uint64_t sizeInBytes, const void* initialData)
{
	const uint64_t alignedSize = (sizeInBytes + 255u) & ~255u;
	RHIBufferDesc desc = {};
	desc.sizeInBytes = alignedSize;
	desc.strideInBytes = 0;
	desc.usage = RHIBufferUsage::Constant;
	desc.cpuAccess = RHICpuAccess::Write;
	return CreateBuffer(desc, initialData);
}

std::shared_ptr<IRHITexture> DX12Device::CreateTexture2D(const RHITextureDesc& desc)
{
	if (!m_device || desc.width == 0 || desc.height == 0)
	{
		return {};
	}

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = desc.width;
	texDesc.Height = desc.height;
	texDesc.DepthOrArraySize = desc.arrayLayers;
	texDesc.MipLevels = desc.mipLevels;
	texDesc.Format = ToDxgiFormat(desc.format);
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = GetTextureFlags(desc);

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	if (desc.renderTarget)
	{
		initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (desc.depthStencil)
	{
		initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		initialState, nullptr, IID_PPV_ARGS(&resource))))
	{
		return {};
	}

	return std::make_shared<DX12Texture>(desc, resource);
}

std::shared_ptr<IRHIShader> DX12Device::CreateShader(const RHIShaderDesc& desc)
{
	if (desc.bytecode.empty())
	{
		return {};
	}
	return std::make_shared<DX12Shader>(desc);
}
