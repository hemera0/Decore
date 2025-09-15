#pragma pack_matrix(row_major)

struct PointLight {
    float4 Position;
    float4 Color;
    float Intensity;
    float Radius;
};

[[vk::binding(0, 1)]]
StructuredBuffer<PointLight> PointLights;

const int LOCAL_SIZE = 128;

[numthreads(LOCAL_SIZE, 1, 1)]
void main() {

}