#pragma once

#include "Runtime/Rendering/RHI/RHICommandList.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12CommandList final : public IRHICommandList
{
public:
	bool Initialize(ID3D12Device* device);
	void Shutdown();

	ID3D12GraphicsCommandList* GetCommandList() const;
	ID3D12CommandAllocator* GetAllocator() const;
	void* GetNativeList() const override;
	void Reset() override;
	void Close() override;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
};
