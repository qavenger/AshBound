#pragma once

#include "Runtime/Rendering/RHI/RHIBuffer.h"
#include "Runtime/Rendering/RHI/RHITexture.h"
#include "Runtime/Rendering/RHI/RHIShader.h"

#include <cstdint>
#include <memory>

class IRHIDevice
{
public:
	virtual ~IRHIDevice() = default;
	virtual void* GetNativeDevice() const = 0;

	virtual std::shared_ptr<IRHIBuffer> CreateBuffer(const RHIBufferDesc& desc, const void* initialData) = 0;
	virtual std::shared_ptr<IRHIBuffer> CreateConstantBuffer(uint64_t sizeInBytes, const void* initialData) = 0;
	virtual std::shared_ptr<IRHITexture> CreateTexture2D(const RHITextureDesc& desc) = 0;
	virtual std::shared_ptr<IRHIShader> CreateShader(const RHIShaderDesc& desc) = 0;
};
