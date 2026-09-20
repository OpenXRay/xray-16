#define SM_6_0

#include "sw_dispatch_common.h"

ByteAddressBuffer g_ArgsLod1 : register(t0);
ByteAddressBuffer g_ArgsLod2 : register(t1);
RWByteAddressBuffer g_SwArgs : register(u0);

[numthreads(1, 1, 1)]
void main()
{
    uint c1 = g_ArgsLod1.Load(4);
    uint c2 = g_ArgsLod2.Load(4);
    g_SwArgs.Store4(0, uint4(SwDispatchDims((c1 + 63u) / 64u), 0u));
    g_SwArgs.Store4(16, uint4(SwDispatchDims((c2 + 63u) / 64u), 0u));
}
