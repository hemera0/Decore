#include "Util.hlsli"

#pragma pack_matrix( row_major )

const int SKY_INDEX = 1;

[[vk::binding(0, 0)]]
cbuffer _ {
	float4x4 ModelMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;

	float4x4 LightViewMatrix;
	float4x4 LightProjMatrix;
	
	float4 CamPos;
	float4 LightPos;
	float4 LightColor;
};

[[vk::binding(0, 1)]]
TextureCube Textures[];

[[vk::binding(0, 1)]]
SamplerState Samplers[];

struct PS_Input {
	float4 Position : SV_Position;
	float3 UVW;
};

float4 main(PS_Input input) : SV_TARGET
{
    uint width, height, levels;
    Textures[SKY_INDEX].GetDimensions(0, width, height, levels);

	float4 color = sRGBToLinear(Textures[SKY_INDEX].SampleLevel(Samplers[SKY_INDEX], input.UVW, 0));

	color.rgb = tonemap(color.rgb);
	color.rgb = LinearTosRGB(color.rgb);

	return color;
}