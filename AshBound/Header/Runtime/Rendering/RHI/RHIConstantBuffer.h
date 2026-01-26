#pragma once

#include "Runtime/Rendering/RHI/RHIResource.h"

#include <cstdint>

struct RHIConstantBufferDesc
{
	uint64_t sizeInBytes = 0;
};

class IRHIConstantBuffer : public IRHIResource
{
public:
	virtual ~IRHIConstantBuffer() = default;
	virtual void* GetNativeResource() const = 0;
	virtual const RHIConstantBufferDesc& GetDesc() const = 0;
	virtual void Update(const void* data, uint64_t sizeInBytes) = 0;
	RHIResourceType GetResourceType() const override { return RHIResourceType::ConstantBuffer; }
};
