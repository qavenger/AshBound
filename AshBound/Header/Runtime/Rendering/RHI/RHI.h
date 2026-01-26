#pragma once

#include <cstdint>
#include <memory>

#include "Runtime/Rendering/RHI/RHIEnum.h"

class PlatformViewport;

struct RHIRendererInitInfo
{
	PlatformViewport* viewport = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	RHIEnum::Format backbufferFormat = RHIEnum::Format::R10G10B10A2_UNORM;
	RHIEnum::ColorSpace colorSpace = RHIEnum::ColorSpace::SDR_G22_P709;
	bool enableDebugLayer = false;
	bool enableVsync = true;
};

class IRHIDevice;
class IRHISwapChain;
class IRHICommandQueue;
class IRHIComputeCommandQueue;
class IRHICopyCommandQueue;
class IRHICommandList;

class IRHIRenderer
{
public:
	virtual ~IRHIRenderer() = default;

	virtual bool Initialize(const RHIRendererInitInfo& info) = 0;
	virtual void BeginFrame(const float clearColor[4]) = 0;
	virtual void EndFrame() = 0;
	virtual void Present() = 0;
	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual bool RecreateSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace) = 0;
	virtual void Shutdown() = 0;

	virtual IRHIDevice* GetDevice() = 0;
	virtual IRHISwapChain* GetSwapChain() = 0;
	virtual IRHICommandQueue* GetCommandQueue() = 0;
	virtual IRHIComputeCommandQueue* GetComputeCommandQueue() = 0;
	virtual IRHICopyCommandQueue* GetCopyCommandQueue() = 0;
	virtual IRHICommandList* GetCommandList() = 0;
};

std::unique_ptr<IRHIRenderer> CreateRendererForPlatform();
