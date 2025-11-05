// compute_test_matrices.cs - Matrix operations compute shader test
// Tests various matrix operations: multiplication, transpose, transformations
//
#define SM_5_0
#include "common.h"

// Test structure - contains matrices and vectors
struct MatrixTestData
{
    float4x4 transform;        // 64 bytes (16 floats)
    float4 position;           // 16 bytes
    float4 result;             // 16 bytes
    uint flags;                // 4 bytes
    uint padding[3];           // 12 bytes (total: 112 bytes - structured buffer friendly)
};

// Input buffer
StructuredBuffer<MatrixTestData> g_input : register(t0);

// Output buffer
RWStructuredBuffer<MatrixTestData> g_output : register(u0);

// Counter
RWStructuredBuffer<uint> g_counter : register(u1);

// Constant buffer
cbuffer TestParams : register(b0)
{
    uint g_element_count;       // 4 bytes
    uint g_iteration_count;     // 4 bytes
    float g_rotation_angle;     // 4 bytes - angle for rotation matrices
    float g_scale_factor;       // 4 bytes - scale factor for transformations
};

// Helper: Create rotation matrix around Y axis
float4x4 CreateRotationY(float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    float4x4 result = float4x4(
        c,    0.0,  s,    0.0,
        0.0,  1.0,  0.0,  0.0,
        -s,   0.0,  c,    0.0,
        0.0,  0.0,  0.0,  1.0
    );

    return result;
}

// Helper: Create scale matrix
float4x4 CreateScale(float3 scale)
{
    return float4x4(
        scale.x, 0.0,     0.0,     0.0,
        0.0,     scale.y, 0.0,     0.0,
        0.0,     0.0,     scale.z, 0.0,
        0.0,     0.0,     0.0,     1.0
    );
}

// Helper: Create translation matrix
float4x4 CreateTranslation(float3 translation)
{
    return float4x4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        translation.x, translation.y, translation.z, 1.0
    );
}

// Helper: Transpose matrix
float4x4 Transpose(float4x4 m)
{
    return float4x4(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]
    );
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

    if (idx >= g_element_count)
        return;

    // Read input
    MatrixTestData input_data = g_input[idx];
    MatrixTestData output_data = input_data;

    // Test various matrix operations over multiple iterations
    float4x4 result_matrix = input_data.transform;
    float4 result_vec = input_data.position;

    // Thread-specific offset to prevent convergence
    float thread_offset = float(idx) * 0.0001;

    for (uint i = 0; i < g_iteration_count; ++i)
    {
        // Build transform with thread-specific variation (prevents convergence!)
        float angle = g_rotation_angle * float(i + 1) * 0.001 + thread_offset;
        float4x4 rotation = CreateRotationY(angle);

        // Thread-specific oscillating scale (0.95 to 1.05)
        float scale = 1.0 + sin(float(i) * 0.05 + thread_offset * 10.0) * 0.05;
        float4x4 scale_mat = CreateScale(float3(scale, scale, scale));

        // Thread-specific translation
        float3 translation = float3(
            sin(angle + thread_offset) * 0.001,
            cos(angle + thread_offset) * 0.001,
            thread_offset * 0.0001
        );
        float4x4 translation_mat = CreateTranslation(translation);

        // Build complete transform from scratch (T * R * S)
        float4x4 transform = mul(scale_mat, rotation);
        transform = mul(transform, translation_mat);

        // Transform vector
        result_vec = mul(result_vec, transform);

        // Clamp vector every iteration to prevent overflow
        result_vec.xyz = clamp(result_vec.xyz, -100000.0, 100000.0);
        result_vec.w = 1.0;

        // Re-inject thread variation periodically to fight convergence
        if ((i % 100) == 0)
        {
            result_vec.xyz += float3(
                thread_offset * sin(float(i) * 0.01),
                thread_offset * cos(float(i) * 0.01),
                thread_offset * sin(float(i) * 0.02)
            );
        }

        // Update result matrix occasionally for variety
        if ((i % 10) == 0)
        {
            result_matrix = mul(result_matrix, transform);

            // Clamp matrix translation to prevent accumulation
            result_matrix[3][0] = clamp(result_matrix[3][0], -1000.0, 1000.0);
            result_matrix[3][1] = clamp(result_matrix[3][1], -1000.0, 1000.0);
            result_matrix[3][2] = clamp(result_matrix[3][2], -1000.0, 1000.0);
        }

        // Apply transpose occasionally (thread-specific timing)
        if ((i % (17 + idx % 7)) == 0)
        {
            result_matrix = Transpose(result_matrix);
        }

        // Extract and manipulate matrix elements
        float det = result_matrix[0][0] * result_matrix[1][1] -
                   result_matrix[0][1] * result_matrix[1][0];

        // Use determinant to affect result (clamped, thread-specific)
        result_vec.xyz *= (1.0 + clamp(abs(det), 0.0, 0.1) * 0.0001 * (1.0 + thread_offset));
    }

    // Store results
    output_data.transform = result_matrix;
    output_data.result = result_vec;
    output_data.flags = input_data.flags + g_iteration_count; // Deterministic update

    // Write output
    g_output[idx] = output_data;

    // Increment counter atomically
    InterlockedAdd(g_counter[0], 1);
}
