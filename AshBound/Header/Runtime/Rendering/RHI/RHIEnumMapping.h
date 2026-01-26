#pragma once

#include "Runtime/Rendering/RHI/RHIEnum.h"

// API-specific mapping utilities should be implemented in each backend module
// (e.g., DX12/Vulkan/OpenGL) without including API headers in the RHI layer.
namespace RHIMapping
{
	// Intentionally left empty as a mapping extension point.
}
