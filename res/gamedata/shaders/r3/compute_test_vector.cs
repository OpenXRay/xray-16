// compute_test.cs - Simple compute shader test
// Reads input buffer and writes to output buffer to validate compute pipeline
//
#define SM_5_0
#include "common.h"

// Test structure
struct TestData
{
    uint value;
    float x;
    float y;
    float z;
};

// Input buffer
StructuredBuffer<TestData> g_input : register(t0);

// Output buffer
RWStructuredBuffer<TestData> g_output : register(u0);

// Counter
RWStructuredBuffer<uint> g_counter : register(u1);

// Constant buffer
// Note: Must be multiple of 16 bytes for DX11
cbuffer TestParams : register(b0)
{
    uint g_element_count;    // 4 bytes
    uint g_iteration_count;  // 4 bytes - number of computation cycles to run
    float g_multiplier;      // 4 bytes
    float g_padding;         // 4 bytes (total: 16 bytes)
};

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

    if (idx >= g_element_count)
        return;

    // Force padding to be included in the constant buffer (compiler won't optimize it away)
    // This is a common trick to ensure proper constant buffer alignment
    if (g_padding < -1e30) return;  // Never true, but compiler doesn't know that

    // Read input
    TestData input_data = g_input[idx];

    // Process: Do some actual GPU work
    TestData output_data;
    output_data.value = input_data.value * 2 + idx;

    // Simulate complex computation - N iterations of heavy math
    float3 vec = float3(input_data.x, input_data.y, input_data.z);

    // Run g_iteration_count cycles of complex math operations
    // Note: Can't unroll dynamic loop count, but that's fine - we want to test actual iteration overhead
    for (uint i = 0; i < g_iteration_count; ++i)
    {
        // Complex vector operations
        vec = normalize(vec + float3(0.01, 0.02, 0.03));
        vec *= g_multiplier;
        vec = abs(sin(vec * 3.14159265f));
        vec = sqrt(vec + 0.001f);

        // Mix in some data dependency
        float len = length(vec);
        vec = vec / max(len, 0.001f);
        vec = vec * vec; // square each component
        vec = vec + float3(input_data.x, input_data.y, input_data.z) * 0.001f;
    }

    output_data.x = vec.x;
    output_data.y = vec.y;
    output_data.z = vec.z;

    // Write output
    g_output[idx] = output_data;

    // Increment counter atomically (once per element, regardless of iteration count)
    InterlockedAdd(g_counter[0], 1);
}
