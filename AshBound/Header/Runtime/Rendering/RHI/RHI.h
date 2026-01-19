#pragma once

#include <cstdint>
#include <windows.h>

struct RHIRendererInitInfo
{
	HWND windowHandle = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	bool enableDebugLayer = false;
	bool enableVsync = true;
};

class IRHIDevice;
class IRHISwapChain;
class IRHICommandQueue;
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
	virtual void Shutdown() = 0;

	virtual IRHIDevice* GetDevice() = 0;
	virtual IRHISwapChain* GetSwapChain() = 0;
	virtual IRHICommandQueue* GetCommandQueue() = 0;
	virtual IRHICommandList* GetCommandList() = 0;
};
