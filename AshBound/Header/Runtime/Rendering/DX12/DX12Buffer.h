#pragma once

#include "Runtime/Rendering/RHI/RHIBuffer.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12Buffer final : public IRHIBuffer
{
public:
	DX12Buffer(const RHIBufferDesc& desc, Microsoft::WRL::ComPtr<ID3D12Resource> resource);

	void* GetNativeResource() const override;
	const RHIBufferDesc& GetDesc() const override;
	ID3D12Resource* GetResource() const;

private:
	RHIBufferDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};
