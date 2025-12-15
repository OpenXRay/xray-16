// detail_cull.cs - GPU-driven frustum + Hi-Z occlusion culling for detail objects
// Hierarchical culling: test slot AABBs first, then instances within visible slots
// LOD classification: instances sorted into 3 LOD buckets by distance
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
    float4x4 g_view_proj;          // Current frame view-projection (for frustum culling)
    float4x4 g_prev_view_proj;     // Previous frame view-projection (for temporal Hi-Z sampling)
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_total_instance_count;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_lod_distance_close_sqr;  // LOD0 -> LOD1 threshold (squared)
    float g_lod_distance_mid_sqr;    // LOD1 -> LOD2 threshold (squared)
    uint g_pad;
};

// Input buffers
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t0);
StructuredBuffer<InstanceData> g_all_instances : register(t1);

// Hi-Z pyramid for occlusion culling
Texture2D<float> g_hiz_pyramid : register(t2);
SamplerState g_point_sampler : register(s0);

// ===========================
// Output Buffers (Per-LOD)
// ===========================

// LOD0 (close - highest detail)
RWStructuredBuffer<InstanceData> g_visible_lod0 : register(u0);
RWByteAddressBuffer g_visible_count_lod0 : register(u1);
RWByteAddressBuffer g_indirect_args_lod0 : register(u2);

// LOD1 (mid - medium detail)
RWStructuredBuffer<InstanceData> g_visible_lod1 : register(u3);
RWByteAddressBuffer g_visible_count_lod1 : register(u4);
RWByteAddressBuffer g_indirect_args_lod1 : register(u5);

// LOD2 (far - low detail)
RWStructuredBuffer<InstanceData> g_visible_lod2 : register(u6);
RWByteAddressBuffer g_visible_count_lod2 : register(u7);
RWByteAddressBuffer g_indirect_args_lod2 : register(u8);

// Visible slot tracking (for page table system)
RWStructuredBuffer<uint> g_visible_slot_ids : register(u33);
RWByteAddressBuffer g_visible_slot_counter : register(u34);


// Append instance to appropriate LOD buffer based on distance
void AppendInstanceLOD(InstanceData inst)
{
    float3 to_camera = inst.pos - g_camera_pos;
    float dist_sqr = dot(to_camera, to_camera);

    uint idx;
    if (dist_sqr < g_lod_distance_close_sqr)
    {
        // LOD0 - close range, highest detail
        g_visible_count_lod0.InterlockedAdd(0, 1, idx);
        g_visible_lod0[idx] = inst;
        g_indirect_args_lod0.InterlockedAdd(4, 1, idx);  // InstanceCount at offset 4
    }
    else if (dist_sqr < g_lod_distance_mid_sqr)
    {
        // LOD1 - mid range, medium detail
        g_visible_count_lod1.InterlockedAdd(0, 1, idx);
        g_visible_lod1[idx] = inst;
        g_indirect_args_lod1.InterlockedAdd(4, 1, idx);
    }
    else
    {
        // LOD2 - far range, low detail
        g_visible_count_lod2.InterlockedAdd(0, 1, idx);
        g_visible_lod2[idx] = inst;
        g_indirect_args_lod2.InterlockedAdd(4, 1, idx);
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
        // TEMPORAL HI-Z: Use prev_view_proj for Hi-Z lookup since pyramid was built from previous frame's depth
        if (!HiZTestSphereTemporal(inst.pos, bounds_radius, g_camera_pos, g_view_proj, g_prev_view_proj,
                            g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
            continue;

        // Instance passed all culling tests - append to appropriate LOD buffer
        AppendInstanceLOD(inst);
    }
}
