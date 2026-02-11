#include "cull_utils.h"

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
    float4x4 g_prev_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_visible_blade_capacity;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_lod_distance_close_sqr;
    float g_lod_distance_mid_sqr;
    float g_detail_density;
    uint g_visible_decal_capacity;
    uint g_grass_mode;
    uint g_visible_billboard_capacity;
    uint g_cull_pad2;
};

StructuredBuffer<SlotAABB> g_slot_aabbs : register(t0);

RWStructuredBuffer<uint> g_slot_visibility : register(u0);

RWStructuredBuffer<uint> g_visible_slot_ids : register(u1);
RWByteAddressBuffer g_visible_slot_counter : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint slot_idx = dispatch_thread_id.x;

    if (slot_idx >= g_total_slot_count)
        return;

    SlotAABB slot = g_slot_aabbs[slot_idx];

    if (slot.instance_count == 0)
    {
        g_slot_visibility[slot_idx] = 0;
        return;
    }

    if (!DistanceTestAABB(slot.aabb_min, slot.aabb_max, g_camera_pos, g_fade_distance_sqr))
    {
        g_slot_visibility[slot_idx] = 0;
        return;
    }

    if (!FrustumTestAABB(slot.aabb_min, slot.aabb_max, g_frustum_planes))
    {
        g_slot_visibility[slot_idx] = 0;
        return;
    }

    g_slot_visibility[slot_idx] = 1;

    uint insert_index;
    g_visible_slot_counter.InterlockedAdd(0, 1, insert_index);
    g_visible_slot_ids[insert_index] = slot_idx;
}
