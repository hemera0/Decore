// Credits
// https://learnopengl.com/PBR/Lighting
// https://github.com/Nadrin/PBR
// https://seblagarde.wordpress.com/2011/08/17/hello-world/

const float PI = 3.141592;
const float Epsilon = 0.00001;

// Constant normal incidence Fresnel factor for all dielectrics.
const float3 Fdielectric = 0.04;

// Basic PBR Point Lights
struct PointLight {
	float4 Position;
	float4 Color;
	
	float Intensity;
	float Range;
};

struct PBRData {
	float3 F0; // Fresnel reflectance at normal incidence.
	float3 N; // Shading normal.
	float3 Lo; // Outgoing light dir.
	float3 Lr; // Specular reflection vector.
	float cosLo; // aka NdotLo
	float3 surfaceColor; // Surface albedo.
	float3 lightDir; // Light direction.
	float3 lightColor; // Light color.
	float metalness;
	float roughness;
};

// Material Info
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

float GGX_ndf(float cosLh, float roughness)
{
	// float a = cosLh * roughness;
	// float k = roughness / (1.f - cosLh * cosLh + a * a);
	// return k * k * ( 1.0 / PI );
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float GGX_gaSchlick(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float GetRangeAttenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0 / pow(distance, 2.0);
    }
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

float3 GetPointLightIntensity(PointLight Light, float3 Li)
{
    return GetRangeAttenuation(Light.Range, length(Li)) * Light.Intensity * Light.Color;
}

float3 ComputeDirectionalLight(PBRData data) 
{
	float3 Li = data.lightDir;
	float3 Lradiance = data.lightColor;
	
	float3 Lh = normalize(Li + data.Lo);
	
	float cosLi = max(0.f, dot(data.N, Li));
	float cosLh = max(0.f, dot(data.N, Lh));

	float3 F = fresnelSchlick(data.F0, max(0.f, dot(Lh, data.Lo)));
	
	float D = GGX_ndf(cosLh, data.roughness);
	// float G = V_SmithGGXCorrelated(data.cosLo, cosLi, data.roughness * data.roughness);
	float G = GGX_gaSchlick(cosLi, data.cosLo, data.roughness);
	
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), data.metalness); 
	float3 diffuse = kd * data.surfaceColor;
	float3 specular = (F*D*G) / max(Epsilon, 4.0 * cosLi * data.cosLo);
	
	return (diffuse + specular) * Lradiance * cosLi;
}

float3 ComputeDirectionalLightNoSpecular(PBRData data) 
{
	float3 Li = data.lightDir;
	float3 Lradiance = data.lightColor;
	float3 Lh = normalize(Li + data.Lo);

	float cosLi = max(0.f, dot(data.N, Li));
	float3 F = fresnelSchlick(data.F0, max(0.f, dot(Lh, data.Lo)));
	
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), data.metalness); 
	float3 diffuse = kd * data.surfaceColor;
	
	return diffuse * Lradiance * cosLi;
}

float3 ComputePointLight(float3 worldPos, PointLight Light, PBRData pbrData) 
{
	return float3(0.f);
	// Light Incidence
	// float3 pointToLight = Light.Position - worldPos;

	// float3 Lh = normalize(Li + Lo);
	
	// float3 lightIntensity = GetPointLightIntensity(Light, Li);
       
	// float cosLi = max(0.f, dot(N, Li));
	// float cosLh = max(0.f, dot(N, Lh));

	// float3 F = fresnelSchlick(F0, max(0.f, dot(Lh, Lo)));
	// float D = GGX_ndf(cosLh, roughness);
	// float G = GGX_gaSchlick(cosLi, cosLo, roughness);
	
	// float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness); 
	
	// float3 diffuse = lightIntensity * kd * albedo;
	// float3 specular = (F*D*G) / max(Epsilon, 4.0 * cosLi * cosLo);
	
	// return (diffuse + specular) * Light.Color.rgb * cosLi; 
}