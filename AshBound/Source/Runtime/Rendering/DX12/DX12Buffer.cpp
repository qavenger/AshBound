#include "Runtime/Rendering/DX12/DX12Buffer.h"

DX12Buffer::DX12Buffer(const RHIBufferDesc& desc, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
	: m_desc(desc), m_resource(resource)
{
}

void* DX12Buffer::GetNativeResource() const
{
	return m_resource.Get();
}

const RHIBufferDesc& DX12Buffer::GetDesc() const
{
	return m_desc;
}

ID3D12Resource* DX12Buffer::GetResource() const
{
	return m_resource.Get();
}
