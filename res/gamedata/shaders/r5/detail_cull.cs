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

struct DetailModelGPU
{
    float minScale;
    float maxScale;
    float flags;
    float geomExtentX;
    float geomExtentZ;
    float uv_min_x;
    float uv_min_y;
    float uv_max_x;
    float uv_max_y;
    uint decalVertexBase;
    uint decalIndexCount;
    uint pad;
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
    float g_detail_density;
};

StructuredBuffer<InstanceData> g_all_instances : register(t0);
StructuredBuffer<uint> g_visible_slot_ids : register(t1);
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t2);
Texture2D<float> g_hiz_pyramid : register(t3);
StructuredBuffer<DetailModelGPU> g_detail_models : register(t4);
SamplerState g_point_sampler : register(s0);

RWStructuredBuffer<InstanceData> g_visible_lod0 : register(u0);
RWByteAddressBuffer g_indirect_args_lod0 : register(u1);
RWStructuredBuffer<InstanceData> g_visible_lod1 : register(u2);
RWByteAddressBuffer g_indirect_args_lod1 : register(u3);
RWStructuredBuffer<InstanceData> g_visible_lod2 : register(u4);
RWByteAddressBuffer g_indirect_args_lod2 : register(u5);
RWStructuredBuffer<InstanceData> g_visible_decals : register(u6);
RWByteAddressBuffer g_indirect_args_decal : register(u7);

static const uint DO_NO_WAVING = 0x0001;

void AppendBladeLOD(InstanceData inst)
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

void AppendDecal(InstanceData inst)
{
    uint idx;
    g_indirect_args_decal.InterlockedAdd(4, 1, idx);
    g_visible_decals[idx] = inst;
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint slot_id = g_visible_slot_ids[group_id.x];
    SlotAABB slot = g_slot_aabbs[slot_id];

    for (uint i = thread_id.x; i < slot.instance_count; i += 64)
    {
        uint inst_idx = slot.instance_base + i;
        InstanceData inst = g_all_instances[inst_idx];

        float bounds_radius = inst.scale * 0.5;

        if (!DistanceTestSphere(inst.pos, bounds_radius, g_camera_pos, g_fade_distance_sqr))
            continue;

        if (!FrustumTestSphere(inst.pos, bounds_radius, g_frustum_planes))
            continue;

        if (!HiZTestSphereTemporal(inst.pos, bounds_radius, g_camera_pos, g_view_proj, g_prev_view_proj,
                            g_hiz_pyramid, g_point_sampler, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
            continue;

        uint flags = asuint(g_detail_models[inst.object_id].flags);
        if ((flags & DO_NO_WAVING) != 0)
            AppendDecal(inst);
        else
            AppendBladeLOD(inst);
    }
}
