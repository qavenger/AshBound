#pragma once

#include "Runtime/Engine/SceneObject.h"
#include "Runtime/Engine/RenderComponent.h"
#include "Runtime/Engine/Component.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class Scene
{
public:
	Scene() = default;

	SceneObject& AddObject(std::unique_ptr<SceneObject> object);

	template<typename T, typename... Args>
	T& EmplaceObject(Args&&... args)
	{
		static_assert(std::is_base_of_v<SceneObject, T>, "T must derive from SceneObject.");
		auto object = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *object;
		m_objects.emplace_back(std::move(object));
		return ref;
	}

	const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const;
	const std::vector<RenderComponent*>& GetRenderComponents() const { return m_renderComponents; }

	template<typename T, typename... Args>
	T& CreateComponent(SceneObject& owner, Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");
		auto component = std::make_unique<T>(&owner, std::forward<Args>(args)...);
		T& ref = *component;
		owner.AddComponent(component.get());
		if (auto renderComponent = dynamic_cast<RenderComponent*>(component.get()))
		{
			m_renderComponents.push_back(renderComponent);
		}
		m_components.emplace_back(std::move(component));
		return ref;
	}

	void MarkComponentForDestroy(Component* component);
	void Update(float deltaTime);

private:
	std::vector<std::unique_ptr<SceneObject>> m_objects;
	std::vector<std::unique_ptr<Component>> m_components;
	std::vector<RenderComponent*> m_renderComponents;
	std::vector<Component*> m_pendingDestroy;
	std::vector<std::unique_ptr<Component>> m_recycledComponents;
	float m_timeSinceCollectSeconds = 0.0f;
	static constexpr float kCollectIntervalSeconds = 30.0f;
};
