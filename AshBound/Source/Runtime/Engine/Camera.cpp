#include "Runtime/Engine/Camera.h"

#include "Runtime/Core/Math/Matrix3.h"

#include <cmath>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;

	float DegreesToRadians(float degrees)
	{
		return degrees * (kPi / 180.0f);
	}
}

Camera::Camera()
	: m_fovDegrees(90.0f)
	, m_isOrthographic(false)
	, m_orthoSize(10.0f)
	, m_aspectRatio(16.0f / 9.0f)
	, m_nearPlane(0.1f)
	, m_farPlane(1000.0f)
{
}

float Camera::GetFovDegrees() const
{
	return m_fovDegrees;
}

void Camera::SetFovDegrees(float fovDegrees)
{
	m_fovDegrees = fovDegrees;
}

bool Camera::IsOrthographic() const
{
	return m_isOrthographic;
}

void Camera::SetOrthographic(bool orthographic)
{
	m_isOrthographic = orthographic;
}

float Camera::GetOrthoSize() const
{
	return m_orthoSize;
}

void Camera::SetOrthoSize(float orthoSize)
{
	m_orthoSize = orthoSize;
}

float Camera::GetAspectRatio() const
{
	return m_aspectRatio;
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

float Camera::GetNearPlane() const
{
	return m_nearPlane;
}

float Camera::GetFarPlane() const
{
	return m_farPlane;
}

void Camera::SetClipPlanes(float nearPlane, float farPlane)
{
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
}

Math::Matrix4 Camera::GetViewMatrix() const
{
	return GetTransform().Inverse().GetMatrix();
}

Math::Matrix4 Camera::GetProjectionMatrix() const
{
	return m_isOrthographic ? BuildOrthographicMatrix() : BuildPerspectiveMatrix();
}

Math::Matrix4 Camera::GetViewProjectionMatrix() const
{
	const Math::Matrix4 view = GetViewMatrix();
	const Math::Matrix4 projection = GetProjectionMatrix();
	return view * projection;
}

Math::Matrix4 Camera::GetInverseViewMatrix() const
{
	return GetTransform().GetMatrix();
}

Math::Matrix4 Camera::GetInverseProjectionMatrix() const
{
	return GetProjectionMatrix().Inverse();
}

Math::Matrix4 Camera::GetInverseViewProjectionMatrix() const
{
	return GetViewProjectionMatrix().Inverse();
}

void Camera::LookAt(const Math::Vector3& target, const Math::Vector3& up)
{
	Transform& transform = GetTransform();
	const Math::Vector3 position = transform.GetPosition();
	Math::Vector3 forward = target - position;
	if (forward.LengthSquared() <= 1e-6f)
	{
		return;
	}
	forward.Normalize();

	Math::Vector3 upDir = up;
	if (upDir.LengthSquared() <= 1e-6f)
	{
		upDir = Math::Vector3(0.0f, 1.0f, 0.0f);
	}
	upDir.Normalize();

	Math::Vector3 right = Math::Cross(upDir, forward);
	if (right.LengthSquared() <= 1e-6f)
	{
		upDir = Math::Vector3(0.0f, 0.0f, 1.0f);
		right = Math::Cross(upDir, forward);
		if (right.LengthSquared() <= 1e-6f)
		{
			return;
		}
	}
	right.Normalize();
	const Math::Vector3 trueUp = Math::Cross(forward, right);

	Math::Matrix3 basis = {};
	basis.m[0][0] = right.x;
	basis.m[1][0] = right.y;
	basis.m[2][0] = right.z;
	basis.m[0][1] = trueUp.x;
	basis.m[1][1] = trueUp.y;
	basis.m[2][1] = trueUp.z;
	basis.m[0][2] = forward.x;
	basis.m[1][2] = forward.y;
	basis.m[2][2] = forward.z;

	Math::Quaternion rotation = basis.ToQuaternion();
	rotation.Normalize();
	transform.SetRotation(rotation);
}

Math::Matrix4 Camera::BuildPerspectiveMatrix() const
{
	const float aspect = (m_aspectRatio > 0.0f) ? m_aspectRatio : 1.0f;
	const float fovRadians = DegreesToRadians(m_fovDegrees);
	const float halfFov = 0.5f * fovRadians;
	const float tanHalfFov = std::tan(halfFov);
	if (tanHalfFov == 0.0f)
	{
		return Math::Matrix4::Identity();
	}

	const float yScale = 1.0f / tanHalfFov;
	const float xScale = yScale / aspect;
	const float zn = m_nearPlane;
	const float zf = m_farPlane;
	if (zn == zf)
	{
		return Math::Matrix4::Identity();
	}

	const float a = zn / (zn - zf);
	const float b = (zn * zf) / (zf - zn);

	Math::Matrix4 result = {};
	result.m[0][0] = xScale;
	result.m[1][1] = yScale;
	result.m[2][2] = a;
	result.m[2][3] = 1.0f;
	result.m[3][2] = b;
	return result;
}

Math::Matrix4 Camera::BuildOrthographicMatrix() const
{
	const float aspect = (m_aspectRatio > 0.0f) ? m_aspectRatio : 1.0f;
	const float height = m_orthoSize;
	const float width = height * aspect;
	const float halfWidth = width * 0.5f;
	const float halfHeight = height * 0.5f;

	const float xScale = (halfWidth != 0.0f) ? (1.0f / halfWidth) : 0.0f;
	const float yScale = (halfHeight != 0.0f) ? (1.0f / halfHeight) : 0.0f;
	const float zn = m_nearPlane;
	const float zf = m_farPlane;
	if (zn == zf)
	{
		return Math::Matrix4::Identity();
	}

	const float a = 1.0f / (zn - zf);
	const float b = zf / (zf - zn);

	Math::Matrix4 result = {};
	result.m[0][0] = xScale;
	result.m[1][1] = yScale;
	result.m[2][2] = a;
	result.m[2][3] = b;
	result.m[3][3] = 1.0f;
	return result;
}
