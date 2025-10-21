#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"

namespace xray::render::RENDER_NAMESPACE
{
namespace detail_manager
{
extern const int quant;
//extern const int c_hdr;
}

void CDetailManager::hw_Load_Shaders()
{
    // Create shader to access constant storage
    ref_shader S;
    S.create("details\\set");
    R_constant_table& T0 = *(S->E[0]->passes[0]->constants);
    R_constant_table& T1 = *(S->E[1]->passes[0]->constants);
    hwc_consts = T0.get("consts");
    hwc_wave = T0.get("wave");
    hwc_wind = T0.get("dir2D");   // dir1 for wave1
    hwc_wind2 = T0.get("dir2D_2"); // dir2 for wave2
    hwc_array = T0.get("array");
    hwc_s_consts = T1.get("consts");
    hwc_s_xform = T1.get("xform");
    hwc_s_array = T1.get("array");

    // Phase 1, Milestone 1.1: Create 3 structured buffers (one per vis_id: still, wave1, wave2)
    // Use 32K instances per buffer (we've seen up to ~22K for vis_id=0 still grass)
    const u32 initialBufferSize = 2048 * 2048;

    // Instance data structure (must match shader)
    // Phase 2.0.3: Updated to include object_id instead of padding
    struct InstanceData
    {
        Fvector hpb;    // Heading, pitch, bank rotation
        float scale;    // Scale factor
        Fvector pos;    // Position
        float hemi;     // Hemisphere lighting
        u32 vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2)
        u32 object_id;  // Which grass object type (0-63)
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(InstanceData);
    bufferDesc.ByteWidth = initialBufferSize * sizeof(InstanceData);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementWidth = initialBufferSize;

    // Create 3 buffers, one for each vis_id
    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        // Create buffer
        ID3DBuffer* buffer = nullptr;
        CHK_DX(HW.pDevice->CreateBuffer(&bufferDesc, nullptr, &buffer));
        detailBuffer_vis[vis_id] = buffer;
        detailBufferSize_vis[vis_id] = initialBufferSize;

        // Create SRV
        ID3DShaderResourceView* srv = nullptr;
        CHK_DX(HW.pDevice->CreateShaderResourceView(buffer, &srvDesc, &srv));
        detailSRV_vis[vis_id] = srv;
    }

    Msg("* [DetailManager] Created 3 instance buffers (vis_id: still/wave1/wave2), %u instances each", initialBufferSize);
}

// Phase 2.0.3: Create persistent GPU buffer for all level instances
void CDetailManager::CreatePersistentInstanceBuffer()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating persistent GPU instance buffer...");

    // Instance data structure matching shader
    struct InstanceData
    {
        Fvector hpb;
        float scale;
        Fvector pos;
        float hemi;
        u32 vis_id;
        u32 object_id;
    };

    persistent_buffer_capacity = total_instance_count;

    // Prepare instance data for upload
    xr_vector<InstanceData> upload_data;
    upload_data.reserve(total_instance_count);

    for (u32 i = 0; i < total_instance_count; i++)
    {
        const SlotItemWithObject& src = all_level_instances[i];
        InstanceData dst = {};

        // Extract rotation from matrix
        const Fmatrix& M = src.item.mRotY;
        dst.hpb.x = atan2f(M._13, M._11);  // heading
        dst.hpb.y = 0.0f;
        dst.hpb.z = 0.0f;

        dst.scale = src.item.scale;
        dst.pos = M.c;
        dst.hemi = src.item.c_hemi;
        dst.vis_id = src.item.vis_ID;
        dst.object_id = src.object_id;

        upload_data.push_back(dst);
    }

    // Create immutable buffer (never changes after upload)
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;  // No CPU access needed
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(InstanceData);
    desc.ByteWidth = persistent_buffer_capacity * sizeof(InstanceData);

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = upload_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&desc, &init_data, &persistent_instance_buffer));

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.NumElements = persistent_buffer_capacity;

    CHK_DX(HW.pDevice->CreateShaderResourceView(
        persistent_instance_buffer, &srv_desc, &persistent_instance_srv));

    float memory_mb = (persistent_buffer_capacity * sizeof(InstanceData)) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] Persistent GPU buffer created:");
    Msg("  - Instances: %u", persistent_buffer_capacity);
    Msg("  - VRAM usage: %.2f MB", memory_mb);
    Msg("  - Per-instance size: %u bytes", sizeof(InstanceData));
}

// Phase 4A.2: Create GPU slot AABB buffer
void CDetailManager::CreateSlotAABBBuffer()
{
    VERIFY(slot_count > 0);
    VERIFY(!slot_aabb_buffer);

    Msg("* [DetailManager] Creating GPU slot AABB buffer...");

    // Create structured buffer
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_IMMUTABLE;  // Never changes after upload
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(SlotAABB);
    desc.ByteWidth = slot_count * sizeof(SlotAABB);

    // Initial data
    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = slot_aabbs.data();

    HRESULT hr = HW.pDevice->CreateBuffer(&desc, &init_data, &slot_aabb_buffer);
    CHK_DX(hr);

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = slot_count;

    hr = HW.pDevice->CreateShaderResourceView(slot_aabb_buffer, &srv_desc, &slot_aabb_srv);
    CHK_DX(hr);

    float memory_kb = (slot_count * sizeof(SlotAABB)) / 1024.0f;

    Msg("* [DetailManager] Slot AABB buffer created:");
    Msg("  - Slots: %u", slot_count);
    Msg("  - VRAM usage: %.2f KB", memory_kb);
    Msg("  - Per-slot size: %u bytes", sizeof(SlotAABB));
}

void CDetailManager::DestroySlotAABBBuffer()
{
    _RELEASE(slot_aabb_srv);
    _RELEASE(slot_aabb_buffer);
    slot_count = 0;
    slot_aabbs.clear();
}

// Phase 2.1: Create GPU culling buffers and infrastructure
void CDetailManager::CreateGPUCullingBuffers()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating GPU culling infrastructure...");

    // Load compute shader
    cull_compute_shader.create("detail_cull");

    // Estimate max visible instances per object (conservative: average instances per object / 2)
    gpu_visible_buffer_capacity = 2048 * 2048;
    if (gpu_visible_buffer_capacity < 10000)
        gpu_visible_buffer_capacity = 10000;  // Minimum 10K per object

    struct InstanceData
    {
        Fvector hpb;
        float scale;
        Fvector pos;
        float hemi;
        u32 vis_id;
        u32 object_id;
    };

    // Create per-object visible instance buffers
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(InstanceData);
        desc.ByteWidth = gpu_visible_buffer_capacity * sizeof(InstanceData);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &gpu_visible_buffers[i]));

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
        CHK_DX(HW.pDevice->CreateShaderResourceView(gpu_visible_buffers[i], &srvDesc, &gpu_visible_srvs[i]));

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
        CHK_DX(HW.pDevice->CreateUnorderedAccessView(gpu_visible_buffers[i], &uavDesc, &gpu_visible_uavs[i]));
    }

    // Create counter buffer (one u32 per object, up to 64 objects)
    D3D11_BUFFER_DESC counterDesc = {};
    counterDesc.Usage = D3D11_USAGE_DEFAULT;
    counterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    counterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    counterDesc.ByteWidth = dm_max_objects * sizeof(u32);
    CHK_DX(HW.pDevice->CreateBuffer(&counterDesc, nullptr, &gpu_visible_counts_buffer));

    D3D11_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = {};
    counterUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    counterUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    counterUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    counterUavDesc.Buffer.NumElements = dm_max_objects;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(gpu_visible_counts_buffer, &counterUavDesc, &gpu_visible_counts_uav));

    // Create readback buffer for CPU access
    D3D11_BUFFER_DESC readbackDesc = counterDesc;
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.BindFlags = 0;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackDesc.MiscFlags = 0;
    CHK_DX(HW.pDevice->CreateBuffer(&readbackDesc, nullptr, &gpu_visible_counts_readback));

    // Create constant buffer for culling parameters
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = 256;  // Generous size for alignment
    CHK_DX(HW.pDevice->CreateBuffer(&cbDesc, nullptr, &cull_constant_buffer));

    // Phase 2.2.1: Create indirect draw args buffers
    // D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS structure:
    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;         // Written by compute shader
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    for (u32 i = 0; i < max_gpu_culled_objects && i < objects.size(); i++)
    {
        CDetail& obj = *objects[i];

        // Initialize args with static values
        IndirectDrawArgs initial_args = {};
        initial_args.IndexCountPerInstance = obj.number_indices;
        initial_args.InstanceCount = 0;  // Will be written by compute shader
        initial_args.StartIndexLocation = vis_geometry_index_offsets[0][i];
        initial_args.BaseVertexLocation = 0;
        initial_args.StartInstanceLocation = 0;

        // Create buffer with DRAWINDIRECT_ARGS and RAW flags
        D3D11_BUFFER_DESC argsDesc = {};
        argsDesc.Usage = D3D11_USAGE_DEFAULT;
        argsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        argsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        argsDesc.ByteWidth = sizeof(IndirectDrawArgs);

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &initial_args;

        CHK_DX(HW.pDevice->CreateBuffer(&argsDesc, &initData, &gpu_indirect_args[i]));

        // Create RAW UAV for compute shader to write InstanceCount (RWByteAddressBuffer requires RAW)
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = 5;  // 5 u32s in IndirectDrawArgs
        uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

        CHK_DX(HW.pDevice->CreateUnorderedAccessView(
            gpu_indirect_args[i], &uavDesc, &gpu_indirect_args_uavs[i]));
    }

    float vram_mb = (max_gpu_culled_objects * gpu_visible_buffer_capacity * sizeof(InstanceData)) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] GPU culling infrastructure created:");
    Msg("  - Per-object buffer capacity: %u instances", gpu_visible_buffer_capacity);
    Msg("  - Object buffers: %u (first %u objects)", max_gpu_culled_objects, max_gpu_culled_objects);
    Msg("  - VRAM for output buffers: %.2f MB", vram_mb);
    Msg("  - Indirect args buffers: %u created", _min(max_gpu_culled_objects, objects.size()));
}

// Phase 2.1: Dispatch GPU culling compute shader
void CDetailManager::DispatchGPUCulling(CBackend& cmd_list)
{
    ZoneScoped;

    auto context = HW.get_context(cmd_list.context_id);

    struct DetailCullParams
    {
        Fmatrix view_proj;
        Fvector3 camera_pos;
        float fade_distance_sqr;
        Fvector4 frustum_planes[6];
        u32 total_instance_count;
        u32 total_slot_count;
        u32 object_count;
        u32 pad0;
    };

    // Extract frustum planes from view-projection matrix
    // NOTE: Use same planes as old cache system (LRTB + FAR, no NEAR plane)
    // This prevents culling grass that's close to camera
    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

    DetailCullParams params = {};
    params.view_proj = Device.mFullTransform;
    params.camera_pos = Device.vCameraPosition;
    params.fade_distance_sqr = fade_distance * fade_distance;
    params.total_instance_count = total_instance_count;
    params.total_slot_count = slot_count;
    params.object_count = objects.size();

    // Copy frustum planes (5 planes: LRTB + FAR, no NEAR)
    // Initialize all planes to disable them (pointing away)
    for (u32 i = 0; i < 6; i++)
        params.frustum_planes[i].set(0, 0, 0, 1000000.0f);  // Very far away

    // Copy actual planes from frustum
    for (u32 i = 0; i < frustum.p_count && i < 6; i++)
    {
        params.frustum_planes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y,
                                      frustum.planes[i].n.z, frustum.planes[i].d);
    }

    // DEBUG: Log first time
    static bool logged = false;
    if (!logged)
    {
        Msg("! [DetailManager] GPU Culling Setup:");
        Msg("  - fade_distance: %.2f", fade_distance);
        Msg("  - fade_distance_sqr: %.2f", params.fade_distance_sqr);
        Msg("  - camera_pos: %.2f, %.2f, %.2f",
            params.camera_pos.x, params.camera_pos.y, params.camera_pos.z);
        Msg("  - total_instance_count: %u", total_instance_count);
        Msg("  - Frustum planes (%u active):", frustum.p_count);
        for (u32 i = 0; i < frustum.p_count; i++)
        {
            Msg("    Plane %u: n=(%.3f, %.3f, %.3f), d=%.3f", i,
                params.frustum_planes[i].x, params.frustum_planes[i].y,
                params.frustum_planes[i].z, params.frustum_planes[i].w);
        }
        logged = true;
    }

    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(cull_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(cull_constant_buffer, 0);

    // Phase 2.2.1: Reset indirect args buffers (InstanceCount to 0, keep static fields)
    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;         // Reset to 0 each frame
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    for (u32 i = 0; i < max_gpu_culled_objects && i < objects.size(); i++)
    {
        CDetail& obj = *objects[i];

        IndirectDrawArgs args = {};
        args.IndexCountPerInstance = obj.number_indices;
        args.InstanceCount = 0;  // Reset to 0
        args.StartIndexLocation = vis_geometry_index_offsets[0][i];
        args.BaseVertexLocation = 0;
        args.StartInstanceLocation = 0;

        // Update the args buffer (small upload: 20 bytes)
        context->UpdateSubresource(gpu_indirect_args[i], 0, nullptr, &args, 0, 0);
    }

    // Clear counter buffer to zero (still used for debugging/validation)
    UINT zero_counts[64] = {0};
    context->ClearUnorderedAccessViewUint(gpu_visible_counts_uav, zero_counts);

    // Bind constant buffer
    context->CSSetConstantBuffers(0, 1, &cull_constant_buffer);

    ID3DShaderResourceView* srvs[2] = {slot_aabb_srv, persistent_instance_srv};
    context->CSSetShaderResources(0, 2, srvs);

    // Bind outputs: per-object visible buffers + counter buffer + indirect args
    // u0-u15: visible instance buffers
    // u16: counter buffer
    // u17-u32: indirect args buffers
    ID3DUnorderedAccessView* uavs[33];
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
        uavs[i] = gpu_visible_uavs[i];
    uavs[16] = gpu_visible_counts_uav;  // Counter buffer at u16
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
        uavs[17 + i] = gpu_indirect_args_uavs[i];  // Indirect args at u17-u32

    context->CSSetUnorderedAccessViews(0, 33, uavs, nullptr);

    context->CSSetShader(cull_compute_shader->sh, nullptr, 0);

    u32 num_groups = (slot_count + 255) / 256;
    context->Dispatch(num_groups, 1, 1);

    // Unbind UAVs (prepare for rendering)
    ID3DUnorderedAccessView* null_uavs[33] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 33, null_uavs, nullptr);

    ID3DShaderResourceView* null_srvs[2] = {nullptr, nullptr};
    context->CSSetShaderResources(0, 2, null_srvs);

    // Unbind compute shader
    context->CSSetShader(nullptr, nullptr, 0);

    // Copy counter buffer to readback for CPU access
    context->CopyResource(gpu_visible_counts_readback, gpu_visible_counts_buffer);
}

// Phase 2.1: Render using GPU-culled instances
void CDetailManager::hw_Render_FullLevel(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

    // Reset detail count
    RImplementation.BasicStats.DetailCount = 0;

    // Phase 2.1: Dispatch GPU culling compute shader
    DispatchGPUCulling(cmd_list);

    // Update animation timers
    float fDelta = Device.fTimeGlobal - m_global_time_old;
    if ((fDelta < 0) || (fDelta > 1))
        fDelta = 0.03f;
    m_global_time_old = Device.fTimeGlobal;

    m_time_rot_1 += (PI_MUL_2 * fDelta / swing_current.rot1);
    m_time_rot_2 += (PI_MUL_2 * fDelta / swing_current.rot2);
    m_time_pos += fDelta * swing_current.speed;

    float tm_rot1 = m_time_rot_1;
    float tm_rot2 = m_time_rot_2;

    Fvector4 dir1, dir2;
    dir1.set(_sin(tm_rot1), 0, _cos(tm_rot1), 0).normalize().mul(swing_current.amp1);
    dir2.set(_sin(tm_rot2), 0, _cos(tm_rot2), 0).normalize().mul(swing_current.amp2);

    float scale = 1.f / float(quant);
    Fvector4 wave;
    Fvector4 consts;
    consts.set(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");    // dir1 for wave1
    static shared_str strDir2D_2("dir2D_2"); // dir2 for wave2

    auto context = HW.get_context(cmd_list.context_id);

    // Phase 2.2.2: Optional DEBUG logging (read back counts for validation)
    // This can be removed once indirect draw is confirmed working
    static bool first_frame = true;
    if (first_frame)
    {
        context->CopyResource(gpu_visible_counts_readback, gpu_visible_counts_buffer);
        D3D11_MAPPED_SUBRESOURCE mapped;
        CHK_DX(context->Map(gpu_visible_counts_readback, 0, D3D11_MAP_READ, 0, &mapped));
        u32* counts = (u32*)mapped.pData;

        u32 total_visible = 0;
        for (u32 i = 0; i < objects.size() && i < max_gpu_culled_objects; i++)
            total_visible += counts[i];

        Msg("! [DetailManager] GPU Indirect Draw - First Frame:");
        Msg("  - Total instances: %u", total_instance_count);
        Msg("  - Visible instances: %u (%.1f%%)", total_visible,
            (total_visible * 100.0f) / total_instance_count);

        for (u32 i = 0; i < objects.size() && i < max_gpu_culled_objects; i++)
        {
            if (counts[i] > 0)
                Msg("  - Object %u: %u instances", i, counts[i]);
        }

        context->Unmap(gpu_visible_counts_readback, 0);
        first_frame = false;
    }

    // Phase 2.2.2: Render using DrawIndexedInstancedIndirect (GPU controls instance count)
    u32 objects_to_render = _min(objects.size(), max_gpu_culled_objects);
    for (u32 O = 0; O < objects_to_render; O++)
    {
        CDetail& Object = *objects[O];

        // Set rendering state
        cmd_list.set_CullMode(CULL_NONE);
        cmd_list.set_xform_world(Fidentity);
        cmd_list.SRVSManager.SetVSResource(0, gpu_visible_srvs[O]);
        cmd_list.set_Geometry(vis_unified_geom[0]);
        cmd_list.set_c(strConsts, consts);
        cmd_list.set_c(strWave, wave.div(PI_MUL_2));
        cmd_list.set_c(strDir2D, dir1);    // dir1 for wave1 (vis_id=1)
        cmd_list.set_c(strDir2D_2, dir2);   // dir2 for wave2 (vis_id=2)

        cmd_list.set_Element(Object.shader->E[0], 0);
        cmd_list.apply_lmaterial();

        // Phase 2.2.2: DrawIndexedInstancedIndirect
        // The GPU determines instance count from the indirect args buffer
        cmd_list.RenderIndexedInstancedIndirect(D3DPT_TRIANGLELIST, gpu_indirect_args[O], 0);

        // Note: We can't accurately update stats without CPU readback
        // For now, just increment draw call count
        cmd_list.set_CullMode(CULL_CCW);
    }

    // Phase 2.2.2: Clean up - unbind our resources to avoid affecting subsequent draws
    cmd_list.SRVSManager.SetVSResource(0, nullptr);
    // Clear any PS resources that might have been set by the detail shader
    for (u32 i = 0; i < 16; i++)
        cmd_list.SRVSManager.SetPSResource(i, nullptr);

    // Update basic stats (instance count is now GPU-driven, so we use total as estimate)
    RImplementation.BasicStats.DetailCount = total_instance_count;
}

void CDetailManager::hw_Render(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

#ifdef USE_DX11
    // Phase 2.0.4: Use full-level rendering if available
    if (full_level_loaded)
    {
        hw_Render_FullLevel(cmd_list);
        return;
    }
#endif

    // Reset detail count once per frame, not per vis_id
    RImplementation.BasicStats.DetailCount = 0;

    // Render-prepare
    //	Update timer
    //	Can't use Device.fTimeDelta since it is smoothed! Don't know why, but smoothed value looks more choppy!
    float fDelta = Device.fTimeGlobal - m_global_time_old;
    if ((fDelta < 0) || (fDelta > 1))
        fDelta = 0.03f;
    m_global_time_old = Device.fTimeGlobal;

    m_time_rot_1 += (PI_MUL_2 * fDelta / swing_current.rot1);
    m_time_rot_2 += (PI_MUL_2 * fDelta / swing_current.rot2);
    m_time_pos += fDelta * swing_current.speed;

    float tm_rot1 = m_time_rot_1;
    float tm_rot2 = m_time_rot_2;

    Fvector4 dir1, dir2;
    dir1.set(_sin(tm_rot1), 0, _cos(tm_rot1), 0).normalize().mul(swing_current.amp1);
    dir2.set(_sin(tm_rot2), 0, _cos(tm_rot2), 0).normalize().mul(swing_current.amp2);

    // Phase 1.3: Simplified unified rendering
    // Use single wave parameters for now (will be handled per-instance in future phase)
    float scale = 1.f / float(quant);
    Fvector4 wave;
    Fvector4 consts;
    consts.set(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);

    // Loop over objects, rendering all instances (still + wave1 + wave2) per object
    for (u32 O = 0; O < objects.size(); O++)
    {
        hw_Render_object(cmd_list, consts, wave.div(PI_MUL_2), dir1, O);
    }
}

void CDetailManager::hw_Render_dump(CBackend& cmd_list,
    const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id) {}

void CDetailManager::hw_Render_object(CBackend& cmd_list,
    const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 object_id)
{
    ZoneScoped;

    // Instance data structure (must match shader)
    // Phase 2.0.3: Updated to include object_id instead of padding
    struct InstanceData
    {
        Fvector hpb;    // Heading, pitch, bank rotation
        float scale;    // Scale factor
        Fvector pos;    // Position
        float hemi;     // Hemisphere lighting
        u32 vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2)
        u32 object_id;  // Which grass object type (0-63)
    };

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");

    CDetail& Object = *objects[object_id];

    // Phase 1.3: Gather instances from all 3 vis_ids for this object
    // Count total instances across all vis_ids
    u32 totalInstanceCount = 0;
    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        vis_list& list = m_visibles[vis_id];
        xr_vector<SlotItemVec*>& vis = list[object_id];
        for (SlotItemVec* items : vis)
            totalInstanceCount += items->size();
    }

    if (totalInstanceCount == 0)
        return;

    // Use first buffer for unified rendering (all vis_ids share same buffer)
    ID3DBuffer* currentBuffer = detailBuffer_vis[0];
    ID3DShaderResourceView* currentSRV = detailSRV_vis[0];
    u32 bufferSize = detailBufferSize_vis[0];

    if (totalInstanceCount > bufferSize)
    {
        Msg("! [DetailManager] Too many instances for object=%u: need %u, have %u. Clamping.",
            object_id, totalInstanceCount, bufferSize);
        totalInstanceCount = bufferSize;
    }

    // Fill instance buffer with all instances from all vis_ids
    D3D11_MAPPED_SUBRESOURCE pSubRes;
    CHK_DX(HW.get_context(cmd_list.context_id)->Map(currentBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pSubRes));
    InstanceData* c_storage = reinterpret_cast<InstanceData*>(pSubRes.pData);

    u32 instanceIdx = 0;
    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        vis_list& list = m_visibles[vis_id];
        xr_vector<SlotItemVec*>& vis = list[object_id];

        for (SlotItemVec* items : vis)
        {
            for (SlotItem* item : *items)
            {
                if (instanceIdx >= totalInstanceCount)
                    break;

                SlotItem& Instance = *item;
                Fmatrix& M = Instance.mRotY;

                // Extract heading from Y-rotation matrix
                Fvector3 hpb;
                hpb.x = atan2f(M._13, M._11);  // heading (rotation around Y)
                hpb.y = 0.0f;                   // pitch
                hpb.z = 0.0f;                   // bank

                c_storage[instanceIdx].hpb = hpb;
                c_storage[instanceIdx].scale = Instance.scale;
                c_storage[instanceIdx].pos = M.c;
                c_storage[instanceIdx].hemi = Instance.c_hemi;
                c_storage[instanceIdx].vis_id = vis_id;
                c_storage[instanceIdx].object_id = object_id;

                instanceIdx++;
                RImplementation.BasicStats.DetailCount++;
            }
        }
    }

    HW.get_context(cmd_list.context_id)->Unmap(currentBuffer, 0);

    // Set shader constants and state
    cmd_list.set_CullMode(CULL_NONE);
    cmd_list.set_xform_world(Fidentity);
    cmd_list.SRVSManager.SetVSResource(0, currentSRV);
    cmd_list.set_Geometry(vis_unified_geom[0]);
    cmd_list.set_c(strConsts, consts);
    cmd_list.set_c(strWave, wave);
    cmd_list.set_c(strDir2D, wind);

    // Set shader for this object (use lod_id=0 for wave shader)
    cmd_list.set_Element(Object.shader->E[0], 0);
    cmd_list.apply_lmaterial();

    // Draw using unified geometry with proper offsets
    u32 baseIndex = vis_geometry_index_offsets[0][object_id];
    u32 numVertices = Object.number_vertices;
    u32 numIndices = Object.number_indices;

    cmd_list.RenderInstancedIndexed(
        D3DPT_TRIANGLELIST,
        0, 0,
        numVertices,
        baseIndex,
        numIndices / 3,
        instanceIdx,  // Use actual instance count written
        0);

    cmd_list.stat.r.s_details.add(numVertices * instanceIdx);

    cmd_list.set_CullMode(CULL_CCW);
}
} // namespace xray::render::RENDER_NAMESPACE
