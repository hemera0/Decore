[[vk::binding(0, 0)]]
Texture2D DrawTexture;
[[vk::binding(0, 0)]]
SamplerState DrawSampler;

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float4 main(VS_Output input) {
	return float4(DrawTexture.Sample(DrawSampler, input.UV).rgb, 1.f);
}