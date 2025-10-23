#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"
#include "xrEngine/GrassInteractionCollector.h"

extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;

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
    hwc_wind = T0.get("dir2D");
    hwc_wind2 = T0.get("dir2D_2");
    hwc_array = T0.get("array");
    hwc_s_consts = T1.get("consts");
    hwc_s_xform = T1.get("xform");
    hwc_s_array = T1.get("array");
    hwc_detail_params = T0.get("detail_params");
    hwc_grass_wind_displacement = T0.get("grass_wind_displacement");
    hwc_grass_interaction_displacement = T0.get("grass_interaction_displacement");

    const u32 initialBufferSize = 2048 * 2048;

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

    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        ID3DBuffer* buffer = nullptr;
        CHK_DX(HW.pDevice->CreateBuffer(&bufferDesc, nullptr, &buffer));
        detailBuffer_vis[vis_id] = buffer;
        detailBufferSize_vis[vis_id] = initialBufferSize;

        ID3DShaderResourceView* srv = nullptr;
        CHK_DX(HW.pDevice->CreateShaderResourceView(buffer, &srvDesc, &srv));
        detailSRV_vis[vis_id] = srv;
    }

    Msg("* [DetailManager] Created 3 instance buffers (vis_id: still/wave1/wave2), %u instances each", initialBufferSize);

    interaction_compute_shader.create("detail_interaction");
    wind_compute_shader.create("detail_wind_fbm");

    if (interaction_compute_shader && interaction_compute_shader->sh)
        Msg("* [DetailManager] Loaded interaction compute shader: OK");
    else
        Msg("! [DetailManager] Failed to load interaction compute shader!");

    if (wind_compute_shader && wind_compute_shader->sh)
        Msg("* [DetailManager] Loaded wind compute shader: OK");
    else
        Msg("! [DetailManager] Failed to load wind compute shader!");

    interaction_update_cs.create("detail_interaction_apply");

    if (interaction_update_cs && interaction_update_cs->sh)
        Msg("* [DetailManager] Loaded interaction update shader: OK");
    else
        Msg("! [DetailManager] Failed to load interaction update shader!");

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof(InteractionUpdateCB);

    CHK_DX(HW.pDevice->CreateBuffer(&cbDesc, nullptr, &interaction_update_cb));
}

void CDetailManager::CreatePersistentInstanceBuffer()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating persistent GPU instance buffer...");

    persistent_buffer_capacity = total_instance_count;

    xr_vector<InstanceData> upload_data;
    upload_data.reserve(total_instance_count);

    for (u32 i = 0; i < total_instance_count; i++)
    {
        const SlotItemWithObject& src = all_level_instances[i];
        InstanceData dst = {};

        const Fmatrix& M = src.item.mRotY;
        dst.m0.set(M._11, M._21, M._31);
        dst.m1.set(M._12, M._22, M._32);
        dst.m2.set(M._13, M._23, M._33);

        dst.scale = src.item.scale;
        dst.pos = M.c;
        dst.hemi = src.item.c_hemi;
        dst.vis_id = src.item.vis_ID;
        dst.object_id = src.object_id;

        upload_data.push_back(dst);
    }

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(InstanceData);
    desc.ByteWidth = persistent_buffer_capacity * sizeof(InstanceData);

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = upload_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&desc, &init_data, &persistent_instance_buffer));

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

void CDetailManager::CreateSlotAABBBuffer()
{
    VERIFY(slot_count > 0);
    VERIFY(!slot_aabb_buffer);

    Msg("* [DetailManager] Creating GPU slot AABB buffer...");

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(SlotAABB);
    desc.ByteWidth = slot_count * sizeof(SlotAABB);

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = slot_aabbs.data();

    HRESULT hr = HW.pDevice->CreateBuffer(&desc, &init_data, &slot_aabb_buffer);
    CHK_DX(hr);

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

void CDetailManager::CreateGPUCullingBuffers()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating GPU culling infrastructure...");

    cull_compute_shader.create("detail_cull");

    gpu_visible_buffer_capacity = 2048 * 2048;
    if (gpu_visible_buffer_capacity < 10000)
        gpu_visible_buffer_capacity = 10000;

    for (u32 i = 0; i < max_gpu_culled_objects; i++)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(InstanceData);
        desc.ByteWidth = gpu_visible_buffer_capacity * sizeof(InstanceData);

        CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &gpu_visible_buffers[i]));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
        CHK_DX(HW.pDevice->CreateShaderResourceView(gpu_visible_buffers[i], &srvDesc, &gpu_visible_srvs[i]));

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
        CHK_DX(HW.pDevice->CreateUnorderedAccessView(gpu_visible_buffers[i], &uavDesc, &gpu_visible_uavs[i]));
    }

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

    D3D11_BUFFER_DESC readbackDesc = counterDesc;
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.BindFlags = 0;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackDesc.MiscFlags = 0;
    CHK_DX(HW.pDevice->CreateBuffer(&readbackDesc, nullptr, &gpu_visible_counts_readback));

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = 256;
    CHK_DX(HW.pDevice->CreateBuffer(&cbDesc, nullptr, &cull_constant_buffer));

    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    for (u32 i = 0; i < max_gpu_culled_objects && i < objects.size(); i++)
    {
        CDetail& obj = *objects[i];

        IndirectDrawArgs initial_args = {};
        initial_args.IndexCountPerInstance = obj.number_indices;
        initial_args.InstanceCount = 0;
        initial_args.StartIndexLocation = vis_geometry_index_offsets[0][i];
        initial_args.BaseVertexLocation = 0;
        initial_args.StartInstanceLocation = 0;

        D3D11_BUFFER_DESC argsDesc = {};
        argsDesc.Usage = D3D11_USAGE_DEFAULT;
        argsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        argsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        argsDesc.ByteWidth = sizeof(IndirectDrawArgs);

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &initial_args;

        CHK_DX(HW.pDevice->CreateBuffer(&argsDesc, &initData, &gpu_indirect_args[i]));

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = 5;
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

    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

    DetailCullParams params = {};
    params.view_proj = Device.mFullTransform;
    params.camera_pos = Device.vCameraPosition;
    params.fade_distance_sqr = fade_distance * fade_distance;
    params.total_instance_count = total_instance_count;
    params.total_slot_count = slot_count;
    params.object_count = objects.size();

    for (u32 i = 0; i < 6; i++)
        params.frustum_planes[i].set(0, 0, 0, 1000000.0f);

    for (u32 i = 0; i < frustum.p_count && i < 6; i++)
    {
        params.frustum_planes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y,
                                      frustum.planes[i].n.z, frustum.planes[i].d);
    }

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

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(cull_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(cull_constant_buffer, 0);

    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    for (u32 i = 0; i < max_gpu_culled_objects && i < objects.size(); i++)
    {
        CDetail& obj = *objects[i];

        IndirectDrawArgs args = {};
        args.IndexCountPerInstance = obj.number_indices;
        args.InstanceCount = 0;
        args.StartIndexLocation = vis_geometry_index_offsets[0][i];
        args.BaseVertexLocation = 0;
        args.StartInstanceLocation = 0;

        context->UpdateSubresource(gpu_indirect_args[i], 0, nullptr, &args, 0, 0);
    }

    UINT zero_counts[64] = {0};
    context->ClearUnorderedAccessViewUint(gpu_visible_counts_uav, zero_counts);

    uint32_t zero = 0;
    context->UpdateSubresource(visible_slot_counter_gpu, 0, nullptr, &zero, 0, 0);

    context->CSSetConstantBuffers(0, 1, &cull_constant_buffer);

    ID3DShaderResourceView* srvs[2] = {slot_aabb_srv, persistent_instance_srv};
    context->CSSetShaderResources(0, 2, srvs);

    ID3DUnorderedAccessView* uavs[35];
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
        uavs[i] = gpu_visible_uavs[i];
    uavs[16] = gpu_visible_counts_uav;
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
        uavs[17 + i] = gpu_indirect_args_uavs[i];
    uavs[33] = visible_slot_ids_uav;
    uavs[34] = visible_slot_counter_uav;

    context->CSSetUnorderedAccessViews(0, 35, uavs, nullptr);

    context->CSSetShader(cull_compute_shader->sh, nullptr, 0);

    u32 num_groups = (slot_count + 255) / 256;
    context->Dispatch(num_groups, 1, 1);

    ID3DUnorderedAccessView* null_uavs[35] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 35, null_uavs, nullptr);

    ID3DShaderResourceView* null_srvs[2] = {nullptr, nullptr};
    context->CSSetShaderResources(0, 2, null_srvs);

    context->CSSetShader(nullptr, nullptr, 0);

    context->CopyResource(gpu_visible_counts_readback, gpu_visible_counts_buffer);
}

void CDetailManager::hw_Render_FullLevel(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

    RImplementation.BasicStats.DetailCount = 0;

    ProcessThreadSafeRequests();

    UpdatePageTable();

    UpdateIndirectionBuffer(cmd_list);

    UpdateInteractiveEntities(cmd_list);
    UpdateWind(cmd_list);
    RenderInteractions(cmd_list);

    DispatchGPUCulling(cmd_list);

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
    static shared_str strDir2D("dir2D");
    static shared_str strDir2D_2("dir2D_2");

    auto context = HW.get_context(cmd_list.context_id);

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

    u32 objects_to_render = _min(objects.size(), max_gpu_culled_objects);
    for (u32 O = 0; O < objects_to_render; O++)
    {
        CDetail& Object = *objects[O];

        cmd_list.set_CullMode(CULL_NONE);
        cmd_list.set_xform_world(Fidentity);
        cmd_list.SRVSManager.SetVSResource(0, gpu_visible_srvs[O]);
        cmd_list.set_Geometry(vis_unified_geom[0]);
        cmd_list.set_c(strConsts, consts);
        cmd_list.set_c(strWave, wave.div(PI_MUL_2));
        cmd_list.set_c(strDir2D, dir1);
        cmd_list.set_c(strDir2D_2, dir2);

        Fvector4 detail_params_vec;
        detail_params_vec.x = (float)dtH.x_size();
        detail_params_vec.y = (float)dtH.z_size();
        detail_params_vec.z = (float)dtH.x_offs();
        detail_params_vec.w = (float)dtH.z_offs();
        cmd_list.set_c(hwc_detail_params._get(), detail_params_vec);
        cmd_list.set_c(hwc_grass_wind_displacement._get(), ps_r3_grass_wind_displacement);
        cmd_list.set_c(hwc_grass_interaction_displacement._get(), ps_r3_grass_interaction_displacement);

        cmd_list.set_Element(Object.shader->E[0], 0);
        cmd_list.apply_lmaterial();

        if (interaction_srv)
            cmd_list.SRVSManager.SetVSResource(1, interaction_srv);
        if (wind_srv)
            cmd_list.SRVSManager.SetVSResource(2, wind_srv);
        if (indirection_srv)
            cmd_list.SRVSManager.SetVSResource(3, indirection_srv);
        if (interaction_sampler)
            HW.get_context(cmd_list.context_id)->VSSetSamplers(0, 1, &interaction_sampler);

        cmd_list.RenderIndexedInstancedIndirect(D3DPT_TRIANGLELIST, gpu_indirect_args[O], 0);

        cmd_list.set_CullMode(CULL_CCW);
    }

    cmd_list.SRVSManager.SetVSResource(0, nullptr);
    for (u32 i = 0; i < 16; i++)
        cmd_list.SRVSManager.SetPSResource(i, nullptr);

    RImplementation.BasicStats.DetailCount = total_instance_count;
}

void CDetailManager::hw_Render(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

#ifdef USE_DX11
    if (full_level_loaded)
    {
        hw_Render_FullLevel(cmd_list);
        return;
    }
#endif

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

    float scale = 1.f / float(quant);
    Fvector4 wave;
    Fvector4 consts;
    consts.set(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);

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

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");

    CDetail& Object = *objects[object_id];

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

    ID3DBuffer* currentBuffer = detailBuffer_vis[0];
    ID3DShaderResourceView* currentSRV = detailSRV_vis[0];
    u32 bufferSize = detailBufferSize_vis[0];

    if (totalInstanceCount > bufferSize)
    {
        Msg("! [DetailManager] Too many instances for object=%u: need %u, have %u. Clamping.",
            object_id, totalInstanceCount, bufferSize);
        totalInstanceCount = bufferSize;
    }

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

                Fvector3 hpb;
                hpb.x = atan2f(M._13, M._11);
                hpb.y = 0.0f;
                hpb.z = 0.0f;

                c_storage[instanceIdx].m0.set(M._11, M._21, M._31);
                c_storage[instanceIdx].m1.set(M._12, M._22, M._32);
                c_storage[instanceIdx].m2.set(M._13, M._23, M._33);
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

    cmd_list.set_CullMode(CULL_NONE);
    cmd_list.set_xform_world(Fidentity);
    cmd_list.SRVSManager.SetVSResource(0, currentSRV);
    cmd_list.set_Geometry(vis_unified_geom[0]);
    cmd_list.set_c(strConsts, consts);
    cmd_list.set_c(strWave, wave);
    cmd_list.set_c(strDir2D, wind);

    Fvector4 detail_params_vec;
    detail_params_vec.x = (float)dtH.x_size();
    detail_params_vec.y = (float)dtH.z_size();
    detail_params_vec.z = (float)dtH.x_offs();
    detail_params_vec.w = (float)dtH.z_offs();
    cmd_list.set_c(hwc_detail_params._get(), detail_params_vec);
    cmd_list.set_c(hwc_grass_wind_displacement._get(), ps_r3_grass_wind_displacement);
    cmd_list.set_c(hwc_grass_interaction_displacement._get(), ps_r3_grass_interaction_displacement);

    cmd_list.set_Element(Object.shader->E[0], 0);
    cmd_list.apply_lmaterial();

    if (interaction_srv)
        cmd_list.SRVSManager.SetVSResource(1, interaction_srv);
    if (wind_srv)
        cmd_list.SRVSManager.SetVSResource(2, wind_srv);
    if (indirection_srv)
        cmd_list.SRVSManager.SetVSResource(3, indirection_srv);
    if (interaction_sampler)
        HW.get_context(cmd_list.context_id)->VSSetSamplers(0, 1, &interaction_sampler);

    u32 baseIndex = vis_geometry_index_offsets[0][object_id];
    u32 numVertices = Object.number_vertices;
    u32 numIndices = Object.number_indices;

    cmd_list.RenderInstancedIndexed(
        D3DPT_TRIANGLELIST,
        0, 0,
        numVertices,
        baseIndex,
        numIndices / 3,
        instanceIdx,
        0);

    cmd_list.stat.r.s_details.add(numVertices * instanceIdx);

    cmd_list.set_CullMode(CULL_CCW);
}

void CDetailManager::CreateInteractionAtlas()
{
    VERIFY(!interaction_atlas);
    VERIFY(slot_count > 0);

    Msg("* [DetailManager] Creating interaction atlas...");

    atlas_width = 2048;
    atlas_height = 2048;
    slot_texture_size = 32;

    u32 slots_per_row = atlas_width / slot_texture_size;
    u32 max_atlas_slots = slots_per_row * (atlas_height / slot_texture_size);

    if (slot_count > max_atlas_slots)
    {
        Msg("! [DetailManager] WARNING: %u world slots exceeds atlas capacity %u", slot_count, max_atlas_slots);
        Msg("! [DetailManager] Using modulo wrapping - only slots near camera will have accurate interaction");
        Msg("! [DetailManager] This is expected for large levels (atlas tracks ~%u slots at a time)", max_atlas_slots);
    }

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = atlas_width;
    tex_desc.Height = atlas_height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    CHK_DX(HW.pDevice->CreateTexture2D(&tex_desc, nullptr, &interaction_atlas));

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = tex_desc.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateRenderTargetView(interaction_atlas, &rtv_desc, &interaction_rtv));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    CHK_DX(HW.pDevice->CreateShaderResourceView(interaction_atlas, &srv_desc, &interaction_srv));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = tex_desc.Format;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(interaction_atlas, &uav_desc, &interaction_uav));

    float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    HW.get_context(CHW::IMM_CTX_ID)->ClearRenderTargetView(interaction_rtv, clear_color);

    xr_vector<Fvector4> slot_uvs;
    u32 uv_buffer_size = std::min(slot_count, max_atlas_slots);
    slot_uvs.resize(uv_buffer_size);

    for (u32 i = 0; i < uv_buffer_size; i++)
    {
        u32 slot_x = i % slots_per_row;
        u32 slot_z = i / slots_per_row;

        float u_min = (slot_x * slot_texture_size) / (float)atlas_width;
        float v_min = (slot_z * slot_texture_size) / (float)atlas_height;
        float u_max = ((slot_x + 1) * slot_texture_size) / (float)atlas_width;
        float v_max = ((slot_z + 1) * slot_texture_size) / (float)atlas_height;

        slot_uvs[i].set(u_min, v_min, u_max, v_max);
    }

    D3D11_BUFFER_DESC buf_desc = {};
    buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
    buf_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buf_desc.ByteWidth = uv_buffer_size * sizeof(Fvector4);
    buf_desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = slot_uvs.data();

    CHK_DX(HW.pDevice->CreateBuffer(&buf_desc, &init_data, &slot_atlas_uv_buffer));

    D3D11_SHADER_RESOURCE_VIEW_DESC uv_srv_desc = {};
    uv_srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uv_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    uv_srv_desc.Buffer.FirstElement = 0;
    uv_srv_desc.Buffer.NumElements = uv_buffer_size;
    CHK_DX(HW.pDevice->CreateShaderResourceView(slot_atlas_uv_buffer, &uv_srv_desc, &slot_atlas_uv_srv));

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 1;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 0;
    CHK_DX(HW.pDevice->CreateSamplerState(&sampler_desc, &interaction_sampler));

    float memory_mb = (atlas_width * atlas_height * 8) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] Interaction atlas created:");
    Msg("  - Resolution: %ux%u", atlas_width, atlas_height);
    Msg("  - Slot texture size: %ux%u", slot_texture_size, slot_texture_size);
    Msg("  - Atlas capacity: %u slots", max_atlas_slots);
    Msg("  - World slots: %u (wrapping enabled for large levels)", slot_count);
    Msg("  - VRAM usage: %.2f MB", memory_mb);
}

void CDetailManager::DestroyInteractionAtlas()
{
    _RELEASE(interaction_uav);
    _RELEASE(interaction_srv);
    _RELEASE(interaction_rtv);
    _RELEASE(interaction_atlas);
    _RELEASE(slot_atlas_uv_srv);
    _RELEASE(slot_atlas_uv_buffer);
    _RELEASE(interaction_sampler);

    interaction_update_cs.destroy();
    _RELEASE(interaction_update_cb);
}

void CDetailManager::CreateEntityTrackingBuffers()
{
    VERIFY(!entity_buffer);

    max_entities = 256;

    Msg("* [DetailManager] Creating entity tracking buffers...");

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(InteractiveEntity);
    desc.ByteWidth = max_entities * sizeof(InteractiveEntity);

    CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &entity_buffer));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = max_entities;
    CHK_DX(HW.pDevice->CreateShaderResourceView(entity_buffer, &srv_desc, &entity_srv));

    entity_count_this_frame = 0;

    Msg("* [DetailManager] Entity tracking buffers created (max %u entities)", max_entities);

    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    struct InteractionParams
    {
        u32 entity_count;
        float time;
        float delta_time;
        float decay_rate;
        u32 slot_count;
        u32 slots_per_row;
        u32 slot_texture_size;
        u32 atlas_width;
    };
    static_assert(sizeof(InteractionParams) == 32, "InteractionParams size check");
    cb_desc.ByteWidth = ((sizeof(InteractionParams) + 15) / 16) * 16;
    CHK_DX(HW.pDevice->CreateBuffer(&cb_desc, nullptr, &interaction_constant_buffer));

    Msg("* [DetailManager] Interaction constant buffer created");
}

void CDetailManager::DestroyEntityTrackingBuffers()
{
    _RELEASE(entity_srv);
    _RELEASE(entity_buffer);
    interactive_entities.clear();
}

void CDetailManager::CreateWindTexture()
{
    VERIFY(!wind_texture);

    wind_texture_size = 512;

    Msg("* [DetailManager] Creating wind texture...");

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = wind_texture_size;
    tex_desc.Height = wind_texture_size;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    CHK_DX(HW.pDevice->CreateTexture2D(&tex_desc, nullptr, &wind_texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    CHK_DX(HW.pDevice->CreateShaderResourceView(wind_texture, &srv_desc, &wind_srv));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = tex_desc.Format;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(wind_texture, &uav_desc, &wind_uav));

    float memory_mb = (wind_texture_size * wind_texture_size * 8) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] Wind texture created:");
    Msg("  - Resolution: %ux%u", wind_texture_size, wind_texture_size);
    Msg("  - VRAM usage: %.2f MB", memory_mb);

    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    struct WindParams
    {
        float time;
        Fvector2 wind_direction;
        float wind_speed;
        u32 octaves;
        float lacunarity;
        float gain;
        float padding1;
        Fvector2 scroll_speed;
        u32 texture_size;
        u32 pad[3];
    };
    static_assert(sizeof(WindParams) == 56, "WindParams size check");
    cb_desc.ByteWidth = ((sizeof(WindParams) + 15) / 16) * 16;
    CHK_DX(HW.pDevice->CreateBuffer(&cb_desc, nullptr, &wind_constant_buffer));

    Msg("* [DetailManager] Wind constant buffer created");
}

void CDetailManager::DestroyWindTexture()
{
    _RELEASE(wind_uav);
    _RELEASE(wind_srv);
    _RELEASE(wind_texture);
}

void CDetailManager::UpdateInteractiveEntities(CBackend& cmd_list)
{
    if (!entity_buffer)
        return;

    interactive_entities.clear();

    xr_vector<GrassInteractionEntity> game_entities;
    g_GrassInteractionCollector.GetEntitiesForFrame(game_entities);

    for (const auto& game_entity : game_entities)
    {
        InteractiveEntity render_entity;
        render_entity.position = game_entity.position;
        render_entity.radius = game_entity.radius;
        render_entity.velocity = game_entity.velocity;
        render_entity.weight = game_entity.weight;
        render_entity.direction = game_entity.direction;
        render_entity.padding = 0.0f;

        interactive_entities.push_back(render_entity);

        if (interactive_entities.size() >= max_entities)
            break;
    }

    entity_count_this_frame = interactive_entities.size();

    if (entity_count_this_frame > 0 && entity_count_this_frame <= max_entities)
    {
        auto context = HW.get_context(cmd_list.context_id);
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(entity_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, interactive_entities.data(),
                   entity_count_this_frame * sizeof(InteractiveEntity));
            context->Unmap(entity_buffer, 0);
        }
    }
}

void CDetailManager::RenderInteractions(CBackend& cmd_list)
{
    if (!interaction_atlas || entity_count_this_frame == 0)
        return;

    if (!interaction_compute_shader)
    {
        Msg("! [DetailManager] Interaction compute shader not loaded!");
        return;
    }

    static bool first_run = true;
    static u32 frame_counter = 0;
    if (first_run || (frame_counter++ % 60 == 0))
    {
        Msg("* [DetailManager] RenderInteractions: entities=%u, slots=%u, atlas=%ux%u, slot_size=%u",
            entity_count_this_frame, slot_count, atlas_width, atlas_height, slot_texture_size);
        if (entity_count_this_frame > 0)
        {
            Msg("  Entity[0]: pos=(%.1f, %.1f, %.1f), radius=%.2f, vel=(%.2f, %.2f, %.2f)",
                interactive_entities[0].position.x, interactive_entities[0].position.y, interactive_entities[0].position.z,
                interactive_entities[0].radius,
                interactive_entities[0].velocity.x, interactive_entities[0].velocity.y, interactive_entities[0].velocity.z);
        }
        first_run = false;
    }

    auto context = HW.get_context(cmd_list.context_id);

    context->CSSetShader(interaction_compute_shader->sh, nullptr, 0);

    struct InteractionParams
    {
        u32 entity_count;
        float time;
        float delta_time;
        float decay_rate;
        u32 slot_count;
        u32 slots_per_row;
        u32 slot_texture_size;
        u32 atlas_width;
    };

    InteractionParams params;
    params.entity_count = entity_count_this_frame;
    params.time = Device.fTimeGlobal;
    params.delta_time = Device.fTimeDelta;
    params.decay_rate = powf(0.05f, Device.fTimeDelta);
    params.slot_count = slot_count;
    params.slots_per_row = atlas_width / slot_texture_size;
    params.slot_texture_size = slot_texture_size;
    params.atlas_width = atlas_width;

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(interaction_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(interaction_constant_buffer, 0);

    context->CSSetConstantBuffers(0, 1, &interaction_constant_buffer);
    context->CSSetShaderResources(0, 1, &entity_srv);
    context->CSSetShaderResources(1, 1, &slot_aabb_srv);
    context->CSSetShaderResources(2, 1, &physical_to_logical_srv);
    context->CSSetUnorderedAccessViews(0, 1, &interaction_uav, nullptr);

    u32 num_groups_x = (atlas_width + 31) / 32;
    u32 num_groups_y = (atlas_height + 31) / 32;

    if (frame_counter < 120)
        Msg("* [DetailManager] Dispatching interaction compute: groups=%ux%u, threads=%ux%u",
            num_groups_x, num_groups_y, num_groups_x * 32, num_groups_y * 32);

    context->Dispatch(num_groups_x, num_groups_y, 1);

    ID3DShaderResourceView* nullSRV[3] = {nullptr, nullptr, nullptr};
    ID3DUnorderedAccessView* nullUAV[1] = {nullptr};
    context->CSSetShaderResources(0, 3, nullSRV);
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
}

void CDetailManager::UpdateWind(CBackend& cmd_list)
{
    if (g_pGamePersistent)
    {
        wind_speed = _max(g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier, ps_r3_grass_wind_min);

        const float envDir = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
        float wind_rad = deg2rad(envDir);
        wind_direction = { cosf(wind_rad), sinf(wind_rad) };
    }
    else
    {
        wind_speed = 0.5f;
        wind_direction.set(1.0f, 0.0f);
    }

    auto context = HW.get_context(cmd_list.context_id);

    context->CSSetShader(wind_compute_shader->sh, nullptr, 0);

    WindParams params;
    memset(&params, 0, sizeof(params));
    params.time = Device.fTimeGlobal;
    params.wind_direction = wind_direction;
    params.wind_speed = wind_speed;
    params.octaves = ps_r3_grass_wind_octaves;
    params.lacunarity = 2.0f;
    params.gain = 0.5f;
    params.scroll_speed.x = wind_direction.x * wind_speed * 5.0f;
    params.scroll_speed.y = wind_direction.y * wind_speed * 5.0f;
    params.texture_size = wind_texture_size;

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(wind_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(wind_constant_buffer, 0);

    context->CSSetConstantBuffers(0, 1, &wind_constant_buffer);
    context->CSSetUnorderedAccessViews(0, 1, &wind_uav, nullptr);

    u32 num_groups = (wind_texture_size + 15) / 16;
    context->Dispatch(num_groups, num_groups, 1);

    ID3DUnorderedAccessView* nullUAV[1] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
}

void CDetailManager::InitializePageTable()
{
    Msg("* [DetailManager] Initializing virtual page table...");

    page_table.resize(slot_count);

    for (uint32_t i = 0; i < slot_count; i++) {
        page_table[i].physical_page = INVALID_PAGE;
        page_table[i].mip_level = 0;
        page_table[i].reference_bit = 0;
        page_table[i].dirty_bit = 0;
        page_table[i].locked_bit = 0;
        page_table[i].last_access_frame = 0;
    }

    for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
        physical_pages[i].logical_slot = UINT32_MAX;
        physical_pages[i].reference_bit = 0;
        physical_pages[i].locked = 0;
    }

    clock_hand = 0;
    resident_page_count = 0;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.ByteWidth = slot_count * sizeof(uint32_t);

    xr_vector<uint32_t> initial_data(slot_count, 0xFFFFFFFF);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = initial_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&bufferDesc, &initData, &indirection_buffer));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = slot_count;

    CHK_DX(HW.pDevice->CreateShaderResourceView(indirection_buffer, &srvDesc, &indirection_srv));

    D3D11_BUFFER_DESC revmapDesc = {};
    revmapDesc.Usage = D3D11_USAGE_DYNAMIC;
    revmapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    revmapDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    revmapDesc.MiscFlags = 0;
    revmapDesc.ByteWidth = PHYSICAL_PAGES * sizeof(uint32_t);

    xr_vector<uint32_t> revmap_data(PHYSICAL_PAGES, 0xFFFFFFFF);
    D3D11_SUBRESOURCE_DATA revmapInitData = {};
    revmapInitData.pSysMem = revmap_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&revmapDesc, &revmapInitData, &physical_to_logical_buffer));

    D3D11_SHADER_RESOURCE_VIEW_DESC revmapSrvDesc = {};
    revmapSrvDesc.Format = DXGI_FORMAT_R32_UINT;
    revmapSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    revmapSrvDesc.Buffer.FirstElement = 0;
    revmapSrvDesc.Buffer.NumElements = PHYSICAL_PAGES;

    CHK_DX(HW.pDevice->CreateShaderResourceView(physical_to_logical_buffer, &revmapSrvDesc, &physical_to_logical_srv));

    ResetPageTableStats();

    Msg("* [DetailManager] Page table initialized: %u logical slots -> %u physical pages",
        slot_count, PHYSICAL_PAGES);
}

void CDetailManager::ShutdownPageTable()
{
    Msg("* [DetailManager] Shutting down page table...");

    _RELEASE(indirection_srv);
    _RELEASE(indirection_buffer);
    _RELEASE(physical_to_logical_srv);
    _RELEASE(physical_to_logical_buffer);

    page_table.clear();
    page_table.shrink_to_fit();

    Msg("* [DetailManager] Page table shutdown complete");
}

void CDetailManager::InitializeVisibilityReadback()
{
    Msg("* [DetailManager] Initializing visibility readback...");

    D3D11_BUFFER_DESC readbackDesc = {};
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.BindFlags = 0;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackDesc.MiscFlags = 0;
    readbackDesc.ByteWidth = MAX_VISIBLE_SLOTS * sizeof(uint32_t);

    CHK_DX(HW.pDevice->CreateBuffer(&readbackDesc, nullptr, &visible_slots_readback));

    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_EVENT;
    CHK_DX(HW.pDevice->CreateQuery(&queryDesc, &readback_query));

    visible_slots_cache.reserve(MAX_VISIBLE_SLOTS);
    visible_slot_count = 0;

    max_uploads_per_frame = 16;

    D3D11_BUFFER_DESC visibleSlotsDesc = {};
    visibleSlotsDesc.Usage = D3D11_USAGE_DEFAULT;
    visibleSlotsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    visibleSlotsDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    visibleSlotsDesc.StructureByteStride = sizeof(uint32_t);
    visibleSlotsDesc.ByteWidth = MAX_VISIBLE_SLOTS * sizeof(uint32_t);

    CHK_DX(HW.pDevice->CreateBuffer(&visibleSlotsDesc, nullptr, &visible_slot_ids_gpu));

    D3D11_UNORDERED_ACCESS_VIEW_DESC visibleSlotsUAVDesc = {};
    visibleSlotsUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    visibleSlotsUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    visibleSlotsUAVDesc.Buffer.FirstElement = 0;
    visibleSlotsUAVDesc.Buffer.NumElements = MAX_VISIBLE_SLOTS;

    CHK_DX(HW.pDevice->CreateUnorderedAccessView(
        visible_slot_ids_gpu, &visibleSlotsUAVDesc, &visible_slot_ids_uav));

    D3D11_BUFFER_DESC counterDesc = {};
    counterDesc.Usage = D3D11_USAGE_DEFAULT;
    counterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    counterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    counterDesc.ByteWidth = sizeof(uint32_t);

    uint32_t zero = 0;
    D3D11_SUBRESOURCE_DATA counterInitData = {};
    counterInitData.pSysMem = &zero;

    CHK_DX(HW.pDevice->CreateBuffer(&counterDesc, &counterInitData, &visible_slot_counter_gpu));

    D3D11_UNORDERED_ACCESS_VIEW_DESC counterUAVDesc = {};
    counterUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    counterUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    counterUAVDesc.Buffer.FirstElement = 0;
    counterUAVDesc.Buffer.NumElements = 1;
    counterUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    CHK_DX(HW.pDevice->CreateUnorderedAccessView(
        visible_slot_counter_gpu, &counterUAVDesc, &visible_slot_counter_uav));

    D3D11_BUFFER_DESC stagingDesc = {};
    stagingDesc.Usage = D3D11_USAGE_DYNAMIC;
    stagingDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.ByteWidth = STAGING_BUFFER_SIZE;

    CHK_DX(HW.pDevice->CreateBuffer(&stagingDesc, nullptr, &upload_staging_buffer));

    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(HW.get_context(CHW::IMM_CTX_ID)->Map(upload_staging_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    staging_buffer_mapped = (uint8_t*)mapped.pData;

    pending_updates.reserve(4096);
    max_pending_updates = 4096;
    ResetALifeStats();

    Msg("* [DetailManager] Visibility readback initialized (max %u slots, %u MB staging)",
        MAX_VISIBLE_SLOTS, STAGING_BUFFER_SIZE / (1024*1024));
    Msg("* [DetailManager] A-Life integration initialized (max %u pending updates)", max_pending_updates);
}

void CDetailManager::ShutdownVisibilityReadback()
{
    Msg("* [DetailManager] Shutting down visibility readback...");

    if (upload_staging_buffer) {
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(upload_staging_buffer, 0);
        staging_buffer_mapped = nullptr;
    }

    _RELEASE(upload_staging_buffer);
    _RELEASE(visible_slot_counter_uav);
    _RELEASE(visible_slot_counter_gpu);
    _RELEASE(visible_slot_ids_uav);
    _RELEASE(visible_slot_ids_gpu);
    _RELEASE(readback_query);
    _RELEASE(visible_slots_readback);

    visible_slots_cache.clear();
    visible_slots_cache.shrink_to_fit();

    while (!upload_queue.empty()) {
        upload_queue.pop();
    }

    pending_updates.clear();
    pending_updates.shrink_to_fit();

    Msg("* [DetailManager] Visibility readback shutdown complete");
    Msg("* [DetailManager] A-Life integration shutdown complete");
}

void CDetailManager::ReadVisibleSlotsFromGPU()
{
    uint32_t counter_value = 0;
    D3D11_BOX counterBox = { 0, 0, 0, sizeof(uint32_t), 1, 1 };
    HW.get_context(CHW::IMM_CTX_ID)->CopySubresourceRegion(
        visible_slots_readback, 0, 0, 0, 0,
        visible_slot_counter_gpu, 0, &counterBox);

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = HW.get_context(CHW::IMM_CTX_ID)->Map(visible_slots_readback, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        counter_value = *(uint32_t*)mapped.pData;
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(visible_slots_readback, 0);
    }

    visible_slot_count = std::min(counter_value, MAX_VISIBLE_SLOTS);

    if (visible_slot_count == 0) {
        return;
    }

    HW.get_context(CHW::IMM_CTX_ID)->CopyResource(visible_slots_readback, visible_slot_ids_gpu);

    HW.get_context(CHW::IMM_CTX_ID)->End(readback_query);

    BOOL data_ready = FALSE;
    hr = HW.get_context(CHW::IMM_CTX_ID)->GetData(readback_query, &data_ready, sizeof(BOOL), 0);
    if (SUCCEEDED(hr) && data_ready) {
        hr = HW.get_context(CHW::IMM_CTX_ID)->Map(visible_slots_readback, 0, D3D11_MAP_READ, 0, &mapped);
        if (SUCCEEDED(hr)) {
            visible_slots_cache.clear();
            uint32_t* slot_ids = (uint32_t*)mapped.pData;

            for (uint32_t i = 0; i < visible_slot_count; i++) {
                visible_slots_cache.push_back(slot_ids[i]);
            }

            HW.get_context(CHW::IMM_CTX_ID)->Unmap(visible_slots_readback, 0);
        }
    }
}

void CDetailManager::RequestVisiblePages()
{
    for (uint32_t slot_id : visible_slots_cache) {
        if (slot_id >= slot_count) continue;

        RequestPageWithPriority(slot_id, 255);
    }
}

void CDetailManager::RequestPageWithPriority(uint32_t logical_slot, uint8_t priority)
{
    if (logical_slot >= slot_count) return;

    if (IsPageResident(logical_slot)) {
        PageTableEntry& entry = page_table[logical_slot];
        entry.reference_bit = 1;
        entry.last_access_frame = Device.dwFrame;

        PhysicalPageInfo& phys = physical_pages[entry.physical_page];
        phys.reference_bit = 1;

        page_table_stats.cache_hits++;
        return;
    }

    page_table_stats.cache_misses++;

    PageUploadRequest request;
    request.logical_slot = logical_slot;
    request.priority = priority;
    request.request_frame = Device.dwFrame;

    upload_queue.push(request);
}

void CDetailManager::ProcessUploadQueue()
{
    uint32_t uploads_this_frame = 0;

    while (!upload_queue.empty() && uploads_this_frame < max_uploads_per_frame) {
        PageUploadRequest request = upload_queue.top();
        upload_queue.pop();

        uint32_t logical_slot = request.logical_slot;

        if (IsPageResident(logical_slot)) {
            continue;
        }

        uint16_t physical_page;
        if (resident_page_count < PHYSICAL_PAGES) {
            for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
                if (physical_pages[i].logical_slot == UINT32_MAX) {
                    physical_page = i;
                    break;
                }
            }
        } else {
            physical_page = FindVictimPage();
            EvictPage(physical_page);
        }

        PromotePage(logical_slot, physical_page);

        uploads_this_frame++;
    }

    if (uploads_this_frame > 0) {
        Msg("* [PageTable] Processed %u page uploads this frame", uploads_this_frame);
    }
}

void CDetailManager::ResetPageTableStats()
{
    page_table_stats.total_requests = 0;
    page_table_stats.cache_hits = 0;
    page_table_stats.cache_misses = 0;
    page_table_stats.evictions = 0;
    page_table_stats.current_resident = 0;
}

void CDetailManager::PrintPageTableStats()
{
    float hit_rate = 0.0f;
    if (page_table_stats.total_requests > 0) {
        hit_rate = 100.0f * float(page_table_stats.cache_hits) / float(page_table_stats.total_requests);
    }

    float atlas_usage = 100.0f * float(resident_page_count) / float(PHYSICAL_PAGES);
    float cull_percent = 100.0f * (1.0f - float(visible_slot_count) / float(slot_count));

    Msg("=== Page Table & GPU Culling Statistics ===");
    Msg("GPU Culling:");
    Msg("  Visible Slots:    %u / %u (%.1f%% culled)", visible_slot_count, slot_count, cull_percent);
    Msg("  Total Slots:      %u", slot_count);
    Msg("");
    Msg("Page Table:");
    Msg("  Total Requests:   %llu", page_table_stats.total_requests);
    Msg("  Cache Hits:       %llu (%.1f%%)", page_table_stats.cache_hits, hit_rate);
    Msg("  Cache Misses:     %llu", page_table_stats.cache_misses);
    Msg("  Evictions:        %llu", page_table_stats.evictions);
    Msg("  Resident Pages:   %u / %u (%.1f%% full)", resident_page_count, PHYSICAL_PAGES, atlas_usage);
    Msg("  Upload Queue:     %zu pending", upload_queue.size());
    Msg("  Max Uploads/Frame: %u", max_uploads_per_frame);
    Msg("===========================================");
}

bool CDetailManager::IsPageResident(uint32_t logical_slot) const
{
    if (logical_slot >= slot_count)
        return false;

    return page_table[logical_slot].physical_page != INVALID_PAGE;
}

uint16_t CDetailManager::FindVictimPage()
{
    int attempts = 0;
    const int max_attempts = PHYSICAL_PAGES * 2;

    while (attempts < max_attempts) {
        PhysicalPageInfo& page = physical_pages[clock_hand];

        if (page.locked) {
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            attempts++;
            continue;
        }

        if (page.logical_slot == UINT32_MAX) {
            uint16_t victim = clock_hand;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            return victim;
        }

        if (page.reference_bit) {
            page.reference_bit = 0;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            attempts++;
        } else {
            uint16_t victim = clock_hand;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            return victim;
        }
    }

    Msg("! [DetailManager] WARNING: Clock algorithm failed, using fallback eviction");

    uint16_t oldest = 0;
    uint64_t oldest_frame = UINT64_MAX;
    for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
        if (!physical_pages[i].locked && physical_pages[i].logical_slot != UINT32_MAX) {
            uint32_t logical = physical_pages[i].logical_slot;
            if (page_table[logical].last_access_frame < oldest_frame) {
                oldest_frame = page_table[logical].last_access_frame;
                oldest = i;
            }
        }
    }

    return oldest;
}

void CDetailManager::EvictPage(uint16_t physical_page)
{
    VERIFY(physical_page < PHYSICAL_PAGES);

    PhysicalPageInfo& page = physical_pages[physical_page];

    if (page.logical_slot == UINT32_MAX)
        return;

    uint32_t logical_slot = page.logical_slot;
    VERIFY(logical_slot < slot_count);

    page_table[logical_slot].physical_page = INVALID_PAGE;
    page_table[logical_slot].reference_bit = 0;
    page_table[logical_slot].dirty_bit = 0;

    page.logical_slot = UINT32_MAX;
    page.reference_bit = 0;

    resident_page_count--;
    page_table_stats.evictions++;

}

void CDetailManager::PromotePage(uint32_t logical_slot, uint16_t physical_page)
{
    VERIFY(logical_slot < slot_count);
    VERIFY(physical_page < PHYSICAL_PAGES);

    if (physical_pages[physical_page].logical_slot != UINT32_MAX) {
        EvictPage(physical_page);
    }

    physical_pages[physical_page].logical_slot = logical_slot;
    physical_pages[physical_page].reference_bit = 1;

    page_table[logical_slot].physical_page = physical_page;
    page_table[logical_slot].reference_bit = 1;
    page_table[logical_slot].last_access_frame = Device.dwFrame;
    page_table[logical_slot].dirty_bit = 0;

    resident_page_count++;

    ApplyPendingUpdatesToSlot(logical_slot);

}

uint16_t CDetailManager::RequestPage(uint32_t logical_slot, uint8_t priority)
{
    VERIFY(logical_slot < slot_count);

    page_table_stats.total_requests++;

    if (page_table[logical_slot].physical_page != INVALID_PAGE) {
        uint16_t physical_page = page_table[logical_slot].physical_page;

        page_table[logical_slot].reference_bit = 1;
        page_table[logical_slot].last_access_frame = Device.dwFrame;
        physical_pages[physical_page].reference_bit = 1;

        page_table_stats.cache_hits++;
        return physical_page;
    }

    page_table_stats.cache_misses++;

    uint16_t physical_page = FindVictimPage();

    PromotePage(logical_slot, physical_page);

    return physical_page;
}

void CDetailManager::UpdateIndirectionBuffer(CBackend& cmd_list)
{
    if (!indirection_buffer)
        return;

    auto context = HW.get_context(cmd_list.context_id);

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(indirection_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (FAILED(hr)) {
        Msg("! [DetailManager] Failed to map indirection buffer");
        return;
    }

    uint32_t* data = (uint32_t*)mapped.pData;

    for (uint32_t i = 0; i < slot_count; i++) {
        uint32_t packed = page_table[i].physical_page;
        packed |= (uint32_t(page_table[i].mip_level) << 16);

        data[i] = packed;
    }

    context->Unmap(indirection_buffer, 0);

    if (physical_to_logical_buffer)
    {
        D3D11_MAPPED_SUBRESOURCE revmap_mapped;
        hr = context->Map(physical_to_logical_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &revmap_mapped);

        if (SUCCEEDED(hr)) {
            uint32_t* revmap_data = (uint32_t*)revmap_mapped.pData;

            for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
                revmap_data[i] = 0xFFFFFFFF;
            }

            for (uint16_t phys_page = 0; phys_page < PHYSICAL_PAGES; phys_page++) {
                uint32_t logical_slot = physical_pages[phys_page].logical_slot;
                if (logical_slot != UINT32_MAX) {
                    revmap_data[phys_page] = logical_slot;
                }
            }

            context->Unmap(physical_to_logical_buffer, 0);
        }
    }

}

void CDetailManager::UpdatePageTable()
{

    ReadVisibleSlotsFromGPU();

    RequestVisiblePages();

    int x_offs = dtH.x_offs();
    int z_offs = dtH.z_offs();
    uint32_t x_size = dtH.x_size();
    uint32_t z_size = dtH.z_size();

    for (const auto& entity : interactive_entities) {
        int slot_x = int(floorf(entity.position.x / DETAIL_SLOT_SIZE));
        int slot_z = int(floorf(entity.position.z / DETAIL_SLOT_SIZE));

        int sx_local = slot_x + x_offs;
        int sz_local = slot_z + z_offs;

        if (sx_local >= 0 && sx_local < (int)x_size && sz_local >= 0 && sz_local < (int)z_size) {
            uint32_t slot_idx = sz_local * x_size + sx_local;
            if (slot_idx < slot_count) {
                RequestPageWithPriority(slot_idx, 255);
            }
        }
    }

    ProcessPendingUpdates();

    ProcessUploadQueue();

    page_table_stats.total_requests += visible_slot_count;
    page_table_stats.current_resident = resident_page_count;

    static u32 last_log_frame = 0;
    static u32 last_visible_count = 0;

    if (Device.dwFrame - last_log_frame >= 60) {
        float cull_percent = 100.0f * (1.0f - float(visible_slot_count) / float(slot_count));
        int delta = int(visible_slot_count) - int(last_visible_count);

        Msg("* [GPU Culling] Visible: %u / %u (%.1f%% culled, %+d), Resident: %u / %u, Queue: %zu",
            visible_slot_count, slot_count, cull_percent, delta,
            resident_page_count, PHYSICAL_PAGES, upload_queue.size());

        last_log_frame = Device.dwFrame;
        last_visible_count = visible_slot_count;
    }
}

void CDetailManager::ResetALifeStats()
{
    alife_stats.total_updates_requested = 0;
    alife_stats.immediate_updates = 0;
    alife_stats.deferred_updates = 0;
    alife_stats.applied_deferred = 0;
    alife_stats.expired_updates = 0;
}

void CDetailManager::PrintALifeStats()
{
    Msg("=== A-Life Integration Statistics ===");
    Msg("Total Requests:   %llu", alife_stats.total_updates_requested);
    Msg("Immediate:        %llu (%.1f%%)",
        alife_stats.immediate_updates,
        100.0f * alife_stats.immediate_updates / std::max(1ull, alife_stats.total_updates_requested));
    Msg("Deferred:         %llu (%.1f%%)",
        alife_stats.deferred_updates,
        100.0f * alife_stats.deferred_updates / std::max(1ull, alife_stats.total_updates_requested));
    Msg("Applied Deferred: %llu", alife_stats.applied_deferred);
    Msg("Expired:          %llu", alife_stats.expired_updates);
    Msg("Pending Queue:    %zu", pending_updates.size());
    Msg("====================================");
}

bool CDetailManager::IsSlotDirty(uint32_t logical_slot) const
{
    if (logical_slot >= slot_count) return false;

    if (IsPageResident(logical_slot) && page_table[logical_slot].dirty_bit) {
        return true;
    }

    for (const auto& update : pending_updates) {
        if (update.logical_slot == logical_slot) {
            return true;
        }
    }

    return false;
}

void CDetailManager::RequestInteractionUpdate(
    const Fvector& world_pos,
    float radius,
    float strength,
    uint8_t type)
{
    alife_stats.total_updates_requested++;

    const float slot_half = DETAIL_SLOT_SIZE_2;
    uint32_t slot_x = uint32_t((world_pos.x + slot_half) / DETAIL_SLOT_SIZE);
    uint32_t slot_z = uint32_t((world_pos.z + slot_half) / DETAIL_SLOT_SIZE);

    uint32_t logical_slot = slot_z * 600 + slot_x;

    if (logical_slot >= slot_count) {
        return;
    }

    if (IsPageResident(logical_slot)) {
        alife_stats.immediate_updates++;

        uint16_t physical_page = page_table[logical_slot].physical_page;

        ApplyInteractionUpdateGPU(physical_page, world_pos, radius, strength);

        page_table[logical_slot].dirty_bit = 1;

        Msg("* [A-Life] IMMEDIATE update at slot %u (phys page %u) - pos (%.1f, %.1f, %.1f) r=%.2f s=%.2f",
            logical_slot, physical_page, world_pos.x, world_pos.y, world_pos.z, radius, strength);

    } else {
        alife_stats.deferred_updates++;

        PendingInteractionUpdate pending;
        pending.logical_slot = logical_slot;
        pending.world_position = world_pos;
        pending.radius = radius;
        pending.strength = strength;
        pending.timestamp = Device.dwFrame;
        pending.interaction_type = type;

        pending_updates.push_back(pending);

        if (pending_updates.size() > max_pending_updates) {
            pending_updates.erase(pending_updates.begin());
            alife_stats.expired_updates++;
        }

        RequestPageWithPriority(logical_slot, PRIORITY_DIRTY);

        Msg("* [A-Life] DEFERRED update queued for slot %u - pos (%.1f, %.1f, %.1f) r=%.2f s=%.2f (queue size: %zu)",
            logical_slot, world_pos.x, world_pos.y, world_pos.z, radius, strength, pending_updates.size());
    }
}

void CDetailManager::ExpireOldPendingUpdates(uint64_t max_age_frames)
{
    uint64_t current_frame = Device.dwFrame;
    uint32_t expired_count = 0;

    auto it = pending_updates.begin();
    while (it != pending_updates.end()) {
        uint64_t age = current_frame - it->timestamp;

        if (age > max_age_frames) {
            alife_stats.expired_updates++;
            expired_count++;
            it = pending_updates.erase(it);
        } else {
            ++it;
        }
    }

    if (expired_count > 0) {
        Msg("* [A-Life] Expired %u old pending updates (age > %llu frames, queue remaining: %zu)",
            expired_count, max_age_frames, pending_updates.size());
    }
}

void CDetailManager::ApplyPendingUpdatesToSlot(uint32_t logical_slot)
{
    if (!IsPageResident(logical_slot)) {
        return;
    }

    uint16_t physical_page = page_table[logical_slot].physical_page;

    uint32_t applied_count = 0;
    auto it = pending_updates.begin();
    while (it != pending_updates.end()) {
        if (it->logical_slot == logical_slot) {
            ApplyInteractionUpdateGPU(
                physical_page,
                it->world_position,
                it->radius,
                it->strength);

            alife_stats.applied_deferred++;
            applied_count++;

            Msg("* [A-Life] APPLIED deferred update #%u to slot %u (phys page %u) - pos (%.1f, %.1f, %.1f)",
                applied_count, logical_slot, physical_page,
                it->world_position.x, it->world_position.y, it->world_position.z);

            it = pending_updates.erase(it);
        } else {
            ++it;
        }
    }

    if (applied_count > 0) {
        page_table[logical_slot].dirty_bit = 1;
        Msg("* [A-Life] Applied %u deferred updates to slot %u (queue remaining: %zu)",
            applied_count, logical_slot, pending_updates.size());
    }
}

void CDetailManager::ProcessPendingUpdates()
{
    ExpireOldPendingUpdates(1800);

    xr_set<uint32_t> dirty_slots;
    for (const auto& update : pending_updates) {
        dirty_slots.insert(update.logical_slot);
    }

    for (uint32_t slot : dirty_slots) {
        if (!IsPageResident(slot)) {
            RequestPageWithPriority(slot, PRIORITY_DIRTY);
        }
    }
}

void CDetailManager::ApplyInteractionUpdateGPU(
    uint32_t physical_page,
    const Fvector& world_center,
    float radius,
    float strength)
{
    if (!interaction_update_cs || !interaction_update_cs->sh) {
        return;
    }

    if (!interaction_atlas || !interaction_uav) {
        return;
    }

    InteractionUpdateCB cb_data;
    cb_data.world_center = Fvector2().set(world_center.x, world_center.z);
    cb_data.radius = radius;
    cb_data.strength = strength;
    cb_data.physical_page = physical_page;
    cb_data.slot_size = DETAIL_SLOT_SIZE;
    cb_data.atlas_width = (float)atlas_width;
    cb_data.slot_texture_size = (float)slot_texture_size;

    auto* context = HW.get_context(CHW::IMM_CTX_ID);
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(interaction_update_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (FAILED(hr)) {
        Msg("! [A-Life] Failed to map interaction update CB");
        return;
    }

    memcpy(mapped.pData, &cb_data, sizeof(cb_data));
    context->Unmap(interaction_update_cb, 0);

    ID3D11ComputeShader* cs = interaction_update_cs->sh;
    context->CSSetShader(cs, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &interaction_update_cb);
    context->CSSetUnorderedAccessViews(0, 1, &interaction_uav, nullptr);

    context->Dispatch(4, 4, 1);

    ID3D11UnorderedAccessView* null_uav = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

}

void CDetailManager::RequestInteractionUpdateThreadSafe(
    const Fvector& world_pos,
    float radius,
    float strength,
    uint8_t type)
{
    ThreadSafeInteractionRequest request;
    request.world_position = world_pos;
    request.radius = radius;
    request.strength = strength;
    request.type = type;

    threadsafe_queue_lock.Enter();
    threadsafe_request_queue.push_back(request);
    threadsafe_queue_lock.Leave();
}

void CDetailManager::ProcessThreadSafeRequests()
{
    if (threadsafe_request_queue.empty())
        return;

    threadsafe_queue_lock.Enter();

    xr_vector<ThreadSafeInteractionRequest> requests_copy;
    requests_copy.swap(threadsafe_request_queue);

    threadsafe_queue_lock.Leave();

    for (const auto& req : requests_copy)
    {
        RequestInteractionUpdate(req.world_position, req.radius, req.strength, req.type);
    }
}

} // namespace xray::render::RENDER_NAMESPACE
