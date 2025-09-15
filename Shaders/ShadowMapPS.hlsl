#include "Util.hlsli"
#include "PBR.hlsli"

// Asset Specific
[[vk::binding(0, 2)]]
StructuredBuffer<Material> Materials;

[[vk::binding(0, 3)]]
Texture2D Textures[];

[[vk::binding(0, 3)]]
SamplerState Samplers[];

struct PS_Input {
	float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
	nointerpolation int index : SV_InstanceID;
};

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
};

[[vk::binding(0, 4)]]
StructuredBuffer<PrimitiveData> Primitives;

void main( PS_Input input ) {
	PrimitiveData data = Primitives[input.index];
	Material mat = Materials[int(data.MaterialIndex.x)];
	
	float4 albedoColor = mat.BaseColor;
	
	if(mat.AlbedoMapIndex > -1) {
		albedoColor *= sRGBToLinear( Textures[mat.AlbedoMapIndex].Sample(Samplers[mat.AlbedoMapIndex], input.UV ) );
	}

	if(albedoColor.a < mat.AlphaCutoff)
		discard;
}