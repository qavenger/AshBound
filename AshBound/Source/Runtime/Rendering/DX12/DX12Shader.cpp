#include "Runtime/Rendering/DX12/DX12Shader.h"

DX12Shader::DX12Shader(const RHIShaderDesc& desc)
	: m_desc(desc)
{
}

const RHIShaderDesc& DX12Shader::GetDesc() const
{
	return m_desc;
}
