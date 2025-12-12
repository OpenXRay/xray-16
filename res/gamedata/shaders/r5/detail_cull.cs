#include "cull_utils.h"

struct InstanceData
{
    float3 pos;
    float scale;
    float rotation;
    float hemi;
    uint vis_id;
    uint object_id;
};

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
    float4 padding2;
};

cbuffer DetailCullParams : register(b5)
{
    float4x4 g_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_total_instance_count;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    uint3 g_pad;
};

StructuredBuffer<SlotAABB> g_slot_aabbs : register(t0);
StructuredBuffer<InstanceData> g_all_instances : register(t1);

Texture2D<float> g_hiz_pyramid : register(t2);
SamplerState g_point_sampler : register(s0);

RWStructuredBuffer<InstanceData> g_visible_unified : register(u0);

RWByteAddressBuffer g_visible_count : register(u1);

RWStructuredBuffer<uint> g_visible_slot_ids : register(u33);
RWByteAddressBuffer g_visible_slot_counter : register(u34);

RWByteAddressBuffer g_indirect_args_unified : register(u2);


void AppendInstance(InstanceData inst)
{
    uint output_idx = 0;
    g_visible_count.InterlockedAdd(0, 1, output_idx);

    g_visible_unified[output_idx] = inst;

    uint dummy;
    g_indirect_args_unified.InterlockedAdd(4, 1, dummy);
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint slot_idx = dispatch_thread_id.x;

    if (slot_idx >= g_total_slot_count)
        return;

    SlotAABB slot = g_slot_aabbs[slot_idx];

    if (slot.instance_count == 0)
        return;

    if (!DistanceTestAABB(slot.aabb_min, slot.aabb_max, g_camera_pos, g_fade_distance_sqr))
        return;

    if (!FrustumTestAABB(slot.aabb_min, slot.aabb_max, g_frustum_planes))
        return;

    uint insert_index = 0;
    g_visible_slot_counter.InterlockedAdd(0, 1, insert_index);
    if (insert_index < 8192)
        g_visible_slot_ids[insert_index] = slot_idx;

    for (uint i = 0; i < slot.instance_count; i++)
    {
        uint inst_idx = slot.instance_base + i;
        InstanceData inst = g_all_instances[inst_idx];

        float bounds_radius = inst.scale * 0.5;

        if (!DistanceTestSphere(inst.pos, bounds_radius, g_camera_pos, g_fade_distance_sqr))
            continue;

        if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
            continue;

        if (!HiZTestSphere(inst.pos, bounds_radius, g_camera_pos, g_view_proj,
                            g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
            continue;

        AppendInstance(inst);
    }
}
