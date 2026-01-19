#include "Runtime/Core/ColorManagement/ColorSpace.h"
#include <cmath>

namespace
{
	constexpr float kChromaticityEpsilon = 1e-6f;
	constexpr double kMatrixEpsilon = 1e-10;

	bool IsNearlyZero(double value)
	{
		return std::abs(value) <= kMatrixEpsilon;
	}

	Math::Vector3 XYToXYZ(const Chromaticity& chroma)
	{
		const float invY = (std::abs(chroma.y) > kChromaticityEpsilon) ? (1.0f / chroma.y) : 0.0f;
		const float X = chroma.x * invY;
		const float Y = 1.0f;
		const float Z = (1.0f - chroma.x - chroma.y) * invY;
		return Math::Vector3(X, Y, Z);
	}

	Math::Matrix3 MakeDiagonal(const Math::Vector3& v)
	{
		Math::Matrix3 result = {};
		result.m[0][0] = v.x;
		result.m[1][1] = v.y;
		result.m[2][2] = v.z;
		return result;
	}

	bool TryInvertMatrix3(const Math::Matrix3& in, Math::Matrix3& out)
	{
		const double m00 = in.m[0][0];
		const double m01 = in.m[0][1];
		const double m02 = in.m[0][2];
		const double m10 = in.m[1][0];
		const double m11 = in.m[1][1];
		const double m12 = in.m[1][2];
		const double m20 = in.m[2][0];
		const double m21 = in.m[2][1];
		const double m22 = in.m[2][2];

		const double det =
			m00 * (m11 * m22 - m12 * m21) -
			m01 * (m10 * m22 - m12 * m20) +
			m02 * (m10 * m21 - m11 * m20);

		if (IsNearlyZero(det))
		{
			return false;
		}

		const double invDet = 1.0 / det;

		out.m[0][0] = static_cast<float>((m11 * m22 - m12 * m21) * invDet);
		out.m[0][1] = static_cast<float>((m02 * m21 - m01 * m22) * invDet);
		out.m[0][2] = static_cast<float>((m01 * m12 - m02 * m11) * invDet);

		out.m[1][0] = static_cast<float>((m12 * m20 - m10 * m22) * invDet);
		out.m[1][1] = static_cast<float>((m00 * m22 - m02 * m20) * invDet);
		out.m[1][2] = static_cast<float>((m02 * m10 - m00 * m12) * invDet);

		out.m[2][0] = static_cast<float>((m10 * m21 - m11 * m20) * invDet);
		out.m[2][1] = static_cast<float>((m01 * m20 - m00 * m21) * invDet);
		out.m[2][2] = static_cast<float>((m00 * m11 - m01 * m10) * invDet);
		return true;
	}

	Math::Matrix3 BuildAdaptationMatrix(ChromaticAdaptationMethod method)
	{
		Math::Matrix3 m = {};
		switch (method)
		{
		case ChromaticAdaptationMethod::Bradford:
			m.m[0][0] = 0.895100f; m.m[0][1] = 0.266400f; m.m[0][2] = -0.161400f;
			m.m[1][0] = -0.750200f; m.m[1][1] = 1.713500f; m.m[1][2] = 0.036700f;
			m.m[2][0] = 0.038900f; m.m[2][1] = -0.068500f; m.m[2][2] = 1.029600f;
			break;
		case ChromaticAdaptationMethod::CAT02:
		default:
			m.m[0][0] = 0.732800f; m.m[0][1] = 0.429600f; m.m[0][2] = -0.162400f;
			m.m[1][0] = -0.703600f; m.m[1][1] = 1.697500f; m.m[1][2] = 0.006100f;
			m.m[2][0] = 0.003000f; m.m[2][1] = 0.013600f; m.m[2][2] = 0.983400f;
			break;
		}
		return m;
	}
}

bool Chromaticity::IsValid() const
{
	if (std::abs(y) <= kChromaticityEpsilon)
	{
		return false;
	}

	constexpr float kLimit = 1.5f;
	return x > -kLimit && x < kLimit && y > -kLimit && y < kLimit;
}

bool ColorPrimaries::IsValid() const
{
	return red.IsValid() && green.IsValid() && blue.IsValid();
}

bool ColorSpace::RecomputeMatrices()
{
	if (!primaries.IsValid() || !whitePoint.IsValid())
	{
		return false;
	}

	const Math::Vector3 r = XYToXYZ(primaries.red);
	const Math::Vector3 g = XYToXYZ(primaries.green);
	const Math::Vector3 b = XYToXYZ(primaries.blue);

	Math::Matrix3 m = {};
	m.m[0][0] = r.x; m.m[0][1] = g.x; m.m[0][2] = b.x;
	m.m[1][0] = r.y; m.m[1][1] = g.y; m.m[1][2] = b.y;
	m.m[2][0] = r.z; m.m[2][1] = g.z; m.m[2][2] = b.z;

	Math::Matrix3 mInv = {};
	if (!TryInvertMatrix3(m, mInv))
	{
		return false;
	}

	const Math::Vector3 whiteXYZ = XYToXYZ(whitePoint);
	const Math::Vector3 scale = mInv.TransformVector(whiteXYZ);
	const Math::Matrix3 scaleMat = MakeDiagonal(scale);
	RGBtoXYZ = m * scaleMat;

	if (!TryInvertMatrix3(RGBtoXYZ, XYZtoRGB))
	{
		return false;
	}

	return true;
}

ColorSpace ColorSpace::CreateSRGB()
{
	ColorSpace result;
	result.primaries.red = { 0.64f, 0.33f };
	result.primaries.green = { 0.30f, 0.60f };
	result.primaries.blue = { 0.15f, 0.06f };
	result.whitePoint = { 0.3127f, 0.3290f };
	result.RecomputeMatrices();
	return result;
}

Math::Matrix3 ComputeChromaticAdaptationMatrix(
	const Chromaticity& sourceWhite,
	const Chromaticity& targetWhite,
	ChromaticAdaptationMethod method)
{
	if (!sourceWhite.IsValid() || !targetWhite.IsValid())
	{
		return Math::Matrix3::Identity();
	}

	const Math::Matrix3 m = BuildAdaptationMatrix(method);
	Math::Matrix3 mInv = {};
	if (!TryInvertMatrix3(m, mInv))
	{
		return Math::Matrix3::Identity();
	}

	const Math::Vector3 srcXYZ = XYToXYZ(sourceWhite);
	const Math::Vector3 dstXYZ = XYToXYZ(targetWhite);

	const Math::Vector3 srcLMS = m.TransformVector(srcXYZ);
	const Math::Vector3 dstLMS = m.TransformVector(dstXYZ);

	Math::Vector3 scale = Math::Vector3::One();
	scale.x = (std::abs(srcLMS.x) > kChromaticityEpsilon) ? (dstLMS.x / srcLMS.x) : 1.0f;
	scale.y = (std::abs(srcLMS.y) > kChromaticityEpsilon) ? (dstLMS.y / srcLMS.y) : 1.0f;
	scale.z = (std::abs(srcLMS.z) > kChromaticityEpsilon) ? (dstLMS.z / srcLMS.z) : 1.0f;

	const Math::Matrix3 diag = MakeDiagonal(scale);
	return mInv * (diag * m);
}
