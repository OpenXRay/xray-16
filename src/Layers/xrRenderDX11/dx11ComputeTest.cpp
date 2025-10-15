// dx11ComputeTest.cpp - Implementation of compute shader test harness
//
#include "stdafx.h"
#include "dx11ComputeTest.h"
#include "dx11HW.h"

namespace xray::render::RENDER_NAMESPACE
{

// Static members
ID3DBuffer* ComputeTest::s_input_buffer = nullptr;
ID3DBuffer* ComputeTest::s_output_buffer = nullptr;
ID3DBuffer* ComputeTest::s_counter_buffer = nullptr;
ID3DBuffer* ComputeTest::s_params_cb = nullptr;
ID3DBuffer* ComputeTest::s_readback_buffer = nullptr;
ID3DShaderResourceView* ComputeTest::s_input_srv = nullptr;
ID3DUnorderedAccessView* ComputeTest::s_output_uav = nullptr;
ID3DUnorderedAccessView* ComputeTest::s_counter_uav = nullptr;
ref_cs ComputeTest::s_compute_shader;
xr_vector<ComputeTest::TestData> ComputeTest::s_input_data;
u32 ComputeTest::s_element_count = 0;

bool ComputeTest::RunTest()
{
    Msg("=== [ComputeTest] Starting compute shader pipeline test ===");

    const u32 test_count = 1024;
    s_element_count = test_count;

    // Create buffers and upload data
    if (!CreateTestBuffers(test_count))
    {
        Msg("! [ComputeTest] FAILED: Buffer creation failed");
        DestroyTestBuffers();
        return false;
    }

    // Run compute shader
    if (!RunComputeShader())
    {
        Msg("! [ComputeTest] FAILED: Compute shader execution failed");
        DestroyTestBuffers();
        return false;
    }

    // Validate results
    if (!ValidateResults())
    {
        Msg("! [ComputeTest] FAILED: Result validation failed");
        DestroyTestBuffers();
        return false;
    }

    // Cleanup
    DestroyTestBuffers();

    Msg("=== [ComputeTest] PASSED: All tests successful! ===");
    return true;
}

bool ComputeTest::CreateTestBuffers(u32 element_count)
{
    Msg("* [ComputeTest] Creating test buffers for %d elements...", element_count);

    const u32 buffer_size = element_count * sizeof(TestData);

    // Generate test data
    s_input_data.resize(element_count);
    for (u32 i = 0; i < element_count; ++i)
    {
        s_input_data[i].value = i * 10;
        s_input_data[i].x = static_cast<float>(i) * 0.5f;
        s_input_data[i].y = static_cast<float>(i) * 1.5f;
        s_input_data[i].z = static_cast<float>(i) * 2.5f;
    }

    // ===========================
    // Input Buffer (SRV)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(TestData);

        D3D_SUBRESOURCE_DATA init_data = {};
        init_data.pSysMem = s_input_data.data();

        CHK_DX(HW.pDevice->CreateBuffer(&desc, &init_data, &s_input_buffer));

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = element_count;

        CHK_DX(HW.pDevice->CreateShaderResourceView(s_input_buffer, &srv_desc, &s_input_srv));
    }

    // ===========================
    // Output Buffer (UAV)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(TestData);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &s_output_buffer));

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = element_count;
        uav_desc.Buffer.Flags = 0;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(s_output_buffer, &uav_desc, &s_output_uav));
    }

    // ===========================
    // Counter Buffer (UAV)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(u32);
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(u32);

        // Initialize with zero
        u32 zero = 0;
        D3D_SUBRESOURCE_DATA init_data = {};
        init_data.pSysMem = &zero;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, &init_data, &s_counter_buffer));

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = 1;
        uav_desc.Buffer.Flags = 0;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(s_counter_buffer, &uav_desc, &s_counter_uav));
    }

    // ===========================
    // Constant Buffer
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(TestParams);
        desc.Usage = D3D_USAGE_DYNAMIC;
        desc.BindFlags = D3D_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &s_params_cb));
    }

    // ===========================
    // Readback Buffer (for CPU readback)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &s_readback_buffer));
    }

    Msg("* [ComputeTest] Buffers created successfully");
    return true;
}

bool ComputeTest::RunComputeShader()
{
    Msg("* [ComputeTest] Running compute shader...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Load/compile compute shader
    s_compute_shader.create("compute_test");
    if (!s_compute_shader)
    {
        Msg("! [ComputeTest] Failed to create compute shader");
        return false;
    }

    // Update constant buffer
    TestParams params;
    params.element_count = s_element_count;
    params.multiplier = 3.0f;
    params.pad0 = 0.0f;
    params.pad1 = 0.0f;

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_params_cb, 0, D3D_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(TestParams));
    context->Unmap(s_params_cb, 0);

    // Bind resources
    context->CSSetShader(s_compute_shader->sh, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &s_params_cb);
    context->CSSetShaderResources(0, 1, &s_input_srv);

    ID3DUnorderedAccessView* uavs[2] = { s_output_uav, s_counter_uav };
    context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    // Dispatch
    const u32 threads_per_group = 256;
    const u32 num_groups = (s_element_count + threads_per_group - 1) / threads_per_group;

    Msg("* [ComputeTest] Dispatching %d groups (%d threads per group)", num_groups, threads_per_group);
    context->Dispatch(num_groups, 1, 1);

    // Unbind resources
    ID3DShaderResourceView* null_srv[1] = { nullptr };
    ID3DUnorderedAccessView* null_uav[2] = { nullptr, nullptr };
    context->CSSetShaderResources(0, 1, null_srv);
    context->CSSetUnorderedAccessViews(0, 2, null_uav, nullptr);

    Msg("* [ComputeTest] Compute shader dispatched");
    return true;
}

bool ComputeTest::ValidateResults()
{
    Msg("* [ComputeTest] Validating results...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Copy output buffer to readback buffer
    context->CopyResource(s_readback_buffer, s_output_buffer);

    // Map and read results
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped));

    TestData* output_data = static_cast<TestData*>(mapped.pData);

    // Validate first few elements
    bool all_correct = true;
    const u32 samples_to_check = std::min(10u, s_element_count);

    for (u32 i = 0; i < samples_to_check; ++i)
    {
        u32 expected_value = s_input_data[i].value * 2 + i;
        float expected_x = s_input_data[i].x * 3.0f;
        float expected_y = s_input_data[i].y * 3.0f;
        float expected_z = s_input_data[i].z * 3.0f;

        bool value_match = (output_data[i].value == expected_value);
        bool x_match = fabs(output_data[i].x - expected_x) < 0.001f;
        bool y_match = fabs(output_data[i].y - expected_y) < 0.001f;
        bool z_match = fabs(output_data[i].z - expected_z) < 0.001f;

        if (!value_match || !x_match || !y_match || !z_match)
        {
            Msg("! [ComputeTest] Element %d mismatch:", i);
            Msg("    Expected: value=%u, x=%.2f, y=%.2f, z=%.2f",
                expected_value, expected_x, expected_y, expected_z);
            Msg("    Got:      value=%u, x=%.2f, y=%.2f, z=%.2f",
                output_data[i].value, output_data[i].x, output_data[i].y, output_data[i].z);
            all_correct = false;
        }
    }

    context->Unmap(s_readback_buffer, 0);

    if (all_correct)
    {
        Msg("* [ComputeTest] All %d sample elements validated successfully!", samples_to_check);
    }

    // Read counter
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(u32);
        desc.Usage = D3D_USAGE_STAGING;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;

        ID3DBuffer* counter_readback = nullptr;
        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &counter_readback));
        context->CopyResource(counter_readback, s_counter_buffer);

        CHK_DX(context->Map(counter_readback, 0, D3D_MAP_READ, 0, &mapped));
        u32 counter = *static_cast<u32*>(mapped.pData);
        context->Unmap(counter_readback, 0);
        _RELEASE(counter_readback);

        Msg("* [ComputeTest] Counter value: %d (expected: %d)", counter, s_element_count);

        if (counter != s_element_count)
        {
            Msg("! [ComputeTest] Counter mismatch!");
            all_correct = false;
        }
    }

    return all_correct;
}

void ComputeTest::DestroyTestBuffers()
{
    _RELEASE(s_input_buffer);
    _RELEASE(s_output_buffer);
    _RELEASE(s_counter_buffer);
    _RELEASE(s_params_cb);
    _RELEASE(s_readback_buffer);
    _RELEASE(s_input_srv);
    _RELEASE(s_output_uav);
    _RELEASE(s_counter_uav);

    s_input_data.clear();
    s_element_count = 0;

    Msg("* [ComputeTest] Buffers destroyed");
}

} // namespace xray::render::RENDER_NAMESPACE
