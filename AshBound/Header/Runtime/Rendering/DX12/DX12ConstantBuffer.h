#pragma once

#include "Runtime/Rendering/RHI/RHIConstantBuffer.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12ConstantBuffer final : public IRHIConstantBuffer
{
public:
	DX12ConstantBuffer(const RHIConstantBufferDesc& desc,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		void* mappedData);
	~DX12ConstantBuffer() override;

	void* GetNativeResource() const override;
	const RHIConstantBufferDesc& GetDesc() const override;
	void Update(const void* data, uint64_t sizeInBytes) override;
	ID3D12Resource* GetResource() const;

private:
	RHIConstantBufferDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	uint8_t* m_mappedData = nullptr;
};
