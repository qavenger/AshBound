#include "Runtime/Rendering/RHI/RHI.h"

#if defined(_WIN32)
#include "Runtime/Rendering/DX12/DX12Renderer.h"
#endif

std::unique_ptr<IRHIRenderer> CreateRendererForPlatform()
{
#if defined(_WIN32)
	return std::make_unique<DX12Renderer>();
#else
	return nullptr;
#endif
}
