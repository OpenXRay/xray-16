#define SM_5_0
#include "common.h"

cbuffer InteractionParams : register(b0)
{
    uint g_entity_count;
    float g_time;
    float g_delta_time;
    float g_decay_rate;
    uint g_slot_count;
    uint g_slots_per_row;
    uint g_slot_texture_size;
    uint g_atlas_width;
};

struct InteractiveEntity
{
    float3 position;
    float radius;
    float3 velocity;
    float weight;
    float3 direction;
    float padding;
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

StructuredBuffer<InteractiveEntity> g_entities : register(t0);
StructuredBuffer<SlotAABB> g_slots : register(t1);
Buffer<uint> g_physical_to_logical : register(t2);

RWTexture2D<float4> g_interaction_atlas : register(u0);

uint2 ComputeAtlasPixel(uint slot_idx, uint2 local_pixel)
{
    uint slot_x = slot_idx % g_slots_per_row;
    uint slot_z = slot_idx / g_slots_per_row;

    uint pixel_x = slot_x * g_slot_texture_size + local_pixel.x;
    uint pixel_y = slot_z * g_slot_texture_size + local_pixel.y;

    return uint2(pixel_x, pixel_y);
}

[numthreads(32, 32, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint2 atlas_pixel = dispatch_id.xy;

    if (atlas_pixel.x >= g_atlas_width || atlas_pixel.y >= g_atlas_width)
        return;

    uint slot_x_in_atlas = atlas_pixel.x / g_slot_texture_size;
    uint slot_z_in_atlas = atlas_pixel.y / g_slot_texture_size;

    uint atlas_slots_per_row = g_atlas_width / g_slot_texture_size;
    uint physical_page = slot_z_in_atlas * atlas_slots_per_row + slot_x_in_atlas;

    if (physical_page >= 4096)
        return;

    uint logical_slot = g_physical_to_logical.Load(physical_page);

    if (logical_slot == 0xFFFFFFFF)
        return;

    if (logical_slot >= g_slot_count)
        return;

    SlotAABB slot = g_slots[logical_slot];

    uint2 local_pixel;
    local_pixel.x = atlas_pixel.x % g_slot_texture_size;
    local_pixel.y = atlas_pixel.y % g_slot_texture_size;

    float2 uv = local_pixel / float(g_slot_texture_size);
    float3 world_pos = lerp(slot.aabb_min, slot.aabb_max, float3(uv.x, 0.5, uv.y));

    float4 current = g_interaction_atlas[atlas_pixel];

    current.a += g_delta_time;

    for (uint i = 0; i < g_entity_count; i++)
    {
        InteractiveEntity entity = g_entities[i];

        float3 to_grass = world_pos - entity.position;
        float dist_xz = length(to_grass.xz);

        if (dist_xz >= entity.radius)
            continue;

        float height_diff = abs(to_grass.y);
        float height_limit = 1.0 - saturate(height_diff / (entity.radius * 2.0));

        float bend_strength = 1.0 - saturate(dist_xz / (entity.radius + 0.001));
        bend_strength = pow(bend_strength, 1.5);

        float3 bend_dir = normalize(to_grass + float3(0.001, 0.001, 0.001));

        float dir_limit = 1.0;
        if (length(entity.direction.xz) > 0.1)
        {
            float facing_dot = dot(normalize(bend_dir.xz), normalize(entity.direction.xz));
            dir_limit = saturate(facing_dot * 5.0);
        }

        float final_strength = bend_strength * height_limit * dir_limit * entity.weight;

        float2 horiz_disp = bend_dir.xz * final_strength * 2.0;
        current.rg += horiz_disp;

        current.b = max(current.b, final_strength);

        current.a = 0.0;
    }

    current.rg = clamp(current.rg, -1.0, 1.0);
    current.b = saturate(current.b);

    g_interaction_atlas[atlas_pixel] = current;
}
