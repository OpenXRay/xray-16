// dx11ComputeTest_SIMD.cpp - Implementation of SIMD compute shader test harness
//
#include "stdafx.h"
#include "dx11ComputeTest_SIMD.h"
#include "dx11HW.h"

namespace xray::render::RENDER_NAMESPACE
{

// Static members
ID3DBuffer* ComputeTest_SIMD::s_input_buffer = nullptr;
ID3DBuffer* ComputeTest_SIMD::s_output_buffer = nullptr;
ID3DBuffer* ComputeTest_SIMD::s_counter_buffer = nullptr;
ID3DBuffer* ComputeTest_SIMD::s_params_cb = nullptr;
ID3DBuffer* ComputeTest_SIMD::s_readback_buffer = nullptr;
ID3DShaderResourceView* ComputeTest_SIMD::s_input_srv = nullptr;
ID3DUnorderedAccessView* ComputeTest_SIMD::s_output_uav = nullptr;
ID3DUnorderedAccessView* ComputeTest_SIMD::s_counter_uav = nullptr;
ref_cs ComputeTest_SIMD::s_compute_shader;
xr_vector<ComputeTest_SIMD::SimdTestData> ComputeTest_SIMD::s_input_data;
u32 ComputeTest_SIMD::s_element_count = 0;
u32 ComputeTest_SIMD::s_iteration_count = 0;

// GPU timing
ID3D11Query* ComputeTest_SIMD::s_timestamp_disjoint = nullptr;
ID3D11Query* ComputeTest_SIMD::s_timestamp_start = nullptr;
ID3D11Query* ComputeTest_SIMD::s_timestamp_end = nullptr;

bool ComputeTest_SIMD::RunTest(u32 iteration_count)
{
    Msg("=== [ComputeTest_SIMD] Starting SIMD operations test ===");
    Msg("* [ComputeTest_SIMD] Iteration count: %d cycles per thread", iteration_count);

    // Use large dataset to show SIMD throughput
    constexpr u32 test_count = 65536; // 64K elements
    s_element_count = test_count;
    s_iteration_count = iteration_count;

    // Create buffers and upload data
    if (!CreateTestBuffers(s_element_count))
    {
        Msg("! [ComputeTest_SIMD] FAILED: Buffer creation failed");
        DestroyTestBuffers();
        return false;
    }

    // Run compute shader
    if (!RunComputeShader(iteration_count))
    {
        Msg("! [ComputeTest_SIMD] FAILED: Compute shader execution failed");
        DestroyTestBuffers();
        return false;
    }

    // Validate results
    if (!ValidateResults())
    {
        Msg("! [ComputeTest_SIMD] FAILED: Result validation failed");
        DestroyTestBuffers();
        return false;
    }

    // Cleanup
    DestroyTestBuffers();

    Msg("=== [ComputeTest_SIMD] PASSED: All tests successful! ===");
    return true;
}

bool ComputeTest_SIMD::CreateTestBuffers(u32 element_count)
{
    Msg("* [ComputeTest_SIMD] Creating test buffers for %d elements...", element_count);

    const u32 buffer_size = element_count * sizeof(SimdTestData);

    // Generate test data
    s_input_data.resize(element_count);
    for (u32 i = 0; i < element_count; ++i)
    {
        // Initialize vectors with varying data
        float base = static_cast<float>(i) * 0.001f;

        s_input_data[i].vec0.set(
            sin(base * 1.0f),
            cos(base * 1.5f),
            sin(base * 2.0f),
            cos(base * 2.5f)
        );

        s_input_data[i].vec1.set(
            cos(base * 3.0f),
            sin(base * 3.5f),
            cos(base * 4.0f),
            sin(base * 4.5f)
        );

        s_input_data[i].vec2.set(
            sin(base * 5.0f) * 0.5f + 0.5f,
            cos(base * 5.5f) * 0.5f + 0.5f,
            sin(base * 6.0f) * 0.5f + 0.5f,
            cos(base * 6.5f) * 0.5f + 0.5f
        );

        s_input_data[i].vec3.set(
            cos(base * 7.0f) * 0.5f + 0.5f,
            sin(base * 7.5f) * 0.5f + 0.5f,
            cos(base * 8.0f) * 0.5f + 0.5f,
            sin(base * 8.5f) * 0.5f + 0.5f
        );

        // Result vectors (will be computed)
        s_input_data[i].result0.set(0.0f, 0.0f, 0.0f, 0.0f);
        s_input_data[i].result1.set(0.0f, 0.0f, 0.0f, 0.0f);

        // Checksum
        s_input_data[i].checksum = 0;

        // Padding
        s_input_data[i].padding[0] = 0;
        s_input_data[i].padding[1] = 0;
        s_input_data[i].padding[2] = 0;
    }

    // Show sample input data
    Msg("* [ComputeTest_SIMD] Sample input data:");
    for (u32 i : {0u, 1u, 2u, element_count-1})
    {
        if (i < element_count)
        {
            Msg("    [%4d] vec0=(%.3f,%.3f,%.3f,%.3f), vec1=(%.3f,%.3f,%.3f,%.3f)",
                i,
                s_input_data[i].vec0.x, s_input_data[i].vec0.y,
                s_input_data[i].vec0.z, s_input_data[i].vec0.w,
                s_input_data[i].vec1.x, s_input_data[i].vec1.y,
                s_input_data[i].vec1.z, s_input_data[i].vec1.w);
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
        desc.StructureByteStride = sizeof(SimdTestData);

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
        desc.StructureByteStride = sizeof(SimdTestData);

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

    Msg("* [ComputeTest_SIMD] Buffers and timing queries created successfully");
    return true;
}

bool ComputeTest_SIMD::RunComputeShader(u32 iteration_count)
{
    Msg("* [ComputeTest_SIMD] Running compute shader...");

    auto* context = HW.get_context(CHW::IMM_CTX_ID);

    // Load/compile compute shader
    s_compute_shader.create("compute_test_simd");
    if (!s_compute_shader)
    {
        Msg("! [ComputeTest_SIMD] Failed to create compute shader");
        return false;
    }

    // Update constant buffer
    TestParams params;
    params.element_count = s_element_count;
    params.iteration_count = iteration_count;
    params.blend_factor = 0.5f;
    params.padding = 0.0f;

    Msg("* [ComputeTest_SIMD] Constant buffer parameters:");
    Msg("    element_count    = %u", params.element_count);
    Msg("    iteration_count  = %u", params.iteration_count);
    Msg("    blend_factor     = %.2f", params.blend_factor);
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

    Msg("* [ComputeTest_SIMD] Dispatching %d groups (%d threads per group)", num_groups, threads_per_group);

    context->Dispatch(num_groups, 1, 1);

    // End GPU timing
    context->End(s_timestamp_end);
    context->End(s_timestamp_disjoint);

    // Unbind resources
    ID3DShaderResourceView* null_srv[1] = { nullptr };
    ID3DUnorderedAccessView* null_uav[2] = { nullptr, nullptr };
    context->CSSetShaderResources(0, 1, null_srv);
    context->CSSetUnorderedAccessViews(0, 2, null_uav, nullptr);

    Msg("* [ComputeTest_SIMD] Compute shader dispatched");
    return true;
}

bool ComputeTest_SIMD::ValidateResults()
{
    Msg("* [ComputeTest_SIMD] Validating results...");

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

            // Estimate SIMD ops: 4 vectors per element, many ops per iteration
            // Rough estimate: ~50 vector ops per iteration
            u64 vector_ops = (u64)s_element_count * s_iteration_count * 50;

            Msg("=== [ComputeTest_SIMD] GPU PERFORMANCE ===");
            Msg("    GPU time:        %.3f ms", gpu_time_ms);
            Msg("    Elements:        %d", s_element_count);
            Msg("    Iterations:      %d cycles per element", s_iteration_count);
            Msg("    Vector ops:      ~%llu", vector_ops);
            Msg("    Throughput:      %.2f million vector ops/sec",
                vector_ops / (gpu_time_ms * 1000.0f));
        }
        else
        {
            Msg("! [ComputeTest_SIMD] GPU timing disjoint - measurements invalid");
        }
    }

    // Copy output buffer to readback buffer
    context->CopyResource(s_readback_buffer, s_output_buffer);

    // Map and read results
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped));

    SimdTestData* output_data = static_cast<SimdTestData*>(mapped.pData);

    // Validate that shader actually ran and modified data
    bool all_correct = true;
    u32 changed_count = 0;
    u32 checksum_valid_count = 0;

    Msg("* [ComputeTest_SIMD] Validating %d elements...", s_element_count);

    for (u32 i = 0; i < s_element_count; ++i)
    {
        // Check that vectors were computed (different from input)
        bool vecs_changed =
            (fabs(output_data[i].vec0.x - s_input_data[i].vec0.x) > 0.0001f ||
             fabs(output_data[i].vec0.y - s_input_data[i].vec0.y) > 0.0001f) &&
            (fabs(output_data[i].vec1.x - s_input_data[i].vec1.x) > 0.0001f ||
             fabs(output_data[i].vec1.y - s_input_data[i].vec1.y) > 0.0001f);

        // Check that results were computed
        bool results_computed =
            (fabs(output_data[i].result0.x) > 0.0001f ||
             fabs(output_data[i].result0.y) > 0.0001f) &&
            (fabs(output_data[i].result1.x) > 0.0001f ||
             fabs(output_data[i].result1.y) > 0.0001f);

        // Check checksum was computed
        bool checksum_valid = (output_data[i].checksum != 0);
        if (checksum_valid)
            checksum_valid_count++;

        // Check no NaN or Inf
        bool data_valid =
            !isnan(output_data[i].vec0.x) && !isinf(output_data[i].vec0.x) &&
            !isnan(output_data[i].vec1.x) && !isinf(output_data[i].vec1.x) &&
            !isnan(output_data[i].result0.x) && !isinf(output_data[i].result0.x) &&
            !isnan(output_data[i].result1.x) && !isinf(output_data[i].result1.x);

        if (vecs_changed && results_computed && checksum_valid && data_valid)
            changed_count++;

        if (!vecs_changed || !results_computed || !checksum_valid || !data_valid)
        {
            if (all_correct) // Log first few errors only
            {
                Msg("! [ComputeTest_SIMD] Element %d suspicious:", i);
                Msg("    vec0: (%.4f, %.4f, %.4f, %.4f) %s",
                    output_data[i].vec0.x, output_data[i].vec0.y,
                    output_data[i].vec0.z, output_data[i].vec0.w,
                    vecs_changed ? "CHANGED" : "UNCHANGED");
                Msg("    result0: (%.4f, %.4f, %.4f, %.4f) %s",
                    output_data[i].result0.x, output_data[i].result0.y,
                    output_data[i].result0.z, output_data[i].result0.w,
                    results_computed ? "COMPUTED" : "NOT_COMPUTED");
                Msg("    checksum: %u %s", output_data[i].checksum,
                    checksum_valid ? "VALID" : "INVALID(0)");
                Msg("    data: %s", data_valid ? "VALID" : "INVALID(NaN/Inf)");
            }
            all_correct = false;
        }
    }

    context->Unmap(s_readback_buffer, 0);

    // Summary
    Msg("* [ComputeTest_SIMD] Validation Summary:");
    Msg("    Total elements:   %d", s_element_count);
    Msg("    Checksums valid:  %d (%.1f%%)", checksum_valid_count, 100.0f * checksum_valid_count / s_element_count);
    Msg("    Data processed:   %d (%.1f%%)", changed_count, 100.0f * changed_count / s_element_count);

    // Check a few specific elements for detailed output
    Msg("* [ComputeTest_SIMD] Sample outputs:");
    u32 samples[] = {0, 1, 2, s_element_count/4, s_element_count/2, s_element_count-1};
    context->Map(s_readback_buffer, 0, D3D_MAP_READ, 0, &mapped);
    output_data = static_cast<SimdTestData*>(mapped.pData);
    for (u32 idx : samples)
    {
        if (idx < s_element_count)
        {
            Msg("    [%4d] result0=(%.4f,%.4f,%.4f,%.4f), checksum=%u",
                idx,
                output_data[idx].result0.x, output_data[idx].result0.y,
                output_data[idx].result0.z, output_data[idx].result0.w,
                output_data[idx].checksum);
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
        Msg("* [ComputeTest_SIMD] Atomic counter: %d (expected: %d) %s",
            counter, expected_counter,
            (counter == expected_counter) ? "PASS" : "FAIL");

        if (counter != expected_counter)
        {
            Msg("! [ComputeTest_SIMD] Counter mismatch - some threads may not have executed!");
            all_correct = false;
        }
    }

    if (all_correct)
    {
        Msg("=== [ComputeTest_SIMD] VALIDATION PASSED: All computations correct! ===");
    }
    else
    {
        u32 error_count = s_element_count - changed_count;
        Msg("=== [ComputeTest_SIMD] VALIDATION FAILED: %d/%d elements had issues ===",
            error_count, s_element_count);
    }

    return all_correct;
}

void ComputeTest_SIMD::DestroyTestBuffers()
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

    Msg("* [ComputeTest_SIMD] Buffers and queries destroyed");
}

} // namespace xray::render::RENDER_NAMESPACE
