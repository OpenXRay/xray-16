// Level and object geometry share the Vulkan G-buffer vertex layout.
[[vk::push_constant]] cbuffer Camera { float4x4 mvp; };

struct Vertex
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct Fragment
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
};

Fragment main(Vertex vertex)
{
    Fragment fragment;
    fragment.position = mul(mvp, float4(vertex.position, 1.0));
    fragment.normal = vertex.normal;
    fragment.uv = vertex.uv;
    return fragment;
}
