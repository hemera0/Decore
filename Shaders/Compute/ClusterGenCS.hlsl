#pragma pack_matrix(row_major)

const int LightsPerCluster = 100;

struct Cluster {
    float4 MinPos;
    float4 MaxPos;
    uint LightCount;
    uint LightIndices[LightsPerCluster];
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<Cluster> Clusters; 

[[vk::binding(1, 0)]]
cbuffer ClusterGenUBO {
    float2 NearFar;
    float4x4 InverseProjection;
    float3 GridSize;
    float2 ScreenDimensions;
};

float3 ScreenToView(float2 screenCoord) {
    float4 ndc = float4(screenCoord / ScreenDimensions * 2.f - 1.f, -1.f, 1.f);
    float4 viewCoord = mul(ndc, InverseProjection);

    viewCoord /= viewCoord.w; 
    return viewCoord.xyz;
}

float3 LineIntersectionWithZPlane(float3 startPos, float3 endPos, float zDist) {
    float3 dir = endPos - startPos;
    float3 normal = float3(0.f, 0.f, -1.f);

    float t = (zDist - dot(normal, startPos)) / dot(normal, dir);
    return startPos + t * dir;
}

[numthreads(1, 1, 1)]
void main( uint3 groupId : SV_GroupId) {
    float3 eyePos = float3(0.f);
    uint tileIndex = groupId.x + (groupId.y * GridSize.x) + (groupId.z * GridSize.x * GridSize.y);

    float2 tileSize = ScreenDimensions / GridSize.xy;

    float2 minTile_screenSpace = groupId.xy * tileSize;
    float2 maxTile_screenSpace = (groupId.xy + 1) * tileSize;

    float3 minTile_viewSpace = ScreenToView(minTile_screenSpace);
    float3 maxTile_viewSpace = ScreenToView(maxTile_screenSpace);

    float zNear = NearFar.x;
    float zFar = NearFar.y;

    float planeNear = zNear * pow(zFar / zNear, groupId.z / float(GridSize.z));
    float planeFar = zNear * pow(zFar / zNear, (groupId.z + 1) / float(GridSize.z));

    float3 minPointNear = LineIntersectionWithZPlane(eyePos, minTile_viewSpace, planeNear);
    float3 minPointFar = LineIntersectionWithZPlane(eyePos, minTile_viewSpace, planeFar);
    float3 maxPointNear = LineIntersectionWithZPlane(eyePos, maxTile_viewSpace, planeNear);
    float3 maxPointFar = LineIntersectionWithZPlane(eyePos, maxTile_viewSpace, planeFar);

    float3 minPointAABB = min(minPointNear, minPointFar);
    float3 maxPointAABB = max(maxPointNear, maxPointFar);

    Clusters[tileIndex].MinPos = float4(minPointAABB, 0.0);
    Clusters[tileIndex].MaxPos = float4(maxPointAABB, 0.0);
}