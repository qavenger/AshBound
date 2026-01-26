#pragma once

#include "Runtime/Rendering/RHI/RHIEnum.h"

#include <cstdint>

class IRHIRootSignature;
class IRHIGraphicsPipeline;
class IRHIBuffer;
class IRHIConstantBuffer;

struct RHIViewport
{
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float minDepth = 0.0f;
	float maxDepth = 1.0f;
};

struct RHIRect
{
	int32_t left = 0;
	int32_t top = 0;
	int32_t right = 0;
	int32_t bottom = 0;
};

class IRHICommandList
{
public:
	virtual ~IRHICommandList() = default;
	virtual void* GetNativeList() const = 0;
	virtual void Reset() = 0;
	virtual void Close() = 0;

	virtual void SetViewport(const RHIViewport& viewport) = 0;
	virtual void SetScissorRect(const RHIRect& rect) = 0;
	virtual void SetPrimitiveTopology(RHIEnum::PrimitiveTopology topology) = 0;
	virtual void SetGraphicsRootSignature(IRHIRootSignature* rootSignature) = 0;
	virtual void SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline) = 0;
	virtual void SetVertexBuffer(IRHIBuffer* buffer) = 0;
	virtual void SetIndexBuffer(IRHIBuffer* buffer) = 0;
	virtual void SetGraphicsConstantBuffer(uint32_t slot, IRHIConstantBuffer* buffer) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) = 0;
	virtual void Draw(uint32_t vertexCount, uint32_t startVertex) = 0;
};
