#pragma once

#include "Runtime/Rendering/RHI/RHIGraphicsPipeline.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12GraphicsPipeline final : public IRHIGraphicsPipeline
{
public:
	DX12GraphicsPipeline(const RHIGraphicsPipelineDesc& desc, Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void* GetNativeHandle() const override;
	const RHIGraphicsPipelineDesc& GetDesc() const override;
	ID3D12PipelineState* GetPipelineState() const;

private:
	RHIGraphicsPipelineDesc m_desc;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};
