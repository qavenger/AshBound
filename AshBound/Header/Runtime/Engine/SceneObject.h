#pragma once

#include "Runtime/Engine/Object.h"
#include "Runtime/Core/Transform.h"
#include "Runtime/Engine/Component.h"

#include <vector>

class SceneObject : public Object
{
public:
	SceneObject() = default;
	~SceneObject() override = default;

	const Transform& GetTransform() const;
	Transform& GetTransform();
	void SetTransform(const Transform& transform);

	virtual void Update(float deltaTime);

	void AddComponent(Component* component);
	const std::vector<Component*>& GetComponents() const { return m_components; }

private:
	Transform m_transform = Transform::Identity();
	std::vector<Component*> m_components;
};
