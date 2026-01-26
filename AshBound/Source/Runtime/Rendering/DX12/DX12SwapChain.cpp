#include "Runtime/Rendering/DX12/DX12SwapChain.h"
#include "Runtime/Rendering/DX12/DX12Format.h"

using Microsoft::WRL::ComPtr;

bool DX12SwapChain::Initialize(HWND windowHandle,
	IDXGIFactory4* factory,
	ID3D12Device* device,
	ID3D12CommandQueue* commandQueue,
	uint32_t width,
	uint32_t height,
	RHIEnum::Format format,
	RHIEnum::ColorSpace colorSpace,
	uint32_t bufferCount)
{
	if (!windowHandle || !factory || !device || !commandQueue)
	{
		return false;
	}

	m_device = device;
	m_width = width;
	m_height = height;
	m_bufferCount = bufferCount;
	m_format = DX12Format::ToDxgiFormat(format);
	m_colorSpace = colorSpace;
	m_hdrMetaInitialized = false;

	DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
	swapDesc.Width = width;
	swapDesc.Height = height;
	swapDesc.Format = m_format;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferCount = bufferCount;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	ComPtr<IDXGISwapChain1> swapChain1;
	if (FAILED(factory->CreateSwapChainForHwnd(commandQueue, windowHandle, &swapDesc, nullptr, nullptr, &swapChain1)))
	{
		return false;
	}
	
	factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);

	if (FAILED(swapChain1.As(&m_swapChain)))
	{
		return false;
	}

	ApplyColorSpace(colorSpace);
	CreateRenderTargetViews(device);
	return true;
}

void DX12SwapChain::Shutdown()
{
	m_backBuffers.clear();
	m_rtvHeap.Reset();
	m_swapChain.Reset();
	m_hdrMetaInitialized = false;
}

void* DX12SwapChain::GetNativeSwapChain() const
{
	return m_swapChain.Get();
}

uint32_t DX12SwapChain::GetCurrentBackBufferIndex() const
{
	return m_swapChain ? m_swapChain->GetCurrentBackBufferIndex() : 0;
}

void DX12SwapChain::Present(bool vsync)
{
	if (m_swapChain)
	{
		m_swapChain->Present(vsync ? 1 : 0, 0);
	}
}

void DX12SwapChain::Resize(uint32_t width, uint32_t height)
{
	if (!m_swapChain || !m_device || width == 0 || height == 0)
	{
		return;
	}

	m_backBuffers.clear();

	if (SUCCEEDED(m_swapChain->ResizeBuffers(m_bufferCount, width, height, m_format, 0)))
	{
		m_width = width;
		m_height = height;
		ApplyColorSpace(m_colorSpace);
		CreateRenderTargetViews(m_device);
	}
}

bool DX12SwapChain::QueryOutputLuminance(float& outMaxNits, float& outMinNits) const
{
	outMaxNits = 0.0f;
	outMinNits = 0.0f;
	if (!m_swapChain)
	{
		return false;
	}

	ComPtr<IDXGIOutput> output;
	if (FAILED(m_swapChain->GetContainingOutput(&output)) || !output)
	{
		return false;
	}

	ComPtr<IDXGIOutput6> output6;
	if (FAILED(output.As(&output6)) || !output6)
	{
		return false;
	}

	DXGI_OUTPUT_DESC1 desc1 = {};
	if (FAILED(output6->GetDesc1(&desc1)))
	{
		return false;
	}

	outMaxNits = desc1.MaxLuminance;
	outMinNits = desc1.MinLuminance;
	return true;
}

ID3D12Resource* DX12SwapChain::GetBackBuffer(uint32_t index) const
{
	if (index >= m_backBuffers.size())
	{
		return nullptr;
	}
	return m_backBuffers[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12SwapChain::GetRTV(uint32_t index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	if (!m_rtvHeap || index >= m_backBuffers.size())
	{
		return handle;
	}
	handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * m_rtvDescriptorSize;
	return handle;
}

ID3D12DescriptorHeap* DX12SwapChain::GetRTVHeap() const
{
	return m_rtvHeap.Get();
}

uint32_t DX12SwapChain::GetBufferCount() const
{
	return m_bufferCount;
}

void DX12SwapChain::ApplyColorSpace(RHIEnum::ColorSpace colorSpace)
{
	if (!m_swapChain)
	{
		return;
	}

	DXGI_COLOR_SPACE_TYPE dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	switch (colorSpace)
	{
	case RHIEnum::ColorSpace::HDR_G10_P709:
		dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
		break;
	case RHIEnum::ColorSpace::HDR_G2084_P2020:
		dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		break;
	case RHIEnum::ColorSpace::SDR_G22_P709:
	default:
		dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		break;
	}

	UINT support = 0;
	if (SUCCEEDED(m_swapChain->CheckColorSpaceSupport(dxgiColorSpace, &support)) &&
		(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
	{
		m_swapChain->SetColorSpace1(dxgiColorSpace);
	}

	ComPtr<IDXGISwapChain4> swapChain4;
	if (SUCCEEDED(m_swapChain.As(&swapChain4)))
	{
		// Temporarily disable manual HDR metadata updates.
		// This avoids redundant SetHDRMetaData calls until we revisit the policy.
		(void)swapChain4;
	}
}

void DX12SwapChain::CreateRenderTargetViews(ID3D12Device* device)
{
	m_backBuffers.clear();
	m_backBuffers.resize(m_bufferCount);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = m_bufferCount;

	if (FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap))))
	{
		return;
	}

	m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < m_bufferCount; ++i)
	{
		if (SUCCEEDED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]))))
		{
			device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
		}
		rtvHandle.ptr += m_rtvDescriptorSize;
	}
}
