#pragma pack_matrix( row_major )

const int AM_OPAQUE = 0;
const int AM_BLEND = 1;
const int AM_MASK = 2;

struct Material {
	int AlbedoMapIndex;
	int NormalMapIndex;
	int MetalRoughnessMapIndex;
	int EmissiveMapIndex;
	int4 OcclusionMapIndex;
	
	float4 BaseColor;
	float4 EmissiveFactor;
	int AlphaMode;
	float AlphaCutoff;
	
	float MetallicFactor;
	float RoughnessFactor;
};

// Asset Specific
[[vk::binding(0, 4)]]
StructuredBuffer<Material> Materials;

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 WorldMatrix;
	float4 NodePos;
};

[[vk::binding(0, 7)]]
StructuredBuffer<PrimitiveData> SSBO;

struct PS_Input {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	
	float3 WorldPos : POSITION;	
	float3 ViewPos;

	// float4 Tangent : TANGENT;
	
	nointerpolation int index : SV_InstanceID;
};

float4 main(PS_Input input) {
	PrimitiveData primData = SSBO[input.index];
	Material mat = Materials[primData.MaterialIndex.x];

	float3 albedo = float3(mat.BaseColor.rgb);	
	float3 lightDirection = float3(-0.3f, -1.f, -0.7f);
	float diffuse = max(0.f, dot(-lightDirection, input.Normal));

    return float4(input.Normal, 1.f);
}