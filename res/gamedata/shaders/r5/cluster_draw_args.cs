#define SM_5_0

cbuffer ClusterArgsParams : register(b5)
{
    uint g_CountBase;
    uint3 g_ArgsPad;
};

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_Args : register(u0);
RWByteAddressBuffer g_TerrainArgs : register(u1);

[numthreads(1, 1, 1)]
void main()
{
    g_Args.Store4(0, uint4(384u, g_Count.Load(g_CountBase), 0u, 0u));
    g_TerrainArgs.Store4(0, uint4(384u, g_Count.Load(g_CountBase + 4u), 0u, 0u));
}
