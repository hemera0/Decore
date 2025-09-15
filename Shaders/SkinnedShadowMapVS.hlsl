#pragma pack_matrix(row_major)

#define SHADOW_MAP_CASCADE_COUNT 4

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
};

const int MAX_JOINTS = 128;

struct BoneData {
	float4x4 JointMatrix[MAX_JOINTS];
	int JointCount;
	int NodeIndex;
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

[[vk::binding(0, 5)]]
StructuredBuffer<float4x4> JointMatrices;

struct VS_Input {
	float3 Position : POSIITON;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float4 Tangent : TANGENT;
	uint4 Joints : JOINTS;
	float4 Weights : WEIGHTS;

	int Index : SV_InstanceID;
};

float4 main(VS_Input input) : SV_POSITION
{
	float4 res = float4(input.Position, 1.f);
	
	PrimitiveData primData = SSBO[input.Index];

	float4x4 skinMat = mul(JointMatrices[input.Joints.x], input.Weights.x) +
		mul(JointMatrices[input.Joints.y], input.Weights.y) +
		mul(JointMatrices[input.Joints.z], input.Weights.z) +
		mul(JointMatrices[input.Joints.w], input.Weights.w);

	res = mul(res, skinMat);
	
	res = mul(res, ModelMatrix);
    res = mul(res, primData.NodeMatrix);
	res = mul(res, CascadeViewMatrices[CascadeIndex]);
	res = mul(res, CascadeProjMatrices[CascadeIndex]);
	
    return res;
}