#pragma once

namespace xray::render::RENDER_NAMESPACE
{

class ComputeTest_SIMD
{
public:
    struct SimdTestData
    {
        Fvector4 vec0;
        Fvector4 vec1;
        Fvector4 vec2;
        Fvector4 vec3;
        Fvector4 result0;
        Fvector4 result1;
        u32 checksum;
        u32 padding[3];
    };

    struct TestParams
    {
        u32 element_count;
        u32 iteration_count;
        float blend_factor;
        float padding;
    };

    static bool RunTest(u32 iteration_count = 10);

private:
    static bool CreateTestBuffers(u32 element_count);
    static bool RunComputeShader(u32 iteration_count);
    static bool ValidateResults();
    static void DestroyTestBuffers();

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

    static ID3D11Query* s_timestamp_disjoint;
    static ID3D11Query* s_timestamp_start;
    static ID3D11Query* s_timestamp_end;
};

}
