#include "Runtime/Engine/StaticMesh.h"

#include <utility>

const std::vector<StaticMeshVertex>& StaticMesh::GetVertices() const
{
	return m_vertices;
}

const std::vector<uint32_t>& StaticMesh::GetIndices() const
{
	return m_indices;
}

void StaticMesh::SetVertices(std::vector<StaticMeshVertex> vertices)
{
	m_vertices = std::move(vertices);
}

void StaticMesh::SetIndices(std::vector<uint32_t> indices)
{
	m_indices = std::move(indices);
}

const std::shared_ptr<IRHIBuffer>& StaticMesh::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

const std::shared_ptr<IRHIBuffer>& StaticMesh::GetIndexBuffer() const
{
	return m_indexBuffer;
}

bool StaticMesh::UploadToGPU(IRHIDevice* device)
{
	if (!device || m_vertices.empty())
	{
		return false;
	}

	RHIBufferDesc vertexDesc = {};
	vertexDesc.sizeInBytes = static_cast<uint64_t>(m_vertices.size() * sizeof(StaticMeshVertex));
	vertexDesc.strideInBytes = sizeof(StaticMeshVertex);
	vertexDesc.usage = RHIEnum::BufferUsage::Vertex;
	vertexDesc.cpuAccess = RHIEnum::CpuAccess::Write;
	m_vertexBuffer = device->CreateBuffer(vertexDesc, m_vertices.data());
	if (!m_vertexBuffer)
	{
		return false;
	}

	if (!m_indices.empty())
	{
		RHIBufferDesc indexDesc = {};
		indexDesc.sizeInBytes = static_cast<uint64_t>(m_indices.size() * sizeof(uint32_t));
		indexDesc.strideInBytes = sizeof(uint32_t);
		indexDesc.usage = RHIEnum::BufferUsage::Index;
		indexDesc.cpuAccess = RHIEnum::CpuAccess::Write;
		m_indexBuffer = device->CreateBuffer(indexDesc, m_indices.data());
		if (!m_indexBuffer)
		{
			return false;
		}
	}

	return true;
}

StaticMesh StaticMesh::CreateCube(float size)
{
	const float half = size * 0.5f;
	StaticMesh mesh;
	mesh.m_vertices = {
		// +X
		{ Math::Vector3(half, -half, -half), Math::Vector3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f },
		{ Math::Vector3(half, -half,  half), Math::Vector3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f },
		{ Math::Vector3(half,  half,  half), Math::Vector3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f },
		{ Math::Vector3(half,  half, -half), Math::Vector3(1.0f, 0.0f, 0.0f), 0.0f, 0.0f },
		// -X
		{ Math::Vector3(-half, -half,  half), Math::Vector3(-1.0f, 0.0f, 0.0f), 0.0f, 1.0f },
		{ Math::Vector3(-half, -half, -half), Math::Vector3(-1.0f, 0.0f, 0.0f), 1.0f, 1.0f },
		{ Math::Vector3(-half,  half, -half), Math::Vector3(-1.0f, 0.0f, 0.0f), 1.0f, 0.0f },
		{ Math::Vector3(-half,  half,  half), Math::Vector3(-1.0f, 0.0f, 0.0f), 0.0f, 0.0f },
		// +Y
		{ Math::Vector3(-half,  half, -half), Math::Vector3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f },
		{ Math::Vector3( half,  half, -half), Math::Vector3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f },
		{ Math::Vector3( half,  half,  half), Math::Vector3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f },
		{ Math::Vector3(-half,  half,  half), Math::Vector3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f },
		// -Y
		{ Math::Vector3(-half, -half,  half), Math::Vector3(0.0f, -1.0f, 0.0f), 0.0f, 1.0f },
		{ Math::Vector3( half, -half,  half), Math::Vector3(0.0f, -1.0f, 0.0f), 1.0f, 1.0f },
		{ Math::Vector3( half, -half, -half), Math::Vector3(0.0f, -1.0f, 0.0f), 1.0f, 0.0f },
		{ Math::Vector3(-half, -half, -half), Math::Vector3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f },
		// +Z
		{ Math::Vector3( half, -half,  half), Math::Vector3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f },
		{ Math::Vector3(-half, -half,  half), Math::Vector3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f },
		{ Math::Vector3(-half,  half,  half), Math::Vector3(0.0f, 0.0f, 1.0f), 1.0f, 0.0f },
		{ Math::Vector3( half,  half,  half), Math::Vector3(0.0f, 0.0f, 1.0f), 0.0f, 0.0f },
		// -Z
		{ Math::Vector3(-half, -half, -half), Math::Vector3(0.0f, 0.0f, -1.0f), 0.0f, 1.0f },
		{ Math::Vector3( half, -half, -half), Math::Vector3(0.0f, 0.0f, -1.0f), 1.0f, 1.0f },
		{ Math::Vector3( half,  half, -half), Math::Vector3(0.0f, 0.0f, -1.0f), 1.0f, 0.0f },
		{ Math::Vector3(-half,  half, -half), Math::Vector3(0.0f, 0.0f, -1.0f), 0.0f, 0.0f }
	};

	mesh.m_indices = {
		0, 1, 2, 0, 2, 3,
		4, 5, 6, 4, 6, 7,
		8, 9, 10, 8, 10, 11,
		12, 13, 14, 12, 14, 15,
		16, 17, 18, 16, 18, 19,
		20, 21, 22, 20, 22, 23
	};

	return mesh;
}

StaticMesh StaticMesh::CreateFullscreenTriangle()
{
	StaticMesh mesh;
	mesh.m_vertices = {
		{ Math::Vector3(-1.0f, -1.0f, 0.0f), Math::Vector3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f },
		{ Math::Vector3( 3.0f, -1.0f, 0.0f), Math::Vector3(0.0f, 0.0f, 1.0f), 2.0f, 1.0f },
		{ Math::Vector3(-1.0f,  3.0f, 0.0f), Math::Vector3(0.0f, 0.0f, 1.0f), 0.0f, -1.0f }
	};

	mesh.m_indices = { 0, 1, 2 };
	return mesh;
}
