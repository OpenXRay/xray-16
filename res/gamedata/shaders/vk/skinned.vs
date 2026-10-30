// GPU skinning variant. Its input and pose-buffer layout is separate from the
// current CPU-skinned LevelVertex path until skeletal GPU pipelines are bound.
#ifndef SKIN_WEIGHTS
#error SKIN_WEIGHTS must be in the range 1..4
#endif
[[vk::push_constant]] cbuffer Camera {
    float4x4 mvp;
};
[[vk::binding(0, 1)]] StructuredBuffer<float4x4> pose : register(t0, space1);

struct Vertex
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
    [[vk::location(3)]] uint4 bones : BLENDINDICES0;
    [[vk::location(4)]] float4 weights : BLENDWEIGHT0;
    [[vk::location(7)]] float3 tangent : TANGENT;
    [[vk::location(8)]] float3 bitangent : BINORMAL;
};

struct Fragment
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(4)]] float3 world_position : TEXCOORD4;
    [[vk::location(5)]] float hemi : TEXCOORD5;
    [[vk::location(6)]] float3 tangent : TEXCOORD6;
    [[vk::location(7)]] float3 bitangent : TEXCOORD7;
};

Fragment main(Vertex vertex)
{
    uint pose_count, pose_stride;
    pose.GetDimensions(pose_count, pose_stride);
    float4x4 object = pose[pose_count - 1];
    float4 position = 0;
    float3 normal = 0;
    float3 tangent = 0, bitangent = 0;
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
        tangent += mul((float3x3)bone, vertex.tangent) * weight;
        bitangent += mul((float3x3)bone, vertex.bitangent) * weight;
    }
    Fragment fragment;
    fragment.position = mul(mvp, position);
    fragment.position.y = -fragment.position.y;
    fragment.normal = mul((float3x3)object, normal);
    fragment.tangent = mul((float3x3)object, tangent);
    fragment.bitangent = mul((float3x3)object, bitangent);
    fragment.hemi = saturate(0.5 + 0.5 * fragment.normal.y);
    if (object[3][0] >= 0.0)
    {
        float3 world_normal = normalize(fragment.normal);
        float3 positive = object[3].xyz;
        float3 negative = float3(object[0][3], object[1][3], object[2][3]);
        fragment.hemi = saturate(dot(lerp(positive, negative,
            float3(world_normal.x < 0.0, world_normal.y < 0.0, world_normal.z < 0.0)),
            abs(world_normal)));
    }
    fragment.uv = vertex.uv;
    fragment.world_position = mul((float3x3)object, position.xyz);
    return fragment;
}
