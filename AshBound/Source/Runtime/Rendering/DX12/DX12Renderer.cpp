#include "Runtime/Rendering/DX12/DX12Renderer.h"

#include "Runtime/Core/Platform/PlatformViewport.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

bool DX12Renderer::Initialize(const RHIRendererInitInfo& info)
{
	if (m_initialized)
	{
		return true;
	}

	if (!info.viewport || info.width == 0 || info.height == 0)
	{
		return false;
	}

	PlatformWindowHandle windowHandle = info.viewport->GetWindowHandle();
	if (!windowHandle.IsValid())
	{
		return false;
	}
	HWND nativeWindowHandle = static_cast<HWND>(windowHandle.handle);

	m_vsync = info.enableVsync;
	m_width = info.width;
	m_height = info.height;
	m_windowHandle = nativeWindowHandle;

	if (info.enableDebugLayer)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}

	UINT factoryFlags = info.enableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;
	if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory))))
	{
		return false;
	}

	if (!m_device.Initialize(info.enableDebugLayer))
	{
		return false;
	}

	if (!m_commandQueue.Initialize(m_device.GetDevice()))
	{
		return false;
	}

	if (!m_computeCommandQueue.Initialize(m_device.GetDevice()))
	{
		return false;
	}

	if (!m_copyCommandQueue.Initialize(m_device.GetDevice()))
	{
		return false;
	}

	if (!m_commandList.Initialize(m_device.GetDevice()))
	{
		return false;
	}

	if (!m_swapChain.Initialize(m_windowHandle, m_factory.Get(), m_device.GetDevice(), m_commandQueue.GetQueue(),
		m_width, m_height, info.backbufferFormat, info.colorSpace, 2))
	{
		return false;
	}

	m_initialized = true;
	return true;
}

void DX12Renderer::BeginFrame(const float clearColor[4])
{
	if (!m_initialized)
	{
		return;
	}

	m_frameIndex = m_swapChain.GetCurrentBackBufferIndex();
	m_commandList.Reset();

	ID3D12GraphicsCommandList* commandList = m_commandList.GetCommandList();
	ID3D12Resource* backBuffer = m_swapChain.GetBackBuffer(m_frameIndex);
	if (!commandList || !backBuffer)
	{
		return;
	}

	Transition(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapChain.GetRTV(m_frameIndex);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void DX12Renderer::EndFrame()
{
	if (!m_initialized)
	{
		return;
	}

	ID3D12GraphicsCommandList* commandList = m_commandList.GetCommandList();
	ID3D12Resource* backBuffer = m_swapChain.GetBackBuffer(m_frameIndex);
	if (!commandList || !backBuffer)
	{
		return;
	}

	Transition(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList.Close();
	m_commandQueue.Execute(&m_commandList);
}

void DX12Renderer::Present()
{
	if (!m_initialized)
	{
		return;
	}

	m_swapChain.Present(m_vsync);
	m_commandQueue.Flush();
}

void DX12Renderer::Resize(uint32_t width, uint32_t height)
{
	if (!m_initialized || width == 0 || height == 0)
	{
		return;
	}

	m_width = width;
	m_height = height;
	m_commandQueue.Flush();
	m_swapChain.Resize(width, height);
}

bool DX12Renderer::RecreateSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace)
{
	if (!m_initialized || !m_windowHandle || width == 0 || height == 0)
	{
		return false;
	}

	m_commandQueue.Flush();
	m_swapChain.Shutdown();

	m_width = width;
	m_height = height;

	return m_swapChain.Initialize(m_windowHandle, m_factory.Get(), m_device.GetDevice(),
		m_commandQueue.GetQueue(), m_width, m_height, backbufferFormat, colorSpace, 2);
}

void DX12Renderer::Shutdown()
{
	if (!m_initialized)
	{
		return;
	}

	m_commandQueue.Flush();
	m_swapChain.Shutdown();
	m_commandList.Shutdown();
	m_computeCommandQueue.Shutdown();
	m_copyCommandQueue.Shutdown();
	m_commandQueue.Shutdown();
	m_device.Shutdown();
	m_factory.Reset();
	m_windowHandle = nullptr;
	m_initialized = false;
}

IRHIDevice* DX12Renderer::GetDevice()
{
	return &m_device;
}

IRHISwapChain* DX12Renderer::GetSwapChain()
{
	return &m_swapChain;
}

IRHICommandQueue* DX12Renderer::GetCommandQueue()
{
	return &m_commandQueue;
}

IRHIComputeCommandQueue* DX12Renderer::GetComputeCommandQueue()
{
	return &m_computeCommandQueue;
}

IRHICopyCommandQueue* DX12Renderer::GetCopyCommandQueue()
{
	return &m_copyCommandQueue;
}

IRHICommandList* DX12Renderer::GetCommandList()
{
	return &m_commandList;
}

void DX12Renderer::Transition(ID3D12GraphicsCommandList* commandList,
	ID3D12Resource* resource,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = beforeState;
	barrier.Transition.StateAfter = afterState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
}
