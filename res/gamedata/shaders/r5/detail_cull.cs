#define SM_5_0
#include "common.h"

struct InstanceData
{
    float3 hpb;
    float scale;
    float3 pos;
    float hemi;
    uint vis_id;
    uint object_id;
};

cbuffer DetailCullParams : register(b0)
{
    float4x4 g_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_total_instance_count;
    uint g_object_count;
    uint g_pad0;
    uint g_pad1;
}
;

StructuredBuffer<InstanceData> g_all_instances : register(t0);

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

RWByteAddressBuffer g_visible_counts : register(u16);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

if (idx >= g_total_instance_count)
    return;

InstanceData inst = g_all_instances[idx];

float3 delta = inst.pos - g_camera_pos;
float dist_sqr = dot(delta, delta);

if (dist_sqr > g_fade_distance_sqr)
    return;

float bounds_radius = inst.scale * 0.75f;

[unroll]
for (uint i = 0; i < 5; ++i)
{
    float dist = dot(g_frustum_planes[i].xyz, inst.pos) + g_frustum_planes[i].w;
    if (dist > bounds_radius)
        return;
}

uint object_id = inst.object_id;
uint output_idx;

g_visible_counts.InterlockedAdd(object_id * 4, 1, output_idx);

if (object_id == 0)
    g_visible_obj0[output_idx] = inst;
else if (object_id == 1)
    g_visible_obj1[output_idx] = inst;
else if (object_id == 2)
    g_visible_obj2[output_idx] = inst;
else if (object_id == 3)
    g_visible_obj3[output_idx] = inst;
else if (object_id == 4)
    g_visible_obj4[output_idx] = inst;
else if (object_id == 5)
    g_visible_obj5[output_idx] = inst;
else if (object_id == 6)
    g_visible_obj6[output_idx] = inst;
else if (object_id == 7)
    g_visible_obj7[output_idx] = inst;
else if (object_id == 8)
    g_visible_obj8[output_idx] = inst;
else if (object_id == 9)
    g_visible_obj9[output_idx] = inst;
else if (object_id == 10)
    g_visible_obj10[output_idx] = inst;
else if (object_id == 11)
    g_visible_obj11[output_idx] = inst;
else if (object_id == 12)
    g_visible_obj12[output_idx] = inst;
else if (object_id == 13)
    g_visible_obj13[output_idx] = inst;
else if (object_id == 14)
    g_visible_obj14[output_idx] = inst;
else if (object_id == 15)
    g_visible_obj15[output_idx] = inst;
}
