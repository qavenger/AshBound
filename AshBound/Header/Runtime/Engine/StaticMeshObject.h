#pragma once

#include "Runtime/Engine/SceneObject.h"
#include "Runtime/Engine/StaticMesh.h"

class StaticMeshObject : public SceneObject
{
public:
	StaticMeshObject() = default;
	explicit StaticMeshObject(StaticMesh* mesh);

	void SetStaticMesh(StaticMesh* mesh);
	StaticMesh* GetStaticMesh() const;

	void Update(float deltaTime) override;

private:
	StaticMesh* m_staticMesh = nullptr;
};
