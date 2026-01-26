#pragma once

#include <cmath>
#include <cstdint>
#include "Runtime/Core/String.h"

namespace Math
{
	template<typename T>
	struct TVector2
	{
		T x = static_cast<T>(0);
		T y = static_cast<T>(0);

		constexpr TVector2() = default;
		constexpr TVector2(T inX, T inY)
			: x(inX)
			, y(inY)
		{
		}

		static constexpr TVector2 Zero() { return TVector2(); }
		static constexpr TVector2 One() { return TVector2(static_cast<T>(1), static_cast<T>(1)); }

		TVector2& operator+=(const TVector2& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		TVector2& operator-=(const TVector2& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		TVector2& operator*=(T scalar)
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		TVector2& operator/=(T scalar)
		{
			const T inv = static_cast<T>(1) / scalar;
			x *= inv;
			y *= inv;
			return *this;
		}

		T LengthSquared() const
		{
			return x * x + y * y;
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

		TVector2 Normalized() const
		{
			TVector2 result = *this;
			result.Normalize();
			return result;
		}

		String ToString() const
		{
			return Text(L"({0}, {1})", x, y);
		}
	};

	template<typename T>
	inline TVector2<T> operator+(TVector2<T> lhs, const TVector2<T>& rhs)
	{
		lhs += rhs;
		return lhs;
	}

	template<typename T>
	inline TVector2<T> operator-(TVector2<T> lhs, const TVector2<T>& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	template<typename T>
	inline TVector2<T> operator*(TVector2<T> lhs, T scalar)
	{
		lhs *= scalar;
		return lhs;
	}

	template<typename T>
	inline TVector2<T> operator*(T scalar, TVector2<T> rhs)
	{
		rhs *= scalar;
		return rhs;
	}

	template<typename T>
	inline TVector2<T> operator/(TVector2<T> lhs, T scalar)
	{
		lhs /= scalar;
		return lhs;
	}

	template<typename T>
	inline T Dot(const TVector2<T>& a, const TVector2<T>& b)
	{
		return a.x * b.x + a.y * b.y;
	}

	using Vector2 = TVector2<float>;
	using Vector2d = TVector2<double>;
	using IntVector2 = TVector2<int32_t>;
}
