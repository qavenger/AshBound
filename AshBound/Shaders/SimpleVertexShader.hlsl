#include "Common/Common.hlsl"
#include "Common/ViewParameters.hlsl"

struct VSInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Texcoord : TEXCOORD0;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Texcoord : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	output.Position = mul(float4(input.Position, 1.0f), ViewParams.ViewProjection);
	output.Normal = input.Normal;
	output.Texcoord = input.Texcoord;
	return output;
}
