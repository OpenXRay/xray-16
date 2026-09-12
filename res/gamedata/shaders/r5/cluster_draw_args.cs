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
    uint visibleCount = g_Count.Load(g_CountBase);
    uint terrainVisibleCount = g_Count.Load(g_CountBase + 4u);
    uint meshGroupCount = visibleCount * 2u;
    uint terrainMeshGroupCount = terrainVisibleCount * 2u;
    g_Args.Store4(0, uint4(384u, visibleCount, 0u, 0u));
    g_Args.Store4(16, uint4(min(meshGroupCount, 256u), (meshGroupCount + 255u) / 256u, 1u, 0u));
    g_TerrainArgs.Store4(0, uint4(384u, terrainVisibleCount, 0u, 0u));
    g_TerrainArgs.Store4(16, uint4(min(terrainMeshGroupCount, 256u), (terrainMeshGroupCount + 255u) / 256u, 1u, 0u));
}
