#pragma once

#include <cstdint>

#include "Runtime/Rendering/RHI/RHIEnum.h"
#include "Runtime/Rendering/RHI/RHIResource.h"

struct RHITextureDesc
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint16_t mipLevels = 1;
	uint16_t arrayLayers = 1;
	RHIEnum::Format format = RHIEnum::Format::RGBA8_UNORM;
	bool renderTarget = false;
	bool depthStencil = false;
	bool unorderedAccess = false;
};

class IRHITexture : public IRHIResource
{
public:
	virtual ~IRHITexture() = default;
	virtual void* GetNativeResource() const = 0;
	virtual const RHITextureDesc& GetDesc() const = 0;
	RHIResourceType GetResourceType() const override { return RHIResourceType::Texture; }
};
