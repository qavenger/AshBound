#pragma once

#include "Runtime/Rendering/RHI/RHICommandQueue.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12CommandQueue final : public IRHICommandQueue
{
public:
	bool Initialize(ID3D12Device* device);
	void Shutdown();

	ID3D12CommandQueue* GetQueue() const;
	void* GetNativeQueue() const override;
	void Execute(IRHICommandList* commandList) override;
	void Flush() override;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;
};
