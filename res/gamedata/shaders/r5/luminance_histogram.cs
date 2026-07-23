// luminance_histogram.cs - Compute scene luminance histogram for auto-exposure
// Uses 64 bins in log2 luminance space
//
#define SM_5_0
#include "common.h"

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer ExposureParams : register(b5)  // b5 to avoid conflicts with common.h (b0-b2)
{
    float g_min_log_luminance;   // Minimum log2 luminance (e.g., -10)
    float g_log_luminance_range; // Range of log2 luminance (e.g., 14)
    uint g_screen_width;
    uint g_screen_height;
};

// ═══════════════════════════════════════════════════════
//  RESOURCES
// ═══════════════════════════════════════════════════════

Texture2D<float4> g_scene_color : register(t0);  // HDR scene color
RWStructuredBuffer<uint> g_histogram : register(u0);  // 64 bins

// Group shared memory for local histogram accumulation
groupshared uint gs_histogram[64];

// ═══════════════════════════════════════════════════════
//  LUMINANCE CALCULATION
// ═══════════════════════════════════════════════════════

float ComputeLuminance(float3 color)
{
    // Classic CoP LUMINANCE_VECTOR (common_defines.h) — not BT.709
    return dot(color, float3(0.3, 0.38, 0.22));
}

uint ComputeBinIndex(float luminance)
{
    // Avoid log2(0) by clamping to small positive value
    float lum = max(luminance, 0.00001);

    // Convert to log2 space
    float logLum = log2(lum);

    // Map to 0-1 range based on min/max log luminance
    float normalized = saturate((logLum - g_min_log_luminance) / g_log_luminance_range);

    // Map to bin index (0-63)
    return (uint)(normalized * 63.0);
}

// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    // Initialize shared histogram (first 64 threads)
    if (group_index < 64)
    {
        gs_histogram[group_index] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // Check if this thread is within screen bounds
    bool inBounds = (dispatch_id.x < g_screen_width) && (dispatch_id.y < g_screen_height);

    // Only process pixels within bounds (no early return to avoid sync issues)
    if (inBounds)
    {
        // Sample scene color
        float4 color = g_scene_color.Load(int3(dispatch_id.xy, 0));

        // Compute luminance
        float luminance = ComputeLuminance(color.rgb);

        // Include dark pixels too (classic averages the whole bloom buffer;
        // skipping them made night meter sky-only and blew exposure).
        if (luminance >= 0.0)
        {
            uint binIndex = ComputeBinIndex(max(luminance, 0.00001));
            InterlockedAdd(gs_histogram[binIndex], 1);
        }
    }

    // Wait for all threads to finish - ALL threads must hit this barrier
    GroupMemoryBarrierWithGroupSync();

    // Merge local histogram to global (first 64 threads)
    if (group_index < 64)
    {
        uint localCount = gs_histogram[group_index];
        if (localCount > 0)
        {
            InterlockedAdd(g_histogram[group_index], localCount);
        }
    }
}
