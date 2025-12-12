// detail_cull.cs - GPU-driven frustum + Hi-Z occlusion culling for detail objects
// Hierarchical culling: test slot AABBs first, then instances within visible slots
//
// NOTE: Don't include common.h - it's for graphics shaders, not compute shaders
#include "cull_utils.h"

// Must match C++ FGDetailManager::InstanceData exactly!
struct InstanceData
{
    float3 pos;      // World position (12 bytes)
    float scale;     // Scale factor (4 bytes)
    float rotation;  // Y-axis rotation in radians (4 bytes)
    float hemi;      // Hemisphere lighting (4 bytes)
    uint vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2) (4 bytes)
    uint object_id;  // Which grass object type (0-63) (4 bytes)
};  // Total: 32 bytes

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

// Use b5 to avoid collision with common.h buffers (b0, b1, b2, b3, b4)
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

// Input buffers
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t0);
StructuredBuffer<InstanceData> g_all_instances : register(t1);

// Hi-Z pyramid for occlusion culling
Texture2D<float> g_hiz_pyramid : register(t2);
SamplerState g_point_sampler : register(s0);

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

    // ─────────────────────────────────────────────────────
    //  SLOT-LEVEL CULLING (AABB tests - cheap)
    // ─────────────────────────────────────────────────────

    // 1. Distance culling for slot AABB
    if (!DistanceTestAABB(slot.aabb_min, slot.aabb_max, g_camera_pos, g_fade_distance_sqr))
        return;

    // 2. Frustum culling for slot AABB
    if (!FrustumTestAABB(slot.aabb_min, slot.aabb_max, g_frustum_planes))
        return;

    // Record visible slot (for page table system)
    uint insert_index = 0;
    g_visible_slot_counter.InterlockedAdd(0, 1, insert_index);
    if (insert_index < 8192)
        g_visible_slot_ids[insert_index] = slot_idx;

    // ─────────────────────────────────────────────────────
    //  PER-INSTANCE CULLING
    // ─────────────────────────────────────────────────────

    for (uint i = 0; i < slot.instance_count; i++)
    {
        uint inst_idx = slot.instance_base + i;
        InstanceData inst = g_all_instances[inst_idx];

        // Approximate bounding sphere radius from scale
        float bounds_radius = inst.scale * 0.5;

        // 1. Distance culling (cheapest)
        if (!DistanceTestSphere(inst.pos, bounds_radius, g_camera_pos, g_fade_distance_sqr))
            continue;

        // 2. Frustum culling (cheap)
        if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
            continue;

        // 3. Hi-Z occlusion culling (expensive but effective)
        if (!HiZTestSphere(inst.pos, bounds_radius, g_camera_pos, g_view_proj,
                            g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
            continue;

        // Instance passed all culling tests - append to visible buffer
        AppendInstance(inst);
    }
}
