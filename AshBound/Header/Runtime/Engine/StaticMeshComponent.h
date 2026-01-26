#pragma once

#include "Runtime/Engine/RenderComponent.h"
#include "Runtime/Engine/StaticMesh.h"

class StaticMeshComponent : public RenderComponent
{
public:
	explicit StaticMeshComponent(SceneObject* owner = nullptr, StaticMesh* mesh = nullptr);

	void SetStaticMesh(StaticMesh* mesh) { m_mesh = mesh; }
	StaticMesh* GetStaticMesh() const { return m_mesh; }

private:
	StaticMesh* m_mesh = nullptr;
};
