// Level and object geometry share the Vulkan G-buffer vertex layout.
[[vk::push_constant]] cbuffer Camera {
    float4x4 mvp;
    float4 material_padding[4]; // bytes 64..127 belong to fragment material data
    float4 base_matrix_u;
    float4 base_matrix_v;
    float4 detail_matrix_u;
    float4 detail_matrix_v;
    float4 environment_matrix_u;
    float4 environment_matrix_v;
    float4 environment_matrix_z;
    float4 object_rotation; // bytes 240..255 never alias material data

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

float3 rotate_object(float3 value)
{
    return value + 2.0 * cross(object_rotation.xyz,
        cross(object_rotation.xyz, value) + object_rotation.w * value);
}

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
    fragment.normal = normalize(rotate_object(vertex.normal));
#endif
    fragment.hemi = saturate(0.5 + 0.5 * normalize(fragment.normal).y);
#if !defined(LEVEL_GEOMETRY)
    if (base_matrix_u.z >= 0.0)
    {
        float3 world_normal = normalize(fragment.normal);
        float3 packed = float3(base_matrix_u.z, base_matrix_v.z, detail_matrix_u.z);
        float3 positive = floor(packed) / 1023.0;
        float3 negative = frac(packed) * (1024.0 / 1023.0);
        fragment.hemi = saturate(dot(lerp(positive, negative,
            float3(world_normal.x < 0.0, world_normal.y < 0.0, world_normal.z < 0.0)),
            abs(world_normal)));
    }
#endif
    fragment.uv = vertex.uv;
    fragment.world_position = vertex.position;
#if !defined(LEVEL_GEOMETRY)
    fragment.world_position = rotate_object(vertex.position);
#endif
#if defined(LEVEL_GEOMETRY)
    fragment.lightmap_uv = vertex.lightmap_uv;
    fragment.baked = vertex.baked;
#endif
    return fragment;
}
