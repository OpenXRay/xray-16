#define SM_5_0
#include "common.h"
#include "vsm_common.h"

#include "vsm_resid_params.h"
#include "geometry_cut_common.h"

StructuredBuffer<uint> g_Needed;
StructuredBuffer<float4> g_SlotPivot;
StructuredBuffer<float4> g_SlotSun;
RWStructuredBuffer<uint> g_PageTable;
RWStructuredBuffer<uint4> g_PageList;
StructuredBuffer<uint2> g_PhysTile;
RWStructuredBuffer<uint> g_SlotDirty;
RWStructuredBuffer<uint4> g_CandList;
RWByteAddressBuffer g_Counters;
StructuredBuffer<uint> g_SlotFrame;
RWStructuredBuffer<uint2> g_GeometryDirty;

void updateGeometryDirty(uint slot, uint level)
{
    uint2 state = g_GeometryDirty[slot];
    uint2 header = g_GeometryCuts.Load2(0u);
    if (state.x >= header.y)
        return;
    if (state.y == 0u && any(g_SlotSun[slot].xyz != 0.0))
    {
        float4 pivot = g_SlotPivot[slot];
        float4 sun = g_SlotSun[slot];
        float3 up = abs(sun.y) > 0.99 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
        up = normalize(up - sun.xyz * dot(up, sun.xyz));
        float3 right = cross(up, sun.xyz);
        float2 origin = float2(pivot.w, sun.w);
        float width = g_LevelOrigin[level].z;
        for (uint i = geometryFirstCutAfter(state.x, header.x); i < header.x; ++i)
        {
            float3 center, extent;
            geometryCutBounds(i, center, extent);
            float3 delta = center - pivot.xyz;
            float3 local = float3(dot(right, delta), dot(up, delta), dot(sun.xyz, delta));
            float3 radius = float3(dot(abs(right), extent), dot(abs(up), extent), dot(abs(sun.xyz), extent));
            if (all(local.xy + radius.xy >= origin) && all(local.xy - radius.xy <= origin + width)
                && local.z + radius.z >= g_Pivot.w && local.z - radius.z <= g_Sun.w)
            {
                state.y = 1u;
                break;
            }
        }
    }
    state.x = header.y;
    g_GeometryDirty[slot] = state;
}

int2 pageBaseOf(int L)
{
    int4 v = g_PageBase[L >> 1];
    return ((L & 1) == 0) ? v.xy : v.zw;
}

uint intervalOf(int L)
{
    uint4 v = g_Interval[L >> 2];
    int c = L & 3;
    return (c == 0) ? v.x : ((c == 1) ? v.y : ((c == 2) ? v.z : v.w));
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint vp = dtID.x;
    bool inRange = vp < uint(VSM_PAGE_COUNT);
    bool needed = inRange && g_Needed[vp] != 0u;
    int level = int(min(vp, uint(VSM_PAGE_COUNT - 1))) / VSM_PAGES_PER_LVL;
    uint nWave = WaveActiveCountBits(needed);
    if (WaveIsFirstLane() && nWave != 0u)
    {
        uint d;
        g_Counters.InterlockedAdd(32u + 4u * uint(level), nWave, d);
    }
    if (!inRange)
        return;

    g_CandList[uint(VSM_MAX_PHYS_S) + vp] = uint4(0u, 0u, 0u, 0u);
    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    int2 absPage = pageBaseOf(level) + wpage;
    int slot = vsmToroidalSlot(level, absPage);
    updateGeometryDirty(uint(slot), uint(level));
    g_SlotDirty[slot] = 0u;
    if (!needed)
    {
        g_PageTable[vp] = VSM_UNMAPPED;
        return;
    }
    g_PageList[slot] = uint4(uint(level), uint(wpage.x), uint(wpage.y), 0u);

    uint2 tile = uint2(uint(absPage.x), uint(absPage.y));
    bool force = g_ForceDirty != 0u || g_GeometryDirty[slot].y != 0u;
    bool wrong = any(g_PhysTile[slot] != tile) || !any(g_SlotSun[slot].xyz != 0.0);
    uint kind = 0u;

    if (wrong)
    {
        uint n;
        g_Counters.InterlockedAdd(16u, 1u, n);
        kind = 1u;
    }
    else if (force)
    {
        kind = 4u;
    }
    else
    {
        uint age = g_Frame - g_SlotFrame[slot];
        if (age >= intervalOf(level))
        {
            uint n;
            g_Counters.InterlockedAdd(20u, 1u, n);
            kind = 2u;
        }
    }

    g_PageTable[vp] = (kind == 1u || g_GeometryDirty[slot].y == 2u) ? VSM_UNMAPPED : uint(slot);
    if (kind == 0u)
        return;

    g_CandList[uint(VSM_MAX_PHYS_S) + vp] = uint4(uint(slot), vp, kind, 0u);
}
