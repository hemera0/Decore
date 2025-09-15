#pragma pack_matrix( row_major )

[[vk::push_constant]]
cbuffer _ {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

float4 main(float3 position) : SV_POSITION
{
    float4 res =  float4(position, 1.f);

    res = mul( res, ViewMatrix ); 
    res = mul( res, ProjectionMatrix );
    return res;
}