#pragma once

#include <cstdint>

enum class RHITextureFormat
{
	RGBA8_UNORM,
	BGRA8_UNORM,
	R32_FLOAT,
	R16_FLOAT,
	R8_UNORM,
	D24S8
};

enum class RHITextureBind
{
	ShaderResource,
	RenderTarget,
	DepthStencil,
	UnorderedAccess
};

struct RHITextureDesc
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint16_t mipLevels = 1;
	uint16_t arrayLayers = 1;
	RHITextureFormat format = RHITextureFormat::RGBA8_UNORM;
	bool renderTarget = false;
	bool depthStencil = false;
	bool unorderedAccess = false;
};

class IRHITexture
{
public:
	virtual ~IRHITexture() = default;
	virtual void* GetNativeResource() const = 0;
	virtual const RHITextureDesc& GetDesc() const = 0;
};
