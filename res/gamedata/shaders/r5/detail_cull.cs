// detail_cull.cs - Pass 2: Per-instance culling (indirect dispatch, 1 group per visible slot)
// Uses DispatchIndirect driven by Pass 1's visible slot count.
// Each thread group processes one visible slot's instances via stride loop.
//
// KEY OPTIMIZATION: Only launches groups for slots that passed Pass 1 culling.
// Eliminates instanceToSlot lookups and slotVisibility reads entirely.
// All threads in a group read from the same slot's contiguous instance range.
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

// Must match C++ FGDetailManager::SlotAABB exactly!
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
};  // Total: 64 bytes

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
StructuredBuffer<InstanceData> g_all_instances : register(t0);
StructuredBuffer<uint> g_visible_slot_ids : register(t1);       // Visible slot IDs from Pass 1
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t2);         // Slot metadata (instance_base, instance_count)

// Hi-Z pyramid for occlusion culling
Texture2D<float> g_hiz_pyramid : register(t3);
SamplerState g_point_sampler : register(s0);

// Output Buffers (Per-LOD)
RWStructuredBuffer<InstanceData> g_visible_lod0 : register(u0);
RWByteAddressBuffer g_indirect_args_lod0 : register(u1);

RWStructuredBuffer<InstanceData> g_visible_lod1 : register(u2);
RWByteAddressBuffer g_indirect_args_lod1 : register(u3);

RWStructuredBuffer<InstanceData> g_visible_lod2 : register(u4);
RWByteAddressBuffer g_indirect_args_lod2 : register(u5);


void AppendInstanceLOD(InstanceData inst)
{
    float3 to_camera = inst.pos - g_camera_pos;
    float dist_sqr = dot(to_camera, to_camera);

    uint idx;
    if (dist_sqr < g_lod_distance_close_sqr)
    {
        g_indirect_args_lod0.InterlockedAdd(4, 1, idx);
        g_visible_lod0[idx] = inst;
    }
    else if (dist_sqr < g_lod_distance_mid_sqr)
    {
        g_indirect_args_lod1.InterlockedAdd(4, 1, idx);
        g_visible_lod1[idx] = inst;
    }
    else
    {
        g_indirect_args_lod2.InterlockedAdd(4, 1, idx);
        g_visible_lod2[idx] = inst;
    }
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    // Each group handles one visible slot (driven by DispatchIndirect)
    uint slot_id = g_visible_slot_ids[group_id.x];
    SlotAABB slot = g_slot_aabbs[slot_id];

    // Stride loop: threads cooperatively process all instances in this slot
    for (uint i = thread_id.x; i < slot.instance_count; i += 64)
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
