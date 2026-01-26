#include "Runtime/Rendering/DX12/DX12ComputePipeline.h"

#include <utility>

DX12ComputePipeline::DX12ComputePipeline(const RHIComputePipelineDesc& desc, Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
	: m_desc(desc)
	, m_pipelineState(std::move(pipelineState))
{
}

void* DX12ComputePipeline::GetNativeHandle() const
{
	return m_pipelineState.Get();
}

const RHIComputePipelineDesc& DX12ComputePipeline::GetDesc() const
{
	return m_desc;
}

ID3D12PipelineState* DX12ComputePipeline::GetPipelineState() const
{
	return m_pipelineState.Get();
}
