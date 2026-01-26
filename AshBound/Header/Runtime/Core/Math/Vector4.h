#pragma once

#include <cmath>
#include <cstdint>
#include "Runtime/Core/String.h"

namespace Math
{
	template<typename T>
	struct TVector4
	{
		T x = static_cast<T>(0);
		T y = static_cast<T>(0);
		T z = static_cast<T>(0);
		T w = static_cast<T>(0);

		constexpr TVector4() = default;
		constexpr TVector4(T inX, T inY, T inZ, T inW)
			: x(inX)
			, y(inY)
			, z(inZ)
			, w(inW)
		{
		}

		static constexpr TVector4 Zero() { return TVector4(); }
		static constexpr TVector4 One() { return TVector4(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)); }

		TVector4& operator+=(const TVector4& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}

		TVector4& operator-=(const TVector4& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}

		TVector4& operator*=(T scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		TVector4& operator/=(T scalar)
		{
			const T inv = static_cast<T>(1) / scalar;
			x *= inv;
			y *= inv;
			z *= inv;
			w *= inv;
			return *this;
		}

		T LengthSquared() const
		{
			return x * x + y * y + z * z + w * w;
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

		TVector4 Normalized() const
		{
			TVector4 result = *this;
			result.Normalize();
			return result;
		}

		String ToString() const
		{
			return Text(L"({0}, {1}, {2}, {3})", x, y, z, w);
		}
	};

	template<typename T>
	inline TVector4<T> operator+(TVector4<T> lhs, const TVector4<T>& rhs)
	{
		lhs += rhs;
		return lhs;
	}

	template<typename T>
	inline TVector4<T> operator-(TVector4<T> lhs, const TVector4<T>& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	template<typename T>
	inline TVector4<T> operator*(TVector4<T> lhs, T scalar)
	{
		lhs *= scalar;
		return lhs;
	}

	template<typename T>
	inline TVector4<T> operator*(T scalar, TVector4<T> rhs)
	{
		rhs *= scalar;
		return rhs;
	}

	template<typename T>
	inline TVector4<T> operator/(TVector4<T> lhs, T scalar)
	{
		lhs /= scalar;
		return lhs;
	}

	using Vector4 = TVector4<float>;
	using Vector4d = TVector4<double>;
	using IntVector4 = TVector4<int32_t>;
}
