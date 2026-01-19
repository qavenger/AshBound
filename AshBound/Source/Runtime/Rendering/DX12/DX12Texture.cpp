#include "Runtime/Rendering/DX12/DX12Texture.h"

DX12Texture::DX12Texture(const RHITextureDesc& desc, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
	: m_desc(desc), m_resource(resource)
{
}

void* DX12Texture::GetNativeResource() const
{
	return m_resource.Get();
}

const RHITextureDesc& DX12Texture::GetDesc() const
{
	return m_desc;
}

ID3D12Resource* DX12Texture::GetResource() const
{
	return m_resource.Get();
}
