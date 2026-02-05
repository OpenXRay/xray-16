// detail_cull.cs - Pass 2: Per-instance culling (flat dispatch, 1 thread per instance)
// Reads slot visibility from Pass 1 (detail_cell_cull.cs), then does per-instance
// distance + frustum + Hi-Z culling. LOD classification into 3 buckets.
//
// KEY OPTIMIZATION: Flat dispatch eliminates warp divergence from variable-length
// per-slot loops. Adjacent instances share the same slot, so slot visibility reads
// are coherent within warps.
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
StructuredBuffer<uint> g_slot_visibility : register(t1);      // From Pass 1
StructuredBuffer<uint> g_instance_to_slot : register(t2);     // Maps instance_idx -> slot_idx

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

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint inst_idx = dispatch_thread_id.x;

    if (inst_idx >= g_total_instance_count)
        return;

    // Read which slot this instance belongs to
    uint slot_idx = g_instance_to_slot[inst_idx];

    // Early-out: slot was culled in Pass 1 (coherent read - adjacent instances share slot)
    if (g_slot_visibility[slot_idx] == 0)
        return;

    // Load instance data (coalesced read - adjacent threads read adjacent instances)
    InstanceData inst = g_all_instances[inst_idx];

    // Approximate bounding sphere radius from scale
    float bounds_radius = inst.scale * 0.5;

    // 1. Distance culling (cheapest)
    if (!DistanceTestSphere(inst.pos, bounds_radius, g_camera_pos, g_fade_distance_sqr))
        return;

    // 2. Frustum culling (cheap)
    if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
        return;

    // 3. Hi-Z occlusion culling (expensive but effective)
    // TEMPORAL HI-Z: Use prev_view_proj for Hi-Z lookup since pyramid was built from previous frame's depth
    if (!HiZTestSphereTemporal(inst.pos, bounds_radius, g_camera_pos, g_view_proj, g_prev_view_proj,
                        g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
        return;

    // Instance passed all culling tests - append to appropriate LOD buffer
    AppendInstanceLOD(inst);
}
