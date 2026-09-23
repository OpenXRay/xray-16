#define RT_WORLD_CACHE 1
#include "rt_world_cache.h"

[numthreads(256, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint cell = dispatchID.x;
    if (cell > g_WorldCacheCapacityMask)
        return;
    uint life = u_WorldCacheLife[cell];
    if (life == 0u)
        return;
    life -= 1u;
    u_WorldCacheLife[cell] = life;
    if (life == 0u)
    {
        u_WorldCacheChecksum[cell] = RT_WORLD_CACHE_EMPTY;
        u_WorldCacheRadiance[cell] = 0.0;
        u_WorldCachePosition[cell] = 0.0;
        u_WorldCacheNormal[cell] = 0.0;
        return;
    }
    InterlockedAdd(u_WorldCacheStats[0], 1u);
}
