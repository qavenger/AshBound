#pragma once

#ifndef VIEW_PARAMETERS_SHARED_H
#define VIEW_PARAMETERS_SHARED_H

#ifdef VIEW_PARAMETERS_HLSL
	#define VP_ALIGN
	#define VP_MATRIX float4x4
	#define VP_FLOAT float
	#define VP_UINT uint
#else
	#include <cstdint>
	#include "Runtime/Core/Math/Matrix4.h"

	#define VP_ALIGN alignas(16)
	#define VP_MATRIX Math::Matrix4
	#define VP_FLOAT float
	#define VP_UINT uint32_t
#endif

struct VP_ALIGN TonemapParams
{
	VP_FLOAT Exposure;
	VP_FLOAT Contrast;
	VP_FLOAT ToeStrength;
	VP_FLOAT ToeLength;

	VP_FLOAT ShoulderStrength;
	VP_FLOAT ShoulderLength;
	VP_FLOAT ShoulderAngle;
	VP_FLOAT Gamma;

	VP_FLOAT Saturation;
	VP_FLOAT Lift;
	VP_FLOAT Gain;
	VP_FLOAT WhitePoint;

	VP_FLOAT BlackPoint;
	VP_FLOAT MaxLuminance;
	VP_FLOAT MidGrey;
	VP_FLOAT Padding0;
};

struct VP_ALIGN ViewParameters
{
	VP_MATRIX View;
	VP_MATRIX Projection;
	VP_MATRIX ViewProjection;
	VP_MATRIX InverseView;
	VP_MATRIX InverseProjection;
	VP_MATRIX InverseViewProjection;

	VP_FLOAT CameraPositionX;
	VP_FLOAT CameraPositionY;
	VP_FLOAT CameraPositionZ;
	VP_FLOAT CameraPositionW;

	VP_FLOAT CameraFovDegrees;
	VP_FLOAT CameraOrthoSize;
	VP_FLOAT CameraAspectRatio;
	VP_FLOAT CameraIsOrthographic;

	VP_FLOAT CameraNearPlane;
	VP_FLOAT CameraFarPlane;
	VP_FLOAT CameraPadding0;
	VP_FLOAT CameraPadding1;

	VP_FLOAT ViewportWidth;
	VP_FLOAT ViewportHeight;
	VP_FLOAT ViewportInvWidth;
	VP_FLOAT ViewportInvHeight;

	VP_FLOAT ViewportAspectRatio;
	VP_FLOAT ViewportInvAspectRatio;
	VP_FLOAT ViewportCenterX;
	VP_FLOAT ViewportCenterY;

	VP_UINT ViewportWorkingColorSpaceId;
	VP_UINT ViewportOutputGamutId;
	VP_UINT ViewportEotfId;
	VP_UINT TonemapEnabled;
	VP_UINT TonemapMode;
	VP_UINT TonemapPadding0;
	VP_UINT TonemapPadding1;
	VP_UINT TonemapPadding2;

	VP_FLOAT ViewportMaxLuminance;
	VP_FLOAT ViewportMinLuminanceLog10;
	VP_FLOAT ViewportGamma;
	VP_FLOAT ViewportInvGamma;

	VP_FLOAT WorkingToOutputRow0X;
	VP_FLOAT WorkingToOutputRow0Y;
	VP_FLOAT WorkingToOutputRow0Z;
	VP_FLOAT WorkingToOutputRow0W;

	VP_FLOAT WorkingToOutputRow1X;
	VP_FLOAT WorkingToOutputRow1Y;
	VP_FLOAT WorkingToOutputRow1Z;
	VP_FLOAT WorkingToOutputRow1W;

	VP_FLOAT WorkingToOutputRow2X;
	VP_FLOAT WorkingToOutputRow2Y;
	VP_FLOAT WorkingToOutputRow2Z;
	VP_FLOAT WorkingToOutputRow2W;

	TonemapParams Tonemap;
};

#ifdef VIEW_PARAMETERS_HLSL
cbuffer ViewParametersBuffer : register(b0)
{
	ViewParameters ViewParams;
};
#else
static_assert(sizeof(ViewParameters) % 16 == 0, "ViewParameters must be 16-byte aligned.");
#endif

#undef VP_ALIGN
#undef VP_MATRIX
#undef VP_FLOAT
#undef VP_UINT

#endif
