[[vk::binding(0, 0)]]
Texture2D InputTexture;
[[vk::binding(0, 0)]]
SamplerState InputSampler;

[[vk::push_constant]]
cbuffer _ {
    float2 SrcResolution;
};

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

const float filterRadius = 0.005f;

float4 main(VS_Output input) {
	float x = filterRadius;
	float y = filterRadius;

    // https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom + Call Of Duty: Advanced Warfare
    float3 a = InputTexture.Sample(InputSampler, float2(input.UV.x - x, input.UV.y + y)).rgb;
    float3 b = InputTexture.Sample(InputSampler, float2(input.UV.x,     input.UV.y + y)).rgb;
    float3 c = InputTexture.Sample(InputSampler, float2(input.UV.x + x, input.UV.y + y)).rgb;

    float3 d = InputTexture.Sample(InputSampler, float2(input.UV.x - x, input.UV.y)).rgb;
    float3 e = InputTexture.Sample(InputSampler, float2(input.UV.x,     input.UV.y)).rgb;
    float3 f = InputTexture.Sample(InputSampler, float2(input.UV.x + x, input.UV.y)).rgb;

    float3 g = InputTexture.Sample(InputSampler, float2(input.UV.x - x, input.UV.y - y)).rgb;
    float3 h = InputTexture.Sample(InputSampler, float2(input.UV.x,     input.UV.y - y)).rgb;
    float3 i = InputTexture.Sample(InputSampler, float2(input.UV.x + x, input.UV.y - y)).rgb;

	float3 upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;

	return float4(upsample, 1.f);
}