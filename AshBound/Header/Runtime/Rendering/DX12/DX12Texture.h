#pragma once

#include "Runtime/Rendering/RHI/RHITexture.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12Texture final : public IRHITexture
{
public:
	DX12Texture(const RHITextureDesc& desc, Microsoft::WRL::ComPtr<ID3D12Resource> resource);

	void* GetNativeResource() const override;
	const RHITextureDesc& GetDesc() const override;
	ID3D12Resource* GetResource() const;

private:
	RHITextureDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};
