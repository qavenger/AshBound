#pragma once

#include "Runtime/Rendering/RHI/RHIComputePipeline.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12ComputePipeline final : public IRHIComputePipeline
{
public:
	DX12ComputePipeline(const RHIComputePipelineDesc& desc, Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void* GetNativeHandle() const override;
	const RHIComputePipelineDesc& GetDesc() const override;
	ID3D12PipelineState* GetPipelineState() const;

private:
	RHIComputePipelineDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};
