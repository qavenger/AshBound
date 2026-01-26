#ifndef COMMON_HLSL
#define COMMON_HLSL

#pragma pack_matrix(row_major)

static const float3 Luma_sRGB = float3(0.212639, 0.7151687, 0.0721923);
static const float3 Luma_P3D65 = float3(0.2289746, 0.6917385, 0.0792869);
static const float3 Luma_Rec2020 = float3(0.2627002, 0.6779981, 0.0593017);
static const float3 Luma_AP1 = float3(0.2722287, 0.6740818, 0.0536895);
static const float3 Luma_AP0 = float3(0.3439664, 0.7281661, -0.0721325);

#define WORKING_COLORSPACE_SRGB 0u
#define WORKING_COLORSPACE_P3D65 1u
#define WORKING_COLORSPACE_BT2020 2u
#define WORKING_COLORSPACE_AP1 3u
#define WORKING_COLORSPACE_AP0 4u

float LuminanceFromWorkingRGB(float3 rgb, uint workingColorSpaceId)
{
	switch (workingColorSpaceId)
	{
	case WORKING_COLORSPACE_P3D65:
		return dot(rgb, Luma_P3D65);
	case WORKING_COLORSPACE_BT2020:
		return dot(rgb, Luma_Rec2020);
	case WORKING_COLORSPACE_AP1:
		return dot(rgb, Luma_AP1);
	case WORKING_COLORSPACE_AP0:
		return dot(rgb, Luma_AP0);
	default:
		return dot(rgb, Luma_sRGB);
	}
}

float3 RGBToHSV(float3 rgb)
{
	float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(rgb.bg, K.wz), float4(rgb.gb, K.xy), step(rgb.b, rgb.g));
	float4 q = lerp(float4(p.xyw, rgb.r), float4(rgb.r, p.yzx), step(p.x, rgb.r));

	float d = q.x - min(q.w, q.y);
	float e = 1e-10;
	float3 hsv;
	hsv.x = abs(q.z + (q.w - q.y) / (6.0 * d + e));
	hsv.y = d / (q.x + e);
	hsv.z = q.x;
	return hsv;
}

float3 HSVToRGB(float3 hsv)
{
	float3 p = abs(frac(hsv.xxx + float3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
	return hsv.z * lerp(float3(1.0, 1.0, 1.0), saturate(p - 1.0), hsv.y);
}

float3 HueShift(float3 rgb, float hueShift)
{
	float3 hsv = RGBToHSV(rgb);
	hsv.x = frac(hsv.x + hueShift);
	return HSVToRGB(hsv);
}

float3 RotateAngleAxis(float3 v, float3 axis, float angleRadians)
{
	axis = normalize(axis);
	const float s = sin(angleRadians);
	const float c = cos(angleRadians);
	return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0 - c);
}

#endif
