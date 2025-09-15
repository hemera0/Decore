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

float4 main(VS_Output input) {
	float2 texScale = float2(1.f / SrcResolution);

	float x = texScale.x;
	float y = texScale.y;

	// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom + Call Of Duty: Advanced Warfare
	float3 a = InputTexture.Sample(InputSampler, float2(input.UV.x - 2 * x, input.UV.y + 2 * y)).rgb;
    float3 b = InputTexture.Sample(InputSampler, float2(input.UV.x,       input.UV.y + 2 * y)).rgb;
    float3 c = InputTexture.Sample(InputSampler, float2(input.UV.x + 2 * x, input.UV.y + 2 * y)).rgb;

    float3 d = InputTexture.Sample(InputSampler, float2(input.UV.x - 2 * x, input.UV.y)).rgb;
    float3 e = InputTexture.Sample(InputSampler, float2(input.UV.x,       input.UV.y)).rgb;
    float3 f = InputTexture.Sample(InputSampler, float2(input.UV.x + 2 * x, input.UV.y)).rgb;

    float3 g = InputTexture.Sample(InputSampler, float2(input.UV.x - 2 * x, input.UV.y - 2 * y)).rgb;
    float3 h = InputTexture.Sample(InputSampler, float2(input.UV.x,       input.UV.y - 2 * y)).rgb;
    float3 i = InputTexture.Sample(InputSampler, float2(input.UV.x + 2 * x, input.UV.y - 2 * y)).rgb;

    float3 j =  InputTexture.Sample(InputSampler, float2(input.UV.x - x, input.UV.y + y)).rgb;
    float3 k =  InputTexture.Sample(InputSampler, float2(input.UV.x + x, input.UV.y + y)).rgb;
    float3 l =  InputTexture.Sample(InputSampler, float2(input.UV.x - x, input.UV.y - y)).rgb;
    float3 m =  InputTexture.Sample(InputSampler, float2(input.UV.x + x, input.UV.y - y)).rgb;

	float3 downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;
    downsample = max(downsample, 0.0001f);

	return float4(downsample, 1.f);
}