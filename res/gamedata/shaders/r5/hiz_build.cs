// hiz_build.cs - Hi-Z Pyramid Generation Compute Shader
// Downsamples depth buffer, keeping MAX depth per 2x2 block
// Creates a conservative depth pyramid for GPU occlusion culling
//
// Usage: Dispatch once per mip level, reading from previous mip
// Mip 0: Reads from full-res depth buffer
// Mip N: Reads from mip N-1 of Hi-Z pyramid
//
#define SM_6_0
#include "common.h"

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer HiZParams : register(b5)  // b5 to avoid conflicts with common.h
{
    uint2 g_output_dimensions;    // Output mip dimensions (width, height)
    uint g_input_mip_level;       // Which mip level to read from (0 = full res depth)
    uint g_is_first_mip;          // 1 if reading from depth buffer, 0 if reading from Hi-Z
};

// ═══════════════════════════════════════════════════════
//  RESOURCES
// ═══════════════════════════════════════════════════════

// Input: Full-res depth buffer OR previous Hi-Z mip
Texture2D<float> g_input_depth : register(t0);

// Output: Current Hi-Z mip level (R32_FLOAT UAV)
RWTexture2D<float> g_output_hiz : register(u0);


// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    // Bounds check
    if (dispatch_id.x >= g_output_dimensions.x || dispatch_id.y >= g_output_dimensions.y)
        return;

    // Calculate input texel coordinates (2x2 block)
    // Each output texel corresponds to a 2x2 block in the input
    uint2 input_coord = dispatch_id.xy * 2;

    // Sample 2x2 quad from input
    // Using Load() for exact texel access
    float d0 = g_input_depth.Load(int3(input_coord + uint2(0, 0), g_input_mip_level));
    float d1 = g_input_depth.Load(int3(input_coord + uint2(1, 0), g_input_mip_level));
    float d2 = g_input_depth.Load(int3(input_coord + uint2(0, 1), g_input_mip_level));
    float d3 = g_input_depth.Load(int3(input_coord + uint2(1, 1), g_input_mip_level));

    // ─────────────────────────────────────────────────────
    //  CONSERVATIVE DEPTH SELECTION
    // ─────────────────────────────────────────────────────
    // For standard Z-buffer (0=near, 1=far):
    //   - Take MAX depth (farthest surface in region)
    //   - Object is OCCLUDED if objectDepth > maxDepth (behind EVERYTHING in region)
    //   - Object is VISIBLE if objectDepth <= maxDepth (might be in front of something)
    //   - This is CONSERVATIVE: never incorrectly cull visible objects
    //
    float max_depth = max(max(d0, d1), max(d2, d3));

    // Write to output mip
    g_output_hiz[dispatch_id.xy] = max_depth;
}
