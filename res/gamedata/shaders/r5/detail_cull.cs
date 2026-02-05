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

cbuffer DetailCullParams : register(b5)
{
    float4x4 g_view_proj;
    float4x4 g_prev_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_total_instance_count;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_lod_distance_close_sqr;
    float g_lod_distance_mid_sqr;
    uint g_pad;
};

StructuredBuffer<InstanceData> g_all_instances : register(t0);
StructuredBuffer<uint> g_slot_visibility : register(t1);
StructuredBuffer<uint> g_instance_to_slot : register(t2);

Texture2D<float> g_hiz_pyramid : register(t3);
SamplerState g_point_sampler : register(s0);

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

    uint slot_idx = g_instance_to_slot[inst_idx];

    if (g_slot_visibility[slot_idx] == 0)
        return;

    InstanceData inst = g_all_instances[inst_idx];

    float bounds_radius = inst.scale * 0.5;

    if (!DistanceTestSphere(inst.pos, bounds_radius, g_camera_pos, g_fade_distance_sqr))
        return;

    if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
        return;

    if (!HiZTestSphereTemporal(inst.pos, bounds_radius, g_camera_pos, g_view_proj, g_prev_view_proj,
                        g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
        return;

    AppendInstanceLOD(inst);
}
