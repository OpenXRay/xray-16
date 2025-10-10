#define SM_5_0
#include "common.h"

struct DetailInstanceGPU
{
    float3 position;
    float scale;
    float rotation_y;
    float3 padding0;

    float c_hemi;
    float c_sun;
    uint object_id;
    uint vis_id;
    float3 color_rgb;
    float padding1;

    float3 bounds_min;
    float bounds_radius;
    float3 bounds_max;
    float padding2;

    uint slot_x;
    uint slot_z;
    uint flags;
    float fade_distance_sqr;
};

struct FrustumPlane
{
    float4 plane;
};

struct IndirectDrawArgs
{
    uint index_count;
    uint instance_count;
    uint start_index;
    int base_vertex;
    uint start_instance;
};

cbuffer CullParams : register(b0)
{
    float3 g_camera_pos;
    float g_fade_limit_sqr;

    float3 g_camera_dir;
    float g_fade_start_sqr;

    float g_r_ssa_discard;
    float g_r_ssa_cheap;
    uint g_instance_count;
    uint g_frame_number;

    float4 g_frustum_planes[6];
};

StructuredBuffer<DetailInstanceGPU> g_instances : register(t0);

RWStructuredBuffer<uint> g_visible_still : register(u0);
RWStructuredBuffer<uint> g_visible_wave1 : register(u1);
RWStructuredBuffer<uint> g_visible_wave2 : register(u2);

RWStructuredBuffer<uint> g_counters : register(u3);

RWStructuredBuffer<IndirectDrawArgs> g_indirect_args : register(u4);

bool SphereInsidePlane(float3 center, float radius, float4 plane)
{
    float distance = dot(plane.xyz, center) + plane.w;
    return distance > -radius;
}

bool FrustumCullSphere(float3 center, float radius)
{
    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        if (!SphereInsidePlane(center, radius, g_frustum_planes[i]))
            return false;
    }
    return true;
}

float ComputeSSA(float3 world_pos, float radius, float scale)
{
    float dist_sqr = dot(world_pos - g_camera_pos, world_pos - g_camera_pos);
    if (dist_sqr < 0.001f)
        return 1e6f;

    float scaled_radius = scale * radius;
    return (scaled_radius * scaled_radius) / dist_sqr;
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint instance_idx = dispatch_thread_id.x;

    if (instance_idx >= g_instance_count)
        return;

    DetailInstanceGPU inst = g_instances[instance_idx];

    float3 to_camera = g_camera_pos - inst.position;
    float dist_sqr = dot(to_camera, to_camera);

    if (dist_sqr > g_fade_limit_sqr)
        return;

    float world_radius = inst.bounds_radius * inst.scale;
    if (!FrustumCullSphere(inst.position, world_radius))
        return;

    float ssa = ComputeSSA(inst.position, inst.bounds_radius, inst.scale);
    if (ssa < g_r_ssa_discard)
        return;

    float fade_alpha = 1.0f;
    if (dist_sqr > g_fade_start_sqr)
    {
        float fade_range = g_fade_limit_sqr - g_fade_start_sqr;
        fade_alpha = 1.0f - ((dist_sqr - g_fade_start_sqr) / fade_range);
    }

    uint output_idx = 0;

    if (inst.vis_id == 0)
    {
        InterlockedAdd(g_counters[0], 1, output_idx);
        g_visible_still[output_idx] = instance_idx;
    }
    else if (inst.vis_id == 1)
    {
        InterlockedAdd(g_counters[1], 1, output_idx);
        g_visible_wave1[output_idx] = instance_idx;
    }
    else
    {
        InterlockedAdd(g_counters[2], 1, output_idx);
        g_visible_wave2[output_idx] = instance_idx;
    }
}
