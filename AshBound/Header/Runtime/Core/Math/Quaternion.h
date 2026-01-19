#pragma once

#include <cmath>
#include <type_traits>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Math/VectorRegister.h"
#include "Runtime/Core/String.h"

namespace Math
{
	template<typename T>
	struct TQuaternion
	{
		T x = static_cast<T>(0);
		T y = static_cast<T>(0);
		T z = static_cast<T>(0);
		T w = static_cast<T>(1);

		constexpr TQuaternion() = default;
		constexpr TQuaternion(T inX, T inY, T inZ, T inW)
			: x(inX)
			, y(inY)
			, z(inZ)
			, w(inW)
		{
		}

		static constexpr TQuaternion Identity()
		{
			return TQuaternion(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
		}

		void Normalize()
		{
			const T lenSq = x * x + y * y + z * z + w * w;
			if (lenSq > static_cast<T>(0))
			{
				const T invLen = static_cast<T>(1) / Math::SqrtScalar(lenSq);
				x *= invLen;
				y *= invLen;
				z *= invLen;
				w *= invLen;
			}
		}

		TQuaternion Normalized() const
		{
			TQuaternion result = *this;
			result.Normalize();
			return result;
		}

		static TQuaternion FromAxisAngle(T axisX, T axisY, T axisZ, T radians)
		{
			const T half = radians * static_cast<T>(0.5);
			if constexpr (std::is_same_v<T, float>)
			{
				const Math::VectorRegister v = Math::VectorRegister::Load(half, half, half, half);
				Math::VectorRegister sVec;
				Math::VectorRegister cVec;
				Math::SinCos(v, sVec, cVec);
				alignas(16) float sData[4];
				alignas(16) float cData[4];
				_mm_store_ps(sData, sVec.v);
				_mm_store_ps(cData, cVec.v);
				const T s = static_cast<T>(sData[0]);
				const T c = static_cast<T>(cData[0]);
				return TQuaternion(axisX * s, axisY * s, axisZ * s, c);
			}
			else
			{
				const Math::VectorRegisterD v = Math::VectorRegisterD::Load(half, half, half, half);
				Math::VectorRegisterD sVec;
				Math::VectorRegisterD cVec;
				Math::SinCos(v, sVec, cVec);
				alignas(16) double sData[2];
				alignas(16) double cData[2];
				_mm_store_pd(sData, sVec.lo);
				_mm_store_pd(cData, cVec.lo);
				const T s = static_cast<T>(sData[0]);
				const T c = static_cast<T>(cData[0]);
				return TQuaternion(axisX * s, axisY * s, axisZ * s, c);
			}
		}

		static TQuaternion FromEuler(T pitch, T yaw, T roll)
		{
			const T halfPitch = pitch * static_cast<T>(0.5);
			const T halfYaw = yaw * static_cast<T>(0.5);
			const T halfRoll = roll * static_cast<T>(0.5);

			T sp;
			T cp;
			T sy;
			T cy;
			T sr;
			T cr;

			if constexpr (std::is_same_v<T, float>)
			{
				Math::VectorRegister sVec;
				Math::VectorRegister cVec;
				Math::SinCos(Math::VectorRegister::Load(halfPitch, halfPitch, halfPitch, halfPitch), sVec, cVec);
				alignas(16) float sData[4];
				alignas(16) float cData[4];
				_mm_store_ps(sData, sVec.v);
				_mm_store_ps(cData, cVec.v);
				sp = static_cast<T>(sData[0]);
				cp = static_cast<T>(cData[0]);

				Math::SinCos(Math::VectorRegister::Load(halfYaw, halfYaw, halfYaw, halfYaw), sVec, cVec);
				_mm_store_ps(sData, sVec.v);
				_mm_store_ps(cData, cVec.v);
				sy = static_cast<T>(sData[0]);
				cy = static_cast<T>(cData[0]);

				Math::SinCos(Math::VectorRegister::Load(halfRoll, halfRoll, halfRoll, halfRoll), sVec, cVec);
				_mm_store_ps(sData, sVec.v);
				_mm_store_ps(cData, cVec.v);
				sr = static_cast<T>(sData[0]);
				cr = static_cast<T>(cData[0]);
			}
			else
			{
				Math::VectorRegisterD sVec;
				Math::VectorRegisterD cVec;
				Math::SinCos(Math::VectorRegisterD::Load(halfPitch, halfPitch, halfPitch, halfPitch), sVec, cVec);
				alignas(16) double sData[2];
				alignas(16) double cData[2];
				_mm_store_pd(sData, sVec.lo);
				_mm_store_pd(cData, cVec.lo);
				sp = static_cast<T>(sData[0]);
				cp = static_cast<T>(cData[0]);

				Math::SinCos(Math::VectorRegisterD::Load(halfYaw, halfYaw, halfYaw, halfYaw), sVec, cVec);
				_mm_store_pd(sData, sVec.lo);
				_mm_store_pd(cData, cVec.lo);
				sy = static_cast<T>(sData[0]);
				cy = static_cast<T>(cData[0]);

				Math::SinCos(Math::VectorRegisterD::Load(halfRoll, halfRoll, halfRoll, halfRoll), sVec, cVec);
				_mm_store_pd(sData, sVec.lo);
				_mm_store_pd(cData, cVec.lo);
				sr = static_cast<T>(sData[0]);
				cr = static_cast<T>(cData[0]);
			}

			TQuaternion result;
			result.x = cr * sp * cy + sr * cp * sy;
			result.y = cr * cp * sy - sr * sp * cy;
			result.z = sr * cp * cy - cr * sp * sy;
			result.w = cr * cp * cy + sr * sp * sy;
			return result;
		}

		String ToString() const
		{
			return Text(L"({0}, {1}, {2}, {3})", x, y, z, w);
		}
	};

	inline TVectorRegister<float> TVectorRegister<float>::Load(const TQuaternion<float>& value)
	{
		return TVectorRegister(_mm_set_ps(value.w, value.z, value.y, value.x));
	}

	inline void TVectorRegister<float>::Store(TQuaternion<float>& out) const
	{
		alignas(16) float data[4];
		_mm_store_ps(data, v);
		out.x = data[0];
		out.y = data[1];
		out.z = data[2];
		out.w = data[3];
	}

	inline TVectorRegister<double> TVectorRegister<double>::Load(const TQuaternion<double>& value)
	{
		return TVectorRegister(_mm_set_pd(value.y, value.x), _mm_set_pd(value.w, value.z));
	}

	inline void TVectorRegister<double>::Store(TQuaternion<double>& out) const
	{
		alignas(16) double dataLo[2];
		alignas(16) double dataHi[2];
		_mm_store_pd(dataLo, lo);
		_mm_store_pd(dataHi, hi);
		out.x = dataLo[0];
		out.y = dataLo[1];
		out.z = dataHi[0];
		out.w = dataHi[1];
	}

	template<typename T>
	inline TQuaternion<T> operator*(const TQuaternion<T>& a, const TQuaternion<T>& b)
	{
		return TQuaternion<T>(
			a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
			a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
			a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
			a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
	}

	template<typename T>
	inline T Dot(const TQuaternion<T>& a, const TQuaternion<T>& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	template<typename T>
	inline TQuaternion<T> Conjugate(const TQuaternion<T>& q)
	{
		return TQuaternion<T>(-q.x, -q.y, -q.z, q.w);
	}

	template<typename T>
	inline TQuaternion<T> Inverse(const TQuaternion<T>& q)
	{
		const T lenSq = Dot(q, q);
		if (lenSq <= static_cast<T>(0))
		{
			return TQuaternion<T>::Identity();
		}
		const T invLenSq = static_cast<T>(1) / lenSq;
		TQuaternion<T> result = Conjugate(q);
		result.x *= invLenSq;
		result.y *= invLenSq;
		result.z *= invLenSq;
		result.w *= invLenSq;
		return result;
	}

	template<typename T>
	inline TVector3<T> RotateVector(const TQuaternion<T>& q, const TVector3<T>& v)
	{
		const TQuaternion<T> p(v.x, v.y, v.z, static_cast<T>(0));
		const TQuaternion<T> r = q * p * Inverse(q);
		return TVector3<T>(r.x, r.y, r.z);
	}

	template<typename T>
	inline TQuaternion<T> Slerp(const TQuaternion<T>& a, const TQuaternion<T>& b, T t)
	{
		TQuaternion<T> bAdjusted = b;
		T cosOmega = Dot(a, b);
		if (cosOmega < static_cast<T>(0))
		{
			cosOmega = -cosOmega;
			bAdjusted.x = -bAdjusted.x;
			bAdjusted.y = -bAdjusted.y;
			bAdjusted.z = -bAdjusted.z;
			bAdjusted.w = -bAdjusted.w;
		}

		const T one = static_cast<T>(1);
		if (cosOmega > one - static_cast<T>(1e-6))
		{
			TQuaternion<T> result(
				a.x + (bAdjusted.x - a.x) * t,
				a.y + (bAdjusted.y - a.y) * t,
				a.z + (bAdjusted.z - a.z) * t,
				a.w + (bAdjusted.w - a.w) * t);
			result.Normalize();
			return result;
		}

		T omega;
		T sinOmega;
		if constexpr (std::is_same_v<T, float>)
		{
			const Math::VectorRegister v = Math::VectorRegister::Load(cosOmega, cosOmega, cosOmega, cosOmega);
			const Math::VectorRegister acosVec = Math::Acos(v);
			alignas(16) float aData[4];
			_mm_store_ps(aData, acosVec.v);
			omega = static_cast<T>(aData[0]);

			const Math::VectorRegister sinVec = Math::Sin(Math::VectorRegister::Load(omega, omega, omega, omega));
			_mm_store_ps(aData, sinVec.v);
			sinOmega = static_cast<T>(aData[0]);
		}
		else
		{
			const Math::VectorRegisterD v = Math::VectorRegisterD::Load(cosOmega, cosOmega, cosOmega, cosOmega);
			const Math::VectorRegisterD acosVec = Math::Acos(v);
			alignas(16) double aData[2];
			_mm_store_pd(aData, acosVec.lo);
			omega = static_cast<T>(aData[0]);

			const Math::VectorRegisterD sinVec = Math::Sin(Math::VectorRegisterD::Load(omega, omega, omega, omega));
			_mm_store_pd(aData, sinVec.lo);
			sinOmega = static_cast<T>(aData[0]);
		}
		const T invSinOmega = one / sinOmega;
		const T scale0 = static_cast<T>(std::sin((one - t) * omega)) * invSinOmega;
		const T scale1 = static_cast<T>(std::sin(t * omega)) * invSinOmega;

		return TQuaternion<T>(
			a.x * scale0 + bAdjusted.x * scale1,
			a.y * scale0 + bAdjusted.y * scale1,
			a.z * scale0 + bAdjusted.z * scale1,
			a.w * scale0 + bAdjusted.w * scale1);
	}

	using Quaternion = TQuaternion<float>;
	using Quaterniond = TQuaternion<double>;
}
