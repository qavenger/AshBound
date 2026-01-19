#pragma once

#include "Runtime/Rendering/RHI/RHIShader.h"

class DX12Shader final : public IRHIShader
{
public:
	explicit DX12Shader(const RHIShaderDesc& desc);

	const RHIShaderDesc& GetDesc() const override;

private:
	RHIShaderDesc m_desc;
};
