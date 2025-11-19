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
u32 ComputeTest::s_iteration_count = 0;

// GPU timing
ID3D11Query* ComputeTest::s_timestamp_disjoint = nullptr;
ID3D11Query* ComputeTest::s_timestamp_start = nullptr;
ID3D11Query* ComputeTest::s_timestamp_end = nullptr;

bool ComputeTest::RunTest(u32 iteration_count)
{
    Msg("=== [ComputeTest] Starting compute shader pipeline test ===");
    Msg("* [ComputeTest] Iteration count: %d cycles per thread", iteration_count);

    // Use larger dataset to really show GPU parallel processing power
    constexpr u32 test_count = 65536; // 64K elements
    s_element_count = test_count;
    s_iteration_count = iteration_count;

    // Create buffers and upload data
    if (!CreateTestBuffers(s_element_count))
    {
        Msg("! [ComputeTest] FAILED: Buffer creation failed");
        DestroyTestBuffers();
        return false;
    }

    // Run compute shader
    if (!RunComputeShader(iteration_count))
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

    // Show sample input data
    Msg("* [ComputeTest] Sample input data:");
    for (u32 i : {0u, 1u, 2u, element_count-1})
    {
        if (i < element_count)
        {
            Msg("    [%4d] value=%u, x=%.2f, y=%.2f, z=%.2f",
                i, s_input_data[i].value, s_input_data[i].x, s_input_data[i].y, s_input_data[i].z);
        }
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

    // ===========================
    // GPU Timing Queries
    // ===========================
    {
        D3D11_QUERY_DESC query_desc = {};

        // Disjoint query (for frequency and validity)
        query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        CHK_DX(HW.pDevice->CreateQuery(&query_desc, &s_timestamp_disjoint));

        // Timestamp queries
        query_desc.Query = D3D11_QUERY_TIMESTAMP;
        CHK_DX(HW.pDevice->CreateQuery(&query_desc, &s_timestamp_start));
        CHK_DX(HW.pDevice->CreateQuery(&query_desc, &s_timestamp_end));
    }

    Msg("* [ComputeTest] Buffers and timing queries created successfully");
    return true;
}

bool ComputeTest::RunComputeShader(u32 iteration_count)
{
    Msg("* [ComputeTest] Running compute shader...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Load/compile compute shader
    s_compute_shader.create("compute_test_vector");
    if (!s_compute_shader)
    {
        Msg("! [ComputeTest] Failed to create compute shader");
        return false;
    }

    // Update constant buffer
    TestParams params;
    params.element_count = s_element_count;
    params.iteration_count = iteration_count;
    params.multiplier = 3.0f;
    params.padding = 0.0f;

    Msg("* [ComputeTest] Constant buffer parameters:");
    Msg("    element_count    = %u", params.element_count);
    Msg("    iteration_count  = %u", params.iteration_count);
    Msg("    multiplier       = %.2f", params.multiplier);
    Msg("    sizeof(TestParams) = %u bytes", sizeof(TestParams));

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_params_cb, 0, D3D_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(TestParams));
    context->Unmap(s_params_cb, 0);

    // Start GPU timing
    context->Begin(s_timestamp_disjoint);
    context->End(s_timestamp_start);

    // Bind resources
    // TODO: Port to NVRHI compute dispatch
    // context->CSSetShader(s_compute_shader->sh, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &s_params_cb);
    context->CSSetShaderResources(0, 1, &s_input_srv);

    ID3DUnorderedAccessView* uavs[2] = { s_output_uav, s_counter_uav };
    context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    // Dispatch
    const u32 threads_per_group = 256;
    const u32 num_groups = (s_element_count + threads_per_group - 1) / threads_per_group;

    Msg("* [ComputeTest] Dispatching %d groups (%d threads per group)", num_groups, threads_per_group);

    // Dispatch once - shader will run iteration_count cycles internally
    context->Dispatch(num_groups, 1, 1);

    // End GPU timing
    context->End(s_timestamp_end);
    context->End(s_timestamp_disjoint);

    // Unbind resources
    ID3DShaderResourceView* null_srv[1] = { nullptr };
    ID3DUnorderedAccessView* null_uav[2] = { nullptr, nullptr };
    context->CSSetShaderResources(0, 1, null_srv);
    context->CSSetUnorderedAccessViews(0, 2, null_uav, nullptr);

    Msg("* [ComputeTest] Compute shader dispatched (1 dispatch with %u cycles per thread)", iteration_count);
    return true;
}

bool ComputeTest::ValidateResults()
{
    Msg("* [ComputeTest] Validating results...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // ===========================
    // Read GPU Timing
    // ===========================
    float gpu_time_ms = 0.0f;
    {
        // Wait for queries to complete
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
        while (context->GetData(s_timestamp_disjoint, &disjoint_data, sizeof(disjoint_data), 0) == S_FALSE)
        {
            // Wait...
        }

        if (!disjoint_data.Disjoint)
        {
            UINT64 start_time, end_time;
            context->GetData(s_timestamp_start, &start_time, sizeof(start_time), 0);
            context->GetData(s_timestamp_end, &end_time, sizeof(end_time), 0);

            UINT64 delta = end_time - start_time;
            gpu_time_ms = (delta * 1000.0f) / disjoint_data.Frequency;

            Msg("=== [ComputeTest] GPU PERFORMANCE ===");
            Msg("    GPU time:        %.3f ms", gpu_time_ms);
            Msg("    Elements:        %d", s_element_count);
            Msg("    Iterations:      %d cycles per element", s_iteration_count);
            Msg("    Total ops:       %llu", (u64)s_element_count * s_iteration_count);
            Msg("    Throughput:      %.2f million ops/sec", ((u64)s_element_count * s_iteration_count) / (gpu_time_ms * 1000.0f));
        }
        else
        {
            Msg("! [ComputeTest] GPU timing disjoint - measurements invalid");
        }
    }

    // Copy output buffer to readback buffer
    context->CopyResource(s_readback_buffer, s_output_buffer);

    // Map and read results
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped));

    TestData* output_data = static_cast<TestData*>(mapped.pData);

    // Validate that shader actually ran and modified data
    // Note: With complex math, we can't predict exact values,
    // but we can verify the data changed and looks reasonable
    bool all_correct = true;
    u32 changed_count = 0;
    u32 value_correct_count = 0;

    Msg("* [ComputeTest] Validating %d elements...", s_element_count);

    for (u32 i = 0; i < s_element_count; ++i)
    {
        // Check value field (this is deterministic)
        u32 expected_value = s_input_data[i].value * 2 + i;
        bool value_correct = (output_data[i].value == expected_value);
        if (value_correct)
            value_correct_count++;

        // Check that vector was modified (different from input)
        // Due to complex math operations, we can't predict exact output values,
        // but we can verify the shader processed the data
        bool vec_changed = (fabs(output_data[i].x - s_input_data[i].x) > 0.0001f ||
                           fabs(output_data[i].y - s_input_data[i].y) > 0.0001f ||
                           fabs(output_data[i].z - s_input_data[i].z) > 0.0001f);

        // Check that output is not NaN or Inf (indicates shader error)
        bool vec_valid = !isnan(output_data[i].x) && !isinf(output_data[i].x) &&
                         !isnan(output_data[i].y) && !isinf(output_data[i].y) &&
                         !isnan(output_data[i].z) && !isinf(output_data[i].z);

        if (vec_changed && vec_valid)
            changed_count++;

        if (!value_correct || !vec_changed || !vec_valid)
        {
            if (all_correct) // Log first few errors only
            {
                Msg("! [ComputeTest] Element %d suspicious:", i);
                Msg("    value: %u (expected %u) %s", output_data[i].value, expected_value,
                    value_correct ? "OK" : "FAIL");
                Msg("    vec: (%.4f, %.4f, %.4f) %s %s",
                    output_data[i].x, output_data[i].y, output_data[i].z,
                    vec_changed ? "CHANGED" : "UNCHANGED",
                    vec_valid ? "VALID" : "INVALID(NaN/Inf)");
                Msg("    input was: (%.4f, %.4f, %.4f)",
                    s_input_data[i].x, s_input_data[i].y, s_input_data[i].z);
            }
            all_correct = false;
        }
    }

    context->Unmap(s_readback_buffer, 0);

    // Summary
    Msg("* [ComputeTest] Validation Summary:");
    Msg("    Total elements:   %d", s_element_count);
    Msg("    Value correct:    %d (%.1f%%)", value_correct_count, 100.0f * value_correct_count / s_element_count);
    Msg("    Vector processed: %d (%.1f%%)", changed_count, 100.0f * changed_count / s_element_count);

    // Check a few specific elements for detailed output
    Msg("* [ComputeTest] Sample outputs:");
    u32 samples[] = {0, 1, 2, s_element_count/4, s_element_count/2, s_element_count-3, s_element_count-2, s_element_count-1};
    context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped);
    output_data = static_cast<TestData*>(mapped.pData);
    for (u32 idx : samples)
    {
        if (idx < s_element_count)
        {
            Msg("    [%4d] in=(val=%u x=%.2f y=%.2f z=%.2f) -> out=(val=%u x=%.2f y=%.2f z=%.2f)",
                idx,
                s_input_data[idx].value, s_input_data[idx].x, s_input_data[idx].y, s_input_data[idx].z,
                output_data[idx].value, output_data[idx].x, output_data[idx].y, output_data[idx].z);
        }
    }
    context->Unmap(s_readback_buffer, 0);

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

        // Counter is incremented once per element (regardless of iteration count)
        u32 expected_counter = s_element_count;
        Msg("* [ComputeTest] Atomic counter: %d (expected: %d) %s",
            counter, expected_counter,
            (counter == expected_counter) ? "PASS" : "FAIL");

        if (counter != expected_counter)
        {
            Msg("! [ComputeTest] Counter mismatch - some threads may not have executed!");
            all_correct = false;
        }
    }

    if (all_correct)
    {
        Msg("=== [ComputeTest] VALIDATION PASSED: All computations correct! ===");
    }
    else
    {
        u32 error_count = s_element_count - changed_count;
        Msg("=== [ComputeTest] VALIDATION FAILED: %d/%d elements had issues ===",
            error_count, s_element_count);
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

    _RELEASE(s_timestamp_disjoint);
    _RELEASE(s_timestamp_start);
    _RELEASE(s_timestamp_end);

    s_input_data.clear();
    s_element_count = 0;

    Msg("* [ComputeTest] Buffers and queries destroyed");
}

} // namespace xray::render::RENDER_NAMESPACE
