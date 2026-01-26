#pragma once

#include <cstdint>

struct RHIRootSignatureDesc
{
	uint32_t constantBufferCount = 1;
	bool allowInputLayout = true;
};

class IRHIRootSignature
{
public:
	virtual ~IRHIRootSignature() = default;
	virtual void* GetNativeHandle() const = 0;
	virtual const RHIRootSignatureDesc& GetDesc() const = 0;
};
