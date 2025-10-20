// detail_cull.cs - GPU-driven frustum culling for detail objects
// Phase 2.1: Processes all level instances and outputs visible instances per object
//
#define SM_5_0
#include "common.h"

// ===========================
// Structures (must match C++)
// ===========================

// Phase 2.0.3: InstanceData structure (32 bytes)
struct InstanceData
{
    float3 hpb;         // Heading, pitch, bank (12 bytes)
    float scale;        // Scale factor (4 bytes)
    float3 pos;         // World position (12 bytes)
    float hemi;         // Hemisphere lighting (4 bytes)
    uint vis_id;        // Animation type (4 bytes)
    uint object_id;     // Grass object type (4 bytes)
};

// ===========================
// Input Buffers
// ===========================

// Must match C++ DetailCullParams struct in dx11DetailManager_VS.cpp exactly!
cbuffer DetailCullParams : register(b0)
{
    float4x4 g_view_proj;           // View-projection matrix (64 bytes)
    float3 g_camera_pos;            // Camera position (12 bytes)
    float g_fade_distance_sqr;      // Max distance squared (4 bytes)
    float4 g_frustum_planes[6];     // Frustum planes (6 * 16 = 96 bytes)
    uint g_total_instance_count;    // Total instances to process (4 bytes)
    uint g_object_count;            // Number of grass object types (4 bytes)
    uint g_pad0;                    // Padding (4 bytes)
    uint g_pad1;                    // Padding (4 bytes)
};

// Input: All level instances (immutable persistent buffer)
StructuredBuffer<InstanceData> g_all_instances : register(t0);

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

// ===========================
// Compute Shader Entry Point
// ===========================

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

    // Early exit if beyond instance count
    if (idx >= g_total_instance_count)
        return;

    // Load instance data
    InstanceData inst = g_all_instances[idx];

    // =========================
    // Distance Culling
    // =========================
    float3 delta = inst.pos - g_camera_pos;
    float dist_sqr = dot(delta, delta);

    if (dist_sqr > g_fade_distance_sqr)
        return;  // Too far, culled

    // =========================
    // Frustum Culling
    // =========================
    // Estimate grass bounds (typically ~1m tall, affected by scale)
    float bounds_radius = inst.scale * 0.75f;

    // Frustum culling (only test first 5 planes - LRTB + FAR, no NEAR)
    // The 6th plane is unused/dummy
    // X-Ray plane format: dot(n, p) + d >= 0 for inside (confirmed in _plane.h:65)
    // BUT: Planes seem to be inverted, so test with POSITIVE side outside
    [unroll]
    for (uint i = 0; i < 5; ++i)
    {
        float dist = dot(g_frustum_planes[i].xyz, inst.pos) + g_frustum_planes[i].w;
        if (dist > bounds_radius)
            return;  // Outside frustum, culled
    }

    // =========================
    // Append to Per-Object Visible List
    // =========================
    uint object_id = inst.object_id;
    uint output_idx;

    // Atomic increment count for this object (legacy counter buffer, kept for debugging)
    g_visible_counts.InterlockedAdd(object_id * 4, 1, output_idx);

    // Phase 2.2.1: Also increment InstanceCount in indirect args buffer
    // InstanceCount field is at offset 4 bytes (second u32)
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
    // Objects 16+ will be ignored for now (can expand to 64 later with more UAV slots)
}
