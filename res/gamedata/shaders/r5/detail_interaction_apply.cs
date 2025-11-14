// detail_interaction_apply.cs
// Phase 3: Apply pending interaction updates to the interaction atlas
// Dispatched when a deferred A-Life interaction needs to be applied

RWTexture2D<float4> interaction_atlas : register(u0);

cbuffer UpdateParams : register(b0) {
    float2 world_center;      // 0-7: Center of interaction in world space (XZ)
    float radius;             // 8-11: Interaction radius
    float strength;           // 12-15: Trampling strength (0-1)
    uint physical_page;       // 16-19: Which atlas page to update
    float slot_size;          // 20-23: Size of one slot in world units (e.g. 2.0)
    float atlas_width;        // 24-27: Atlas texture width (e.g. 2048)
    float slot_texture_size;  // 28-31: Size of one slot in atlas pixels (e.g. 32)
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 1. Compute which atlas texel we're updating
    const uint pages_per_row = 64;  // 2048 / 32 = 64 pages per row
    uint page_x = physical_page % pages_per_row;
    uint page_y = physical_page / pages_per_row;

    uint atlas_x = page_x * uint(slot_texture_size) + dispatchThreadID.x;
    uint atlas_y = page_y * uint(slot_texture_size) + dispatchThreadID.y;

    // Bounds check
    if (atlas_x >= uint(atlas_width) || atlas_y >= uint(atlas_width)) {
        return;  // Out of bounds
    }

    // 2. Compute world position of this texel
    // Map from [0, slot_texture_size) to slot's world space
    float2 slot_local_uv = float2(dispatchThreadID.xy) / slot_texture_size;

    // Slot spans [world_center - slot_size/2, world_center + slot_size/2]
    float slot_half = slot_size * 0.5;
    float2 slot_min = world_center - float2(slot_half, slot_half);
    float2 world_pos = slot_min + slot_local_uv * slot_size;

    // 3. Compute distance from interaction center
    float dist = distance(world_pos, world_center);

    // 4. Apply interaction if within radius
    if (dist < radius) {
        // Smooth falloff from center to edge
        float falloff = 1.0 - (dist / radius);  // Linear falloff
        falloff = smoothstep(0.0, 1.0, falloff);  // Smooth it

        float interaction_value = strength * falloff;

        // Read current value
        float4 current = interaction_atlas[uint2(atlas_x, atlas_y)];

        // Accumulate interaction (take max to avoid over-trampling)
        // R channel: displacement amount
        current.r = max(current.r, interaction_value);

        // G channel: could be used for direction or other data
        // B/A channels: reserved for future use

        // Write back
        interaction_atlas[uint2(atlas_x, atlas_y)] = current;
    }
}
