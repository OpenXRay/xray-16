RWTexture2D<float4> interaction_atlas : register(u0);

cbuffer UpdateParams : register(b0) {
    float2 world_center;
    float radius;
    float strength;
    uint physical_page;
    float slot_size;
    float atlas_width;
    float slot_texture_size;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint pages_per_row = 64;
    uint page_x = physical_page % pages_per_row;
    uint page_y = physical_page / pages_per_row;

    uint atlas_x = page_x * uint(slot_texture_size) + dispatchThreadID.x;
    uint atlas_y = page_y * uint(slot_texture_size) + dispatchThreadID.y;

    if (atlas_x >= uint(atlas_width) || atlas_y >= uint(atlas_width)) {
        return;
    }

    float2 slot_local_uv = float2(dispatchThreadID.xy) / slot_texture_size;

    float slot_half = slot_size * 0.5;
    float2 slot_min = world_center - float2(slot_half, slot_half);
    float2 world_pos = slot_min + slot_local_uv * slot_size;

    float dist = distance(world_pos, world_center);

    if (dist < radius) {
        float falloff = 1.0 - (dist / radius);
        falloff = smoothstep(0.0, 1.0, falloff);

        float interaction_value = strength * falloff;

        float4 current = interaction_atlas[uint2(atlas_x, atlas_y)];

        current.r = max(current.r, interaction_value);

        interaction_atlas[uint2(atlas_x, atlas_y)] = current;
    }
}
