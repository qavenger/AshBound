#pragma once

#include <cstdint>

#include "Runtime/Rendering/RHI/RHIEnum.h"
#include "Runtime/Rendering/RHI/RHIResource.h"

struct RHIBufferDesc
{
	uint64_t sizeInBytes = 0;
	uint32_t strideInBytes = 0;
	RHIEnum::BufferUsage usage = RHIEnum::BufferUsage::Default;
	RHIEnum::CpuAccess cpuAccess = RHIEnum::CpuAccess::None;
};

class IRHIBuffer : public IRHIResource
{
public:
	virtual ~IRHIBuffer() = default;
	virtual void* GetNativeResource() const = 0;
	virtual const RHIBufferDesc& GetDesc() const = 0;
	RHIResourceType GetResourceType() const override { return RHIResourceType::Buffer; }
};
