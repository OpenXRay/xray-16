// dx11ComputeTest_Matrices.cpp - Implementation of matrix compute shader test harness
//
#include "stdafx.h"
#include "dx11ComputeTest_Matrices.h"
#include "dx11HW.h"

namespace xray::render::RENDER_NAMESPACE
{

// Static members
ID3DBuffer* ComputeTest_Matrices::s_input_buffer = nullptr;
ID3DBuffer* ComputeTest_Matrices::s_output_buffer = nullptr;
ID3DBuffer* ComputeTest_Matrices::s_counter_buffer = nullptr;
ID3DBuffer* ComputeTest_Matrices::s_params_cb = nullptr;
ID3DBuffer* ComputeTest_Matrices::s_readback_buffer = nullptr;
ID3DShaderResourceView* ComputeTest_Matrices::s_input_srv = nullptr;
ID3DUnorderedAccessView* ComputeTest_Matrices::s_output_uav = nullptr;
ID3DUnorderedAccessView* ComputeTest_Matrices::s_counter_uav = nullptr;
ref_cs ComputeTest_Matrices::s_compute_shader;
xr_vector<ComputeTest_Matrices::MatrixTestData> ComputeTest_Matrices::s_input_data;
u32 ComputeTest_Matrices::s_element_count = 0;
u32 ComputeTest_Matrices::s_iteration_count = 0;

// GPU timing
ID3D11Query* ComputeTest_Matrices::s_timestamp_disjoint = nullptr;
ID3D11Query* ComputeTest_Matrices::s_timestamp_start = nullptr;
ID3D11Query* ComputeTest_Matrices::s_timestamp_end = nullptr;

bool ComputeTest_Matrices::RunTest(u32 iteration_count)
{
    Msg("=== [ComputeTest_Matrices] Starting matrix operations test ===");
    Msg("* [ComputeTest_Matrices] Iteration count: %d cycles per thread", iteration_count);

    constexpr u32 test_count = 65536; // 64K elements
    s_element_count = test_count;
    s_iteration_count = iteration_count;

    // Create buffers and upload data
    if (!CreateTestBuffers(s_element_count))
    {
        Msg("! [ComputeTest_Matrices] FAILED: Buffer creation failed");
        DestroyTestBuffers();
        return false;
    }

    // Run compute shader
    if (!RunComputeShader(iteration_count))
    {
        Msg("! [ComputeTest_Matrices] FAILED: Compute shader execution failed");
        DestroyTestBuffers();
        return false;
    }

    // Validate results
    if (!ValidateResults())
    {
        Msg("! [ComputeTest_Matrices] FAILED: Result validation failed");
        DestroyTestBuffers();
        return false;
    }

    // Cleanup
    DestroyTestBuffers();

    Msg("=== [ComputeTest_Matrices] PASSED: All tests successful! ===");
    return true;
}

bool ComputeTest_Matrices::CreateTestBuffers(u32 element_count)
{
    Msg("* [ComputeTest_Matrices] Creating test buffers for %d elements...", element_count);

    const u32 buffer_size = element_count * sizeof(MatrixTestData);

    // Generate test data
    s_input_data.resize(element_count);
    for (u32 i = 0; i < element_count; ++i)
    {
        // Initialize with identity-ish matrices with slight variations
        float angle = static_cast<float>(i) * 0.01f;
        float scale = 1.0f + static_cast<float>(i % 100) * 0.001f;

        // Create a simple rotation + scale matrix (Y-axis rotation)
        float s = sin(angle);
        float c = cos(angle);

        // Column-major layout for HLSL (matches float4x4 memory layout)
        // Column 0 (x-axis after transform)
        s_input_data[i].transform[0] = c * scale;
        s_input_data[i].transform[1] = 0.0f;
        s_input_data[i].transform[2] = -s * scale;
        s_input_data[i].transform[3] = 0.0f;

        // Column 1 (y-axis after transform)
        s_input_data[i].transform[4] = 0.0f;
        s_input_data[i].transform[5] = scale;
        s_input_data[i].transform[6] = 0.0f;
        s_input_data[i].transform[7] = 0.0f;

        // Column 2 (z-axis after transform)
        s_input_data[i].transform[8] = s * scale;
        s_input_data[i].transform[9] = 0.0f;
        s_input_data[i].transform[10] = c * scale;
        s_input_data[i].transform[11] = 0.0f;

        // Column 3 (translation)
        s_input_data[i].transform[12] = static_cast<float>(i % 10);
        s_input_data[i].transform[13] = static_cast<float>((i / 10) % 10);
        s_input_data[i].transform[14] = static_cast<float>((i / 100) % 10);
        s_input_data[i].transform[15] = 1.0f;

        // Position vector
        s_input_data[i].position.set(
            static_cast<float>(i) * 0.1f,
            static_cast<float>(i) * 0.2f,
            static_cast<float>(i) * 0.3f,
            1.0f // w component
        );

        // Result vector (will be computed)
        s_input_data[i].result.set(0.0f, 0.0f, 0.0f, 0.0f);

        // Flags
        s_input_data[i].flags = i * 10;

        // Padding
        s_input_data[i].padding[0] = 0;
        s_input_data[i].padding[1] = 0;
        s_input_data[i].padding[2] = 0;
    }

    // Show sample input data
    Msg("* [ComputeTest_Matrices] Sample input data:");
    for (u32 i : {0u, 1u, 2u, element_count-1})
    {
        if (i < element_count)
        {
            Msg("    [%4d] pos=(%.2f,%.2f,%.2f,%.2f), flags=%u",
                i,
                s_input_data[i].position.x, s_input_data[i].position.y,
                s_input_data[i].position.z, s_input_data[i].position.w,
                s_input_data[i].flags);
        }
    }

    // Input Buffer (SRV)
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(MatrixTestData);

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

    // Output Buffer (UAV)
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(MatrixTestData);

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

    // Counter Buffer (UAV)
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

    // Constant Buffer
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(TestParams);
        desc.Usage = D3D_USAGE_DYNAMIC;
        desc.BindFlags = D3D_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &s_params_cb));
    }

    // Readback Buffer (for CPU readback)
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = buffer_size;
        desc.Usage = D3D_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &s_readback_buffer));
    }

    // GPU Timing Queries
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

    Msg("* [ComputeTest_Matrices] Buffers and timing queries created successfully");
    return true;
}

bool ComputeTest_Matrices::RunComputeShader(u32 iteration_count)
{
    Msg("* [ComputeTest_Matrices] Running compute shader...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Load/compile compute shader
    s_compute_shader.create("compute_test_matrices");
    if (!s_compute_shader)
    {
        Msg("! [ComputeTest_Matrices] Failed to create compute shader");
        return false;
    }

    // Update constant buffer
    TestParams params;
    params.element_count = s_element_count;
    params.iteration_count = iteration_count;
    params.rotation_angle = PI / 4.0f; // 45 degrees
    params.scale_factor = 1.5f;

    Msg("* [ComputeTest_Matrices] Constant buffer parameters:");
    Msg("    element_count    = %u", params.element_count);
    Msg("    iteration_count  = %u", params.iteration_count);
    Msg("    rotation_angle   = %.2f", params.rotation_angle);
    Msg("    scale_factor     = %.2f", params.scale_factor);
    Msg("    sizeof(TestParams) = %u bytes", sizeof(TestParams));

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_params_cb, 0, D3D_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(TestParams));
    context->Unmap(s_params_cb, 0);

    // Start GPU timing
    context->Begin(s_timestamp_disjoint);
    context->End(s_timestamp_start);

    // Bind resources
    context->CSSetShader(s_compute_shader->sh, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &s_params_cb);
    context->CSSetShaderResources(0, 1, &s_input_srv);

    ID3DUnorderedAccessView* uavs[2] = { s_output_uav, s_counter_uav };
    context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    // Dispatch
    const u32 threads_per_group = 256;
    const u32 num_groups = (s_element_count + threads_per_group - 1) / threads_per_group;

    Msg("* [ComputeTest_Matrices] Dispatching %d groups (%d threads per group)", num_groups, threads_per_group);

    context->Dispatch(num_groups, 1, 1);

    // End GPU timing
    context->End(s_timestamp_end);
    context->End(s_timestamp_disjoint);

    // Unbind resources
    ID3DShaderResourceView* null_srv[1] = { nullptr };
    ID3DUnorderedAccessView* null_uav[2] = { nullptr, nullptr };
    context->CSSetShaderResources(0, 1, null_srv);
    context->CSSetUnorderedAccessViews(0, 2, null_uav, nullptr);

    Msg("* [ComputeTest_Matrices] Compute shader dispatched");
    return true;
}

bool ComputeTest_Matrices::ValidateResults()
{
    Msg("* [ComputeTest_Matrices] Validating results...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Read GPU Timing
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

            Msg("=== [ComputeTest_Matrices] GPU PERFORMANCE ===");
            Msg("    GPU time:        %.3f ms", gpu_time_ms);
            Msg("    Elements:        %d", s_element_count);
            Msg("    Iterations:      %d cycles per element", s_iteration_count);
            Msg("    Matrix ops:      ~%llu", (u64)s_element_count * s_iteration_count * 20); // Rough estimate
            Msg("    Throughput:      %.2f million matrix ops/sec",
                ((u64)s_element_count * s_iteration_count * 20) / (gpu_time_ms * 1000.0f));
        }
        else
        {
            Msg("! [ComputeTest_Matrices] GPU timing disjoint - measurements invalid");
        }
    }

    // Copy output buffer to readback buffer
    context->CopyResource(s_readback_buffer, s_output_buffer);

    // Map and read results
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped));

    MatrixTestData* output_data = static_cast<MatrixTestData*>(mapped.pData);

    // Validate that shader actually ran and modified data
    bool all_correct = true;
    u32 changed_count = 0;
    u32 flags_correct_count = 0;

    Msg("* [ComputeTest_Matrices] Validating %d elements...", s_element_count);

    for (u32 i = 0; i < s_element_count; ++i)
    {
        // Check flags field (deterministic)
        u32 expected_flags = s_input_data[i].flags + s_iteration_count;
        bool flags_correct = (output_data[i].flags == expected_flags);
        if (flags_correct)
            flags_correct_count++;

        // Check that result vector was computed (different from input)
        bool result_changed = (fabs(output_data[i].result.x) > 0.0001f ||
                              fabs(output_data[i].result.y) > 0.0001f ||
                              fabs(output_data[i].result.z) > 0.0001f ||
                              fabs(output_data[i].result.w) > 0.0001f);

        // Check that result is not NaN or Inf
        bool result_valid = !isnan(output_data[i].result.x) && !isinf(output_data[i].result.x) &&
                           !isnan(output_data[i].result.y) && !isinf(output_data[i].result.y) &&
                           !isnan(output_data[i].result.z) && !isinf(output_data[i].result.z) &&
                           !isnan(output_data[i].result.w) && !isinf(output_data[i].result.w);

        if (result_changed && result_valid)
            changed_count++;

        if (!flags_correct || !result_changed || !result_valid)
        {
            if (all_correct) // Log first few errors only
            {
                Msg("! [ComputeTest_Matrices] Element %d suspicious:", i);
                Msg("    flags: %u (expected %u) %s", output_data[i].flags, expected_flags,
                    flags_correct ? "OK" : "FAIL");
                Msg("    result: (%.4f, %.4f, %.4f, %.4f) %s %s",
                    output_data[i].result.x, output_data[i].result.y,
                    output_data[i].result.z, output_data[i].result.w,
                    result_changed ? "CHANGED" : "UNCHANGED",
                    result_valid ? "VALID" : "INVALID(NaN/Inf)");
            }
            all_correct = false;
        }
    }

    context->Unmap(s_readback_buffer, 0);

    // Summary
    Msg("* [ComputeTest_Matrices] Validation Summary:");
    Msg("    Total elements:   %d", s_element_count);
    Msg("    Flags correct:    %d (%.1f%%)", flags_correct_count, 100.0f * flags_correct_count / s_element_count);
    Msg("    Results computed: %d (%.1f%%)", changed_count, 100.0f * changed_count / s_element_count);

    // Check a few specific elements for detailed output
    Msg("* [ComputeTest_Matrices] Sample outputs:");
    u32 samples[] = {0, 1, 2, s_element_count/4, s_element_count/2, s_element_count-1};
    context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped);
    output_data = static_cast<MatrixTestData*>(mapped.pData);
    for (u32 idx : samples)
    {
        if (idx < s_element_count)
        {
            Msg("    [%4d] flags=%u, result=(%.4f,%.4f,%.4f,%.4f)",
                idx,
                output_data[idx].flags,
                output_data[idx].result.x, output_data[idx].result.y,
                output_data[idx].result.z, output_data[idx].result.w);
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

        u32 expected_counter = s_element_count;
        Msg("* [ComputeTest_Matrices] Atomic counter: %d (expected: %d) %s",
            counter, expected_counter,
            (counter == expected_counter) ? "PASS" : "FAIL");

        if (counter != expected_counter)
        {
            Msg("! [ComputeTest_Matrices] Counter mismatch - some threads may not have executed!");
            all_correct = false;
        }
    }

    if (all_correct)
    {
        Msg("=== [ComputeTest_Matrices] VALIDATION PASSED: All computations correct! ===");
    }
    else
    {
        u32 error_count = s_element_count - changed_count;
        Msg("=== [ComputeTest_Matrices] VALIDATION FAILED: %d/%d elements had issues ===",
            error_count, s_element_count);
    }

    return all_correct;
}

void ComputeTest_Matrices::DestroyTestBuffers()
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

    Msg("* [ComputeTest_Matrices] Buffers and queries destroyed");
}

} // namespace xray::render::RENDER_NAMESPACE
