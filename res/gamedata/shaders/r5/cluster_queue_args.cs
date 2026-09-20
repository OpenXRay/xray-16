#define SM_5_0

#include "sw_dispatch_common.h"

cbuffer ClusterQueueParams : register(b5)
{
    uint g_SrcCountOffset;
    uint g_ResetCountOffset;
    uint g_ArgsSlot;
    uint g_GroupSize;
};

RWByteAddressBuffer g_Count : register(u0);
RWByteAddressBuffer g_QueueArgs : register(u1);

[numthreads(1, 1, 1)]
void main()
{
    uint count = g_Count.Load(g_SrcCountOffset);
    uint groups = (count + g_GroupSize - 1u) / g_GroupSize;
    uint3 dims = SwDispatchDims(groups);
    g_QueueArgs.Store4(g_ArgsSlot * 16u, uint4(dims, count));
    if (g_ResetCountOffset != 0xFFFFFFFFu)
        g_Count.Store(g_ResetCountOffset, 0u);
}
