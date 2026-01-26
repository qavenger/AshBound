#pragma once

#include "Runtime/Rendering/RHI/RHIBuffer.h"
#include "Runtime/Rendering/RHI/RHIConstantBuffer.h"
#include "Runtime/Rendering/RHI/RHITexture.h"
#include "Runtime/Rendering/RHI/RHIShader.h"
#include "Runtime/Rendering/RHI/RHIRootSignature.h"
#include "Runtime/Rendering/RHI/RHIGraphicsPipeline.h"
#include "Runtime/Rendering/RHI/RHIComputePipeline.h"
#include "Runtime/Rendering/RHI/RHICommandQueue.h"

#include <cstdint>
#include <memory>

class IRHIDevice
{
public:
	virtual ~IRHIDevice() = default;
	virtual void* GetNativeDevice() const = 0;

	virtual std::shared_ptr<IRHIBuffer> CreateBuffer(const RHIBufferDesc& desc, const void* initialData) = 0;
	virtual std::shared_ptr<IRHIConstantBuffer> CreateConstantBuffer(uint64_t sizeInBytes, const void* initialData) = 0;
	virtual std::shared_ptr<IRHITexture> CreateTexture2D(const RHITextureDesc& desc) = 0;
	virtual std::shared_ptr<IRHIShader> CreateShader(const RHIShaderDesc& desc) = 0;
	virtual std::shared_ptr<IRHIRootSignature> CreateRootSignature(const RHIRootSignatureDesc& desc) = 0;
	virtual std::shared_ptr<IRHIGraphicsPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) = 0;
	virtual std::shared_ptr<IRHIComputePipeline> CreateComputePipeline(const RHIComputePipelineDesc& desc) = 0;
	virtual std::shared_ptr<IRHIComputeCommandQueue> CreateComputeCommandQueue() = 0;
	virtual std::shared_ptr<IRHICopyCommandQueue> CreateCopyCommandQueue() = 0;
};
