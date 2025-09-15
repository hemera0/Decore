struct PS_Input {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	
	float3 WorldPos : POSITION;	
	float3 ViewPos;
	
	nointerpolation int index : SV_InstanceID;
};

float3 CompressNormals(const float3 normal)
{
    return normal * 0.5 + 0.5;
}

float4 main( PS_Input input ) {
    return float4( CompressNormals( normalize(input.Normal) ), 1.f );
}