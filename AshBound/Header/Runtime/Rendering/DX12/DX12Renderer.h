#pragma once

#include "Runtime/Rendering/RHI/RHI.h"
#include "Runtime/Rendering/DX12/DX12Device.h"
#include "Runtime/Rendering/DX12/DX12CommandQueue.h"
#include "Runtime/Rendering/DX12/DX12CommandList.h"
#include "Runtime/Rendering/DX12/DX12SwapChain.h"

#include <dxgi1_6.h>
#include <wrl/client.h>

class DX12Renderer final : public IRHIRenderer
{
public:
	bool Initialize(const RHIRendererInitInfo& info) override;
	void BeginFrame(const float clearColor[4]) override;
	void EndFrame() override;
	void Present() override;
	void Resize(uint32_t width, uint32_t height) override;
	void Shutdown() override;

	IRHIDevice* GetDevice() override;
	IRHISwapChain* GetSwapChain() override;
	IRHICommandQueue* GetCommandQueue() override;
	IRHICommandList* GetCommandList() override;

private:
	void Transition(ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState);

	bool m_initialized = false;
	bool m_vsync = true;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_frameIndex = 0;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;

	DX12Device m_device;
	DX12CommandQueue m_commandQueue;
	DX12CommandList m_commandList;
	DX12SwapChain m_swapChain;
};
