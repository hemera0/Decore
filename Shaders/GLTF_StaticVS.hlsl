#pragma pack_matrix(row_major)

const int MAX_JOINTS = 256;

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
	//float4x4 JointMatrix[MAX_JOINTS];
    //int4 JointCount;
};

[[vk::binding(0, 7)]]
StructuredBuffer<PrimitiveData> SSBO;

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
	float4 Joints : JOINTS;
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
	PrimitiveData data = SSBO[input.index];

    VS_Output res;
    res.Position = float4(input.Position, 1.f);
	
	/*
	if(data.JointCount.x > 0) {
		float4x4 jointMatrix0 = data.JointMatrix[int(input.Joints.x)];
        float4x4 jointMatrix1 = data.JointMatrix[int(input.Joints.y)];
        float4x4 jointMatrix2 = data.JointMatrix[int(input.Joints.z)];
        float4x4 jointMatrix3 = data.JointMatrix[int(input.Joints.w)];

		float4x4 skinMat = mul(jointMatrix0, input.Weights.x) +
            mul(jointMatrix1, input.Weights.y) +
            mul(jointMatrix2, input.Weights.z) +
            mul(jointMatrix3, input.Weights.w);

		res.Position = mul(res.Position, skinMat);
	}
	*/

	res.Position = mul(res.Position, data.NodeMatrix);
	res.Position = mul(res.Position, ModelMatrix);
	
	res.WorldPos = res.Position.xyz;
	
	// Apply View & Projection Matrices.
	res.Position = mul(res.Position, ViewMatrix);
	res.ViewPos = res.Position.xyz;

	res.Position = mul(res.Position, ProjectionMatrix);
	
	// float3x3 normMatrix = transpose(Inverse(mul(float3x3(ModelMatrix), float3x3(ViewMatrix)))); 

	float3 norm = mul(input.Normal, float3x3(data.NodeMatrix));
	norm = mul(norm, float3x3(ModelMatrix));
	// norm = mul(norm, normMatrix);
	res.Normal = normalize(norm);

	res.UV = input.UV;
	res.Tangent = input.Tangent;
	res.index = input.index;

    return res;
}