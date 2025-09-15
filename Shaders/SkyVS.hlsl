#pragma pack_matrix(row_major)

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

struct VS_Input {
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD;

	
};

struct VS_Output {
	float4 Position : SV_Position;
	float3 UVW;
};

const float4x4 Model =
{
    { 1, 0, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 0, -1, 0 },
    { 0, 0, 0, 1 }
};

VS_Output main(VS_Input input) {
	VS_Output res;
	
	res.UVW = input.Position;
	
	float4x4 mat = ViewMatrix;
	mat[3][0] = mat[3][1] = mat[3][2] = 0.f;

	res.Position = mul(float4(input.Position, 1.f), Model);
	res.Position = mul(res.Position, mat);
	res.Position = mul(res.Position, ProjectionMatrix);
	
	return res;
}