#pragma once

#include <cstdint>

namespace RHIEnum
{
	enum class Format : uint32_t
	{
		Unknown = 0,
		R32G32B32_FLOAT,
		R32G32_FLOAT,
		RGBA8_UNORM,
		BGRA8_UNORM,
		R10G10B10A2_UNORM,
		RGBA16_FLOAT,
		R32_FLOAT,
		R16_FLOAT,
		R8_UNORM,
		D24S8,
		D32_FLOAT
	};

	enum class ColorSpace : uint32_t
	{
		SDR_G22_P709,
		HDR_G10_P709,
		HDR_G2084_P2020
	};

	enum class ShaderStage : uint32_t
	{
		Vertex,
		Fragment,
		Compute
	};

	enum class PrimitiveTopology : uint32_t
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip
	};

	enum class ResourceState : uint32_t
	{
		Undefined,
		CopySrc,
		CopyDst,
		VertexBuffer,
		IndexBuffer,
		ConstantBuffer,
		ShaderResource,
		UnorderedAccess,
		RenderTarget,
		DepthWrite,
		DepthRead,
		Present
	};

	enum class BufferUsage : uint32_t
	{
		Default,
		Vertex,
		Index,
		Constant,
		Structured,
		Storage,
		Indirect
	};

	enum class TextureDimension : uint32_t
	{
		Texture1D,
		Texture2D,
		Texture3D,
		TextureCube,
		Texture2DArray,
		TextureCubeArray
	};

	enum class TextureBind : uint32_t
	{
		ShaderResource,
		RenderTarget,
		DepthStencil,
		UnorderedAccess
	};

	enum class SamplerFilter : uint32_t
	{
		Nearest,
		Linear,
		Anisotropic
	};

	enum class SamplerAddressMode : uint32_t
	{
		Repeat,
		Mirror,
		Clamp,
		Border
	};

	enum class CompareOp : uint32_t
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};

	enum class BlendFactor : uint32_t
	{
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha
	};

	enum class BlendOp : uint32_t
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	enum class CullMode : uint32_t
	{
		None,
		Front,
		Back
	};

	enum class FrontFace : uint32_t
	{
		Clockwise,
		CounterClockwise
	};

	enum class FillMode : uint32_t
	{
		Solid,
		Wireframe
	};

	enum class DepthStencilOp : uint32_t
	{
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap
	};

	enum class LoadOp : uint32_t
	{
		Load,
		Clear,
		DontCare
	};

	enum class StoreOp : uint32_t
	{
		Store,
		DontCare
	};

	enum class SampleCount : uint32_t
	{
		Count1 = 1,
		Count2 = 2,
		Count4 = 4,
		Count8 = 8,
		Count16 = 16
	};

	enum class CpuAccess : uint32_t
	{
		None,
		Read,
		Write
	};
}
