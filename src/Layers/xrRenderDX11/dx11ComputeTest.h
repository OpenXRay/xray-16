#pragma once

namespace xray::render::RENDER_NAMESPACE
{

class ComputeTest
{
public:
    struct TestData
    {
        u32 value;
        float x;
        float y;
        float z;
    };

    struct TestParams
    {
        u32 element_count;
        u32 iteration_count;
        float multiplier;
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

    static xr_vector<TestData> s_input_data;
    static u32 s_element_count;
    static u32 s_iteration_count;

    static ID3D11Query* s_timestamp_disjoint;
    static ID3D11Query* s_timestamp_start;
    static ID3D11Query* s_timestamp_end;
};

}
