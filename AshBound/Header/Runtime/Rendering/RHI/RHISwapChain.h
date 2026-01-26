#pragma once

#include <cstdint>

class IRHISwapChain
{
public:
	virtual ~IRHISwapChain() = default;
	virtual void* GetNativeSwapChain() const = 0;
	virtual uint32_t GetCurrentBackBufferIndex() const = 0;
	virtual void Present(bool vsync) = 0;
	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual bool QueryOutputLuminance(float& outMaxNits, float& outMinNits) const = 0;
};
