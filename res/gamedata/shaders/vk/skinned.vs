// GPU skinning variant. Its input and pose-buffer layout is separate from the
// current CPU-skinned LevelVertex path until skeletal GPU pipelines are bound.
#ifndef SKIN_WEIGHTS
#error SKIN_WEIGHTS must be in the range 1..4
#endif
[[vk::push_constant]] cbuffer Camera {
    float4x4 mvp;
    float alpha_threshold;
    float3 padding;
    float4 normal_rows[3];
};
[[vk::binding(0, 1)]] StructuredBuffer<float4x4> pose : register(t0, space1);

struct Vertex
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
    [[vk::location(3)]] uint4 bones : BLENDINDICES0;
    [[vk::location(4)]] float4 weights : BLENDWEIGHT0;
};

struct Fragment
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
};

Fragment main(Vertex vertex)
{
    float4 position = 0;
    float3 normal = 0;
    [unroll] for (int i = 0; i < SKIN_WEIGHTS; ++i)
    {
        float weight = vertex.weights[i];
        // The final weight is computed from the preceding weights by the OGF decoder.
        if (i == SKIN_WEIGHTS - 1)
        {
            weight = 1.0;
            [unroll] for (int previous = 0; previous < SKIN_WEIGHTS - 1; ++previous)
                weight -= vertex.weights[previous];
        }
        float4x4 bone = pose[vertex.bones[i]];
        position += mul(bone, float4(vertex.position, 1.0)) * weight;
        normal += mul((float3x3)bone, vertex.normal) * weight;
    }
    Fragment fragment;
    fragment.position = mul(mvp, position);
    fragment.position.y = -fragment.position.y;
    fragment.normal = normalize(float3(dot(normal_rows[0].xyz, normal),
        dot(normal_rows[1].xyz, normal), dot(normal_rows[2].xyz, normal)));
    fragment.uv = vertex.uv;
    return fragment;
}
