#pragma once

#include <cstdint>

enum class RHIBufferUsage
{
	Default,
	Vertex,
	Index,
	Constant,
	Structured
};

enum class RHICpuAccess
{
	None,
	Write
};

struct RHIBufferDesc
{
	uint64_t sizeInBytes = 0;
	uint32_t strideInBytes = 0;
	RHIBufferUsage usage = RHIBufferUsage::Default;
	RHICpuAccess cpuAccess = RHICpuAccess::None;
};

class IRHIBuffer
{
public:
	virtual ~IRHIBuffer() = default;
	virtual void* GetNativeResource() const = 0;
	virtual const RHIBufferDesc& GetDesc() const = 0;
};
