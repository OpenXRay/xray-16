#define SM_5_0
#include "common.h"
#include "vsm_common.h"

RWByteAddressBuffer g_Counters : register(u0);
RWByteAddressBuffer g_BinArgs : register(u1);

[numthreads(1, 1, 1)]
void main()
{
    uint cw = min(g_Counters.Load(8), uint(VSM_MAX_PHYS_S));
    uint cr = min(g_Counters.Load(12), uint(VSM_MAX_PHYS_S));
    g_Counters.Store(8, cw);
    g_Counters.Store(12, cr);
    g_BinArgs.Store3(0, uint3(cw + cr, 1u, 1u));
}
