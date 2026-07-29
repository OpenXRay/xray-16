#define SM_6_0
#include "common.h"

cbuffer HiZParams : register(b5)
{
    uint2 g_output_dimensions;
    uint g_input_mip_level;
    uint g_is_first_mip;
    uint2 g_input_dimensions;
    uint2 g_hiz_pad;
};

Texture2D<float> g_input_depth : register(t0);

RWTexture2D<float> g_output_hiz : register(u0);


[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    if (dispatch_id.x >= g_output_dimensions.x || dispatch_id.y >= g_output_dimensions.y)
        return;

    uint2 input_coord = dispatch_id.xy * 2;

    float d0 = g_input_depth.Load(int3(input_coord + uint2(0, 0), g_input_mip_level));
    float d1 = g_input_depth.Load(int3(input_coord + uint2(1, 0), g_input_mip_level));
    float d2 = g_input_depth.Load(int3(input_coord + uint2(0, 1), g_input_mip_level));
    float d3 = g_input_depth.Load(int3(input_coord + uint2(1, 1), g_input_mip_level));

    float min_depth = min(min(d0, d1), min(d2, d3));

    bool fold_x = (g_input_dimensions.x == 2 * g_output_dimensions.x + 1) && (dispatch_id.x == g_output_dimensions.x - 1);
    bool fold_y = (g_input_dimensions.y == 2 * g_output_dimensions.y + 1) && (dispatch_id.y == g_output_dimensions.y - 1);
    if (fold_x)
    {
        min_depth = min(min_depth, g_input_depth.Load(int3(input_coord + uint2(2, 0), g_input_mip_level)));
        min_depth = min(min_depth, g_input_depth.Load(int3(input_coord + uint2(2, 1), g_input_mip_level)));
    }
    if (fold_y)
    {
        min_depth = min(min_depth, g_input_depth.Load(int3(input_coord + uint2(0, 2), g_input_mip_level)));
        min_depth = min(min_depth, g_input_depth.Load(int3(input_coord + uint2(1, 2), g_input_mip_level)));
    }
    if (fold_x && fold_y)
        min_depth = min(min_depth, g_input_depth.Load(int3(input_coord + uint2(2, 2), g_input_mip_level)));

    g_output_hiz[dispatch_id.xy] = min_depth;
}
