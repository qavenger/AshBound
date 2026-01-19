#pragma once

#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Math/Quaternion.h"
#include "Runtime/Core/Math/VectorRegister.h"
#include "Runtime/Core/String.h"
#include <cmath>
#include <type_traits>

namespace Math
{
	template<typename T>
	struct TMatrix3
	{
		T m[3][3] = {};

		static TMatrix3 Identity()
		{
			TMatrix3 result;
			result.m[0][0] = static_cast<T>(1);
			result.m[1][1] = static_cast<T>(1);
			result.m[2][2] = static_cast<T>(1);
			return result;
		}

		static TMatrix3 Scaling(const TVector3<T>& s)
		{
			TMatrix3 result = Identity();
			result.m[0][0] = s.x;
			result.m[1][1] = s.y;
			result.m[2][2] = s.z;
			return result;
		}

		static TMatrix3 RotationQuaternion(const TQuaternion<T>& q)
		{
			TMatrix3 result = Identity();

			const T xx = q.x * q.x;
			const T yy = q.y * q.y;
			const T zz = q.z * q.z;
			const T xy = q.x * q.y;
			const T xz = q.x * q.z;
			const T yz = q.y * q.z;
			const T wx = q.w * q.x;
			const T wy = q.w * q.y;
			const T wz = q.w * q.z;

			result.m[0][0] = static_cast<T>(1) - static_cast<T>(2) * (yy + zz);
			result.m[0][1] = static_cast<T>(2) * (xy + wz);
			result.m[0][2] = static_cast<T>(2) * (xz - wy);

			result.m[1][0] = static_cast<T>(2) * (xy - wz);
			result.m[1][1] = static_cast<T>(1) - static_cast<T>(2) * (xx + zz);
			result.m[1][2] = static_cast<T>(2) * (yz + wx);

			result.m[2][0] = static_cast<T>(2) * (xz + wy);
			result.m[2][1] = static_cast<T>(2) * (yz - wx);
			result.m[2][2] = static_cast<T>(1) - static_cast<T>(2) * (xx + yy);

			return result;
		}

		TVector3<T> TransformVector(const TVector3<T>& v) const
		{
			if constexpr (std::is_same_v<T, float>)
			{
				const VectorRegister row0 = VectorRegister::Load(m[0][0], m[0][1], m[0][2], 0.0f);
				const VectorRegister row1 = VectorRegister::Load(m[1][0], m[1][1], m[1][2], 0.0f);
				const VectorRegister row2 = VectorRegister::Load(m[2][0], m[2][1], m[2][2], 0.0f);
				const VectorRegister vec = VectorRegister::Load(v.x, v.y, v.z, 0.0f);
				return TVector3<T>(Dot(row0, vec), Dot(row1, vec), Dot(row2, vec));
			}
			else
			{
				const VectorRegisterD row0 = VectorRegisterD::Load(m[0][0], m[0][1], m[0][2], 0.0);
				const VectorRegisterD row1 = VectorRegisterD::Load(m[1][0], m[1][1], m[1][2], 0.0);
				const VectorRegisterD row2 = VectorRegisterD::Load(m[2][0], m[2][1], m[2][2], 0.0);
				const VectorRegisterD vec = VectorRegisterD::Load(v.x, v.y, v.z, 0.0);
				return TVector3<T>(Dot(row0, vec), Dot(row1, vec), Dot(row2, vec));
			}
		}

		TQuaternion<T> ToQuaternion() const
		{
			const T trace = m[0][0] + m[1][1] + m[2][2];
			TQuaternion<T> q;

			if (trace > static_cast<T>(0))
			{
				const T s = static_cast<T>(0.5) / SqrtScalar(trace + static_cast<T>(1));
				q.w = static_cast<T>(0.25) / s;
				q.x = (m[2][1] - m[1][2]) * s;
				q.y = (m[0][2] - m[2][0]) * s;
				q.z = (m[1][0] - m[0][1]) * s;
			}
			else
			{
				if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
				{
					const T s = static_cast<T>(2) * SqrtScalar(static_cast<T>(1) + m[0][0] - m[1][1] - m[2][2]);
					q.w = (m[2][1] - m[1][2]) / s;
					q.x = static_cast<T>(0.25) * s;
					q.y = (m[0][1] + m[1][0]) / s;
					q.z = (m[0][2] + m[2][0]) / s;
				}
				else if (m[1][1] > m[2][2])
				{
					const T s = static_cast<T>(2) * SqrtScalar(static_cast<T>(1) + m[1][1] - m[0][0] - m[2][2]);
					q.w = (m[0][2] - m[2][0]) / s;
					q.x = (m[0][1] + m[1][0]) / s;
					q.y = static_cast<T>(0.25) * s;
					q.z = (m[1][2] + m[2][1]) / s;
				}
				else
				{
					const T s = static_cast<T>(2) * SqrtScalar(static_cast<T>(1) + m[2][2] - m[0][0] - m[1][1]);
					q.w = (m[1][0] - m[0][1]) / s;
					q.x = (m[0][2] + m[2][0]) / s;
					q.y = (m[1][2] + m[2][1]) / s;
					q.z = static_cast<T>(0.25) * s;
				}
			}

			return q;
		}

		String ToString() const
		{
			return Text(L"[[{0}, {1}, {2}], [{3}, {4}, {5}], [{6}, {7}, {8}]]",
				m[0][0], m[0][1], m[0][2],
				m[1][0], m[1][1], m[1][2],
				m[2][0], m[2][1], m[2][2]);
		}
	};

	template<typename T>
	inline TMatrix3<T> operator*(const TMatrix3<T>& a, const TMatrix3<T>& b)
	{
		TMatrix3<T> result = {};
		if constexpr (std::is_same_v<T, float>)
		{
			const VectorRegister col0 = VectorRegister::Load(b.m[0][0], b.m[1][0], b.m[2][0], 0.0f);
			const VectorRegister col1 = VectorRegister::Load(b.m[0][1], b.m[1][1], b.m[2][1], 0.0f);
			const VectorRegister col2 = VectorRegister::Load(b.m[0][2], b.m[1][2], b.m[2][2], 0.0f);
			for (int row = 0; row < 3; ++row)
			{
				const VectorRegister rowReg = VectorRegister::Load(a.m[row][0], a.m[row][1], a.m[row][2], 0.0f);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
				result.m[row][2] = Dot(rowReg, col2);
			}
		}
		else
		{
			const VectorRegisterD col0 = VectorRegisterD::Load(b.m[0][0], b.m[1][0], b.m[2][0], 0.0);
			const VectorRegisterD col1 = VectorRegisterD::Load(b.m[0][1], b.m[1][1], b.m[2][1], 0.0);
			const VectorRegisterD col2 = VectorRegisterD::Load(b.m[0][2], b.m[1][2], b.m[2][2], 0.0);
			for (int row = 0; row < 3; ++row)
			{
				const VectorRegisterD rowReg = VectorRegisterD::Load(a.m[row][0], a.m[row][1], a.m[row][2], 0.0);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
				result.m[row][2] = Dot(rowReg, col2);
			}
		}
		return result;
	}

	using Matrix3 = TMatrix3<float>;
	using Matrix3d = TMatrix3<double>;
}
