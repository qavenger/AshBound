#pragma once

#include "Runtime/Rendering/RHI/RHIShader.h"
#include "Runtime/Rendering/RHI/RHIRootSignature.h"

#include <memory>

struct RHIComputePipelineDesc
{
	std::shared_ptr<IRHIRootSignature> rootSignature;
	std::shared_ptr<IRHIShader> computeShader;
};

class IRHIComputePipeline
{
public:
	virtual ~IRHIComputePipeline() = default;
	virtual void* GetNativeHandle() const = 0;
	virtual const RHIComputePipelineDesc& GetDesc() const = 0;
};
