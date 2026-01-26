#pragma once

class SceneObject;

class Component
{
public:
	explicit Component(SceneObject* owner = nullptr);
	virtual ~Component() = default;

	SceneObject* GetOwner() const { return m_owner; }
	void SetOwner(SceneObject* owner) { m_owner = owner; }

private:
	SceneObject* m_owner = nullptr;
};
