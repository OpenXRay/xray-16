#define SM_5_0

cbuffer VsmArgsParams
{
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_ArgsPad;
};

StructuredBuffer<uint> g_Stats;
RWByteAddressBuffer g_ArgsOpaque;
RWByteAddressBuffer g_ArgsTerrain;
RWByteAddressBuffer g_ArgsAT;

[numthreads(1, 1, 1)]
void main()
{
    g_ArgsOpaque.Store4(0, uint4(384u, min(g_Stats[4], g_CapOpaque), 0u, 0u));
    g_ArgsTerrain.Store4(0, uint4(384u, min(g_Stats[5], g_CapTerrain), 0u, 0u));
    g_ArgsAT.Store4(0, uint4(384u, min(g_Stats[6], g_CapAT), 0u, 0u));
}
