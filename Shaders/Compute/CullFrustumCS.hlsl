#pragma pack_matrix(row_major)

struct IndexedIndirectCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<IndexedIndirectCommand> IndirectDraws;

struct PrimitiveData {
	int4 MaterialIndex;
	float4x4 NodeMatrix;
    float4 NodePos;
};

[[vk::binding(0, 1)]]
StructuredBuffer<PrimitiveData> DrawPrimitives;

[[vk::binding(0, 2)]]
cbuffer CullUBO {
    float4 Frustum;
    float4x4 CameraView;
    float2 NearFar;
};

// vkguide / zeux
[numthreads(16,1,1)]
bool IsVisible(float3 boundsCenter, float radius) {
	float3 center = mul(float4(boundsCenter, 1.f), CameraView).xyz;
	
    bool visible = true;
    visible = visible && center.z * Frustum[1] - abs(center.x) * Frustum[0] > -radius;
	visible = visible && center.z * Frustum[3] - abs(center.y) * Frustum[2] > -radius;

    // the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > NearFar.x && center.z - radius < NearFar.y;

    return visible;
}

[numthreads(16, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID) {
    uint idx = dispatchId.x;

    float4 bounds = DrawPrimitives[idx].NodePos;
    
    float3 pos = bounds.xyz; 
    float radius = bounds.w;
    if( IsVisible(pos, radius) ) {
        IndirectDraws[idx].instanceCount = 1;
    } else {
        IndirectDraws[idx].instanceCount = 0;    
    }
}