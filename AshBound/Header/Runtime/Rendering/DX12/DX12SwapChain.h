#pragma once

#include "Runtime/Rendering/RHI/RHISwapChain.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

class DX12SwapChain final : public IRHISwapChain
{
public:
	bool Initialize(HWND windowHandle,
		IDXGIFactory4* factory,
		ID3D12Device* device,
		ID3D12CommandQueue* commandQueue,
		uint32_t width,
		uint32_t height,
		uint32_t bufferCount);
	void Shutdown();

	void* GetNativeSwapChain() const override;
	uint32_t GetCurrentBackBufferIndex() const override;
	void Present(bool vsync) override;
	void Resize(uint32_t width, uint32_t height) override;

	ID3D12Resource* GetBackBuffer(uint32_t index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32_t index) const;
	ID3D12DescriptorHeap* GetRTVHeap() const;
	uint32_t GetBufferCount() const;

private:
	void CreateRenderTargetViews(ID3D12Device* device);

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_bufferCount = 2;
	DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	ID3D12Device* m_device = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	uint32_t m_rtvDescriptorSize = 0;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;
};
