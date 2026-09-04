#define SM_5_0
#include "common.h"
#include "vsm_common.h"

ByteAddressBuffer g_Counters : register(t0);
RWByteAddressBuffer g_BinArgs : register(u0);

[numthreads(1, 1, 1)]
void main()
{
    uint dirty = min(g_Counters.Load(4), uint(VSM_MAX_PHYS_S));
    g_BinArgs.Store3(0, uint3(dirty, 1u, 1u));
}
