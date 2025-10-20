// dx11DetailManager_Compute.cpp - DirectX 11 implementation of compute-based detail culling
//
#include "stdafx.h"
#include "Layers/xrRender/DetailManager_Compute.h"
#include "dx11HW.h"

namespace xray::render::RENDER_NAMESPACE
{

extern float r_ssaDISCARD;

// ===========================
// Constructor / Destructor
// ===========================

DetailComputeManager::DetailComputeManager()
    : m_instance_count(0)
    , m_max_instances(0)
    , m_initialized(false)
    , m_needs_upload(false)
{
    ZeroMemory(&m_gpu, sizeof(m_gpu));
    ZeroMemory(&m_stats, sizeof(m_stats));
}

DetailComputeManager::~DetailComputeManager()
{
    Shutdown();
}

// ===========================
// Initialization
// ===========================

void DetailComputeManager::Initialize(u32 max_instances)
{
    if (m_initialized)
        Shutdown();

    m_max_instances = max_instances;
    m_instance_staging.reserve(max_instances);

    // Create GPU resources
    CreateBuffers(max_instances);
    CompileShaders();

    m_initialized = true;

    Msg("* [DetailComputeManager] Initialized with %d max instances", max_instances);
}

void DetailComputeManager::Shutdown()
{
    if (!m_initialized)
        return;

    DestroyBuffers();

    m_instance_staging.clear();
    m_instance_count = 0;
    m_max_instances = 0;
    m_initialized = false;

    Msg("* [DetailComputeManager] Shutdown complete");
}

// ===========================
// Buffer Creation
// ===========================

void DetailComputeManager::CreateBuffers(u32 max_instances)
{
    VERIFY(HW.pDevice);

    const u32 instance_buffer_size = max_instances * sizeof(DetailInstanceGPU);
    const u32 visible_buffer_size = max_instances * sizeof(u32); // indices
    const u32 counter_buffer_size = 3 * sizeof(u32); // 3 counters (still, wave1, wave2)
    const u32 indirect_args_size = sizeof(IndirectDrawArgs);

    // ===========================
    // Instance Buffer (SRV for CS read)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = instance_buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(DetailInstanceGPU);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &m_gpu.instance_buffer));

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = max_instances;

        CHK_DX(HW.pDevice->CreateShaderResourceView(m_gpu.instance_buffer, &srv_desc, &m_gpu.instance_buffer_srv));

#ifdef DEBUG
        m_gpu.instance_buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 21, "DetailInstanceBuffer");
        m_gpu.instance_buffer_srv->SetPrivateData(WKPDID_D3DDebugObjectName, 25, "DetailInstanceBuffer_SRV");
#endif
    }

    // ===========================
    // Visible Indices Buffers (UAV for CS write, SRV for VS read)
    // ===========================
    for (int i = 0; i < 3; ++i)
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = visible_buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE | D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(u32);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &m_gpu.visible_indices[i]));

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = max_instances;
        uav_desc.Buffer.Flags = 0;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(m_gpu.visible_indices[i], &uav_desc, &m_gpu.visible_indices_uav[i]));

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = max_instances;

        CHK_DX(HW.pDevice->CreateShaderResourceView(m_gpu.visible_indices[i], &srv_desc, &m_gpu.visible_indices_srv[i]));

#ifdef DEBUG
        char name[64];
        xr_sprintf(name, "DetailVisibleIndices_%d", i);
        m_gpu.visible_indices[i]->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
        xr_sprintf(name, "DetailVisibleIndices_%d_UAV", i);
        m_gpu.visible_indices_uav[i]->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
        xr_sprintf(name, "DetailVisibleIndices_%d_SRV", i);
        m_gpu.visible_indices_srv[i]->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
#endif
    }

    // ===========================
    // Counter Buffer (UAV for atomic operations)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = counter_buffer_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(u32);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &m_gpu.counter_buffer));

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = 3;
        uav_desc.Buffer.Flags = 0;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(m_gpu.counter_buffer, &uav_desc, &m_gpu.counter_buffer_uav));

#ifdef DEBUG
        m_gpu.counter_buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 19, "DetailCounterBuffer");
        m_gpu.counter_buffer_uav->SetPrivateData(WKPDID_D3DDebugObjectName, 23, "DetailCounterBuffer_UAV");
#endif
    }

    // ===========================
    // Indirect Args Buffers (UAV for CS write, used in DrawIndirect)
    // ===========================
    for (int i = 0; i < 3; ++i)
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = indirect_args_size;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D_RESOURCE_MISC_DRAWINDIRECT_ARGS; // Required for DrawIndexedInstancedIndirect

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &m_gpu.indirect_args[i]));

        // Create UAV (for writing from compute shader)
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_R32_UINT;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = sizeof(IndirectDrawArgs) / sizeof(u32);
        uav_desc.Buffer.Flags = 0;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(m_gpu.indirect_args[i], &uav_desc, &m_gpu.indirect_args_uav[i]));

#ifdef DEBUG
        char name[64];
        xr_sprintf(name, "DetailIndirectArgs_%d", i);
        m_gpu.indirect_args[i]->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
        xr_sprintf(name, "DetailIndirectArgs_%d_UAV", i);
        m_gpu.indirect_args_uav[i]->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
#endif
    }

    // ===========================
    // Constant Buffer (cull params)
    // ===========================
    {
        D3D_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(DetailCullParams);
        desc.Usage = D3D_USAGE_DYNAMIC;
        desc.BindFlags = D3D_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &m_gpu.cull_params_cb));

#ifdef DEBUG
        m_gpu.cull_params_cb->SetPrivateData(WKPDID_D3DDebugObjectName, 20, "DetailCullParams_CB");
#endif
    }

    // ===========================
    // Staging Buffer (for readback - debug/stats)
    // ===========================
    {
        m_gpu.counter_readback = xr_alloc<u32>(3);
    }

    Msg("* [DetailComputeManager] GPU buffers created (max %d instances)", max_instances);
}

void DetailComputeManager::DestroyBuffers()
{
    _RELEASE(m_gpu.instance_buffer);
    _RELEASE(m_gpu.instance_buffer_srv);

    for (int i = 0; i < 3; ++i)
    {
        _RELEASE(m_gpu.visible_indices[i]);
        _RELEASE(m_gpu.visible_indices_uav[i]);
        _RELEASE(m_gpu.visible_indices_srv[i]);
        _RELEASE(m_gpu.indirect_args[i]);
        _RELEASE(m_gpu.indirect_args_uav[i]);
    }

    _RELEASE(m_gpu.counter_buffer);
    _RELEASE(m_gpu.counter_buffer_uav);
    _RELEASE(m_gpu.cull_params_cb);

    if (m_gpu.counter_readback)
        xr_free(m_gpu.counter_readback);

    ZeroMemory(&m_gpu, sizeof(m_gpu));
}

// ===========================
// Shader Compilation
// ===========================

void DetailComputeManager::CompileShaders()
{
    // The shader will be compiled by resource manager when first accessed
    m_gpu.cull_shader.create("detail_cull");

    Msg("* [DetailComputeManager] Compute shader compiled/loaded");
}

// ===========================
// Instance Management
// ===========================

void DetailComputeManager::BeginInstanceUpdate()
{
    m_instance_staging.clear();
    m_needs_upload = false;
}

void DetailComputeManager::AddInstance(const DetailInstanceGPU& instance)
{
    if (m_instance_staging.size() >= m_max_instances)
    {
        Msg("! [DetailComputeManager] Max instances reached (%d), skipping", m_max_instances);
        return;
    }

    m_instance_staging.push_back(instance);
    m_needs_upload = true;
}

void DetailComputeManager::EndInstanceUpdate()
{
    m_instance_count = m_instance_staging.size();

    if (m_needs_upload && m_instance_count > 0)
    {
        // Upload will happen before dispatch
    }
}

// ===========================
// Instance Upload
// ===========================

void DetailComputeManager::UploadInstances(CBackend& cmd_list)
{
    if (!m_needs_upload || m_instance_count == 0)
        return;

    VERIFY(m_gpu.instance_buffer);

    // Update instance buffer on GPU
    auto* context = HW.get_context(CHW::IMM_CTX_ID); // TODO: use cmd_list context

    D3D11_BOX box = {};
    box.left = 0;
    box.right = m_instance_count * sizeof(DetailInstanceGPU);
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;

    context->UpdateSubresource(
        m_gpu.instance_buffer,
        0,
        &box,
        m_instance_staging.data(),
        0,
        0
    );

    m_needs_upload = false;
}

// ===========================
// Compute Dispatch
// ===========================

void DetailComputeManager::DispatchCulling(CBackend& cmd_list, const Fmatrix& view_proj)
{
    if (!m_initialized || m_instance_count == 0)
        return;

    ZoneScoped;

    // Upload instances if needed
    UploadInstances(cmd_list);

    auto* context = HW.get_context(CHW::IMM_CTX_ID); // TODO: use cmd_list context

    // ===========================
    // Clear counter buffer
    // ===========================
    {
        u32 zeros[3] = { 0, 0, 0 };
        D3D11_BOX box = {};
        box.left = 0;
        box.right = sizeof(zeros);
        box.top = 0;
        box.bottom = 1;
        box.front = 0;
        box.back = 1;

        context->UpdateSubresource(m_gpu.counter_buffer, 0, &box, zeros, 0, 0);
    }

    // ===========================
    // Update constant buffer
    // ===========================
    DetailCullParams params = {};
    params.camera_pos = Device.vCameraPosition;
    params.camera_dir = Device.vCameraDirection;
    params.fade_limit_sqr = dm_fade * dm_fade;
    params.fade_start_sqr = 1.f; // TODO: configurable
    params.r_ssa_discard = r_ssaDISCARD;
    params.r_ssa_cheap = 16.f * r_ssaDISCARD;
    params.instance_count = m_instance_count;
    params.frame_number = Device.dwFrame;

    // Extract frustum planes and copy to params
    FrustumGPU frustum = BuildFrustumGPU(view_proj);
    for (int i = 0; i < 6; ++i)
        params.frustum_planes[i] = frustum.planes[i];

    // Map and update CB
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        CHK_DX(context->Map(m_gpu.cull_params_cb, 0, D3D_MAP_WRITE_DISCARD, 0, &mapped));
        memcpy(mapped.pData, &params, sizeof(DetailCullParams));
        context->Unmap(m_gpu.cull_params_cb, 0);
    }

    // ===========================
    // Bind resources
    // ===========================

    // Set compute shader
    cmd_list.set_CS(m_gpu.cull_shader);

    // Bind constant buffer
    context->CSSetConstantBuffers(0, 1, &m_gpu.cull_params_cb);

    // Bind input SRV (instance buffer)
    context->CSSetShaderResources(0, 1, &m_gpu.instance_buffer_srv);

    // Bind output UAVs
    ID3DUnorderedAccessView* uavs[5] = {
        m_gpu.visible_indices_uav[0], // still
        m_gpu.visible_indices_uav[1], // wave1
        m_gpu.visible_indices_uav[2], // wave2
        m_gpu.counter_buffer_uav,      // counters
        m_gpu.indirect_args_uav[0]     // indirect args (placeholder)
    };
    context->CSSetUnorderedAccessViews(0, 5, uavs, nullptr);

    // ===========================
    // Dispatch
    // ===========================
    const u32 threads_per_group = 256;
    const u32 num_groups = (m_instance_count + threads_per_group - 1) / threads_per_group;

    cmd_list.Compute(num_groups, 1, 1);

    // ===========================
    // Unbind resources
    // ===========================
    ID3DShaderResourceView* null_srv[1] = { nullptr };
    ID3DUnorderedAccessView* null_uav[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    context->CSSetShaderResources(0, 1, null_srv);
    context->CSSetUnorderedAccessViews(0, 5, null_uav, nullptr);

    // Update stats
    m_stats.total_instances = m_instance_count;
    m_stats.compute_dispatches++;
}

// ===========================
// Indirect Rendering
// ===========================

void DetailComputeManager::RenderIndirect(CBackend& cmd_list, u32 object_id, u32 vis_id, u32 lod_id)
{
    VERIFY(vis_id < 3);
    VERIFY(m_gpu.indirect_args[vis_id]);

    // TODO: Implement indirect rendering
    // This will use DrawIndexedInstancedIndirect with the indirect args buffer
    // For now, placeholder

    m_stats.culled_instances[vis_id]++;
}

// ===========================
// Statistics
// ===========================

void DetailComputeManager::ResetStats()
{
    ZeroMemory(&m_stats, sizeof(m_stats));
}

} // namespace xray::render::RENDER_NAMESPACE
