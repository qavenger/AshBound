#include "Runtime/Engine/StaticMeshObject.h"

StaticMeshObject::StaticMeshObject(StaticMesh* mesh)
	: m_staticMesh(mesh)
{
}

void StaticMeshObject::SetStaticMesh(StaticMesh* mesh)
{
	m_staticMesh = mesh;
}

StaticMesh* StaticMeshObject::GetStaticMesh() const
{
	return m_staticMesh;
}

void StaticMeshObject::Update(float deltaTime)
{
	(void)deltaTime;
}
