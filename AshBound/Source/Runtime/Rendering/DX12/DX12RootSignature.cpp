#include "Runtime/Rendering/DX12/DX12RootSignature.h"

#include <utility>

DX12RootSignature::DX12RootSignature(const RHIRootSignatureDesc& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature)
	: m_desc(desc)
	, m_rootSignature(std::move(rootSignature))
{
}

void* DX12RootSignature::GetNativeHandle() const
{
	return m_rootSignature.Get();
}

const RHIRootSignatureDesc& DX12RootSignature::GetDesc() const
{
	return m_desc;
}

ID3D12RootSignature* DX12RootSignature::GetRootSignature() const
{
	return m_rootSignature.Get();
}
