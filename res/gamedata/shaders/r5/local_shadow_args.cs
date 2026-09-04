#define SM_5_0

cbuffer LocalShadowArgsParams : register(b5)
{
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_RefreshStaticCount;
};

StructuredBuffer<uint> g_Stats : register(t14);
RWByteAddressBuffer g_ArgsOpaque : register(u0);
RWByteAddressBuffer g_ArgsTerrain : register(u1);
RWByteAddressBuffer g_ArgsAT : register(u2);
RWByteAddressBuffer g_ArgsClear : register(u3);

[numthreads(1, 1, 1)]
void main()
{
    g_ArgsOpaque.Store4(0, uint4(384u, min(g_Stats[4], g_CapOpaque), 0u, 0u));
    g_ArgsTerrain.Store4(0, uint4(384u, min(g_Stats[5], g_CapTerrain), 0u, 0u));
    g_ArgsAT.Store4(0, uint4(384u, min(g_Stats[6], g_CapAT), 0u, 0u));
    g_ArgsClear.Store4(0, uint4(6u, g_RefreshStaticCount, 0u, 0u));
}
