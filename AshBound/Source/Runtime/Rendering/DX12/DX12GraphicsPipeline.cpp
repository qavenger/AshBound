#include "Runtime/Rendering/DX12/DX12GraphicsPipeline.h"

#include <utility>

DX12GraphicsPipeline::DX12GraphicsPipeline(const RHIGraphicsPipelineDesc& desc, Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
	: m_desc(desc)
	, m_pipelineState(std::move(pipelineState))
{
}

void* DX12GraphicsPipeline::GetNativeHandle() const
{
	return m_pipelineState.Get();
}

const RHIGraphicsPipelineDesc& DX12GraphicsPipeline::GetDesc() const
{
	return m_desc;
}

ID3D12PipelineState* DX12GraphicsPipeline::GetPipelineState() const
{
	return m_pipelineState.Get();
}
