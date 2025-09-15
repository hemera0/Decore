#include "../Util.hlsli"
#include "../PBR.hlsli"

// Asset Specific
[[vk::binding(0, 5)]]
StructuredBuffer<Material> Materials;

[[vk::binding(0, 6)]]
Texture2D Textures[];

[[vk::binding(0, 6)]]
SamplerState Samplers[];

struct PS_Input {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	
	float3 WorldPos : POSITION;
	float4 Tangent : TANGENT;
	float3 ViewPos;

	nointerpolation int index : SV_InstanceID;
};

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
};

[[vk::binding(0, 7)]]
StructuredBuffer<PrimitiveData> SSBO;

float3 CompressNormals(const float3 normal)
{
    return normal * 0.5 + 0.5;
}

float4 main( PS_Input input ) {
	PrimitiveData data = SSBO[input.index];
	Material mat = Materials[int(data.MaterialIndex.x)];
	
	float4 albedoColor = mat.BaseColor;
	
	if(mat.AlbedoMapIndex > -1) {
		albedoColor *= sRGBToLinear( Textures[mat.AlbedoMapIndex].Sample(Samplers[mat.AlbedoMapIndex], input.UV ) );
	}

	if(albedoColor.a < mat.AlphaCutoff)
		discard;
	
	return float4( CompressNormals( normalize(input.Normal) ), 1.f );
}