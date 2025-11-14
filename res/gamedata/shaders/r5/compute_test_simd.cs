// compute_test_simd.cs - SIMD operations compute shader test
// Tests packed vector operations: dot products, cross products, vector math
// These operations are optimized for SIMD execution on GPU
//
#define SM_5_0
#include "common.h"

// Test structure - multiple vectors for SIMD processing
struct SimdTestData
{
    float4 vec0;           // 16 bytes - packed vector 0
    float4 vec1;           // 16 bytes - packed vector 1
    float4 vec2;           // 16 bytes - packed vector 2
    float4 vec3;           // 16 bytes - packed vector 3
    float4 result0;        // 16 bytes - result vector 0
    float4 result1;        // 16 bytes - result vector 1
    uint checksum;         // 4 bytes
    uint padding[3];       // 12 bytes (total: 112 bytes)
};

// Input buffer
StructuredBuffer<SimdTestData> g_input : register(t0);

// Output buffer
RWStructuredBuffer<SimdTestData> g_output : register(u0);

// Counter
RWStructuredBuffer<uint> g_counter : register(u1);

// Constant buffer
cbuffer TestParams : register(b0)
{
    uint g_element_count;       // 4 bytes
    uint g_iteration_count;     // 4 bytes
    float g_blend_factor;       // 4 bytes - for lerp operations
    float g_padding;            // 4 bytes
};

// Helper: Fast vector operations using SIMD-friendly patterns
float4 ProcessVectorSIMD(float4 a, float4 b, float4 c, float factor)
{
    // Dot products (SIMD-friendly)
    float dot_ab = dot(a, b);
    float dot_bc = dot(b, c);
    float dot_ca = dot(c, a);

    // Cross products (3D only, but still SIMD)
    float3 cross_ab = cross(a.xyz, b.xyz);
    float3 cross_bc = cross(b.xyz, c.xyz);

    // Blend using dot products (clamped, not saturated)
    float blend_ab = clamp(abs(dot_ab) * 0.1, 0.0, 0.5);
    float blend_bc = clamp(abs(dot_bc) * 0.1, 0.0, 0.5);

    float4 result = lerp(a, b, blend_ab);
    result = lerp(result, c, blend_bc);

    // Mix in cross products
    result.xyz += (cross_ab + cross_bc) * 0.01;
    result.w = abs(dot_ca) * 0.1;

    // Scale (no saturate - causes convergence)
    result = result * factor;

    return result;
}

// Helper: Packed vector operations (4 operations in parallel)
void ProcessPackedVectors(inout float4 v0, inout float4 v1, inout float4 v2, inout float4 v3)
{
    // Element-wise operations (perfect for SIMD)
    float4 sum = v0 + v1 + v2 + v3;
    float4 product = v0 * v1 * v2 * v3;

    // Horizontal operations
    float sum_v0 = v0.x + v0.y + v0.z + v0.w;
    float sum_v1 = v1.x + v1.y + v1.z + v1.w;
    float sum_v2 = v2.x + v2.y + v2.z + v2.w;
    float sum_v3 = v3.x + v3.y + v3.z + v3.w;

    // Swizzle operations (SIMD shuffle)
    v0 = v0.yzwx * sum_v0 * 0.1;
    v1 = v1.zwxy * sum_v1 * 0.1;
    v2 = v2.wxyz * sum_v2 * 0.1;
    v3 = v3.xyzw * sum_v3 * 0.1;

    // Normalize all (parallel SIMD ops)
    v0 = normalize(v0 + float4(0.001, 0.002, 0.003, 0.004));
    v1 = normalize(v1 + float4(0.002, 0.003, 0.004, 0.001));
    v2 = normalize(v2 + float4(0.003, 0.004, 0.001, 0.002));
    v3 = normalize(v3 + float4(0.004, 0.001, 0.002, 0.003));

    // Min/max operations (SIMD)
    float4 min_vec = min(min(v0, v1), min(v2, v3));
    float4 max_vec = max(max(v0, v1), max(v2, v3));

    // Blend with min/max
    v0 = lerp(v0, min_vec, 0.1);
    v1 = lerp(v1, max_vec, 0.1);
    v2 = lerp(v2, min_vec, 0.2);
    v3 = lerp(v3, max_vec, 0.2);
}

// Helper: Compute checksum from vectors
uint ComputeChecksum(float4 v0, float4 v1, float4 v2, float4 v3)
{
    // Convert float data to checksum (for validation)
    uint check = 0;

    // Hash each component
    check ^= asuint(v0.x) ^ asuint(v0.y) ^ asuint(v0.z) ^ asuint(v0.w);
    check ^= asuint(v1.x) ^ asuint(v1.y) ^ asuint(v1.z) ^ asuint(v1.w);
    check ^= asuint(v2.x) ^ asuint(v2.y) ^ asuint(v2.z) ^ asuint(v2.w);
    check ^= asuint(v3.x) ^ asuint(v3.y) ^ asuint(v3.z) ^ asuint(v3.w);

    return check;
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

    if (idx >= g_element_count)
        return;

    // Force padding to be used
    if (g_padding < -1e30) return;

    // Read input
    SimdTestData input_data = g_input[idx];

    // Working vectors
    float4 v0 = input_data.vec0;
    float4 v1 = input_data.vec1;
    float4 v2 = input_data.vec2;
    float4 v3 = input_data.vec3;

    // Thread-specific offset to prevent convergence (defined once outside loop)
    float thread_offset = float(idx) * 0.0001;

    // Run multiple iterations of SIMD operations
    // IMPORTANT: Maintain input dependency to prevent convergence
    for (uint i = 0; i < g_iteration_count; ++i)
    {
        // Use iteration offset
        float iter_offset = float(i) * 0.01;

        // 1. Process packed vectors (4 vectors in parallel)
        ProcessPackedVectors(v0, v1, v2, v3);

        // 2. Apply SIMD-friendly operations with thread-specific factors
        float factor = 1.0 + iter_offset + thread_offset;
        v0 = ProcessVectorSIMD(v0, v1, v2, factor);
        v1 = ProcessVectorSIMD(v1, v2, v3, factor * 1.1);
        v2 = ProcessVectorSIMD(v2, v3, v0, factor * 1.2);
        v3 = ProcessVectorSIMD(v3, v0, v1, factor * 1.3);

        // 3. Parallel trigonometric operations (SIMD) with thread variation
        float4 angles = float4(
            (i + idx) * 0.001,
            (i + idx) * 0.002,
            (i + idx) * 0.003,
            (i + idx) * 0.004
        );
        float4 sin_vals = sin(angles);
        float4 cos_vals = cos(angles);

        // Mix with trig but don't let it dominate
        v0 = v0 * 0.9 + (v0 * cos_vals + v1 * sin_vals) * 0.1;
        v1 = v1 * 0.9 + (v1 * cos_vals - v2 * sin_vals) * 0.1;
        v2 = v2 * 0.9 + (v2 * sin_vals + v3 * cos_vals) * 0.1;
        v3 = v3 * 0.9 + (v3 * sin_vals - v0 * cos_vals) * 0.1;

        // 4. Blend operations with thread-specific blend factor (avoid convergence)
        float blend = g_blend_factor * (float(i % 10) * 0.05 + thread_offset);
        blend = clamp(blend, 0.01, 0.5); // Don't blend too much
        v0 = lerp(v0, v1, blend);
        v1 = lerp(v1, v2, blend * 0.9);
        v2 = lerp(v2, v3, blend * 0.8);
        v3 = lerp(v3, v0, blend * 0.7);

        // 5. Distance and length operations
        float d01 = distance(v0, v1);
        float d12 = distance(v1, v2);
        float d23 = distance(v2, v3);
        float d30 = distance(v3, v0);

        // Use distances to modulate results (scaled down to prevent overflow)
        v0 *= (1.0 + d01 * 0.01);
        v1 *= (1.0 + d12 * 0.01);
        v2 *= (1.0 + d23 * 0.01);
        v3 *= (1.0 + d30 * 0.01);

        // Scale back if growing too large (but don't normalize - that causes convergence)
        float len0 = length(v0);
        float len1 = length(v1);
        float len2 = length(v2);
        float len3 = length(v3);

        if (len0 > 10.0) v0 *= 0.5;
        if (len1 > 10.0) v1 *= 0.5;
        if (len2 > 10.0) v2 *= 0.5;
        if (len3 > 10.0) v3 *= 0.5;

        // Add back thread-specific variation every iteration
        v0 += float4(thread_offset, thread_offset * 2, thread_offset * 3, thread_offset * 4) * 0.01;
        v1 += float4(thread_offset * 2, thread_offset * 3, thread_offset * 4, thread_offset) * 0.01;
        v2 += float4(thread_offset * 3, thread_offset * 4, thread_offset, thread_offset * 2) * 0.01;
        v3 += float4(thread_offset * 4, thread_offset, thread_offset * 2, thread_offset * 3) * 0.01;
    }

    // Store results
    SimdTestData output_data;
    output_data.vec0 = v0;
    output_data.vec1 = v1;
    output_data.vec2 = v2;
    output_data.vec3 = v3;

    // Compute result vectors (additional operations)
    output_data.result0 = (v0 + v1) * 0.5;
    output_data.result1 = (v2 + v3) * 0.5;

    // Compute checksum for validation
    output_data.checksum = ComputeChecksum(v0, v1, v2, v3);

    // Clear padding
    output_data.padding[0] = 0;
    output_data.padding[1] = 0;
    output_data.padding[2] = 0;

    // Write output
    g_output[idx] = output_data;

    // Increment counter atomically
    InterlockedAdd(g_counter[0], 1);
}
