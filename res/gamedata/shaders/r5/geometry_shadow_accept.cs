#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "local_shadow_common.h"

StructuredBuffer<uint> g_DirtyList;
ByteAddressBuffer g_DrawArgs;
RWStructuredBuffer<uint2> g_GeometryDirty;
StructuredBuffer<uint4> g_PageList;
RWStructuredBuffer<uint> g_PageTable;
RWStructuredBuffer<LocalShadowView> g_TileState;

[numthreads(64, 1, 1)]
void publishVSM(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_DrawArgs.Load(4u))
        return;
    uint slot = g_DirtyList[id.x];
    uint2 state = g_GeometryDirty[slot];
    g_GeometryDirty[slot] = uint2(state.x, 0u);
    uint4 page = g_PageList[slot];
    g_PageTable[vsmPageIndex(int(page.x), int2(page.yz))] = slot;
}

[numthreads(64, 1, 1)]
void publishLocal(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_DrawArgs.Load(4u))
        return;
    uint slot = g_DirtyList[id.x];
    uint2 state = g_GeometryDirty[slot];
    g_GeometryDirty[slot] = uint2(state.x, 0u);
    LocalShadowView tile = g_TileState[slot];
    tile.zparams.w = 1.0;
    g_TileState[slot] = tile;
}
