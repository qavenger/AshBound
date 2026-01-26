#include "Common/Common.hlsl"
#include "Common/ColorManagement.hlsl"
#include "Common/ViewParameters.hlsl"

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	const float2 uv = input.Position.xy * float2(ViewParams.ViewportInvWidth, ViewParams.ViewportInvHeight);

	const float2 uvInset = uv;
	const float2 gridSize = float2(12.0, 6.0);
	const float2 gridPos = uvInset * gridSize;
	const float2 cellId = floor(gridPos);
	const float2 cellUv = frac(gridPos) - 0.5;

	const float radius = 0.38;
	const float cellAspect = (ViewParams.ViewportWidth * gridSize.y) * ViewParams.ViewportInvHeight / max(gridSize.x, 1e-4);
	const float2 cellUvAspect = float2(cellUv.x * cellAspect, cellUv.y);
	const float dist = length(cellUvAspect);
	const float mask = step(dist, radius);

	const float hue = cellId.x / max(gridSize.x, 1.0);
	const float rowIndex = cellId.y;
	const float saturation = 1.0;
	const float maxNits = max(ViewParams.ViewportMaxLuminance, 1.0);
	const float value = (100.0 / maxNits) * exp2(rowIndex*2);
    float3 circleColor = HSVToRGB(float3(hue, saturation, value));
	//circleColor = mul(sRGB_2_AP1_MAT, srgbColor);

	const float3 background = float3(0.0, 0.0, 0.0);
	const float3 linearColor = background + circleColor * mask;
	float3 tonemapped = linearColor;
	const bool isHdrEotf = (ViewParams.ViewportEotfId == 4u);
	if (ViewParams.TonemapEnabled != 0u)
	{
		tonemapped = ApplyTonemap(linearColor, ViewParams.Tonemap, ViewParams.TonemapMode,
			ViewParams.ViewportWorkingColorSpaceId);
	}
	const float3 outputLinear = float3(
		dot(float3(ViewParams.WorkingToOutputRow0X, ViewParams.WorkingToOutputRow0Y, ViewParams.WorkingToOutputRow0Z), tonemapped),
		dot(float3(ViewParams.WorkingToOutputRow1X, ViewParams.WorkingToOutputRow1Y, ViewParams.WorkingToOutputRow1Z), tonemapped),
		dot(float3(ViewParams.WorkingToOutputRow2X, ViewParams.WorkingToOutputRow2Y, ViewParams.WorkingToOutputRow2Z), tonemapped));
	const float3 encoded = EncodeByEotf(outputLinear, ViewParams.ViewportEotfId, ViewParams.ViewportInvGamma,
		ViewParams.ViewportMaxLuminance);
	return float4(encoded, 1.0f);
}
