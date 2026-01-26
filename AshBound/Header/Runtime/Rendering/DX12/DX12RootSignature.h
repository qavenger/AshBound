#pragma once

#include "Runtime/Rendering/RHI/RHIRootSignature.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12RootSignature final : public IRHIRootSignature
{
public:
	DX12RootSignature(const RHIRootSignatureDesc& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	void* GetNativeHandle() const override;
	const RHIRootSignatureDesc& GetDesc() const override;
	ID3D12RootSignature* GetRootSignature() const;

private:
	RHIRootSignatureDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};
