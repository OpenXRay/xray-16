#define SM_5_0

#include "sw_dispatch_common.h"

cbuffer ClusterArgsParams : register(b5)
{
    uint g_CountBase;
    uint g_SwCountOffset;
    uint2 g_ArgsPad;
};

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_Args : register(u0);
RWByteAddressBuffer g_TerrainArgs : register(u1);
RWByteAddressBuffer g_SwArgs : register(u2);
RWByteAddressBuffer g_RetestArgs : register(u3);

[numthreads(1, 1, 1)]
void main()
{
    uint visibleCount = g_Count.Load(g_CountBase);
    uint terrainVisibleCount = g_Count.Load(g_CountBase + 4u);
    uint meshGroupCount = visibleCount;
    uint terrainMeshGroupCount = terrainVisibleCount;
    g_Args.Store4(0, uint4(384u, visibleCount, 0u, 0u));
    g_Args.Store4(16, uint4(min(meshGroupCount, 256u), (meshGroupCount + 255u) / 256u, 1u, 0u));
    g_TerrainArgs.Store4(0, uint4(384u, terrainVisibleCount, 0u, 0u));
    g_TerrainArgs.Store4(16, uint4(min(terrainMeshGroupCount, 256u), (terrainMeshGroupCount + 255u) / 256u, 1u, 0u));
    uint swCount = g_Count.Load(g_SwCountOffset);
    uint3 swDims = SwDispatchDims(swCount);
    g_SwArgs.Store4(0, uint4(swDims, 0u));
    g_SwArgs.Store4(16, uint4(swCount, 0u, 0u, 0u));

    uint deferredInstances = g_Count.Load(60u);
    uint candidates = g_Count.Load(16u);
    uint3 instanceDims = SwDispatchDims((deferredInstances + 63u) / 64u);
    uint3 candidateDims = SwDispatchDims((candidates + 63u) / 64u);
    g_RetestArgs.Store4(0, uint4(instanceDims, deferredInstances));
    g_RetestArgs.Store4(16, uint4(candidateDims, candidates));
}
