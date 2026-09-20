#define DETAIL_VISIBILITY_BUILD
#include "detail_visibility_common.h"
#include "sw_dispatch_common.h"

cbuffer DetailWorkParams : register(b5)
{
    uint g_EntryCapacity;
    uint3 g_BladeCapacity;
    uint g_MeshCapacity;
    uint g_DecalCapacity;
    uint2 g_WorkPadding;
};

ByteAddressBuffer g_VisibleSlotCount : register(t0);
RWByteAddressBuffer g_ArgsLod0 : register(u0);
RWByteAddressBuffer g_ArgsLod1 : register(u1);
RWByteAddressBuffer g_ArgsLod2 : register(u2);
RWByteAddressBuffer g_ArgsMesh : register(u3);
RWByteAddressBuffer g_ArgsDecal : register(u4);
RWByteAddressBuffer g_SwArgs : register(u5);
RWStructuredBuffer<DetailVisibilityRange> g_DetailPackets : register(u6);
RWByteAddressBuffer g_SlotDispatch : register(u7);
RWByteAddressBuffer g_WorkStatus : register(u8);

[numthreads(1, 1, 1)]
void main_slots()
{
    uint count = g_VisibleSlotCount.Load(0);
    uint rows = count / 65535u + uint(count % 65535u != 0u);
    if (rows > 65535u)
    {
        g_WorkStatus.InterlockedOr(28, 4u);
        g_SlotDispatch.Store3(0, uint3(0u, 1u, 1u));
        return;
    }
    g_SlotDispatch.Store3(0, uint3(min(count, 65535u), max(rows, 1u), 1u));
}

[numthreads(1, 1, 1)]
void main()
{
    uint counts[5] = { g_ArgsLod0.Load(4), g_ArgsLod1.Load(4), g_ArgsLod2.Load(4), g_ArgsMesh.Load(4), g_ArgsDecal.Load(4) };
    uint capacities[5] = { g_BladeCapacity.x, g_BladeCapacity.y, g_BladeCapacity.z, g_MeshCapacity, g_DecalCapacity };
    uint blades[5] = { 7u, 18u, 42u, 1u, 1u };
    uint triangles[5] = { 17u, 7u, 3u, 128u, 128u };
    uint packets = 0;
    uint overflow = g_WorkStatus.Load(28);
    [unroll]
    for (uint i = 0u; i < 5u; ++i)
    {
        DetailVisibilityRange range;
        range.packetBase = packets;
        range.instanceCount = counts[i];
        range.instancesPerPacket = blades[i];
        range.trianglesPerInstance = triangles[i];
        g_DetailPackets[i] = range;
        uint count = DetailPacketCount(counts[i], blades[i]);
        if (count > g_EntryCapacity - min(packets, g_EntryCapacity))
            overflow |= 2u;
        packets += min(count, 0xffffffffu - packets);
        if (counts[i] > capacities[i])
            overflow |= 1u;
    }
    g_WorkStatus.Store4(0, uint4(g_VisibleSlotCount.Load(0), counts[0], counts[1], counts[2]));
    g_WorkStatus.Store4(16, uint4(counts[4], counts[3], packets, overflow));
    if (overflow != 0u)
    {
        g_ArgsLod0.Store(4, 0u);
        g_ArgsLod1.Store(4, 0u);
        g_ArgsLod2.Store(4, 0u);
        g_ArgsMesh.Store(4, 0u);
        g_ArgsDecal.Store(4, 0u);
        [unroll]
        for (uint i = 0u; i < 5u; ++i)
            g_DetailPackets[i].instanceCount = 0u;
        counts[1] = 0u;
        counts[2] = 0u;
    }
    uint groups1 = counts[1] / 64u + uint(counts[1] % 64u != 0u);
    uint groups2 = counts[2] / 64u + uint(counts[2] % 64u != 0u);
    g_SwArgs.Store4(0, uint4(SwDispatchDims(groups1), 0u));
    g_SwArgs.Store4(16, uint4(SwDispatchDims(groups2), 0u));
}
