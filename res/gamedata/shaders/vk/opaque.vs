// Level and object geometry share the Vulkan G-buffer vertex layout.
[[vk::push_constant]] cbuffer Camera { float4x4 mvp; };

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
#if defined(LEVEL_GEOMETRY)
    [[vk::location(2)]] float2 lightmap_uv : TEXCOORD2;
    [[vk::location(3)]] float4 baked : COLOR0;
#endif
};

Fragment main(Vertex vertex)
{
    Fragment fragment;
    fragment.position = mul(mvp, float4(vertex.position, 1.0));
    fragment.normal = vertex.normal;
    fragment.uv = vertex.uv;
#if defined(LEVEL_GEOMETRY)
    fragment.lightmap_uv = vertex.lightmap_uv;
    fragment.baked = vertex.baked;
#endif
    return fragment;
}
