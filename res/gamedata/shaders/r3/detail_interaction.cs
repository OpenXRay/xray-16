// detail_interaction.cs - Interactive grass displacement from entities
// Phase 5, Milestone 5.3: Render entity interactions to per-slot texture atlas
//
#define SM_5_0
#include "common.h"

// Constant buffer for interaction parameters
cbuffer InteractionParams : register(b0)
{
    uint g_entity_count;
    float g_time;
    float g_delta_time;
    float g_decay_rate;           // How fast displacement fades (e.g., 0.95 = 5% decay per second)
    uint g_slot_count;
    uint g_slots_per_row;
    uint g_slot_texture_size;     // 32 pixels per slot
    uint g_atlas_width;           // 2048
};

// Entity data structure
struct InteractiveEntity
{
    float3 position;
    float radius;              // Interaction radius
    float3 velocity;           // Movement direction/speed
    float weight;              // 0-1, affects displacement strength
    float2 padding;
};

// Slot AABB structure (matches C++)
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

// Input buffers
StructuredBuffer<InteractiveEntity> g_entities : register(t0);
StructuredBuffer<SlotAABB> g_slots : register(t1);
Buffer<uint> g_physical_to_logical : register(t2);  // Maps physical page → logical slot index

// Output: Interaction atlas (RG=displacement XZ, B=bend amount, A=age)
RWTexture2D<float4> g_interaction_atlas : register(u0);

// Helper: Compute atlas pixel coordinates from slot index and local UV
uint2 ComputeAtlasPixel(uint slot_idx, uint2 local_pixel)
{
    uint slot_x = slot_idx % g_slots_per_row;
    uint slot_z = slot_idx / g_slots_per_row;

    uint pixel_x = slot_x * g_slot_texture_size + local_pixel.x;
    uint pixel_y = slot_z * g_slot_texture_size + local_pixel.y;

    return uint2(pixel_x, pixel_y);
}

// Per-pixel dispatch: process entire atlas texture
[numthreads(32, 32, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint2 atlas_pixel = dispatch_id.xy;

    // Bounds check
    if (atlas_pixel.x >= g_atlas_width || atlas_pixel.y >= g_atlas_width)
        return;

    // Compute physical atlas slot index from pixel coordinates
    uint slot_x_in_atlas = atlas_pixel.x / g_slot_texture_size;
    uint slot_z_in_atlas = atlas_pixel.y / g_slot_texture_size;

    // Calculate physical page index (0-4095)
    uint atlas_slots_per_row = g_atlas_width / g_slot_texture_size;  // 64
    uint physical_page = slot_z_in_atlas * atlas_slots_per_row + slot_x_in_atlas;

    // Only process slots that fit in the atlas
    if (physical_page >= 4096)
        return;

    // Phase 6: Map physical page to logical world slot
    uint logical_slot = g_physical_to_logical.Load(physical_page);

    // Check if page is not resident
    if (logical_slot == 0xFFFFFFFF)
        return;  // Empty/not promoted yet

    // Bounds check logical slot
    if (logical_slot >= g_slot_count)
        return;

    SlotAABB slot = g_slots[logical_slot];

    // TEMPORARILY DISABLED: Don't skip empty slots - process all slots for debugging
    // if (slot.instance_count == 0)
    //     return;

    // Compute local pixel within slot (0-31, 0-31)
    uint2 local_pixel;
    local_pixel.x = atlas_pixel.x % g_slot_texture_size;
    local_pixel.y = atlas_pixel.y % g_slot_texture_size;

    // Map to world position within slot
    float2 uv = local_pixel / float(g_slot_texture_size);  // 0-1 within slot
    float3 world_pos = lerp(slot.aabb_min, slot.aabb_max, float3(uv.x, 0.5, uv.y));

    // Read current interaction state
    float4 current = g_interaction_atlas[atlas_pixel];

    // Decay existing displacement
    current.rg *= g_decay_rate;
    current.b *= g_decay_rate;
    current.a += g_delta_time;  // Age tracker

    // Apply entity interactions
    for (uint i = 0; i < g_entity_count; i++)
    {
        InteractiveEntity entity = g_entities[i];

        float3 to_grass = world_pos - entity.position;
        float dist = length(to_grass.xz);

        // Early out if outside radius
        if (dist >= entity.radius)
            continue;

        // Compute influence falloff
        float influence = 1.0 - saturate(dist / entity.radius);  // 1 at center, 0 at edge
        influence = pow(influence, 2.0);  // Smooth falloff (quadratic)

        // Direction away from entity (radial push)
        float2 push_dir = normalize(to_grass.xz + float2(0.001, 0.001));  // Add epsilon to avoid NaN

        // DEBUG: Write influence directly to see falloff pattern
        current.rgb = influence;  // White = close to entity, black = far away
        current.a = 1.0;

        // COMMENTED OUT - Will restore after verification
        // float velocity_strength = length(entity.velocity.xz) * 0.5;
        // float base_strength = velocity_strength + entity.weight * 2.0;
        // float displacement_strength = base_strength * influence * 10.0;
        // current.rg += push_dir * displacement_strength;
        // current.b = max(current.b, influence);
        // current.a = 0.0;
    }

    // Clamp displacement to reasonable range
    current.rg = clamp(current.rg, -1.0, 1.0);
    current.b = saturate(current.b);

    // Write back to atlas
    g_interaction_atlas[atlas_pixel] = current;
}
