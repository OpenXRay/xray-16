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

// ===========================
// Output Buffers (UNIFIED - Single draw call optimization)
// ===========================

// UNIFIED: All grass types use same shader/geometry, so merge into one buffer
RWStructuredBuffer<InstanceData> g_visible_unified : register(u0);

// Single atomic counter for all visible instances
RWByteAddressBuffer g_visible_count : register(u1);

// Phase 6B: Output buffer for visible slot IDs (for page table system)
RWStructuredBuffer<uint> g_visible_slot_ids : register(u33);  // Append visible slots here
RWByteAddressBuffer g_visible_slot_counter : register(u34);  // Atomic counter (ByteAddressBuffer for InterlockedAdd)

// Phase 2.2.1: UNIFIED indirect draw args buffer
// D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS:
// struct { u32 IndexCount; u32 InstanceCount; u32 StartIndex; s32 BaseVertex; u32 StartInstance; }
// We only write to InstanceCount field (offset 4 bytes)
RWByteAddressBuffer g_indirect_args_unified : register(u2);


bool AABBDistanceTest(float3 aabb_min, float3 aabb_max, float3 camera_pos, float max_dist_sqr)
{
    float3 closest = clamp(camera_pos, aabb_min, aabb_max);
    float dist_sqr = dot(closest - camera_pos, closest - camera_pos);
    return dist_sqr < max_dist_sqr;
}

bool SphereFrustumTest(float3 center, float radius, float4 planes[6])
{
    for (uint i = 0; i < 5; ++i)
    {
        float dist = dot(planes[i].xyz, center) + planes[i].w;
        if (dist > radius)
            return false;
    }
    return true;
}

// UNIFIED: Single buffer append (replaces 16-way branch)
void AppendInstance(InstanceData inst)
{
    // Atomically allocate slot in unified buffer
    uint output_idx = 0;
    g_visible_count.InterlockedAdd(0, 1, output_idx);

    // Write instance to unified buffer
    // object_id is already stored in inst, so shader can handle variation if needed
    g_visible_unified[output_idx] = inst;

    // Update indirect draw args (InstanceCount at offset 4)
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

        // UNIFIED: Append to single buffer (object_id already in inst)
        AppendInstance(inst);
    }
}
