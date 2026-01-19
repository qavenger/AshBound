#pragma once

#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Math/Quaternion.h"
#include "Runtime/Core/Math/Matrix3.h"
#include "Runtime/Core/Math/VectorRegister.h"
#include "Runtime/Core/String.h"
#include <type_traits>
#include <utility>

namespace Math
{
	template<typename T>
	struct TMatrix4
	{
		T m[4][4] = {};

		static TMatrix4 Identity()
		{
			TMatrix4 result;
			result.m[0][0] = static_cast<T>(1);
			result.m[1][1] = static_cast<T>(1);
			result.m[2][2] = static_cast<T>(1);
			result.m[3][3] = static_cast<T>(1);
			return result;
		}

		static TMatrix4 Translation(const TVector3<T>& t)
		{
			TMatrix4 result = Identity();
			result.m[3][0] = t.x;
			result.m[3][1] = t.y;
			result.m[3][2] = t.z;
			return result;
		}

		static TMatrix4 Scaling(const TVector3<T>& s)
		{
			TMatrix4 result = Identity();
			result.m[0][0] = s.x;
			result.m[1][1] = s.y;
			result.m[2][2] = s.z;
			return result;
		}

		static TMatrix4 RotationQuaternion(const TQuaternion<T>& q)
		{
			TMatrix4 result = Identity();

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

		static TMatrix4 FromQuaternion(const TQuaternion<T>& q)
		{
			return RotationQuaternion(q);
		}

		TVector3<T> TransformPoint(const TVector3<T>& v) const
		{
			if constexpr (std::is_same_v<T, float>)
			{
				const VectorRegister row0 = VectorRegister::Load(m[0][0], m[0][1], m[0][2], m[0][3]);
				const VectorRegister row1 = VectorRegister::Load(m[1][0], m[1][1], m[1][2], m[1][3]);
				const VectorRegister row2 = VectorRegister::Load(m[2][0], m[2][1], m[2][2], m[2][3]);
				const VectorRegister vec = VectorRegister::Load(v.x, v.y, v.z, static_cast<float>(1));
				return TVector3<T>(
					Dot(row0, vec),
					Dot(row1, vec),
					Dot(row2, vec)) + TVector3<T>(m[3][0], m[3][1], m[3][2]);
			}
			else
			{
				const VectorRegisterD row0 = VectorRegisterD::Load(m[0][0], m[0][1], m[0][2], m[0][3]);
				const VectorRegisterD row1 = VectorRegisterD::Load(m[1][0], m[1][1], m[1][2], m[1][3]);
				const VectorRegisterD row2 = VectorRegisterD::Load(m[2][0], m[2][1], m[2][2], m[2][3]);
				const VectorRegisterD row3 = VectorRegisterD::Load(m[3][0], m[3][1], m[3][2], m[3][3]);
				const VectorRegisterD vec = VectorRegisterD::Load(v.x, v.y, v.z, static_cast<double>(1));
				return TVector3<T>(
					Dot(row0, vec),
					Dot(row1, vec),
					Dot(row2, vec)) + TVector3<T>(m[3][0], m[3][1], m[3][2]);
			}
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
			TMatrix3<T> r;
			r.m[0][0] = m[0][0];
			r.m[0][1] = m[0][1];
			r.m[0][2] = m[0][2];
			r.m[1][0] = m[1][0];
			r.m[1][1] = m[1][1];
			r.m[1][2] = m[1][2];
			r.m[2][0] = m[2][0];
			r.m[2][1] = m[2][1];
			r.m[2][2] = m[2][2];
			return r.ToQuaternion();
		}

		bool TryInverse(TMatrix4& out) const
		{
			T aug[4][8] = {};
			for (int row = 0; row < 4; ++row)
			{
				for (int col = 0; col < 4; ++col)
				{
					aug[row][col] = m[row][col];
				}
				aug[row][row + 4] = static_cast<T>(1);
			}

			const T epsilon = std::is_same_v<T, float> ? static_cast<T>(1e-6f) : static_cast<T>(1e-12);

			for (int pivot = 0; pivot < 4; ++pivot)
			{
				int bestRow = pivot;
				T bestValue = aug[pivot][pivot] < static_cast<T>(0) ? -aug[pivot][pivot] : aug[pivot][pivot];
				for (int row = pivot + 1; row < 4; ++row)
				{
					const T value = aug[row][pivot] < static_cast<T>(0) ? -aug[row][pivot] : aug[row][pivot];
					if (value > bestValue)
					{
						bestValue = value;
						bestRow = row;
					}
				}

				if (bestValue <= epsilon)
				{
					return false;
				}

				if (bestRow != pivot)
				{
					for (int col = 0; col < 8; ++col)
					{
						std::swap(aug[pivot][col], aug[bestRow][col]);
					}
				}

				const T invPivot = static_cast<T>(1) / aug[pivot][pivot];
				for (int col = 0; col < 8; ++col)
				{
					aug[pivot][col] *= invPivot;
				}

				for (int row = 0; row < 4; ++row)
				{
					if (row == pivot)
					{
						continue;
					}
					const T factor = aug[row][pivot];
					if (factor == static_cast<T>(0))
					{
						continue;
					}
					for (int col = 0; col < 8; ++col)
					{
						aug[row][col] -= factor * aug[pivot][col];
					}
				}
			}

			for (int row = 0; row < 4; ++row)
			{
				for (int col = 0; col < 4; ++col)
				{
					out.m[row][col] = aug[row][col + 4];
				}
			}
			return true;
		}

		TMatrix4 Inverse() const
		{
			TMatrix4 out = Identity();
			if (!TryInverse(out))
			{
				return Identity();
			}
			return out;
		}

		String ToString() const
		{
			return Text(L"[[{0}, {1}, {2}, {3}], [{4}, {5}, {6}, {7}], [{8}, {9}, {10}, {11}], [{12}, {13}, {14}, {15}]]",
				m[0][0], m[0][1], m[0][2], m[0][3],
				m[1][0], m[1][1], m[1][2], m[1][3],
				m[2][0], m[2][1], m[2][2], m[2][3],
				m[3][0], m[3][1], m[3][2], m[3][3]);
		}
	};

	template<typename T>
	inline TMatrix4<T> operator*(const TMatrix4<T>& a, const TMatrix4<T>& b)
	{
		TMatrix4<T> result = {};
		if constexpr (std::is_same_v<T, float>)
		{
			const VectorRegister col0 = VectorRegister::Load(b.m[0][0], b.m[1][0], b.m[2][0], b.m[3][0]);
			const VectorRegister col1 = VectorRegister::Load(b.m[0][1], b.m[1][1], b.m[2][1], b.m[3][1]);
			const VectorRegister col2 = VectorRegister::Load(b.m[0][2], b.m[1][2], b.m[2][2], b.m[3][2]);
			const VectorRegister col3 = VectorRegister::Load(b.m[0][3], b.m[1][3], b.m[2][3], b.m[3][3]);
			for (int row = 0; row < 4; ++row)
			{
				const VectorRegister rowReg = VectorRegister::Load(a.m[row][0], a.m[row][1], a.m[row][2], a.m[row][3]);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
				result.m[row][2] = Dot(rowReg, col2);
				result.m[row][3] = Dot(rowReg, col3);
			}
		}
		else
		{
			const VectorRegisterD col0 = VectorRegisterD::Load(b.m[0][0], b.m[1][0], b.m[2][0], b.m[3][0]);
			const VectorRegisterD col1 = VectorRegisterD::Load(b.m[0][1], b.m[1][1], b.m[2][1], b.m[3][1]);
			const VectorRegisterD col2 = VectorRegisterD::Load(b.m[0][2], b.m[1][2], b.m[2][2], b.m[3][2]);
			const VectorRegisterD col3 = VectorRegisterD::Load(b.m[0][3], b.m[1][3], b.m[2][3], b.m[3][3]);
			for (int row = 0; row < 4; ++row)
			{
				const VectorRegisterD rowReg = VectorRegisterD::Load(a.m[row][0], a.m[row][1], a.m[row][2], a.m[row][3]);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
				result.m[row][2] = Dot(rowReg, col2);
				result.m[row][3] = Dot(rowReg, col3);
			}
		}
		return result;
	}

	using Matrix4 = TMatrix4<float>;
	using Matrix4d = TMatrix4<double>;
}
