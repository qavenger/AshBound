#include "Runtime/Engine/SceneObject.h"

const Transform& SceneObject::GetTransform() const
{
	return m_transform;
}

Transform& SceneObject::GetTransform()
{
	return m_transform;
}

void SceneObject::SetTransform(const Transform& transform)
{
	m_transform = transform;
}

void SceneObject::Update(float deltaTime)
{
	(void)deltaTime;
}

void SceneObject::AddComponent(Component* component)
{
	if (!component)
	{
		return;
	}
	m_components.push_back(component);
	component->SetOwner(this);
}
