#pragma once

#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Rendering/RHI/RHIBuffer.h"
#include "Runtime/Rendering/RHI/RHIDevice.h"

#include <cstdint>
#include <memory>
#include <vector>

struct StaticMeshVertex
{
	Math::Vector3 position;
	Math::Vector3 normal;
	float u = 0.0f;
	float v = 0.0f;

	StaticMeshVertex() = default;
	StaticMeshVertex(const Math::Vector3& inPosition,
		const Math::Vector3& inNormal,
		float inU,
		float inV)
		: position(inPosition)
		, normal(inNormal)
		, u(inU)
		, v(inV)
	{
	}
};

class StaticMesh
{
public:
	StaticMesh() = default;

	const std::vector<StaticMeshVertex>& GetVertices() const;
	const std::vector<uint32_t>& GetIndices() const;

	void SetVertices(std::vector<StaticMeshVertex> vertices);
	void SetIndices(std::vector<uint32_t> indices);

	const std::shared_ptr<IRHIBuffer>& GetVertexBuffer() const;
	const std::shared_ptr<IRHIBuffer>& GetIndexBuffer() const;

	bool UploadToGPU(IRHIDevice* device);

	static StaticMesh CreateCube(float size = 1.0f);
	static StaticMesh CreateFullscreenTriangle();

private:
	std::vector<StaticMeshVertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::shared_ptr<IRHIBuffer> m_vertexBuffer;
	std::shared_ptr<IRHIBuffer> m_indexBuffer;
};
