// detail_cull.cs - GPU-driven frustum culling for detail objects
// Phase 4A.3: Hierarchical culling - test slot AABBs first, then instances within visible slots
//
#define SM_5_0
#include "common.h"

struct InstanceData
{
    float3 pos;      // Position (12 bytes)
    float scale;     // Scale factor (4 bytes)
    float hemi;      // Hemisphere lighting (4 bytes)
    uint vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2) (4 bytes)
    uint object_id;  // Which grass object type (0-63) (4 bytes)
    float padding;   // Padding to align to 16 bytes (4 bytes) = 32 bytes total
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

cbuffer DetailCullParams : register(b0)
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

// ===========================
// Output Buffers (Phase 2.1: Per-Object)
// ===========================

// For now: hardcode first 16 object types (can expand to 64 later)
RWStructuredBuffer<InstanceData> g_visible_obj0 : register(u0);
RWStructuredBuffer<InstanceData> g_visible_obj1 : register(u1);
RWStructuredBuffer<InstanceData> g_visible_obj2 : register(u2);
RWStructuredBuffer<InstanceData> g_visible_obj3 : register(u3);
RWStructuredBuffer<InstanceData> g_visible_obj4 : register(u4);
RWStructuredBuffer<InstanceData> g_visible_obj5 : register(u5);
RWStructuredBuffer<InstanceData> g_visible_obj6 : register(u6);
RWStructuredBuffer<InstanceData> g_visible_obj7 : register(u7);

RWStructuredBuffer<InstanceData> g_visible_obj8 : register(u8);
RWStructuredBuffer<InstanceData> g_visible_obj9 : register(u9);
RWStructuredBuffer<InstanceData> g_visible_obj10 : register(u10);
RWStructuredBuffer<InstanceData> g_visible_obj11 : register(u11);
RWStructuredBuffer<InstanceData> g_visible_obj12 : register(u12);
RWStructuredBuffer<InstanceData> g_visible_obj13 : register(u13);
RWStructuredBuffer<InstanceData> g_visible_obj14 : register(u14);
RWStructuredBuffer<InstanceData> g_visible_obj15 : register(u15);

// Atomic counter buffer (one u32 per object type, up to 64 objects)
RWByteAddressBuffer g_visible_counts : register(u16);

// Phase 6B: Output buffer for visible slot IDs (for page table system)
RWStructuredBuffer<uint> g_visible_slot_ids : register(u33);  // Append visible slots here
RWByteAddressBuffer g_visible_slot_counter : register(u34);  // Atomic counter (ByteAddressBuffer for InterlockedAdd)

// Phase 2.2.1: Indirect draw args buffers (one per object)
// D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS:
// struct { u32 IndexCount; u32 InstanceCount; u32 StartIndex; s32 BaseVertex; u32 StartInstance; }
// We only write to InstanceCount field (offset 4 bytes)
RWByteAddressBuffer g_indirect_args0 : register(u17);
RWByteAddressBuffer g_indirect_args1 : register(u18);
RWByteAddressBuffer g_indirect_args2 : register(u19);
RWByteAddressBuffer g_indirect_args3 : register(u20);
RWByteAddressBuffer g_indirect_args4 : register(u21);
RWByteAddressBuffer g_indirect_args5 : register(u22);
RWByteAddressBuffer g_indirect_args6 : register(u23);
RWByteAddressBuffer g_indirect_args7 : register(u24);
RWByteAddressBuffer g_indirect_args8 : register(u25);
RWByteAddressBuffer g_indirect_args9 : register(u26);
RWByteAddressBuffer g_indirect_args10 : register(u27);
RWByteAddressBuffer g_indirect_args11 : register(u28);
RWByteAddressBuffer g_indirect_args12 : register(u29);
RWByteAddressBuffer g_indirect_args13 : register(u30);
RWByteAddressBuffer g_indirect_args14 : register(u31);
RWByteAddressBuffer g_indirect_args15 : register(u32);


bool AABBDistanceTest(float3 aabb_min, float3 aabb_max, float3 camera_pos, float max_dist_sqr)
{
    float3 closest = clamp(camera_pos, aabb_min, aabb_max);
    float dist_sqr = dot(closest - camera_pos, closest - camera_pos);
    return dist_sqr < max_dist_sqr;
}

bool SphereFrustumTest(float3 center, float radius, float4 planes[6])
{
    [unroll]
    for (uint i = 0; i < 5; ++i)
    {
        float dist = dot(planes[i].xyz, center) + planes[i].w;
        if (dist > radius)
            return false;
    }
    return true;
}

void AppendInstance(uint object_id, InstanceData inst, uint output_idx)
{
    uint dummy;
    if (object_id == 0)
    {
        g_visible_obj0[output_idx] = inst;
        g_indirect_args0.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 1)
    {
        g_visible_obj1[output_idx] = inst;
        g_indirect_args1.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 2)
    {
        g_visible_obj2[output_idx] = inst;
        g_indirect_args2.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 3)
    {
        g_visible_obj3[output_idx] = inst;
        g_indirect_args3.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 4)
    {
        g_visible_obj4[output_idx] = inst;
        g_indirect_args4.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 5)
    {
        g_visible_obj5[output_idx] = inst;
        g_indirect_args5.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 6)
    {
        g_visible_obj6[output_idx] = inst;
        g_indirect_args6.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 7)
    {
        g_visible_obj7[output_idx] = inst;
        g_indirect_args7.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 8)
    {
        g_visible_obj8[output_idx] = inst;
        g_indirect_args8.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 9)
    {
        g_visible_obj9[output_idx] = inst;
        g_indirect_args9.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 10)
    {
        g_visible_obj10[output_idx] = inst;
        g_indirect_args10.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 11)
    {
        g_visible_obj11[output_idx] = inst;
        g_indirect_args11.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 12)
    {
        g_visible_obj12[output_idx] = inst;
        g_indirect_args12.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 13)
    {
        g_visible_obj13[output_idx] = inst;
        g_indirect_args13.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 14)
    {
        g_visible_obj14[output_idx] = inst;
        g_indirect_args14.InterlockedAdd(4, 1, dummy);
    }
    else if (object_id == 15)
    {
        g_visible_obj15[output_idx] = inst;
        g_indirect_args15.InterlockedAdd(4, 1, dummy);
    }
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

    [unroll]
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

    if (!AABBDistanceTest(slot.aabb_min, slot.aabb_max, g_camera_pos, g_fade_distance_sqr))
        return;

    // Phase 6B: Record that this slot is visible (for page table system)
    uint insert_index = 0;
    g_visible_slot_counter.InterlockedAdd(0, 1, insert_index);

    if (insert_index < 8192)  // MAX_VISIBLE_SLOTS guard
    {
        g_visible_slot_ids[insert_index] = slot_idx;
    }

    for (uint i = 0; i < slot.instance_count; i++)
    {
        uint inst_idx = slot.instance_base + i;
        InstanceData inst = g_all_instances[inst_idx];

        float bounds_radius = inst.scale * 0.75f;

        if (!SphereFrustumTest(inst.pos, bounds_radius, g_frustum_planes))
            continue;

        float3 delta = inst.pos - g_camera_pos;
        float dist_sqr = dot(delta, delta);
        if (dist_sqr > g_fade_distance_sqr)
            continue;

        uint object_id = inst.object_id;
        uint output_idx;

        g_visible_counts.InterlockedAdd(object_id * 4, 1, output_idx);

        AppendInstance(object_id, inst, output_idx);
    }
}
