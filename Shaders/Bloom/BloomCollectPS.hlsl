#include "../Util.hlsli"

[[vk::binding(0, 0)]]
Texture2D InputTexture;
[[vk::binding(0, 0)]]
SamplerState InputSampler;

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float4 main(VS_Output input) {
	float4 color = InputTexture.Sample(InputSampler, input.UV);

    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
	if(brightness >= 1.f)
		return float4(color.rgb, 1.f);

	return float4(0.f, 0.f, 0.f, 1.f);
}