// dx11ComputeTest_SIMD.h - SIMD operations compute shader test harness
//
#pragma once

namespace xray::render::RENDER_NAMESPACE
{

class ComputeTest_SIMD
{
public:
    struct SimdTestData
    {
        Fvector4 vec0;             // 16 bytes
        Fvector4 vec1;             // 16 bytes
        Fvector4 vec2;             // 16 bytes
        Fvector4 vec3;             // 16 bytes
        Fvector4 result0;          // 16 bytes
        Fvector4 result1;          // 16 bytes
        u32 checksum;              // 4 bytes
        u32 padding[3];            // 12 bytes (total: 112 bytes)
    };

    struct TestParams
    {
        u32 element_count;         // 4 bytes
        u32 iteration_count;       // 4 bytes
        float blend_factor;        // 4 bytes - for lerp operations
        float padding;             // 4 bytes
    };

    // Run the test and return true if successful
    // iteration_count: number of computation cycles to run in the shader (default: 10)
    static bool RunTest(u32 iteration_count = 10);

private:
    static bool CreateTestBuffers(u32 element_count);
    static bool RunComputeShader(u32 iteration_count);
    static bool ValidateResults();
    static void DestroyTestBuffers();

    // GPU resources
    static ID3DBuffer* s_input_buffer;
    static ID3DBuffer* s_output_buffer;
    static ID3DBuffer* s_counter_buffer;
    static ID3DBuffer* s_params_cb;
    static ID3DBuffer* s_readback_buffer;

    static ID3DShaderResourceView* s_input_srv;
    static ID3DUnorderedAccessView* s_output_uav;
    static ID3DUnorderedAccessView* s_counter_uav;

    static ref_cs s_compute_shader;

    static xr_vector<SimdTestData> s_input_data;
    static u32 s_element_count;
    static u32 s_iteration_count;

    // GPU timing
    static ID3D11Query* s_timestamp_disjoint;
    static ID3D11Query* s_timestamp_start;
    static ID3D11Query* s_timestamp_end;
};

} // namespace xray::render::RENDER_NAMESPACE
