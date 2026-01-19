#pragma once

#include <cstdint>
#include <vector>

enum class RHIShaderStage
{
	Vertex,
	Pixel,
	Geometry,
	Hull,
	Domain,
	Compute,
	Mesh,
	Amplification,
	Library
};

struct RHIShaderDesc
{
	RHIShaderStage stage = RHIShaderStage::Pixel;
	std::vector<uint8_t> bytecode;
};

class IRHIShader
{
public:
	virtual ~IRHIShader() = default;
	virtual const RHIShaderDesc& GetDesc() const = 0;
};
