#pragma pack_matrix(row_major)

#define SHADOW_MAP_CASCADE_COUNT 4

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
};

[[vk::push_constant]]
cbuffer PushConst {
	int CascadeIndex;
};

[[vk::binding(0, 0)]]
cbuffer _ {
	float4x4 ModelMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

[[vk::binding(0, 1)]]
cbuffer ShadowMapUBO {
	float4 CascadeSplits;
	float4x4 CascadeViewMatrices[SHADOW_MAP_CASCADE_COUNT];
	float4x4 CascadeProjMatrices[SHADOW_MAP_CASCADE_COUNT];
};

[[vk::binding(0, 4)]]
StructuredBuffer<PrimitiveData> SSBO;

struct VS_Input {
	float3 Position : POSIITON;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float4 Tangent : TANGENT;

	int Index : SV_InstanceID;
};

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
	nointerpolation int Index : SV_InstanceID;
};

VS_Output main(VS_Input input)
{
	VS_Output outp;

	float4 res = float4(input.Position, 1.f);
	
	PrimitiveData data = SSBO[input.Index];

	res = mul(res, data.NodeMatrix);
	res = mul(res, ModelMatrix);
	res = mul(res, CascadeViewMatrices[CascadeIndex]);
	res = mul(res, CascadeProjMatrices[CascadeIndex]);

	outp.Position = res;
	outp.UV = input.UV;
	outp.Index = input.Index;

    return outp;
}