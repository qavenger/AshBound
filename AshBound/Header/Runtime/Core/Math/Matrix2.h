#pragma once

#include "Runtime/Core/Math/VectorRegister.h"
#include "Runtime/Core/String.h"
#include <cmath>
#include <type_traits>

namespace Math
{
	template<typename T>
	struct TMatrix2
	{
		T m[2][2] = {};

		static TMatrix2 Identity()
		{
			TMatrix2 result;
			result.m[0][0] = static_cast<T>(1);
			result.m[1][1] = static_cast<T>(1);
			return result;
		}

		static TMatrix2 Rotation(T radians)
		{
		T s;
		T c;
		if constexpr (std::is_same_v<T, float>)
		{
			VectorRegister sVec;
			VectorRegister cVec;
			SinCos(VectorRegister::Load(radians, radians, radians, radians), sVec, cVec);
			alignas(16) float sData[4];
			alignas(16) float cData[4];
			_mm_store_ps(sData, sVec.v);
			_mm_store_ps(cData, cVec.v);
			s = static_cast<T>(sData[0]);
			c = static_cast<T>(cData[0]);
		}
		else
		{
			VectorRegisterD sVec;
			VectorRegisterD cVec;
			SinCos(VectorRegisterD::Load(radians, radians, radians, radians), sVec, cVec);
			alignas(16) double sData[2];
			alignas(16) double cData[2];
			_mm_store_pd(sData, sVec.lo);
			_mm_store_pd(cData, cVec.lo);
			s = static_cast<T>(sData[0]);
			c = static_cast<T>(cData[0]);
		}
			TMatrix2 result = Identity();
			result.m[0][0] = c;
			result.m[0][1] = s;
			result.m[1][0] = -s;
			result.m[1][1] = c;
			return result;
		}

		static TMatrix2 Scaling(T x, T y)
		{
			TMatrix2 result = Identity();
			result.m[0][0] = x;
			result.m[1][1] = y;
			return result;
		}

		String ToString() const
		{
			return Text(L"[[{0}, {1}], [{2}, {3}]]",
				m[0][0], m[0][1],
				m[1][0], m[1][1]);
		}
	};

	template<typename T>
	inline TMatrix2<T> operator*(const TMatrix2<T>& a, const TMatrix2<T>& b)
	{
		TMatrix2<T> result = {};
		if constexpr (std::is_same_v<T, float>)
		{
			const VectorRegister col0 = VectorRegister::Load(b.m[0][0], b.m[1][0], 0.0f, 0.0f);
			const VectorRegister col1 = VectorRegister::Load(b.m[0][1], b.m[1][1], 0.0f, 0.0f);
			for (int row = 0; row < 2; ++row)
			{
				const VectorRegister rowReg = VectorRegister::Load(a.m[row][0], a.m[row][1], 0.0f, 0.0f);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
			}
		}
		else
		{
			const VectorRegisterD col0 = VectorRegisterD::Load(b.m[0][0], b.m[1][0], 0.0, 0.0);
			const VectorRegisterD col1 = VectorRegisterD::Load(b.m[0][1], b.m[1][1], 0.0, 0.0);
			for (int row = 0; row < 2; ++row)
			{
				const VectorRegisterD rowReg = VectorRegisterD::Load(a.m[row][0], a.m[row][1], 0.0, 0.0);
				result.m[row][0] = Dot(rowReg, col0);
				result.m[row][1] = Dot(rowReg, col1);
			}
		}
		return result;
	}

	using Matrix2 = TMatrix2<float>;
	using Matrix2d = TMatrix2<double>;
}
