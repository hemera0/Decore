#include "Util.hlsli"
#include "ACES.hlsl"

[[vk::binding(0, 0)]]
Texture2D InputTexture;
[[vk::binding(0, 0)]]
SamplerState InputSampler;

[[vk::binding(0, 1)]]
Texture2D BloomTexture;
[[vk::binding(0, 1)]]
SamplerState BloomSampler;

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

const float bloomStrength = 0.06f;

float4 main(VS_Output input) {
	float4 color = InputTexture.Sample(InputSampler, input.UV);
	float4 bloom = BloomTexture.Sample(BloomSampler, input.UV);

	// I think this approach is good for now but naive.
	// [branch]
	//if(bloom.a != 0.f) {
	color.rgb = lerp(color.rgb, bloom.rgb, bloomStrength);
	//}
	color.rgb = tonemap(color.rgb);
    color.rgb = LinearTosRGB(color.rgb);

	return float4(color.rgb, 1.f);
}