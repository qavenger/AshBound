#include "Runtime/Engine/StaticMeshComponent.h"

StaticMeshComponent::StaticMeshComponent(SceneObject* owner, StaticMesh* mesh)
	: RenderComponent(owner)
	, m_mesh(mesh)
{
}
