#pragma once

#include <cmath>
#include "Runtime/Core/String.h"

namespace Math
{
	template<typename T>
	struct TVector3
	{
		T x = static_cast<T>(0);
		T y = static_cast<T>(0);
		T z = static_cast<T>(0);

		constexpr TVector3() = default;
		constexpr TVector3(T inX, T inY, T inZ)
			: x(inX)
			, y(inY)
			, z(inZ)
		{
		}

		static constexpr TVector3 Zero() { return TVector3(); }
		static constexpr TVector3 One() { return TVector3(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)); }

		TVector3& operator+=(const TVector3& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}

		TVector3& operator-=(const TVector3& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}

		TVector3& operator*=(T scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		TVector3& operator/=(T scalar)
		{
			const T inv = static_cast<T>(1) / scalar;
			x *= inv;
			y *= inv;
			z *= inv;
			return *this;
		}

		TVector3& operator*=(const TVector3& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}

		T LengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		T Length() const
		{
			return static_cast<T>(std::sqrt(LengthSquared()));
		}

		void Normalize()
		{
			const T len = Length();
			if (len > static_cast<T>(0))
			{
				*this /= len;
			}
		}

		TVector3 Normalized() const
		{
			TVector3 result = *this;
			result.Normalize();
			return result;
		}

		String ToString() const
		{
			return Text(L"({0}, {1}, {2})", x, y, z);
		}
	};

	template<typename T>
	inline TVector3<T> operator+(TVector3<T> lhs, const TVector3<T>& rhs)
	{
		lhs += rhs;
		return lhs;
	}

	template<typename T>
	inline TVector3<T> operator-(TVector3<T> lhs, const TVector3<T>& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	template<typename T>
	inline TVector3<T> operator*(TVector3<T> lhs, T scalar)
	{
		lhs *= scalar;
		return lhs;
	}

	template<typename T>
	inline TVector3<T> operator*(T scalar, TVector3<T> rhs)
	{
		rhs *= scalar;
		return rhs;
	}

	template<typename T>
	inline TVector3<T> operator/(TVector3<T> lhs, T scalar)
	{
		lhs /= scalar;
		return lhs;
	}

	template<typename T>
	inline TVector3<T> operator*(TVector3<T> lhs, const TVector3<T>& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	template<typename T>
	inline T Dot(const TVector3<T>& a, const TVector3<T>& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	template<typename T>
	inline TVector3<T> Cross(const TVector3<T>& a, const TVector3<T>& b)
	{
		return TVector3<T>(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
	}

	using Vector3 = TVector3<float>;
	using Vector3d = TVector3<double>;
}
