#pragma once

#include "Runtime/Core/Math/Math.h"
#include <type_traits>

template<typename T>
struct TTransform
{
	static_assert(std::is_same_v<T, float>,
		"TTransform currently supports float only with VectorRegister.");
	using Scalar = T;
	using Vector3 = Math::TVector3<T>;
	using Quat = Math::TQuaternion<T>;
	using Matrix4 = Math::TMatrix4<T>;

	TTransform()
		: position(0.0f, 0.0f, 0.0f)
		, rotation(0.0f, 0.0f, 0.0f, 1.0f)
		, scale(1.0f, 1.0f, 1.0f)
	{
	}

	TTransform(const Vector3& inPosition,
		const Quat& inRotation,
		const Vector3& inScale)
		: position(inPosition)
		, rotation(inRotation)
		, scale(inScale)
	{
	}

	static TTransform Identity()
	{
		return TTransform();
	}

	void SetIdentity()
	{
		position = Vector3(0.0f, 0.0f, 0.0f);
		rotation = Quat(0.0f, 0.0f, 0.0f, 1.0f);
		scale = Vector3(1.0f, 1.0f, 1.0f);
	}

	const Vector3& GetPosition() const { return position; }
	const Quat& GetRotation() const { return rotation; }
	const Vector3& GetScale() const { return scale; }

	void SetPosition(const Vector3& inPosition) { position = inPosition; }
	void SetRotation(const Quat& inRotation) { rotation = inRotation; }
	void SetScale(const Vector3& inScale) { scale = inScale; }

	void Translate(const Vector3& delta)
	{
		const Math::VectorRegister p = Math::VectorRegister::Load(position);
		const Math::VectorRegister d = Math::VectorRegister::Load(delta);
		Math::VectorRegister result = p + d;
		result.Store(position);
	}

	void Rotate(const Quat& delta)
	{
		const Math::VectorRegister r = Math::VectorRegister::Load(rotation);
		const Math::VectorRegister d = Math::VectorRegister::Load(delta);
		Math::VectorRegister result = Math::QuaternionMultiply(r, d);
		result.Store(rotation);
	}

	void ScaleBy(const Vector3& factor)
	{
		const Math::VectorRegister s = Math::VectorRegister::Load(scale);
		const Math::VectorRegister f = Math::VectorRegister::Load(factor);
		Math::VectorRegister result = s * f;
		result.Store(scale);
	}

	void ScaleUniform(Scalar factor)
	{
		const Vector3 uniform(factor, factor, factor);
		ScaleBy(uniform);
	}

	Matrix4 GetMatrix() const
	{
		Matrix4 result = Matrix4::RotationQuaternion(rotation);
		const Math::VectorRegister row0 = Math::MultiplyScalar(
			Math::VectorRegister::Load(result.m[0][0], result.m[0][1], result.m[0][2], 0.0f), scale.x);
		const Math::VectorRegister row1 = Math::MultiplyScalar(
			Math::VectorRegister::Load(result.m[1][0], result.m[1][1], result.m[1][2], 0.0f), scale.y);
		const Math::VectorRegister row2 = Math::MultiplyScalar(
			Math::VectorRegister::Load(result.m[2][0], result.m[2][1], result.m[2][2], 0.0f), scale.z);

		alignas(16) float rowData[4];
		_mm_store_ps(rowData, row0.v);
		result.m[0][0] = rowData[0];
		result.m[0][1] = rowData[1];
		result.m[0][2] = rowData[2];

		_mm_store_ps(rowData, row1.v);
		result.m[1][0] = rowData[0];
		result.m[1][1] = rowData[1];
		result.m[1][2] = rowData[2];

		_mm_store_ps(rowData, row2.v);
		result.m[2][0] = rowData[0];
		result.m[2][1] = rowData[1];
		result.m[2][2] = rowData[2];

		result.m[3][0] = position.x;
		result.m[3][1] = position.y;
		result.m[3][2] = position.z;
		result.m[3][3] = static_cast<Scalar>(1);
		return result;
	}

	bool TryInverse(TTransform& out) const
	{
		const Scalar epsilon = static_cast<Scalar>(1e-6f);
		if ((scale.x < epsilon && scale.x > -epsilon) ||
			(scale.y < epsilon && scale.y > -epsilon) ||
			(scale.z < epsilon && scale.z > -epsilon))
		{
			return false;
		}

		const Vector3 invScale(
			static_cast<Scalar>(1) / scale.x,
			static_cast<Scalar>(1) / scale.y,
			static_cast<Scalar>(1) / scale.z);
		const Quat invRot = Math::Inverse(rotation);

		const Math::VectorRegister posReg = Math::VectorRegister::Load(position);
		const Math::VectorRegister invScaleReg = Math::VectorRegister::Load(invScale);
		const Math::VectorRegister scaledPosReg = posReg * invScaleReg;

		const Math::VectorRegister invRotReg = Math::VectorRegister::Load(invRot);
		const Math::VectorRegister invRotConjReg = Math::VectorRegister::Load(Math::Conjugate(invRot));
		const Math::VectorRegister qp = Math::QuaternionMultiply(invRotReg, scaledPosReg);
		const Math::VectorRegister qpq = Math::QuaternionMultiply(qp, invRotConjReg);

		Vector3 rotatedPos;
		qpq.Store(rotatedPos);

		out.position = Vector3(-rotatedPos.x, -rotatedPos.y, -rotatedPos.z);
		out.rotation = invRot;
		out.scale = invScale;
		return true;
	}

	TTransform Inverse() const
	{
		TTransform out = TTransform::Identity();
		if (!TryInverse(out))
		{
			return TTransform::Identity();
		}
		return out;
	}

	static TTransform FromMatrix(const Matrix4& matrix)
	{
		TTransform result;
		result.SetFromMatrix(matrix);
		return result;
	}

	void SetFromMatrix(const Matrix4& matrix)
	{
		position = Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);

		const Math::VectorRegister row0 = Math::VectorRegister::Load(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2], 0.0f);
		const Math::VectorRegister row1 = Math::VectorRegister::Load(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2], 0.0f);
		const Math::VectorRegister row2 = Math::VectorRegister::Load(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2], 0.0f);

		const float sx = Math::SqrtScalar(Math::Dot(row0, row0));
		const float sy = Math::SqrtScalar(Math::Dot(row1, row1));
		const float sz = Math::SqrtScalar(Math::Dot(row2, row2));
		scale = Vector3(sx, sy, sz);

		const float invSx = sx != 0.0f ? 1.0f / sx : 0.0f;
		const float invSy = sy != 0.0f ? 1.0f / sy : 0.0f;
		const float invSz = sz != 0.0f ? 1.0f / sz : 0.0f;

		Math::VectorRegister r0 = Math::MultiplyScalar(row0, invSx);
		Math::VectorRegister r1 = Math::MultiplyScalar(row1, invSy);
		Math::VectorRegister r2 = Math::MultiplyScalar(row2, invSz);

		Vector3 r0v;
		Vector3 r1v;
		Vector3 r2v;
		r0.Store(r0v);
		r1.Store(r1v);
		r2.Store(r2v);

		Math::TMatrix3<float> rot;
		rot.m[0][0] = r0v.x; rot.m[0][1] = r0v.y; rot.m[0][2] = r0v.z;
		rot.m[1][0] = r1v.x; rot.m[1][1] = r1v.y; rot.m[1][2] = r1v.z;
		rot.m[2][0] = r2v.x; rot.m[2][1] = r2v.y; rot.m[2][2] = r2v.z;

		rotation = rot.ToQuaternion();
	}

	String ToString() const
	{
		return Text(L"Position={0}, Rotation={1}, Scale={2}",
			position.ToString().Str().c_str(),
			rotation.ToString().Str().c_str(),
			scale.ToString().Str().c_str());
	}

	Vector3 position;
	Quat rotation;
	Vector3 scale;
};

using Transform = TTransform<float>;
