#define SM_6_0
#include "common.h"

cbuffer ShadowHZBParams : register(b5)
{
    uint2 g_output_dimensions;
    uint g_input_mip_level;
    uint g_is_first_mip;
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
    g_output_hiz[dispatch_id.xy] = max(max(d0, d1), max(d2, d3));
}
