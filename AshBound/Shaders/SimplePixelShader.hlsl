#include "Common/Common.hlsl"
#include "Common/ColorManagement.hlsl"
#include "Common/ViewParameters.hlsl"

struct PSInput
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Texcoord : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	float2 uv = input.Position.xy * float2(ViewParams.ViewportInvWidth, ViewParams.ViewportInvHeight);
    float3 linearColor = 1;
	float3 tonemapped = linearColor;
	const bool isHdrEotf = (ViewParams.ViewportEotfId == 4u);
	if (ViewParams.TonemapEnabled != 0u && !isHdrEotf)
	{
		tonemapped = ApplyTonemap(linearColor, ViewParams.Tonemap, ViewParams.TonemapMode,
			ViewParams.ViewportWorkingColorSpaceId);
	}
	const float3 outputLinear = float3(
		dot(float3(ViewParams.WorkingToOutputRow0X, ViewParams.WorkingToOutputRow0Y, ViewParams.WorkingToOutputRow0Z), tonemapped),
		dot(float3(ViewParams.WorkingToOutputRow1X, ViewParams.WorkingToOutputRow1Y, ViewParams.WorkingToOutputRow1Z), tonemapped),
		dot(float3(ViewParams.WorkingToOutputRow2X, ViewParams.WorkingToOutputRow2Y, ViewParams.WorkingToOutputRow2Z), tonemapped));
	float3 color = EncodeByEotf(outputLinear, ViewParams.ViewportEotfId, ViewParams.ViewportInvGamma,
		ViewParams.ViewportMaxLuminance);
	return float4(color, 1.0f);
}
