#include "Common/Common.hlsl"

struct VSInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Texcoord : TEXCOORD0;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	output.Position = float4(input.Position, 1.0f);
	output.Texcoord = input.Texcoord;
	return output;
}
