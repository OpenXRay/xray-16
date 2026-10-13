#ifndef DETAIL_SLOT_COMMON_H
#define DETAIL_SLOT_COMMON_H

struct SlotAABB
{
    float3 aabb_min;
    float padding0;
    float3 aabb_max;
    float padding1;
    uint instance_base;
    uint instance_count;
    int slot_x;
    int slot_z;
    uint instance_chunk;
    uint padding2;
    uint padding3;
    uint padding4;
};

#endif
