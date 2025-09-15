#pragma pack_matrix(row_major)

#include "Util.hlsli"
#include "PBR.hlsli"

#define SHADOW_MAP_CASCADE_COUNT 4

// Simplifies texture sampling.
#define SampleTexture(index, uv) Textures[index].Sample(Samplers[index], uv)

[[vk::push_constant]]
cbuffer PushConsts {
	uint SSAOEnabled;
};

// Scene Uniforms.
[[vk::binding(0, 0)]]
cbuffer UBO {
	float4x4 ModelMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	
	float4 CamPos;
	float4 LightDirection;
	float4 LightColor;
};

// IBL Environment textures.
[[vk::binding(0, 1)]]
Texture2D GlobalTextures[];

[[vk::binding(0, 1)]]
TextureCube GlobalCubeTextures[];

[[vk::binding(0, 1)]]
SamplerState GlobalSamplers[];

// Shadow map.
[[vk::binding(0, 2)]]
Texture2DArray ShadowMap;

[[vk::binding(0, 2)]]
SamplerState ShadowMapSampler;

[[vk::binding(0, 3)]]
cbuffer ShadowMapUBO {
	float4 CascadeSplits;
	float4x4 CascadeViewMatrices[SHADOW_MAP_CASCADE_COUNT];
	float4x4 CascadeProjMatrices[SHADOW_MAP_CASCADE_COUNT];
};

[[vk::binding(0, 4)]]
Texture2D DepthTexture;

[[vk::binding(0, 4)]]
SamplerState DepthSampler;

// Asset Specific
[[vk::binding(0, 5)]]
StructuredBuffer<Material> Materials;

[[vk::binding(0, 6)]]
Texture2D Textures[];

[[vk::binding(0, 6)]]
SamplerState Samplers[];

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
	float4 NodePos;
};

[[vk::binding(0, 7)]]
StructuredBuffer<PrimitiveData> SSBO;

//[[vk::binding(0, 5)]]
//StructuredBuffer<PointLight> PointLights;

const int IR_DIFFUSE_MAP = 0;
const int IR_SPECULAR_MAP = 1;
const int BRDF_LUT = 2;

struct PS_Input {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	
	float3 WorldPos : POSITION;
	float4 Tangent : TANGENT;
	
	float3 ViewPos;

	nointerpolation int index : SV_InstanceID;
};

float3 ComputeIBLLight(PBRData data) 
{
	float3 irradiance = sRGBToLinear(GlobalCubeTextures[IR_DIFFUSE_MAP].Sample(GlobalSamplers[IR_DIFFUSE_MAP], data.N).rgb);
	
	float3 F = fresnelSchlick(data.F0, data.cosLo);
	
	float3 kd = lerp(1.f - F, 0.f, data.metalness);
	float3 diffuse = kd * data.surfaceColor * irradiance;
	
	// determine number of mipmap levels for specular IBL environment map.
    uint width, height, specularTextureLevels;
    GlobalCubeTextures[IR_SPECULAR_MAP].GetDimensions(0, width, height, specularTextureLevels);
	
	// Sample pre-filtered specular reflection environment at correct mipmap level.
    float3 specularIrradiance = sRGBToLinear(GlobalCubeTextures[IR_SPECULAR_MAP].SampleLevel(GlobalSamplers[IR_SPECULAR_MAP], data.Lr, data.roughness * specularTextureLevels).rgb);

	// Split-sum approximation factors for Cook-Torrance specular BRDF. 
    float2 specularBRDF = GlobalTextures[BRDF_LUT].Sample(GlobalSamplers[BRDF_LUT], float2(data.cosLo, data.roughness)).rg;

	// Total specular IBL contribution.
	float3 specular = (data.F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
	
	return ( diffuse + specular );
}

float GetShadow(float4 lightCoord, float3 N, uint cascadeIndex) {
	float shadow = 0.f;
	
	lightCoord.xy = lightCoord.xy * 0.5 + 0.5;

	uint width;
	uint height;
	uint layer;
	ShadowMap.GetDimensions(width, height, layer);

	float bias = max(0.05 * (1.0 - dot(N, -LightDirection.xyz)), 0.005);  
	
	float2 texelSize = float2(1.f / width, 1.f / height);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = ShadowMap.Sample(ShadowMapSampler, float3( lightCoord.xy + ( float2(x, y) * texelSize ), cascadeIndex ) ).r; 				
			if(pcfDepth + bias >= lightCoord.z )
				shadow += 1.f;				
		}
	}

	return shadow / 9.f;
}

float GetShadowHard(float4 lightCoord, float3 N, uint cascadeIndex) {
	float shadow = 0.f;

	lightCoord.xy = lightCoord.xy * 0.5 + 0.5;

	float bias = 0.005f;
	float depth = ShadowMap.Sample(ShadowMapSampler, float3(lightCoord.xy, cascadeIndex)).r;
	if( depth + bias >= lightCoord.z )
		shadow = 1.f;

	return shadow;
}

struct PS_Output {
	float4 MainColor : SV_Target0;
	// float4 BloomColor : SV_Target1; 
};

PS_Output main(PS_Input input) {
	PrimitiveData data = SSBO[input.index];
	Material mat = Materials[int(data.MaterialIndex.x)];
	
	float4 albedoColor = mat.BaseColor;
	if(mat.AlbedoMapIndex > -1) {
		albedoColor *= sRGBToLinear( SampleTexture( mat.AlbedoMapIndex, input.UV ) );
	}
	
	if(mat.AlphaMode == AM_OPAQUE) {
		albedoColor.a = 1.f;
	}

	float3 MRA = float3(1.f);
	if(mat.MetalRoughnessMapIndex > -1) {
		MRA = SampleTexture( mat.MetalRoughnessMapIndex, input.UV ).rgb;
	}
	
	// emissive factor defaults to 0. 
	float4 emissiveColor = mat.EmissiveFactor;
	if(mat.EmissiveMapIndex > -1) {
		emissiveColor *= sRGBToLinear( SampleTexture( mat.EmissiveMapIndex, input.UV ) );
	}

	float occlusion = MRA.r * 1.f;
	float perceptualRoughness = saturate(MRA.g * mat.RoughnessFactor);
	float metalness = saturate(MRA.b * mat.MetallicFactor);

	if(mat.OcclusionMapIndex.x > -1) {
		occlusion *= SampleTexture( mat.OcclusionMapIndex.x, input.UV ).r;
	}

	float3 N = normalize(input.Normal);
	if(mat.NormalMapIndex > -1 ) {
		float3 nmap = SampleTexture( mat.NormalMapIndex, input.UV ).rgb * 2.f - 1.f;

		// calc tangent
		float3 q1 = ddx(input.WorldPos);
		float3 q2 = ddy(input.WorldPos);
		float2 st1 = ddx(input.UV);
		float2 st2 = ddy(input.UV);

		float3 tangent = normalize(q1 * st2.x - q2 * st1.x);
		float3 bitangent = normalize(cross(N, tangent));
        
        // float3 tangent = normalize(input.Tangent.xyz);
        // float3 bitangent = cross(N, tangent) * input.Tangent.w;
        
		// calc tbn
        float3x3 tbn = float3x3(tangent, bitangent, N);
        
		// apply tbn matrix
        N = normalize(mul(nmap, tbn));
		// N.y *= -1.f;
	}
	
	// Outgoing light direction.
    float3 Lo = normalize(CamPos.xyz - input.WorldPos);
	
	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(N, Lo));
	
	// Specular reflection vector.
	float3 Lr = 2.0 * cosLo * N - Lo;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	float3 F0 = lerp(Fdielectric, albedoColor.rgb, metalness);

	float3 lightDir = -LightDirection.xyz; //normalize(input.WorldPos - LightPos.xyz);

	PBRData pbrData = (PBRData)0;	
	pbrData.F0 = F0;
	pbrData.N = N;
	pbrData.Lo = Lo;
	pbrData.Lr = Lr;
	pbrData.cosLo = cosLo;
	pbrData.surfaceColor = albedoColor.rgb;
	pbrData.lightDir = lightDir;
	pbrData.lightColor = LightColor.rgb;
	pbrData.metalness = metalness;
	pbrData.roughness = perceptualRoughness;

	// Every scene is expected to have at least 1 directional sky light. 
	// If this is not the case then your scene should have an ambient light modifier.
	float3 directLight = ComputeDirectionalLight(pbrData);

	// Fake GI setup. Illuminates the darker parts of the scene by casting a light from the opposite direction.
	// float3 fakeGILightDir = -lightDir;
	// float3 fakeGILightCol = LightColor.rgb * float3(0.2);

	// pbrData.lightDir = fakeGILightDir;
	// pbrData.lightColor = fakeGILightCol;

	// float3 fakeGILight = ComputeDirectionalLightNoSpecular(pbrData);

	/*
	uint lightCount = 0;
	uint lightStride = 0;
	PointLights.GetDimensions(lightCount, lightStride);
	
	for(int i = 0; i < lightCount; i++) {
		PointLight light = PointLights[i];
		if(light.Enabled) {	
			directLight += 	ComputePointLight(light, input.WorldPos, F0, N, Lo, cosLo, albedoColor.rgb, metalness, roughness);
		}
	}
	*/
	float3 iblLight = ComputeIBLLight(pbrData);

	// Get Shadow.
	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(-input.ViewPos.z < CascadeSplits[i]) {
			cascadeIndex = i + 1;
		}
	}
	
	float4 shadowCoord = mul( float4(input.WorldPos, 1.f), CascadeViewMatrices[cascadeIndex]);
	shadowCoord = mul(shadowCoord, CascadeProjMatrices[cascadeIndex]);

	// float shadow = 0.f;
	float shadow = GetShadow(shadowCoord / shadowCoord.w, N, cascadeIndex);

	if( SSAOEnabled == 1 ) {
		uint width;
		uint height;
		DepthTexture.GetDimensions(width, height);

		float2 texScale = float2( 1.f / width, 1.f / height );

		occlusion = DepthTexture.Sample(DepthSampler, input.Position.xy * texScale).r; 
	} else {
		occlusion = 1.f;
	}

	const float iblIntensity = 1.f;

	float3 color = (directLight * shadow);
	// color += fakeGILight;
	color += (iblLight * ( iblIntensity * occlusion ) );
	color += emissiveColor.rgb;

	// CSM Debugging.
	// color *= GetCascadeColorTint(cascadeIndex);
	// color.a = 1.f;

	if(mat.AlphaMode == AM_MASK) {
		if(albedoColor.a < mat.AlphaCutoff)
			discard;

		albedoColor.a = 1.f;
		//albedoColor.a = (albedoColor.a - mat.AlphaCutoff) / max(fwidth(albedoColor.a), 0.0001) + 0.5;
		//albedoColor.a = min(albedoColor.a, 1.f);
	}

	// color.a = 0.f;
	// if(mat.AlphaMode == AM_BLEND) {
	// 	albedoColor.a = 0.5f;
	// }
	// return float4( ( color ), albedoColor.a );
	
	// color = tonemap( color );
	// color = LinearTosRGB( color );
	
	PS_Output output;
	output.MainColor = float4( color, albedoColor.a );

	return output;	
}