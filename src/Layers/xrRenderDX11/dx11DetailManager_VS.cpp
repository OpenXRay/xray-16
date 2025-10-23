#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"

// Phase 5: Grass wind tuning parameters (defined in xrEngine, accessed here)
extern ENGINE_API float ps_r3_grass_wind_multiplier;     // Multiplier for environment wind strength
extern ENGINE_API float ps_r3_grass_wind_min;            // Minimum wind speed
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;      // Speed of wind transitions
extern ENGINE_API float ps_r3_grass_wind_displacement;   // Vertex displacement strength
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;          // FBM octave count

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
    hwc_detail_params = T0.get("detail_params");  // Phase 5: slot grid parameters
    hwc_grass_wind_displacement = T0.get("grass_wind_displacement");  // Phase 5: wind displacement strength
    hwc_grass_interaction_displacement = T0.get("grass_interaction_displacement");  // Phase 5: interaction displacement strength

    // Phase 1, Milestone 1.1: Create 3 structured buffers (one per vis_id: still, wave1, wave2)
    // Use 32K instances per buffer (we've seen up to ~22K for vis_id=0 still grass)
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

    // Phase 5: Load compute shaders for interactive grass
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

    // Phase 6: Initialize page table (NEW)
    InitializePageTable();
}

// Phase 2.0.3: Create persistent GPU buffer for all level instances
void CDetailManager::CreatePersistentInstanceBuffer()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating persistent GPU instance buffer...");

    persistent_buffer_capacity = total_instance_count;

    // Prepare instance data for upload
    xr_vector<InstanceData> upload_data;
    upload_data.reserve(total_instance_count);

    for (u32 i = 0; i < total_instance_count; i++)
    {
        const SlotItemWithObject& src = all_level_instances[i];
        InstanceData dst = {};

        // Pass full rotation matrix (preserves all rotations)
        const Fmatrix& M = src.item.mRotY;
        dst.m0.set(M._11, M._21, M._31);  // First column
        dst.m1.set(M._12, M._22, M._32);  // Second column
        dst.m2.set(M._13, M._23, M._33);  // Third column

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

    // Phase 6: Update page table each frame (NEW)
    UpdatePageTable();

    // Phase 6: Upload page table to GPU (NEW)
    UpdateIndirectionBuffer(cmd_list);

    // Phase 5: Update interactive grass (entity tracking + compute shaders)
    UpdateInteractiveEntities(cmd_list);
    UpdateWind(cmd_list);
    RenderInteractions(cmd_list);

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

        // Phase 5: Set slot grid parameters
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

        // Phase 5: Bind interactive grass textures (AFTER apply_lmaterial to avoid being unbound)
        // Bind to slots 1, 2, 3 to match shader registers (t1, t2, t3)
        if (interaction_srv)
            cmd_list.SRVSManager.SetVSResource(1, interaction_srv);
        if (wind_srv)
            cmd_list.SRVSManager.SetVSResource(2, wind_srv);
        // Phase 6: Bind indirection buffer instead of slot_atlas_uv_srv (NEW)
        if (indirection_srv)
            cmd_list.SRVSManager.SetVSResource(3, indirection_srv);
        if (interaction_sampler)
            HW.get_context(cmd_list.context_id)->VSSetSamplers(0, 1, &interaction_sampler);

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

                c_storage[instanceIdx].m0.set(M._11, M._21, M._31);  // First column
                c_storage[instanceIdx].m1.set(M._12, M._22, M._32);  // Second column
                c_storage[instanceIdx].m2.set(M._13, M._23, M._33);  // Third column
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

    // Phase 5: Set slot grid parameters
    Fvector4 detail_params_vec;
    detail_params_vec.x = (float)dtH.x_size();
    detail_params_vec.y = (float)dtH.z_size();
    detail_params_vec.z = (float)dtH.x_offs();
    detail_params_vec.w = (float)dtH.z_offs();
    cmd_list.set_c(hwc_detail_params._get(), detail_params_vec);
    cmd_list.set_c(hwc_grass_wind_displacement._get(), ps_r3_grass_wind_displacement);
    cmd_list.set_c(hwc_grass_interaction_displacement._get(), ps_r3_grass_interaction_displacement);

    // Set shader for this object (use lod_id=0 for wave shader)
    cmd_list.set_Element(Object.shader->E[0], 0);
    cmd_list.apply_lmaterial();

    // Phase 5: Bind interactive grass textures (AFTER apply_lmaterial to avoid being unbound)
    // Bind to slots 1, 2, 3 to match shader registers (t1, t2, t3)
    if (interaction_srv)
        cmd_list.SRVSManager.SetVSResource(1, interaction_srv);
    if (wind_srv)
        cmd_list.SRVSManager.SetVSResource(2, wind_srv);
    // Phase 6: Bind indirection buffer instead of slot_atlas_uv_srv (NEW)
    if (indirection_srv)
        cmd_list.SRVSManager.SetVSResource(3, indirection_srv);
    if (interaction_sampler)
        HW.get_context(cmd_list.context_id)->VSSetSamplers(0, 1, &interaction_sampler);

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

//-----------------------------------------------------------------------------
// Phase 5: Interactive Grass System
//-----------------------------------------------------------------------------

// Milestone 5.1: Create interaction texture atlas
void CDetailManager::CreateInteractionAtlas()
{
    VERIFY(!interaction_atlas);
    VERIFY(slot_count > 0);

    Msg("* [DetailManager] Creating interaction atlas...");

    // Set atlas parameters
    atlas_width = 2048;
    atlas_height = 2048;
    slot_texture_size = 32;  // 32x32 pixels per slot

    // We don't need to track ALL slots - only those near the camera
    // Use a fixed-size atlas and dynamically map visible slots to atlas space
    // For now, just use the atlas size and modulo wrap slot indices
    u32 slots_per_row = atlas_width / slot_texture_size;
    u32 max_atlas_slots = slots_per_row * (atlas_height / slot_texture_size);

    if (slot_count > max_atlas_slots)
    {
        Msg("! [DetailManager] WARNING: %u world slots exceeds atlas capacity %u", slot_count, max_atlas_slots);
        Msg("! [DetailManager] Using modulo wrapping - only slots near camera will have accurate interaction");
        Msg("! [DetailManager] This is expected for large levels (atlas tracks ~%u slots at a time)", max_atlas_slots);
    }

    // Create 2D texture for interaction atlas
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = atlas_width;
    tex_desc.Height = atlas_height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // High precision for displacement
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    CHK_DX(HW.pDevice->CreateTexture2D(&tex_desc, nullptr, &interaction_atlas));

    // Create RTV
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = tex_desc.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateRenderTargetView(interaction_atlas, &rtv_desc, &interaction_rtv));

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    CHK_DX(HW.pDevice->CreateShaderResourceView(interaction_atlas, &srv_desc, &interaction_srv));

    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = tex_desc.Format;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(interaction_atlas, &uav_desc, &interaction_uav));

    // Clear atlas to zero
    float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    HW.get_context(CHW::IMM_CTX_ID)->ClearRenderTargetView(interaction_rtv, clear_color);

    // Compute UV mapping for atlas slots
    // We only need UV mappings for the atlas capacity, not all world slots
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

    // Create GPU buffer for slot UV mapping
    D3D11_BUFFER_DESC buf_desc = {};
    buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
    buf_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buf_desc.ByteWidth = uv_buffer_size * sizeof(Fvector4);
    buf_desc.StructureByteStride = 0;  // Not a structured buffer

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = slot_uvs.data();

    CHK_DX(HW.pDevice->CreateBuffer(&buf_desc, &init_data, &slot_atlas_uv_buffer));

    // Create SRV for UV buffer
    D3D11_SHADER_RESOURCE_VIEW_DESC uv_srv_desc = {};
    uv_srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uv_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    uv_srv_desc.Buffer.FirstElement = 0;
    uv_srv_desc.Buffer.NumElements = uv_buffer_size;
    CHK_DX(HW.pDevice->CreateShaderResourceView(slot_atlas_uv_buffer, &uv_srv_desc, &slot_atlas_uv_srv));

    // Create sampler state for interaction and wind textures
    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;  // Bilinear filtering
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 1;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 0;
    CHK_DX(HW.pDevice->CreateSamplerState(&sampler_desc, &interaction_sampler));

    float memory_mb = (atlas_width * atlas_height * 8) / (1024.0f * 1024.0f);  // RGBA16F = 8 bytes/pixel

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
}

// Milestone 5.2: Create entity tracking buffers
void CDetailManager::CreateEntityTrackingBuffers()
{
    VERIFY(!entity_buffer);

    max_entities = 256;  // Default max entities

    Msg("* [DetailManager] Creating entity tracking buffers...");

    // Create dynamic structured buffer for entities
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(InteractiveEntity);
    desc.ByteWidth = max_entities * sizeof(InteractiveEntity);

    CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &entity_buffer));

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = max_entities;
    CHK_DX(HW.pDevice->CreateShaderResourceView(entity_buffer, &srv_desc, &entity_srv));

    entity_count_this_frame = 0;

    Msg("* [DetailManager] Entity tracking buffers created (max %u entities)", max_entities);

    // Create constant buffer for interaction compute shader
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
    // D3D11 constant buffers must be multiples of 16 bytes (this one is already 32 = 16*2)
    cb_desc.ByteWidth = ((sizeof(InteractionParams) + 15) / 16) * 16;  // Round up to next multiple of 16
    CHK_DX(HW.pDevice->CreateBuffer(&cb_desc, nullptr, &interaction_constant_buffer));

    Msg("* [DetailManager] Interaction constant buffer created");
}

void CDetailManager::DestroyEntityTrackingBuffers()
{
    _RELEASE(entity_srv);
    _RELEASE(entity_buffer);
    interactive_entities.clear();
}

// Milestone 5.4: Create wind texture
void CDetailManager::CreateWindTexture()
{
    VERIFY(!wind_texture);

    wind_texture_size = 512;  // 512x512 wind field

    Msg("* [DetailManager] Creating wind texture...");

    // Create 2D texture for wind field
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = wind_texture_size;
    tex_desc.Height = wind_texture_size;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // XYZW = wind vector + strength
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    CHK_DX(HW.pDevice->CreateTexture2D(&tex_desc, nullptr, &wind_texture));

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    CHK_DX(HW.pDevice->CreateShaderResourceView(wind_texture, &srv_desc, &wind_srv));

    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = tex_desc.Format;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(wind_texture, &uav_desc, &wind_uav));

    float memory_mb = (wind_texture_size * wind_texture_size * 8) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] Wind texture created:");
    Msg("  - Resolution: %ux%u", wind_texture_size, wind_texture_size);
    Msg("  - VRAM usage: %.2f MB", memory_mb);

    // Create constant buffer for wind compute shader
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    struct WindParams
    {
        float time;              // 0-3
        Fvector2 wind_direction; // 4-11
        float wind_speed;        // 12-15
        u32 octaves;             // 16-19
        float lacunarity;        // 20-23
        float gain;              // 24-27
        float padding1;          // 28-31 (align scroll_speed to 16-byte boundary)
        Fvector2 scroll_speed;   // 32-39
        u32 texture_size;        // 40-43
        u32 pad[3];              // 44-55
    };
    static_assert(sizeof(WindParams) == 56, "WindParams size check");
    // D3D11 constant buffers must be multiples of 16 bytes
    cb_desc.ByteWidth = ((sizeof(WindParams) + 15) / 16) * 16;  // Round up to next multiple of 16
    CHK_DX(HW.pDevice->CreateBuffer(&cb_desc, nullptr, &wind_constant_buffer));

    Msg("* [DetailManager] Wind constant buffer created");
}

void CDetailManager::DestroyWindTexture()
{
    _RELEASE(wind_uav);
    _RELEASE(wind_srv);
    _RELEASE(wind_texture);
}

// Milestone 5.2: Update entity tracking (gather entities and upload to GPU)
void CDetailManager::UpdateInteractiveEntities(CBackend& cmd_list)
{
    if (!entity_buffer)
        return;

    interactive_entities.clear();

    // Track camera/player with velocity
    static Fvector last_camera_pos = Device.vCameraPosition;
    Fvector camera_velocity;
    camera_velocity.sub(Device.vCameraPosition, last_camera_pos);
    camera_velocity.mul(1.0f / Device.fTimeDelta);  // Convert to velocity (m/s)
    last_camera_pos = Device.vCameraPosition;

    InteractiveEntity camera_entity;
    camera_entity.position = Device.vCameraPosition;
    camera_entity.radius = 50.0f;     // TEMP: 50m radius to ensure we hit something
    camera_entity.velocity = camera_velocity;
    camera_entity.weight = 1.0f;      // Full weight
    camera_entity.padding[0] = 0.0f;
    camera_entity.padding[1] = 0.0f;

    interactive_entities.push_back(camera_entity);

    // TODO: Expand to full game object tracking
    // To implement:
    // 1. Get player entity: g_pGameLevel->CurrentViewEntity() or similar
    // 2. Query nearby objects: g_pGameLevel->ObjectSpace.GetNearest(...)
    // 3. Filter to NPCs, physics objects, etc.
    // 4. Extract position, velocity, mass for each
    // Example pseudocode:
    /*
    if (g_pGameLevel)
    {
        // Get all objects within grass render distance
        xr_vector<CObject*> nearby_objects;
        g_pGameLevel->ObjectSpace.GetNearest(
            nearby_objects,
            Device.vCameraPosition,
            fade_distance,
            nullptr
        );

        for (CObject* obj : nearby_objects)
        {
            if (CGameObject* go = smart_cast<CGameObject*>(obj))
            {
                // Filter to interactive types (actors, NPCs, physics)
                if (go->getEnabled() &&
                    (go->CLS_ID == CLSID_OBJECT_ACTOR ||
                     go->CLS_ID == CLSID_OBJECT_PHYSIC ||
                     go->CLS_ID == CLSID_AI_STALKER))
                {
                    InteractiveEntity e;
                    e.position = go->Position();
                    e.radius = go->Radius();
                    e.velocity = go->velocity();
                    e.weight = std::min(go->GetMass() / 100.0f, 1.0f);
                    e.padding[0] = 0.0f;
                    e.padding[1] = 0.0f;
                    interactive_entities.push_back(e);

                    if (interactive_entities.size() >= max_entities)
                        break;
                }
            }
        }
    }
    */

    entity_count_this_frame = interactive_entities.size();

    // Upload to GPU
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

// Milestone 5.3: Render interactions (dispatch interaction compute shader)
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

    // Set compute shader
    context->CSSetShader(interaction_compute_shader->sh, nullptr, 0);

    // Update constant buffer
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
    params.decay_rate = powf(0.05f, Device.fTimeDelta);  // 95% remaining per second
    params.slot_count = slot_count;
    params.slots_per_row = atlas_width / slot_texture_size;
    params.slot_texture_size = slot_texture_size;
    params.atlas_width = atlas_width;

    // Upload constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(interaction_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(interaction_constant_buffer, 0);

    // Bind resources
    context->CSSetConstantBuffers(0, 1, &interaction_constant_buffer);
    context->CSSetShaderResources(0, 1, &entity_srv);
    context->CSSetShaderResources(1, 1, &slot_aabb_srv);
    context->CSSetUnorderedAccessViews(0, 1, &interaction_uav, nullptr);

    // Dispatch: Cover entire atlas texture with 32×32 thread groups
    u32 num_groups_x = (atlas_width + 31) / 32;
    u32 num_groups_y = (atlas_height + 31) / 32;

    if (frame_counter < 120)
        Msg("* [DetailManager] Dispatching interaction compute: groups=%ux%u, threads=%ux%u",
            num_groups_x, num_groups_y, num_groups_x * 32, num_groups_y * 32);

    context->Dispatch(num_groups_x, num_groups_y, 1);

    // Unbind resources
    ID3DShaderResourceView* nullSRV[2] = {nullptr, nullptr};
    ID3DUnorderedAccessView* nullUAV[1] = {nullptr};
    context->CSSetShaderResources(0, 2, nullSRV);
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
}

// Milestone 5.4: Update wind (dispatch wind FBM compute shader)
void CDetailManager::UpdateWind(CBackend& cmd_list)
{
    if (g_pGamePersistent)
    {
        // Get target wind speed from environment
        wind_speed = _max(g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier, ps_r3_grass_wind_min);

        // Get wind direction and convert from degrees to normalized 2D vector
        const float envDir = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
        float wind_rad = deg2rad(envDir);
        wind_direction = { cosf(wind_rad), sinf(wind_rad) };
    }
    else
    {
        wind_speed = 0.5f;  // Default wind speed
        wind_direction.set(1.0f, 0.0f);  // East
    }

    auto context = HW.get_context(cmd_list.context_id);

    // Set compute shader
    context->CSSetShader(wind_compute_shader->sh, nullptr, 0);

    // Update constant buffer
    WindParams params;
    memset(&params, 0, sizeof(params));  // Zero initialize
    params.time = Device.fTimeGlobal;
    params.wind_direction = wind_direction;
    params.wind_speed = wind_speed;
    params.octaves = ps_r3_grass_wind_octaves;  // Use tunable octave count
    params.lacunarity = 2.0f;
    params.gain = 0.5f;
    params.scroll_speed.x = wind_direction.x * wind_speed * 5.0f;  // Increased from 0.1 to 5.0 for visible movement
    params.scroll_speed.y = wind_direction.y * wind_speed * 5.0f;
    params.texture_size = wind_texture_size;

    // Upload constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(context->Map(wind_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    memcpy(mapped.pData, &params, sizeof(params));
    context->Unmap(wind_constant_buffer, 0);

    // Bind resources
    context->CSSetConstantBuffers(0, 1, &wind_constant_buffer);
    context->CSSetUnorderedAccessViews(0, 1, &wind_uav, nullptr);

    // Dispatch: 16×16 thread groups to cover 512×512 texture
    u32 num_groups = (wind_texture_size + 15) / 16;
    context->Dispatch(num_groups, num_groups, 1);

    // Unbind resources
    ID3DUnorderedAccessView* nullUAV[1] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
}

//-----------------------------------------------------------------------------
// Phase 6: Virtual Texturing System
//-----------------------------------------------------------------------------

void CDetailManager::InitializePageTable()
{
    Msg("* [DetailManager] Initializing virtual page table...");

    // 1. Allocate page table (359K entries)
    page_table.resize(TOTAL_WORLD_SLOTS);

    // 2. Initialize all entries to NOT RESIDENT
    for (uint32_t i = 0; i < TOTAL_WORLD_SLOTS; i++) {
        page_table[i].physical_page = INVALID_PAGE;
        page_table[i].mip_level = 0;
        page_table[i].reference_bit = 0;
        page_table[i].dirty_bit = 0;
        page_table[i].locked_bit = 0;
        page_table[i].last_access_frame = 0;
    }

    // 3. Initialize physical pages (4096 pages)
    for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
        physical_pages[i].logical_slot = UINT32_MAX;  // Free
        physical_pages[i].reference_bit = 0;
        physical_pages[i].locked = 0;
    }

    clock_hand = 0;
    resident_page_count = 0;

    // 4. Create indirection buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;  // Not a structured buffer
    bufferDesc.ByteWidth = TOTAL_WORLD_SLOTS * sizeof(uint32_t);

    // Initialize with 0xFFFFFFFF (all invalid)
    xr_vector<uint32_t> initial_data(TOTAL_WORLD_SLOTS, 0xFFFFFFFF);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = initial_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&bufferDesc, &initData, &indirection_buffer));

    // 5. Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = TOTAL_WORLD_SLOTS;

    CHK_DX(HW.pDevice->CreateShaderResourceView(indirection_buffer, &srvDesc, &indirection_srv));

    // 6. Reset stats
    ResetPageTableStats();

    Msg("* [DetailManager] Page table initialized: %u logical slots -> %u physical pages",
        TOTAL_WORLD_SLOTS, PHYSICAL_PAGES);
}

void CDetailManager::ShutdownPageTable()
{
    Msg("* [DetailManager] Shutting down page table...");

    // Release D3D resources
    _RELEASE(indirection_srv);
    _RELEASE(indirection_buffer);

    // Clear vectors
    page_table.clear();
    page_table.shrink_to_fit();

    Msg("* [DetailManager] Page table shutdown complete");
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

    Msg("=== Page Table Statistics ===");
    Msg("Total Requests:   %llu", page_table_stats.total_requests);
    Msg("Cache Hits:       %llu (%.1f%%)", page_table_stats.cache_hits, hit_rate);
    Msg("Cache Misses:     %llu", page_table_stats.cache_misses);
    Msg("Evictions:        %llu", page_table_stats.evictions);
    Msg("Resident Pages:   %u / %u", resident_page_count, PHYSICAL_PAGES);
    Msg("===========================");
}

bool CDetailManager::IsPageResident(uint32_t logical_slot) const
{
    if (logical_slot >= TOTAL_WORLD_SLOTS)
        return false;

    return page_table[logical_slot].physical_page != INVALID_PAGE;
}

uint16_t CDetailManager::FindVictimPage()
{
    // Clock algorithm: sweep through pages, give second chance to referenced pages
    int attempts = 0;
    const int max_attempts = PHYSICAL_PAGES * 2;  // Two full sweeps maximum

    while (attempts < max_attempts) {
        PhysicalPageInfo& page = physical_pages[clock_hand];

        // Skip locked pages (in-flight uploads - will implement later)
        if (page.locked) {
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            attempts++;
            continue;
        }

        // Skip free pages (already available)
        if (page.logical_slot == UINT32_MAX) {
            uint16_t victim = clock_hand;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            return victim;
        }

        // Check reference bit
        if (page.reference_bit) {
            // Give second chance: clear bit and move on
            page.reference_bit = 0;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            attempts++;
        } else {
            // Victim found! This page hasn't been referenced recently
            uint16_t victim = clock_hand;
            clock_hand = (clock_hand + 1) % PHYSICAL_PAGES;
            return victim;
        }
    }

    // Fallback: if all pages are locked or referenced, evict oldest
    // (Shouldn't happen in practice)
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

    // Nothing to evict if page is free
    if (page.logical_slot == UINT32_MAX)
        return;

    uint32_t logical_slot = page.logical_slot;
    VERIFY(logical_slot < TOTAL_WORLD_SLOTS);

    // Update page table entry (mark as not resident)
    page_table[logical_slot].physical_page = INVALID_PAGE;
    page_table[logical_slot].reference_bit = 0;
    page_table[logical_slot].dirty_bit = 0;

    // TODO Phase 2: If dirty, add to writeback queue
    // For now, just discard (data lost on eviction)

    // Mark physical page as free
    page.logical_slot = UINT32_MAX;
    page.reference_bit = 0;

    resident_page_count--;
    page_table_stats.evictions++;

    // Msg("* [DetailManager] Evicted slot %u from physical page %u", logical_slot, physical_page);
}

void CDetailManager::PromotePage(uint32_t logical_slot, uint16_t physical_page)
{
    VERIFY(logical_slot < TOTAL_WORLD_SLOTS);
    VERIFY(physical_page < PHYSICAL_PAGES);

    // Evict current occupant if page is not free
    if (physical_pages[physical_page].logical_slot != UINT32_MAX) {
        EvictPage(physical_page);
    }

    // Update physical page info
    physical_pages[physical_page].logical_slot = logical_slot;
    physical_pages[physical_page].reference_bit = 1;  // Mark as recently used

    // Update page table entry
    page_table[logical_slot].physical_page = physical_page;
    page_table[logical_slot].reference_bit = 1;
    page_table[logical_slot].last_access_frame = Device.dwFrame;
    page_table[logical_slot].dirty_bit = 0;  // Clean initially

    resident_page_count++;

    // TODO Phase 2: Upload slot data from disk/warm cache
    // For now, just allocate the page (will be zero/garbage)

    // Msg("* [DetailManager] Promoted slot %u to physical page %u", logical_slot, physical_page);
}

uint16_t CDetailManager::RequestPage(uint32_t logical_slot, uint8_t priority)
{
    VERIFY(logical_slot < TOTAL_WORLD_SLOTS);

    page_table_stats.total_requests++;

    // Check if already resident (cache hit)
    if (page_table[logical_slot].physical_page != INVALID_PAGE) {
        uint16_t physical_page = page_table[logical_slot].physical_page;

        // Update access tracking
        page_table[logical_slot].reference_bit = 1;
        page_table[logical_slot].last_access_frame = Device.dwFrame;
        physical_pages[physical_page].reference_bit = 1;

        page_table_stats.cache_hits++;
        return physical_page;
    }

    // Cache miss: need to allocate a page
    page_table_stats.cache_misses++;

    // Find victim page to evict
    uint16_t physical_page = FindVictimPage();

    // Promote this slot to the physical page
    PromotePage(logical_slot, physical_page);

    return physical_page;
}

void CDetailManager::UpdateIndirectionBuffer(CBackend& cmd_list)
{
    if (!indirection_buffer)
        return;

    auto context = HW.get_context(cmd_list.context_id);

    // Map buffer for writing
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(indirection_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (FAILED(hr)) {
        Msg("! [DetailManager] Failed to map indirection buffer");
        return;
    }

    // Write packed indirection data
    uint32_t* data = (uint32_t*)mapped.pData;

    for (uint32_t i = 0; i < TOTAL_WORLD_SLOTS; i++) {
        // Pack: physical_page (16 bits) | mip_level (8 bits) | flags (8 bits)
        uint32_t packed = page_table[i].physical_page;  // Lower 16 bits
        packed |= (uint32_t(page_table[i].mip_level) << 16);  // Bits 16-23
        // Bits 24-31 reserved for flags

        data[i] = packed;
    }

    context->Unmap(indirection_buffer, 0);

    // Msg("* [DetailManager] Updated indirection buffer (%u resident pages)", resident_page_count);
}

void CDetailManager::UpdatePageTable()
{
    // This runs AFTER detail_cull.cs has identified visible slots
    // and BEFORE we render grass

    // For now, manually request pages for all slots with visible instances
    // (In Phase 2, we'll use GPU culling results directly)

    // Simple approach: request first 4096 slots for testing
    // TODO: Replace with actual visibility query
    const uint32_t test_slot_count = std::min(4096u, slot_count);

    for (uint32_t i = 0; i < test_slot_count; i++) {
        RequestPage(i, 255);  // Max priority
    }
}

} // namespace xray::render::RENDER_NAMESPACE
