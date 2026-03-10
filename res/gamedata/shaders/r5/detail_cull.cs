#include "common_samplers.h"
#include "cull_utils.h"

struct InstanceData
{
    float3 pos;
    uint packed;
};

static const float PACK_MAX_SCALE = 4.0;

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
    uint pulledVertexBase;
    uint pulledIndexCount;
    float geomExtentY;
};

cbuffer DetailCullParams : register(b5)
{
    float4x4 g_view_proj;
    float4x4 g_prev_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_visible_blade_capacity;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_lod_distance_close_sqr;
    float g_lod_distance_mid_sqr;
    float g_detail_density;
    uint g_visible_decal_capacity;
    uint g_grass_mode;
    uint g_visible_billboard_capacity;
    uint g_cull_pad2;
};

StructuredBuffer<InstanceData> g_all_instances : register(t0);
StructuredBuffer<uint> g_visible_slot_ids : register(t1);
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t2);
Texture2D<float> g_hiz_pyramid : register(t3);
StructuredBuffer<DetailModelGPU> g_detail_models : register(t4);


RWStructuredBuffer<uint> g_visible_lod0 : register(u0);
RWByteAddressBuffer g_indirect_args_lod0 : register(u1);
RWStructuredBuffer<uint> g_visible_lod1 : register(u2);
RWByteAddressBuffer g_indirect_args_lod1 : register(u3);
RWStructuredBuffer<uint> g_visible_lod2 : register(u4);
RWByteAddressBuffer g_indirect_args_lod2 : register(u5);
RWStructuredBuffer<uint> g_visible_decals : register(u6);
RWByteAddressBuffer g_indirect_args_decal : register(u7);
RWStructuredBuffer<uint> g_visible_billboard : register(u8);
RWByteAddressBuffer g_indirect_args_billboard : register(u9);

static const uint DO_NO_WAVING = 0x0001;

void AppendBladeLOD(float3 pos, uint inst_idx)
{
    float3 to_camera = pos - g_camera_pos;
    float dist_sqr = dot(to_camera, to_camera);

    uint idx;
    if (dist_sqr < g_lod_distance_close_sqr)
    {
        g_indirect_args_lod0.InterlockedAdd(4, 1, idx);
        if (idx < g_visible_blade_capacity)
            g_visible_lod0[idx] = inst_idx;
    }
    else if (dist_sqr < g_lod_distance_mid_sqr)
    {
        g_indirect_args_lod1.InterlockedAdd(4, 1, idx);
        if (idx < g_visible_blade_capacity)
            g_visible_lod1[idx] = inst_idx;
    }
    else
    {
        g_indirect_args_lod2.InterlockedAdd(4, 1, idx);
        if (idx < g_visible_blade_capacity)
            g_visible_lod2[idx] = inst_idx;
    }
}

void AppendBillboard(uint inst_idx)
{
    uint idx;
    g_indirect_args_billboard.InterlockedAdd(4, 1, idx);
    if (idx < g_visible_billboard_capacity)
        g_visible_billboard[idx] = inst_idx;
}

void AppendDecal(uint inst_idx)
{
    uint idx;
    g_indirect_args_decal.InterlockedAdd(4, 1, idx);
    if (idx < g_visible_decal_capacity)
        g_visible_decals[idx] = inst_idx;
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

        float scale = float((inst.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;
        uint object_id = inst.packed & 0x3F;

        DetailModelGPU mdl = g_detail_models[object_id];
        float geom_half_height = mdl.geomExtentY * 0.5;
        float bounds_radius = scale * max(geom_half_height, max(mdl.geomExtentX, mdl.geomExtentZ) * 0.5);
        bounds_radius = max(bounds_radius, scale * 0.25);
        float3 bounds_center = inst.pos + float3(0, scale * geom_half_height, 0);

        if (!DistanceTestSphere(bounds_center, bounds_radius, g_camera_pos, g_fade_distance_sqr))
            continue;

        if (!FrustumTestSphere(bounds_center, bounds_radius, g_frustum_planes))
            continue;

        if (!HiZTestSphereTemporal(bounds_center, bounds_radius, g_camera_pos, g_view_proj, g_prev_view_proj,
                            g_hiz_pyramid, smp_nofilter, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
            continue;

        uint flags = asuint(mdl.flags);
        bool is_static = (flags & DO_NO_WAVING) != 0;
        if (is_static)
            AppendDecal(inst_idx);
        else if (g_grass_mode == 0)
            AppendBillboard(inst_idx);
        else
            AppendBladeLOD(inst.pos, inst_idx);
    }
}
