[[vk::push_constant]] cbuffer Camera {
    float4x4 mvp;
    float alpha_ref;
    float blend;
};
struct Vertex {
    [[vk::location(0)]] float3 first : POSITION;
    [[vk::location(1)]] float3 second : NORMAL;
    [[vk::location(2)]] float2 uv0 : TEXCOORD0;
    [[vk::location(5)]] float2 uv1 : TEXCOORD1;
    [[vk::location(6)]] float4 hemi : COLOR0;
    [[vk::location(7)]] float3 n0 : TANGENT;
    [[vk::location(8)]] float3 n1 : BINORMAL;
};
struct Fragment {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : TEXCOORD0;
    [[vk::location(1)]] float2 uv0 : TEXCOORD1;
    [[vk::location(2)]] float2 uv1 : TEXCOORD2;
    [[vk::location(3)]] float hemi : TEXCOORD3;
};
Fragment main(Vertex v) {
    Fragment f;
    f.position=mul(mvp,float4(lerp(v.second,v.first,blend),1));
    f.position.y=-f.position.y;
    f.normal=lerp(v.n1,v.n0,blend);
    f.uv0=v.uv0; f.uv1=v.uv1;
    f.hemi=lerp(v.hemi.y,v.hemi.x,blend);
    return f;
}
