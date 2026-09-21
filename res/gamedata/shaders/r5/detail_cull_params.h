#ifndef DETAIL_CULL_PARAMS_H
#define DETAIL_CULL_PARAMS_H

#include "detail_slot_common.h"

cbuffer DetailCullParams : register(b5)
{
    float4x4 g_view_proj;
    float4x4 g_prev_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint3 g_visible_blade_capacity;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_detail_density;
    float g_lod_distance_close_sqr;
    float g_lod_distance_mid_sqr;
    uint g_visible_decal_capacity;
    uint g_grass_mode;
    uint g_visible_billboard_capacity;
    uint3 g_prepared_capacity;
    uint g_ray_mode;
    float g_ray_radius;
    float g_ray_cell_radius;
    uint g_ray_pad;
};

#endif
