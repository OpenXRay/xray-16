#define RT_WORLD_CACHE 1
#define RT_WORLD_CACHE_MAINTENANCE 1
#define RT_WORLD_CACHE_SELECT 1
#include "rt_world_cache.h"

bool RTWorldCacheSelectCell(uint cell)
{
    if (cell > g_WorldCacheCapacityMask || !RTWorldCacheEnabled())
        return false;
    uint life = u_WorldCacheLife[cell];
    if (life == 0u || life >= g_WorldCacheLifetime)
        return false;
    float samples = u_WorldCacheRadiance[cell].a;
    if (!isfinite(samples))
        samples = 0.0;
    uint liveCells = max(u_WorldCacheStats[RT_WORLD_CACHE_STAT_LIVE], 1u);
    float priority = (life + 1u >= g_WorldCacheLifetime ? 1.0 : 0.5) * (samples < RT_WORLD_CACHE_MIN_HISTORY ? 2.0 : 1.0);
    float acceptance = float(g_WorldCacheUpdateTarget) / float(liveCells) * priority;
    if (acceptance >= 1.0)
        return true;
    uint rng = pcg_hash(cell * 7919u + g_WorldCacheFrame * 48611u + 2u);
    return rand_float(rng) < acceptance;
}

[numthreads(256, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint cell = dispatchID.x;
    bool selected = RTWorldCacheSelectCell(cell);
    uint count = WaveActiveCountBits(selected);
    if (count == 0u)
        return;
    uint base = 0u;
    if (WaveIsFirstLane())
    {
        InterlockedAdd(u_WorldCacheStats[RT_WORLD_CACHE_STAT_UPDATES], count, base);
        uint groupSize = uint(RT_WORLD_CACHE_UPDATE_GROUP_SIZE);
        InterlockedMax(u_WorldCacheUpdateArgs[0], (base + count + groupSize - 1u) / groupSize);
    }
    base = WaveReadLaneFirst(base);
    if (selected)
        u_WorldCacheUpdateList[base + WavePrefixCountBits(selected)] = cell;
}
