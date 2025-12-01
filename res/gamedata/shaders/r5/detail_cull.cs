#define SM_5_0
#include "common.h"
#include "cull_utils.h"

struct InstanceData
{
    float3 pos;
    float scale;
    float hemi;
    uint vis_id;
    uint object_id;
    float padding;
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

cbuffer DetailCullParams : register(b1)
{
    float4x4 g_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_total_instance_count;
    uint g_total_slot_count;
    uint g_object_count;
    uint g_pad0;
};

StructuredBuffer<SlotAABB> g_slot_aabbs : register(t0);
StructuredBuffer<InstanceData> g_all_instances : register(t1);

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

    for (uint i = 0; i < 5; ++i)
    {
        float3 plane_normal = g_frustum_planes[i].xyz;
        float plane_dist = g_frustum_planes[i].w;

        float3 negative_vertex = float3(
            plane_normal.x < 0.0 ? slot.aabb_max.x : slot.aabb_min.x,
            plane_normal.y < 0.0 ? slot.aabb_max.y : slot.aabb_min.y,
            plane_normal.z < 0.0 ? slot.aabb_max.z : slot.aabb_min.z
        );

        float dist = dot(plane_normal, negative_vertex) + plane_dist;

        if (dist > 2.0)
            return;
    }

    if (!DistanceTestAABB(slot.aabb_min, slot.aabb_max, g_camera_pos, g_fade_distance_sqr))
        return;

    uint insert_index = 0;
    g_visible_slot_counter.InterlockedAdd(0, 1, insert_index);

    if (insert_index < 8192)
    {
        g_visible_slot_ids[insert_index] = slot_idx;
    }

    for (uint i = 0; i < slot.instance_count; i++)
    {
        uint inst_idx = slot.instance_base + i;
        InstanceData inst = g_all_instances[inst_idx];

        float bounds_radius = inst.scale * 0.75f;

        if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
            continue;

        float3 delta = inst.pos - g_camera_pos;
        float dist_sqr = dot(delta, delta);
        if (dist_sqr > g_fade_distance_sqr)
            continue;

        AppendInstance(inst);
    }
}
