#pragma once

#include "Runtime/Engine/Component.h"

class RenderComponent : public Component
{
public:
	explicit RenderComponent(SceneObject* owner = nullptr);
	~RenderComponent() override = default;

	bool IsVisible() const { return m_visible; }
	void SetVisible(bool visible) { m_visible = visible; }

private:
	bool m_visible = true;
};
