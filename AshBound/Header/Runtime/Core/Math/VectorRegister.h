#pragma once

#include "Runtime/Core/Math/Vector3.h"

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86_FP))
#if !defined(__SSE2__) && defined(_M_X64)
#define __SSE2__ 1
#endif
#if !defined(__SSE__) && defined(_M_IX86_FP) && (_M_IX86_FP >= 1)
#define __SSE__ 1
#endif
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__SSE__)
#include <xmmintrin.h>
#include <emmintrin.h>
#include <sleef.h>
#else
#error VectorRegister requires SSE support.
#endif

namespace Math
{
	template<typename T>
	struct TVectorRegister;

	template<typename T>
	struct TQuaternion;

	template<>
	struct alignas(16) TVectorRegister<float>
	{
		__m128 v;

		TVectorRegister()
			: v(_mm_setzero_ps())
		{
		}

		explicit TVectorRegister(__m128 in)
			: v(in)
		{
		}

		static TVectorRegister Zero()
		{
			return TVectorRegister(_mm_setzero_ps());
		}

		static TVectorRegister Load(float x, float y, float z, float w)
		{
			return TVectorRegister(_mm_set_ps(w, z, y, x));
		}

		static TVectorRegister Load(const Vector3& value)
		{
			return TVectorRegister(_mm_set_ps(0.0f, value.z, value.y, value.x));
		}

		static TVectorRegister Load(const TQuaternion<float>& value);

		void Store(Vector3& out) const
		{
			alignas(16) float data[4];
			_mm_store_ps(data, v);
			out.x = data[0];
			out.y = data[1];
			out.z = data[2];
		}

		void Store(TQuaternion<float>& out) const;
	};

	template<>
	struct alignas(16) TVectorRegister<double>
	{
		__m128d lo;
		__m128d hi;

		TVectorRegister()
			: lo(_mm_setzero_pd())
			, hi(_mm_setzero_pd())
		{
		}

		TVectorRegister(__m128d inLo, __m128d inHi)
			: lo(inLo)
			, hi(inHi)
		{
		}

		static TVectorRegister Zero()
		{
			return TVectorRegister(_mm_setzero_pd(), _mm_setzero_pd());
		}

		static TVectorRegister Load(double x, double y, double z, double w)
		{
			return TVectorRegister(_mm_set_pd(y, x), _mm_set_pd(w, z));
		}

		static TVectorRegister Load(const Vector3d& value)
		{
			return TVectorRegister(_mm_set_pd(value.y, value.x), _mm_set_pd(0.0, value.z));
		}

		static TVectorRegister Load(const TQuaternion<double>& value);

		void Store(Vector3d& out) const
		{
			alignas(16) double dataLo[2];
			alignas(16) double dataHi[2];
			_mm_store_pd(dataLo, lo);
			_mm_store_pd(dataHi, hi);
			out.x = dataLo[0];
			out.y = dataLo[1];
			out.z = dataHi[0];
		}

		void Store(TQuaternion<double>& out) const;
	};

	using VectorRegister = TVectorRegister<float>;
	using VectorRegisterD = TVectorRegister<double>;

	inline VectorRegister operator+(const VectorRegister& a, const VectorRegister& b)
	{
		return VectorRegister(_mm_add_ps(a.v, b.v));
	}

	inline VectorRegister operator-(const VectorRegister& a, const VectorRegister& b)
	{
		return VectorRegister(_mm_sub_ps(a.v, b.v));
	}

	inline VectorRegister operator*(const VectorRegister& a, const VectorRegister& b)
	{
		return VectorRegister(_mm_mul_ps(a.v, b.v));
	}

	inline VectorRegister MultiplyScalar(const VectorRegister& a, float scalar)
	{
		return VectorRegister(_mm_mul_ps(a.v, _mm_set1_ps(scalar)));
	}

	inline VectorRegisterD operator+(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		return VectorRegisterD(_mm_add_pd(a.lo, b.lo), _mm_add_pd(a.hi, b.hi));
	}

	inline VectorRegisterD operator-(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		return VectorRegisterD(_mm_sub_pd(a.lo, b.lo), _mm_sub_pd(a.hi, b.hi));
	}

	inline VectorRegisterD operator*(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		return VectorRegisterD(_mm_mul_pd(a.lo, b.lo), _mm_mul_pd(a.hi, b.hi));
	}

	inline VectorRegisterD MultiplyScalar(const VectorRegisterD& a, double scalar)
	{
		const __m128d s = _mm_set1_pd(scalar);
		return VectorRegisterD(_mm_mul_pd(a.lo, s), _mm_mul_pd(a.hi, s));
	}

	inline float Dot(const VectorRegister& a, const VectorRegister& b)
	{
		const __m128 mul = _mm_mul_ps(a.v, b.v);
		const __m128 sum = _mm_add_ps(mul, _mm_movehl_ps(mul, mul));
		const __m128 result = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1));
		return _mm_cvtss_f32(result);
	}

	inline double Dot(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		const __m128d mul0 = _mm_mul_pd(a.lo, b.lo);
		const __m128d mul1 = _mm_mul_pd(a.hi, b.hi);
		const __m128d sum = _mm_add_pd(mul0, mul1);
		const __m128d shuf = _mm_shuffle_pd(sum, sum, 0x1);
		const __m128d total = _mm_add_sd(sum, shuf);
		return _mm_cvtsd_f64(total);
	}

	inline VectorRegister Sin(const VectorRegister& a)
	{
#if defined(__SSE2__)
		return VectorRegister(Sleef_sinf4_u10(a.v));
#else
		alignas(16) float data[4];
		_mm_store_ps(data, a.v);
		data[0] = Sleef_sinf_u10(data[0]);
		data[1] = Sleef_sinf_u10(data[1]);
		data[2] = Sleef_sinf_u10(data[2]);
		data[3] = Sleef_sinf_u10(data[3]);
		return VectorRegister::Load(data[0], data[1], data[2], data[3]);
#endif
	}

	inline VectorRegister Cos(const VectorRegister& a)
	{
#if defined(__SSE2__)
		return VectorRegister(Sleef_cosf4_u10(a.v));
#else
		alignas(16) float data[4];
		_mm_store_ps(data, a.v);
		data[0] = Sleef_cosf_u10(data[0]);
		data[1] = Sleef_cosf_u10(data[1]);
		data[2] = Sleef_cosf_u10(data[2]);
		data[3] = Sleef_cosf_u10(data[3]);
		return VectorRegister::Load(data[0], data[1], data[2], data[3]);
#endif
	}

	inline VectorRegister Acos(const VectorRegister& a)
	{
#if defined(__SSE2__)
		return VectorRegister(Sleef_acosf4_u10(a.v));
#else
		alignas(16) float data[4];
		_mm_store_ps(data, a.v);
		data[0] = Sleef_acosf_u10(data[0]);
		data[1] = Sleef_acosf_u10(data[1]);
		data[2] = Sleef_acosf_u10(data[2]);
		data[3] = Sleef_acosf_u10(data[3]);
		return VectorRegister::Load(data[0], data[1], data[2], data[3]);
#endif
	}

	inline void SinCos(const VectorRegister& a, VectorRegister& outSin, VectorRegister& outCos)
	{
#if defined(__SSE2__)
		const Sleef___m128_2 sc = Sleef_sincosf4_u10(a.v);
		outSin = VectorRegister(sc.x);
		outCos = VectorRegister(sc.y);
#else
		alignas(16) float data[4];
		_mm_store_ps(data, a.v);
		Sleef_float2 sc0 = Sleef_sincosf_u10(data[0]);
		Sleef_float2 sc1 = Sleef_sincosf_u10(data[1]);
		Sleef_float2 sc2 = Sleef_sincosf_u10(data[2]);
		Sleef_float2 sc3 = Sleef_sincosf_u10(data[3]);
		outSin = VectorRegister::Load(sc0.x, sc1.x, sc2.x, sc3.x);
		outCos = VectorRegister::Load(sc0.y, sc1.y, sc2.y, sc3.y);
#endif
	}

	inline VectorRegisterD Sin(const VectorRegisterD& a)
	{
#if defined(__SSE2__)
		return VectorRegisterD(Sleef_sind2_u10(a.lo), Sleef_sind2_u10(a.hi));
#else
		alignas(16) double dataLo[2];
		alignas(16) double dataHi[2];
		_mm_store_pd(dataLo, a.lo);
		_mm_store_pd(dataHi, a.hi);
		dataLo[0] = Sleef_sin_u10(dataLo[0]);
		dataLo[1] = Sleef_sin_u10(dataLo[1]);
		dataHi[0] = Sleef_sin_u10(dataHi[0]);
		dataHi[1] = Sleef_sin_u10(dataHi[1]);
		return VectorRegisterD(_mm_set_pd(dataLo[1], dataLo[0]), _mm_set_pd(dataHi[1], dataHi[0]));
#endif
	}

	inline VectorRegisterD Cos(const VectorRegisterD& a)
	{
#if defined(__SSE2__)
		return VectorRegisterD(Sleef_cosd2_u10(a.lo), Sleef_cosd2_u10(a.hi));
#else
		alignas(16) double dataLo[2];
		alignas(16) double dataHi[2];
		_mm_store_pd(dataLo, a.lo);
		_mm_store_pd(dataHi, a.hi);
		dataLo[0] = Sleef_cos_u10(dataLo[0]);
		dataLo[1] = Sleef_cos_u10(dataLo[1]);
		dataHi[0] = Sleef_cos_u10(dataHi[0]);
		dataHi[1] = Sleef_cos_u10(dataHi[1]);
		return VectorRegisterD(_mm_set_pd(dataLo[1], dataLo[0]), _mm_set_pd(dataHi[1], dataHi[0]));
#endif
	}

	inline VectorRegisterD Acos(const VectorRegisterD& a)
	{
#if defined(__SSE2__)
		return VectorRegisterD(Sleef_acosd2_u10(a.lo), Sleef_acosd2_u10(a.hi));
#else
		alignas(16) double dataLo[2];
		alignas(16) double dataHi[2];
		_mm_store_pd(dataLo, a.lo);
		_mm_store_pd(dataHi, a.hi);
		dataLo[0] = Sleef_acos_u10(dataLo[0]);
		dataLo[1] = Sleef_acos_u10(dataLo[1]);
		dataHi[0] = Sleef_acos_u10(dataHi[0]);
		dataHi[1] = Sleef_acos_u10(dataHi[1]);
		return VectorRegisterD(_mm_set_pd(dataLo[1], dataLo[0]), _mm_set_pd(dataHi[1], dataHi[0]));
#endif
	}

	inline void SinCos(const VectorRegisterD& a, VectorRegisterD& outSin, VectorRegisterD& outCos)
	{
#if defined(__SSE2__)
		const Sleef___m128d_2 scLo = Sleef_sincosd2_u10(a.lo);
		const Sleef___m128d_2 scHi = Sleef_sincosd2_u10(a.hi);
		outSin = VectorRegisterD(scLo.x, scHi.x);
		outCos = VectorRegisterD(scLo.y, scHi.y);
#else
		alignas(16) double dataLo[2];
		alignas(16) double dataHi[2];
		_mm_store_pd(dataLo, a.lo);
		_mm_store_pd(dataHi, a.hi);
		Sleef_double2 sc0 = Sleef_sincos_u10(dataLo[0]);
		Sleef_double2 sc1 = Sleef_sincos_u10(dataLo[1]);
		Sleef_double2 sc2 = Sleef_sincos_u10(dataHi[0]);
		Sleef_double2 sc3 = Sleef_sincos_u10(dataHi[1]);
		outSin = VectorRegisterD(_mm_set_pd(sc1.x, sc0.x), _mm_set_pd(sc3.x, sc2.x));
		outCos = VectorRegisterD(_mm_set_pd(sc1.y, sc0.y), _mm_set_pd(sc3.y, sc2.y));
#endif
	}

	inline float SqrtScalar(float value)
	{
		const __m128 v = _mm_set_ss(value);
		return _mm_cvtss_f32(_mm_sqrt_ss(v));
	}

	inline double SqrtScalar(double value)
	{
		const __m128d v = _mm_set_sd(value);
		return _mm_cvtsd_f64(_mm_sqrt_sd(v, v));
	}

	inline VectorRegister Cross(const VectorRegister& a, const VectorRegister& b)
	{
		const __m128 aYZX = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(3, 0, 2, 1));
		const __m128 bZXY = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 1, 0, 2));
		const __m128 aZXY = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(3, 1, 0, 2));
		const __m128 bYZX = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 0, 2, 1));
		return VectorRegister(_mm_sub_ps(_mm_mul_ps(aYZX, bZXY), _mm_mul_ps(aZXY, bYZX)));
	}

	inline VectorRegisterD Cross(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		const __m128d aYZ = _mm_shuffle_pd(a.lo, a.hi, 0x1);
		const __m128d bZX = _mm_shuffle_pd(b.hi, b.lo, 0x0);
		const __m128d aZX = _mm_shuffle_pd(a.hi, a.lo, 0x0);
		const __m128d bYZ = _mm_shuffle_pd(b.lo, b.hi, 0x1);
		const __m128d xy = _mm_sub_pd(_mm_mul_pd(aYZ, bZX), _mm_mul_pd(aZX, bYZ));

		const __m128d aXY = a.lo;
		const __m128d bYX = _mm_shuffle_pd(b.lo, b.lo, 0x1);
		const __m128d mul = _mm_mul_pd(aXY, bYX);
		const __m128d diff = _mm_sub_sd(mul, _mm_shuffle_pd(mul, mul, 0x1));
		const __m128d zPair = _mm_shuffle_pd(diff, diff, 0x0);

		return VectorRegisterD(xy, zPair);
	}

	inline VectorRegister QuaternionMultiply(const VectorRegister& a, const VectorRegister& b)
	{
		const __m128 w1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(3, 3, 3, 3));
		const __m128 x1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 y1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 z1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(2, 2, 2, 2));

		const __m128 term0 = _mm_mul_ps(w1, b.v);

		const __m128 q2WZYX = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(0, 1, 2, 3));
		const __m128 signX = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);
		const __m128 term1 = _mm_mul_ps(_mm_mul_ps(x1, q2WZYX), signX);

		const __m128 q2ZWXY = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(1, 0, 3, 2));
		const __m128 signY = _mm_set_ps(-1.0f, -1.0f, 1.0f, 1.0f);
		const __m128 term2 = _mm_mul_ps(_mm_mul_ps(y1, q2ZWXY), signY);

		const __m128 q2YXWZ = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(2, 3, 0, 1));
		const __m128 signZ = _mm_set_ps(-1.0f, 1.0f, 1.0f, -1.0f);
		const __m128 term3 = _mm_mul_ps(_mm_mul_ps(z1, q2YXWZ), signZ);

		return VectorRegister(_mm_add_ps(_mm_add_ps(term0, term1), _mm_add_ps(term2, term3)));
	}

	inline VectorRegisterD QuaternionMultiply(const VectorRegisterD& a, const VectorRegisterD& b)
	{
		const __m128d ax = _mm_shuffle_pd(a.lo, a.lo, 0x0);
		const __m128d ay = _mm_shuffle_pd(a.lo, a.lo, 0x3);
		const __m128d az = _mm_shuffle_pd(a.hi, a.hi, 0x0);
		const __m128d aw = _mm_shuffle_pd(a.hi, a.hi, 0x3);

		const __m128d bx = _mm_shuffle_pd(b.lo, b.lo, 0x0);
		const __m128d by = _mm_shuffle_pd(b.lo, b.lo, 0x3);
		const __m128d bz = _mm_shuffle_pd(b.hi, b.hi, 0x0);
		const __m128d bw = _mm_shuffle_pd(b.hi, b.hi, 0x3);

		const __m128d termX = _mm_add_pd(
			_mm_add_pd(_mm_mul_pd(aw, bx), _mm_mul_pd(ax, bw)),
			_mm_sub_pd(_mm_mul_pd(ay, bz), _mm_mul_pd(az, by)));

		const __m128d termY = _mm_add_pd(
			_mm_add_pd(_mm_mul_pd(aw, by), _mm_mul_pd(ay, bw)),
			_mm_sub_pd(_mm_mul_pd(az, bx), _mm_mul_pd(ax, bz)));

		const __m128d termZ = _mm_add_pd(
			_mm_add_pd(_mm_mul_pd(aw, bz), _mm_mul_pd(az, bw)),
			_mm_sub_pd(_mm_mul_pd(ax, by), _mm_mul_pd(ay, bx)));

		const __m128d termW = _mm_add_pd(
			_mm_sub_pd(_mm_mul_pd(aw, bw), _mm_mul_pd(ax, bx)),
			_mm_sub_pd(_mm_setzero_pd(), _mm_add_pd(_mm_mul_pd(ay, by), _mm_mul_pd(az, bz))));

		const __m128d outLo = _mm_unpacklo_pd(termX, termY);
		const __m128d outHi = _mm_unpacklo_pd(termZ, termW);

		return VectorRegisterD(outLo, outHi);
	}
}
