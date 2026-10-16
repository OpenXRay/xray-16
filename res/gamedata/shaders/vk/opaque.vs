// Level and object geometry share the Vulkan G-buffer vertex layout.
[[vk::push_constant]] cbuffer Camera {
    float4x4 mvp;
    float alpha_threshold;
    float4 normal_rows[3];
    float4 hemi_negative;
};

struct Vertex
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
#if defined(LEVEL_GEOMETRY)
    [[vk::location(5)]] float2 lightmap_uv : TEXCOORD1;
    [[vk::location(6)]] float4 baked : COLOR0;
#endif
};

struct Fragment
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(4)]] float3 world_position : TEXCOORD4;
    [[vk::location(5)]] float hemi : TEXCOORD5;
#if defined(LEVEL_GEOMETRY)
    [[vk::location(2)]] float2 lightmap_uv : TEXCOORD2;
    [[vk::location(3)]] float4 baked : COLOR0;
#endif
};

Fragment main(Vertex vertex)
{
    Fragment fragment;
    // VK_TREE_STATIC is the D_TREE Object LOD (tree_s) route. Unlike an
    // animated tree, the decoded geometry is already in world space; leave
    // the position, normal and UV untouched for every sliding window.
#if defined(VK_TREE_STATIC) && !defined(LEVEL_GEOMETRY) && !defined(OBJECT_GEOMETRY)
#error VK_TREE_STATIC requires decoded level or object geometry
#endif
    fragment.position = mul(mvp, float4(vertex.position, 1.0));
    // X-Ray's projection is Y-up; a positive-height Vulkan viewport is Y-down.
    fragment.position.y = -fragment.position.y;
#if defined(LEVEL_GEOMETRY)
    fragment.normal = vertex.normal;
#else
    fragment.normal = normalize(float3(dot(normal_rows[0].xyz, vertex.normal),
        dot(normal_rows[1].xyz, vertex.normal), dot(normal_rows[2].xyz, vertex.normal)));
#endif
    fragment.hemi = saturate(0.5 + 0.5 * normalize(fragment.normal).y);
#if !defined(LEVEL_GEOMETRY)
    if (normal_rows[0].w >= 0.0)
    {
        float3 world_normal = normalize(fragment.normal);
        float3 positive = float3(normal_rows[0].w, normal_rows[1].w, normal_rows[2].w);
        float3 negative = hemi_negative.xyz;
        fragment.hemi = saturate(dot(lerp(positive, negative,
            float3(world_normal.x < 0.0, world_normal.y < 0.0, world_normal.z < 0.0)),
            abs(world_normal)));
    }
#endif
    fragment.uv = vertex.uv;
    fragment.world_position = vertex.position;
#if !defined(LEVEL_GEOMETRY)
    fragment.world_position = float3(dot(normal_rows[0].xyz, vertex.position),
        dot(normal_rows[1].xyz, vertex.position), dot(normal_rows[2].xyz, vertex.position));
#endif
#if defined(LEVEL_GEOMETRY)
    fragment.lightmap_uv = vertex.lightmap_uv;
    fragment.baked = vertex.baked;
#endif
    return fragment;
}
