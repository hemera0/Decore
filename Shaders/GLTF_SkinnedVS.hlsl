#pragma pack_matrix(row_major)

const int MAX_JOINTS = 128;

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 WorldMatrix;
	float4 NodePos;
};

[[vk::binding(0, 7)]]
StructuredBuffer<PrimitiveData> SSBO;

[[vk::binding(0, 8)]]
StructuredBuffer<float4x4> JointMatrices;

[[vk::binding(0, 0)]]
cbuffer UBO {
	float4x4 ModelMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;

	float4 CamPos;
	float4 LightDirection;
	float4 LightColor;
};

struct VS_Input {
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float4 Tangent : TANGENT;
	uint4 Joints : JOINTS;
	float4 Weights : WEIGHTS;

	int index : SV_InstanceID;
};

struct VS_Output {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	
	float3 WorldPos : POSITION;
	float4 Tangent : TANGENT;
	float3 ViewPos;

	nointerpolation int index : SV_InstanceID;
};

VS_Output main(VS_Input input)
{
    VS_Output res;
    res.Position = float4(input.Position, 1.f);
 
	PrimitiveData primData = SSBO[input.index];

	float4x4 skinMat = mul(JointMatrices[input.Joints.x], input.Weights.x) +
		mul(JointMatrices[input.Joints.y], input.Weights.y) +
		mul(JointMatrices[input.Joints.z], input.Weights.z) +
		mul(JointMatrices[input.Joints.w], input.Weights.w);

	res.Position = mul(res.Position, skinMat);
	res.Position = mul(res.Position, ModelMatrix);
	res.Position = mul(res.Position, primData.WorldMatrix);
	res.WorldPos = res.Position.xyz;

	res.Normal = mul(input.Normal, float3x3(skinMat));
	res.Normal = mul(res.Normal, float3x3(ModelMatrix));
	res.Normal = mul(res.Normal, float3x3(primData.WorldMatrix));
	res.Normal = normalize(res.Normal);	
	
	// Apply View & Projection Matrices.
	res.Position = mul(res.Position, ViewMatrix);
	res.ViewPos = res.Position;

	res.Position = mul(res.Position, ProjectionMatrix);
	
	res.UV = input.UV;
	res.index = input.index;

    return res;
}