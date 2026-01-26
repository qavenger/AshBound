#pragma once

#include <cstdint>

enum class RHIResourceType : uint32_t
{
	Buffer,
	Texture,
	ConstantBuffer
};

class IRHIResource
{
public:
	virtual ~IRHIResource() = default;
	virtual void* GetNativeResource() const = 0;
	virtual RHIResourceType GetResourceType() const = 0;
};
