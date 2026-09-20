#ifndef DETAIL_VISIBILITY_COMMON_H
#define DETAIL_VISIBILITY_COMMON_H

#include "visbuffer_common.h"

#define DETAIL_VISIBILITY_KIND_COUNT 5u

struct DetailVisibilityRange
{
    uint packetBase;
    uint instanceCount;
    uint instancesPerPacket;
    uint trianglesPerInstance;
};

uint DetailPacketCount(uint instances, uint instancesPerPacket)
{
    return instances / instancesPerPacket + uint(instances % instancesPerPacket != 0u);
}

#ifndef DETAIL_VISIBILITY_BUILD
StructuredBuffer<DetailVisibilityRange> g_DetailPackets : register(t45);

uint DetailVisibilityEntry(uint entryBase, uint kind, uint instance, out uint primitiveBase)
{
    DetailVisibilityRange range = g_DetailPackets[kind];
    primitiveBase = (instance % range.instancesPerPacket) * range.trianglesPerInstance;
    return entryBase + range.packetBase + instance / range.instancesPerPacket;
}

bool DecodeDetailVisibility(uint relativeEntry, uint packedTriangle, out uint kind, out uint instance, out uint triangle)
{
    kind = 0u;
    instance = 0u;
    triangle = 0u;
    [unroll]
    for (uint i = 0u; i < DETAIL_VISIBILITY_KIND_COUNT; ++i)
    {
        DetailVisibilityRange range = g_DetailPackets[i];
        if (range.instanceCount == 0u || relativeEntry < range.packetBase)
            continue;
        uint packet = relativeEntry - range.packetBase;
        if (packet >= DetailPacketCount(range.instanceCount, range.instancesPerPacket))
            continue;
        if (packedTriangle >= range.instancesPerPacket * range.trianglesPerInstance)
            return false;
        kind = i;
        instance = packet * range.instancesPerPacket + packedTriangle / range.trianglesPerInstance;
        triangle = packedTriangle % range.trianglesPerInstance;
        return instance < range.instanceCount;
    }
    return false;
}
#endif

#endif
