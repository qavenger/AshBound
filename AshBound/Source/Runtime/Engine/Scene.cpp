#include "Runtime/Engine/Scene.h"
#include "Runtime/Engine/RenderComponent.h"

#include <algorithm>

SceneObject& Scene::AddObject(std::unique_ptr<SceneObject> object)
{
	SceneObject& ref = *object;
	m_objects.emplace_back(std::move(object));
	return ref;
}

const std::vector<std::unique_ptr<SceneObject>>& Scene::GetObjects() const
{
	return m_objects;
}

void Scene::Update(float deltaTime)
{
	for (const auto& object : m_objects)
	{
		if (object)
		{
			object->Update(deltaTime);
		}
	}

	if (!m_pendingDestroy.empty())
	{
		m_timeSinceCollectSeconds += deltaTime;
		if (m_timeSinceCollectSeconds >= kCollectIntervalSeconds)
		{
			for (Component* component : m_pendingDestroy)
			{
				if (!component)
				{
					continue;
				}

				if (auto renderComponent = dynamic_cast<RenderComponent*>(component))
				{
					auto it = std::find(m_renderComponents.begin(), m_renderComponents.end(), renderComponent);
					if (it != m_renderComponents.end())
					{
						m_renderComponents.erase(it);
					}
				}

				for (auto it = m_components.begin(); it != m_components.end(); ++it)
				{
					if (it->get() == component)
					{
						component->SetOwner(nullptr);
						m_recycledComponents.emplace_back(std::move(*it));
						m_components.erase(it);
						break;
					}
				}
			}

			m_pendingDestroy.clear();
			m_timeSinceCollectSeconds = 0.0f;
		}
	}
}

void Scene::MarkComponentForDestroy(Component* component)
{
	if (!component)
	{
		return;
	}
	m_pendingDestroy.push_back(component);
}
