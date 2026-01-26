#pragma once

#include "Runtime/Engine/SceneObject.h"
#include "Runtime/Core/Math/Matrix4.h"

class Camera : public SceneObject
{
public:
	Camera();

	float GetFovDegrees() const;
	void SetFovDegrees(float fovDegrees);

	bool IsOrthographic() const;
	void SetOrthographic(bool orthographic);

	float GetOrthoSize() const;
	void SetOrthoSize(float orthoSize);

	float GetAspectRatio() const;
	void SetAspectRatio(float aspectRatio);

	float GetNearPlane() const;
	float GetFarPlane() const;
	void SetClipPlanes(float nearPlane, float farPlane);

	Math::Matrix4 GetViewMatrix() const;
	Math::Matrix4 GetProjectionMatrix() const;
	Math::Matrix4 GetViewProjectionMatrix() const;

	Math::Matrix4 GetInverseViewMatrix() const;
	Math::Matrix4 GetInverseProjectionMatrix() const;
	Math::Matrix4 GetInverseViewProjectionMatrix() const;

	void LookAt(const Math::Vector3& target, const Math::Vector3& up = Math::Vector3(0.0f, 1.0f, 0.0f));

private:
	Math::Matrix4 BuildPerspectiveMatrix() const;
	Math::Matrix4 BuildOrthographicMatrix() const;

private:
	float m_fovDegrees;
	bool m_isOrthographic;
	float m_orthoSize;
	float m_aspectRatio;
	float m_nearPlane;
	float m_farPlane;
};
