#pragma once

#include "Runtime/Rendering/RHI/RHIEnum.h"
#include "Runtime/Rendering/RHI/RHIShader.h"
#include "Runtime/Rendering/RHI/RHIRootSignature.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct RHIInputElement
{
	std::string semanticName;
	uint32_t semanticIndex = 0;
	RHIEnum::Format format = RHIEnum::Format::Unknown;
	uint32_t offset = 0;
};

struct RHIInputLayout
{
	std::vector<RHIInputElement> elements;
	uint32_t strideInBytes = 0;
};

struct RHIGraphicsPipelineDesc
{
	std::shared_ptr<IRHIRootSignature> rootSignature;
	std::shared_ptr<IRHIShader> vertexShader;
	std::shared_ptr<IRHIShader> pixelShader;
	RHIInputLayout inputLayout;
	RHIEnum::Format renderTargetFormat = RHIEnum::Format::Unknown;
	RHIEnum::PrimitiveTopology topology = RHIEnum::PrimitiveTopology::TriangleList;
	RHIEnum::CullMode cullMode = RHIEnum::CullMode::Back;
	RHIEnum::FrontFace frontFace = RHIEnum::FrontFace::CounterClockwise;
	bool depthEnabled = false;
};

class IRHIGraphicsPipeline
{
public:
	virtual ~IRHIGraphicsPipeline() = default;
	virtual void* GetNativeHandle() const = 0;
	virtual const RHIGraphicsPipelineDesc& GetDesc() const = 0;
};
