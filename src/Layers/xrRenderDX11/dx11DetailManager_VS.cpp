#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "Layers/xrRender/blenders/Blender_Detail_GPU.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"
#include "xrEngine/GrassInteractionCollector.h"

// Phase 5: Grass wind tuning parameters (defined in xrEngine, accessed here)
extern ENGINE_API float ps_r3_grass_wind_multiplier;     // Multiplier for environment wind strength
extern ENGINE_API float ps_r3_grass_wind_min;            // Minimum wind speed
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;      // Speed of wind transitions
extern ENGINE_API float ps_r3_grass_wind_displacement;   // Vertex displacement strength
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;          // FBM octave count

namespace xray::render::RENDER_NAMESPACE
{
extern int ps_r__detail_gpu;
namespace detail_manager
{
extern const int quant;
//extern const int c_hdr;
}

void CDetailManager::hw_Load_Shaders()
{
    // === VANILLA SHADER CONSTANTS (always loaded) ===
    // Create shader to access constant storage
    ref_shader S;
    S.create("details\\set");
    R_constant_table& T0 = *(S->E[0]->passes[0]->constants);
    R_constant_table& T1 = *(S->E[1]->passes[0]->constants);
    hwc_consts = T0.get("consts");
    hwc_wave = T0.get("wave");
    hwc_wind = T0.get("dir2D");
    hwc_array = T0.get("array");
    hwc_s_consts = T1.get("consts");
    hwc_s_xform = T1.get("xform");
    hwc_s_array = T1.get("array");

#ifdef USE_DX11
    // Note: GPU constants will be initialized from gpu_detail_shader after it's created

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

    // Phase 3: Load interaction update shader (for deferred A-Life updates)
    interaction_update_cs.create("detail_interaction_apply");

    if (interaction_update_cs && interaction_update_cs->sh)
        Msg("* [DetailManager] Loaded interaction update shader: OK");
    else
        Msg("! [DetailManager] Failed to load interaction update shader!");

    // Create constant buffer for interaction updates
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof(InteractionUpdateCB);

    CHK_DX(HW.pDevice->CreateBuffer(&cbDesc, nullptr, &interaction_update_cb));

    // === GPU RENDERING SHADER (always initialized for mode switching) ===
    b_detail_gpu = xr_new<CBlender_Detail_GPU>();
    gpu_detail_shader.create(b_detail_gpu, nullptr, nullptr);

    if (!gpu_detail_shader)
    {
        Msg("! [DetailManager] Failed to create GPU instancing shader");
        Msg("  GPU shaders: detail_gpu.vs, detail_gpu.ps might be missing");
        xr_delete(b_detail_gpu);
    }
    else
    {
        Msg("* [DetailManager] GPU instancing shader created successfully");

        // Initialize GPU constant handles from gpu_detail_shader
        R_constant_table& GPU_T0 = *(gpu_detail_shader->E[0]->passes[0]->constants);
        gpu_wind2 = GPU_T0.get("dir2D_2");
        gpu_detail_params = GPU_T0.get("detail_params");
        gpu_grass_wind_displacement = GPU_T0.get("grass_wind_displacement");
        gpu_grass_interaction_displacement = GPU_T0.get("grass_interaction_displacement");
        g_wind_direction = GPU_T0.get("g_wind_direction");
    }
#endif
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

        // Extract position from rotation matrix
        const Fmatrix& M = src.item.mRotY;
        dst.pos = M.c;
        dst.scale = src.item.scale;
        dst.hemi = src.item.c_hemi;
        dst.vis_id = src.item.vis_ID;
        dst.object_id = src.object_id;
        dst.padding = 0.0f;

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

// Phase 6: Create Bezier curve bladegeometry buffers
void CDetailManager::CreateSDF_BladeGeometry()
{
    Msg("* [PHASE 6] Creating Bezier curve bladegeometry...");

    xr_vector<BladeVertex> blade_verts;
    xr_vector<u16> blade_indices;

    // Generate mesh from SDF samples (6 segments = good quality/performance balance)
    GenerateGrassBlade(blade_verts, blade_indices, 8);

    // Define vertex declaration for blade geometry
    static const VertexElement blade_decl[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},  // pos
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},  // uv
        {0, 20, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},  // t
        {0, 24, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},  // width_scale
        D3DDECL_END()
    };

    // Create vertex buffer
    u32 vb_size = blade_verts.size() * sizeof(BladeVertex);
    blade_vb.Create(vb_size);
    {
        BladeVertex* pV = static_cast<BladeVertex*>(blade_vb.Map());
        memcpy(pV, blade_verts.data(), vb_size);
        blade_vb.Unmap(true);
    }

    // Create index buffer
    u32 ib_size = blade_indices.size() * sizeof(u16);
    blade_ib.Create(ib_size);
    {
        u16* pI = static_cast<u16*>(blade_ib.Map());
        memcpy(pI, blade_indices.data(), ib_size);
        blade_ib.Unmap(true);
    }

    // Create geometry
    blade_geom.create(blade_decl, blade_vb, blade_ib);

    blade_vertex_count = blade_verts.size();
    blade_index_count = blade_indices.size();

    float vram_kb = (vb_size + ib_size) / 1024.0f;

    Msg("* [PHASE 6] Blade geometry created:");
    Msg("  - Vertices: %u (%u bytes each)", blade_vertex_count, sizeof(BladeVertex));
    Msg("  - Indices: %u", blade_index_count);
    Msg("  - Triangles: %u", blade_index_count / 3);
    Msg("  - VRAM usage: %.2f KB", vram_kb);
    Msg("* [PHASE 6] Ready for Shadertoy-quality rendering!");
}

// Phase 2.1: Create GPU culling buffers and infrastructure
void CDetailManager::CreateGPUCullingBuffers()
{
    VERIFY(full_level_loaded);
    VERIFY(total_instance_count > 0);

    Msg("* [DetailManager] Creating GPU culling infrastructure...");

    // Load compute shader
    cull_compute_shader.create("detail_cull");

    // UNIFIED: Estimate max visible instances across ALL grass (conservative: total / 2)
    gpu_visible_buffer_capacity = total_instance_count / 2;
    if (gpu_visible_buffer_capacity < 10000)
        gpu_visible_buffer_capacity = 10000;  // Minimum safety buffer

    // UNIFIED: Create single visible instance buffer for all grass types
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(InstanceData);
    desc.ByteWidth = gpu_visible_buffer_capacity * sizeof(InstanceData);

    CHK_DX(HW.pDevice->CreateBuffer(&desc, nullptr, &gpu_visible_unified_buffer));

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
    CHK_DX(HW.pDevice->CreateShaderResourceView(gpu_visible_unified_buffer, &srvDesc, &gpu_visible_unified_srv));

    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = gpu_visible_buffer_capacity;
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(gpu_visible_unified_buffer, &uavDesc, &gpu_visible_unified_uav));

    // UNIFIED: Create single atomic counter (1 u32 total)
    D3D11_BUFFER_DESC counterDesc = {};
    counterDesc.Usage = D3D11_USAGE_DEFAULT;
    counterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    counterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    counterDesc.ByteWidth = sizeof(u32);  // Single counter
    CHK_DX(HW.pDevice->CreateBuffer(&counterDesc, nullptr, &gpu_visible_count_buffer));

    D3D11_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = {};
    counterUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    counterUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    counterUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    counterUavDesc.Buffer.NumElements = 1;  // Single u32
    CHK_DX(HW.pDevice->CreateUnorderedAccessView(gpu_visible_count_buffer, &counterUavDesc, &gpu_visible_count_uav));

    // Create readback buffer for CPU access
    D3D11_BUFFER_DESC readbackDesc = counterDesc;
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.BindFlags = 0;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackDesc.MiscFlags = 0;
    CHK_DX(HW.pDevice->CreateBuffer(&readbackDesc, nullptr, &gpu_visible_count_readback));

    // Create constant buffer for culling parameters
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = 256;  // Generous size for alignment
    CHK_DX(HW.pDevice->CreateBuffer(&cbDesc, nullptr, &cull_constant_buffer));

    // UNIFIED: Create single indirect draw args buffer
    // D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS structure:
    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;         // Written by compute shader
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    // Phase 6: Initialize args with Bezier curve blade geometry
    IndirectDrawArgs initial_args = {};
    initial_args.IndexCountPerInstance = blade_index_count;  // All grass uses same blade geometry
    initial_args.InstanceCount = 0;  // Will be written by compute shader
    initial_args.StartIndexLocation = 0;  // Blade geometry starts at index 0
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

    CHK_DX(HW.pDevice->CreateBuffer(&argsDesc, &initData, &gpu_indirect_args_unified));

    // Create RAW UAV for compute shader to write InstanceCount (RWByteAddressBuffer requires RAW)
    D3D11_UNORDERED_ACCESS_VIEW_DESC argsUavDesc = {};
    argsUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    argsUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    argsUavDesc.Buffer.FirstElement = 0;
    argsUavDesc.Buffer.NumElements = 5;  // 5 u32s in IndirectDrawArgs
    argsUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    CHK_DX(HW.pDevice->CreateUnorderedAccessView(
        gpu_indirect_args_unified, &argsUavDesc, &gpu_indirect_args_unified_uav));

    float vram_mb = (gpu_visible_buffer_capacity * sizeof(InstanceData)) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] UNIFIED GPU culling infrastructure created:");
    Msg("  - Buffer capacity: %u total instances", gpu_visible_buffer_capacity);
    Msg("  - VRAM for output buffer: %.2f MB", vram_mb);
    Msg("  - Draw calls: 1 (unified)");
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

    // UNIFIED: Reset single indirect args buffer (InstanceCount to 0, keep static fields)
    struct IndirectDrawArgs
    {
        u32 IndexCountPerInstance;
        u32 InstanceCount;         // Reset to 0 each frame
        u32 StartIndexLocation;
        s32 BaseVertexLocation;
        u32 StartInstanceLocation;
    };

    // Phase 6: Use Bezier curve blade geometry for all grass
    IndirectDrawArgs args = {};
    args.IndexCountPerInstance = blade_index_count;  // All grass uses same blade geometry
    args.InstanceCount = 0;  // Reset to 0 (compute shader will write this)
    args.StartIndexLocation = 0;  // Blade geometry starts at index 0
    args.BaseVertexLocation = 0;
    args.StartInstanceLocation = 0;

    // Update the unified args buffer (small upload: 20 bytes)
    context->UpdateSubresource(gpu_indirect_args_unified, 0, nullptr, &args, 0, 0);

    // UNIFIED: Clear single counter buffer to zero
    UINT zero = 0;
    context->ClearUnorderedAccessViewUint(gpu_visible_count_uav, &zero);

    // Phase 6B: Clear visible slot counter to 0 before culling
    uint32_t zero_u32 = 0;
    context->UpdateSubresource(visible_slot_counter_gpu, 0, nullptr, &zero_u32, 0, 0);

    // Bind constant buffer
    context->CSSetConstantBuffers(0, 1, &cull_constant_buffer);

    ID3DShaderResourceView* srvs[2] = {slot_aabb_srv, persistent_instance_srv};
    context->CSSetShaderResources(0, 2, srvs);

    // UNIFIED: Bind UAVs (3 unified buffers + 2 page table buffers)
    // u0: Unified visible instances
    // u1: Single counter (RAW)
    // u2: Unified indirect args (RAW)
    // u33: Visible slot IDs (Phase 6B)
    // u34: Visible slot counter (Phase 6B)
    ID3DUnorderedAccessView* uavs[35] = {nullptr};  // Initialize all to null
    uavs[0] = gpu_visible_unified_uav;
    uavs[1] = gpu_visible_count_uav;
    uavs[2] = gpu_indirect_args_unified_uav;
    uavs[33] = visible_slot_ids_uav;
    uavs[34] = visible_slot_counter_uav;

    context->CSSetUnorderedAccessViews(0, 35, uavs, nullptr);

    context->CSSetShader(cull_compute_shader->sh, nullptr, 0);

    u32 num_groups = (slot_count + 255) / 256;
    context->Dispatch(num_groups, 1, 1);

    // Unbind UAVs (prepare for rendering)
    ID3DUnorderedAccessView* null_uavs[35] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 35, null_uavs, nullptr);

    ID3DShaderResourceView* null_srvs[2] = {nullptr, nullptr};
    context->CSSetShaderResources(0, 2, null_srvs);

    // Unbind compute shader
    context->CSSetShader(nullptr, nullptr, 0);

    // Copy counter buffer to readback for CPU access
    context->CopyResource(gpu_visible_count_readback, gpu_visible_count_buffer);
}

// Phase 2.1: Render using GPU-culled instances
void CDetailManager::hw_Render_FullLevel(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

    // Reset detail count
    RImplementation.BasicStats.DetailCount = 0;

    // Phase 3: Process thread-safe interaction requests from A-Life thread (NEW)
    ProcessThreadSafeRequests();

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
        context->CopyResource(gpu_visible_count_readback, gpu_visible_count_buffer);
        D3D11_MAPPED_SUBRESOURCE mapped;
        CHK_DX(context->Map(gpu_visible_count_readback, 0, D3D11_MAP_READ, 0, &mapped));
        u32 total_visible = *(u32*)mapped.pData;  // Dereference pointer to read value
        Msg("! [DetailManager] UNIFIED GPU Indirect Draw - First Frame:");
        Msg("  - Total instances: %u", total_instance_count);
        Msg("  - Visible instances: %u (%.1f%%)", total_visible,
            (total_visible * 100.0f) / total_instance_count);
        context->Unmap(gpu_visible_count_readback, 0);
        first_frame = false;
    }

    // UNIFIED: Single draw call for all grass (16 → 1 draw calls!)
    // Set rendering state
    cmd_list.set_CullMode(CULL_NONE);
    cmd_list.set_xform_world(Fidentity);
    cmd_list.SRVSManager.SetVSResource(0, gpu_visible_unified_srv);
    // Phase 6: Use Bezier curve blade geometry
    cmd_list.set_Geometry(blade_geom);

    // === USE GPU SHADER (detail_gpu.vs + detail_gpu.ps) ===
    if (gpu_detail_shader)
    {
        cmd_list.set_Element(gpu_detail_shader->E[0], 0);
        cmd_list.apply_lmaterial();
    }
    else if (objects.size() > 0)
    {
        // Fallback to first object's shader if GPU shader failed to load
        cmd_list.set_Element(objects[0]->shader->E[0], 0);
        cmd_list.apply_lmaterial();
    }

    // IMPORTANT: Set constants AFTER apply_lmaterial() to avoid being reset!
    cmd_list.set_c(strConsts, consts);
    cmd_list.set_c(strWave, wave.div(PI_MUL_2));
    cmd_list.set_c(strDir2D, dir1);
    cmd_list.set_c(strDir2D_2, dir2);

    // Phase 5: Set slot grid parameters
    Fvector4 detail_params_vec;
    detail_params_vec.x = (float)dtH.x_size();
    detail_params_vec.y = (float)dtH.z_size();
    detail_params_vec.z = (float)dtH.x_offs();
    detail_params_vec.w = (float)dtH.z_offs();
    cmd_list.set_c(gpu_detail_params._get(), detail_params_vec);
    cmd_list.set_c(gpu_grass_wind_displacement._get(), ps_r3_grass_wind_displacement);
    cmd_list.set_c(gpu_grass_interaction_displacement._get(), ps_r3_grass_interaction_displacement);
    const float wind_angle_deg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
    cmd_list.set_c(g_wind_direction._get(), wind_angle_deg, 0.0f, 0.0f, 0.0f);

    // Phase 5: Bind interactive grass textures (AFTER apply_lmaterial to avoid being unbound)
    // Bind to slots 1, 2, 3 to match shader registers (t1, t2, t3)
    if (interaction_srv)
        cmd_list.SRVSManager.SetVSResource(1, interaction_srv);
    if (wind_srv)
        cmd_list.SRVSManager.SetVSResource(2, wind_srv);
    // Phase 6: Bind indirection buffer
    if (indirection_srv)
        cmd_list.SRVSManager.SetVSResource(3, indirection_srv);
    if (interaction_sampler)
        HW.get_context(cmd_list.context_id)->VSSetSamplers(0, 1, &interaction_sampler);

    // UNIFIED: Single DrawIndexedInstancedIndirect call for ALL grass!
    // The GPU determines instance count from the unified indirect args buffer
    // Use D3DPT_TRIANGLELIST because we generate indexed triangles (blade clumps)

    // DEBUG: Read back indirect args to verify InstanceCount
    static bool logged_draw = false;
    if (!logged_draw)
    {
        // Create staging buffer for readback
        D3D11_BUFFER_DESC argsDesc = {};
        argsDesc.ByteWidth = 20; // sizeof(IndirectDrawArgs)
        argsDesc.Usage = D3D11_USAGE_STAGING;
        argsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ID3D11Buffer* readback = nullptr;
        HW.pDevice->CreateBuffer(&argsDesc, nullptr, &readback);

        auto ctx = HW.get_context(cmd_list.context_id);
        ctx->CopyResource(readback, gpu_indirect_args_unified);

        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(readback, 0, D3D11_MAP_READ, 0, &mapped);
        u32* args = (u32*)mapped.pData;

        Msg("! [DetailManager] UNIFIED Draw Call Indirect Args:");
        Msg("  - IndexCountPerInstance: %u", args[0]);
        Msg("  - InstanceCount: %u", args[1]);
        Msg("  - StartIndexLocation: %u", args[2]);
        Msg("  - BaseVertexLocation: %d", (s32)args[3]);
        Msg("  - StartInstanceLocation: %u", args[4]);
        Msg("  - Blade index count: %u", blade_index_count);
        Msg("  - Unified SRV bound: %s", gpu_visible_unified_srv ? "YES" : "NO");

        ctx->Unmap(readback, 0);
        _RELEASE(readback);
        logged_draw = true;
    }

    cmd_list.RenderIndexedInstancedIndirect(D3DPT_TRIANGLESTRIP, gpu_indirect_args_unified, 0);

    cmd_list.set_CullMode(CULL_CCW);

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
    // === GPU COMPUTE PATH ===
    if (ps_r__detail_gpu && full_level_loaded)
    {
        hw_Render_FullLevel(cmd_list);
        return;
    }
#endif

    // === VANILLA CPU PATH ===
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

    // Setup geometry and DMA
    cmd_list.set_Geometry(hw_Geom);

    // Wave0
    float scale = 1.f / float(quant);
    Fvector4 wave;
    Fvector4 consts;
    consts.set(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir1, 1, 0);

    // Wave1
    wave.set(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos);
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir2, 2, 0);

    // Still
    consts.set(scale, scale, scale, 1.f);
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir2, 0, 1);
}

void CDetailManager::hw_Render_dump(CBackend& cmd_list,
    const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id)
{
    ZoneScoped;

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");
    static shared_str strArray("array");
    static shared_str strXForm("xform");

    RImplementation.BasicStats.DetailCount = 0;

    // Matrices and offsets
    u32 vOffset = 0;
    u32 iOffset = 0;

    vis_list& list = m_visibles[var_id];

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
    Fvector c_sun, c_ambient, c_hemi;
    c_sun.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z);
    c_sun.mul(.5f);
    c_ambient.set(desc.ambient.x, desc.ambient.y, desc.ambient.z);
    c_hemi.set(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z);

    // Iterate
    for (u32 O = 0; O < objects.size(); O++)
    {
        CDetail& Object = *objects[O];
        xr_vector<SlotItemVec*>& vis = list[O];
        if (!vis.empty())
        {
            for (u32 iPass = 0; iPass < Object.shader->E[lod_id]->passes.size(); ++iPass)
            {
                // Setup matrices + colors (and flush it as necessary)
                // RCache.set_Element				(Object.shader->E[lod_id]);
                cmd_list.set_Element(Object.shader->E[lod_id], iPass);
                cmd_list.apply_lmaterial();

                //	This could be cached in the corresponding consatant buffer
                //	as it is done for DX9
                cmd_list.set_c(strConsts, consts);
                cmd_list.set_c(strWave, wave);
                cmd_list.set_c(strDir2D, wind);
                cmd_list.set_c(strXForm, Device.mFullTransform);

                // ref_constant constArray = RCache.get_c(strArray);
                // VERIFY(constArray);

                // u32			c_base				= x_array->vs.index;
                // Fvector4*	c_storage			= RCache.get_ConstantCache_Vertex().get_array_f().access(c_base);
                Fvector4* c_storage = 0;
                //	Map constants to memory directly
                {
                    void* pVData;
                    cmd_list.get_ConstantDirect(strArray, hw_BatchSize * sizeof(Fvector4) * 4, &pVData, 0, 0);
                    c_storage = (Fvector4*)pVData;
                }
                VERIFY(c_storage);

                u32 dwBatch = 0;

                for (SlotItemVec* items : vis)
                {
                    for (SlotItem* item : *items)
                    {
                        SlotItem& Instance = *item;
                        u32 base = dwBatch * 4;

                        // Build matrix ( 3x4 matrix, last row - color )
                        float scale = Instance.scale_calculated;

                        //// Sort of fade using the scale
                        //// fade_distance == -1 use light_position to define "fade", anything else uses fade_distance
                        //if (fade_distance <= -1)
                        //    scale *= 1.0f - Instance.position.distance_to_xz_sqr(light_position) * 0.005f;
                        //else if (Instance.distance > fade_distance)
                        //    scale *= 1.0f - abs(Instance.distance - fade_distance) * 0.005f;

                        if (scale <= 0)
                            break;

                        Fmatrix& M = Instance.mRotY;
                        c_storage[base + 0].set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
                        c_storage[base + 1].set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
                        c_storage[base + 2].set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);
                        // RCache.set_ca(&*constArray, base+0, M._11*scale,	M._21*scale,	M._31*scale,	M._41	);
                        // RCache.set_ca(&*constArray, base+1, M._12*scale,	M._22*scale,	M._32*scale,	M._42	);
                        // RCache.set_ca(&*constArray, base+2, M._13*scale,	M._23*scale,	M._33*scale,	M._43	);

                        // Build color
                        // R2 only needs hemisphere
                        float h = Instance.c_hemi;
                        float s = Instance.c_sun;
                        c_storage[base + 3].set(s, s, s, h);
                        // RCache.set_ca(&*constArray, base+3, s,				s,				s,				h
                        // );
                        dwBatch++;
                        if (dwBatch == hw_BatchSize)
                        {
                            // flush
                            RImplementation.BasicStats.DetailCount += dwBatch;
                            u32 dwCNT_verts = dwBatch * Object.number_vertices;
                            u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;
                            // RCache.get_ConstantCache_Vertex().b_dirty				=	TRUE;
                            // RCache.get_ConstantCache_Vertex().get_array_f().dirty	(c_base,c_base+dwBatch*4);
                            cmd_list.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
                            cmd_list.stat.r.s_details.add(dwCNT_verts);

                            // restart
                            dwBatch = 0;

                            //	Remap constants to memory directly (just in case anything goes wrong)
                            {
                                void* pVData;
                                cmd_list.get_ConstantDirect(strArray, hw_BatchSize * sizeof(Fvector4) * 4, &pVData, 0, 0);
                                c_storage = (Fvector4*)pVData;
                            }
                            VERIFY(c_storage);
                        }
                    }
                }
                // flush if necessary
                if (dwBatch)
                {
                    RImplementation.BasicStats.DetailCount += dwBatch;
                    u32 dwCNT_verts = dwBatch * Object.number_vertices;
                    u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;
                    // RCache.get_ConstantCache_Vertex().b_dirty				=	TRUE;
                    // RCache.get_ConstantCache_Vertex().get_array_f().dirty	(c_base,c_base+dwBatch*4);
                    cmd_list.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
                    cmd_list.stat.r.s_details.add(dwCNT_verts);
                }
            }
        }
        vOffset += hw_BatchSize * Object.number_vertices;
        iOffset += hw_BatchSize * Object.number_indices;
    }
}

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

                c_storage[instanceIdx].pos = M.c;
                c_storage[instanceIdx].scale = Instance.scale;
                c_storage[instanceIdx].hemi = Instance.c_hemi;
                c_storage[instanceIdx].vis_id = vis_id;
                c_storage[instanceIdx].object_id = object_id;
                c_storage[instanceIdx].padding = 0.0f;

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
    // Phase 6: Use Bezier curve bladegeometry instead of old billboard quads
    cmd_list.set_Geometry(blade_geom);
    cmd_list.set_c(strConsts, consts);
    cmd_list.set_c(strWave, wave);
    cmd_list.set_c(strDir2D, wind);

    // Phase 5: Set slot grid parameters
    Fvector4 detail_params_vec;
    detail_params_vec.x = (float)dtH.x_size();
    detail_params_vec.y = (float)dtH.z_size();
    detail_params_vec.z = (float)dtH.x_offs();
    detail_params_vec.w = (float)dtH.z_offs();
    cmd_list.set_c(gpu_detail_params._get(), detail_params_vec);
    cmd_list.set_c(gpu_grass_wind_displacement._get(), ps_r3_grass_wind_displacement);
    cmd_list.set_c(gpu_grass_interaction_displacement._get(), ps_r3_grass_interaction_displacement);
    const float wind_angle_deg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
    cmd_list.set_c(g_wind_direction._get(), wind_angle_deg, 0.0f, 0.0f, 0.0f);

    // === USE GPU SHADER (detail_gpu.vs + detail_gpu.ps) ===
    if (gpu_detail_shader)
    {
        //cmd_list.set_Element(Object.shader->E[0], 0);
        cmd_list.set_Element(gpu_detail_shader->E[0], 0);
        cmd_list.apply_lmaterial();
    }
    else
    {
        // Fallback to vanilla shader if GPU shader failed to load
        cmd_list.set_Element(Object.shader->E[0], 0);
        cmd_list.apply_lmaterial();
    }

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

    // Phase 5: Register interaction atlas as $user$ texture for shader binding
    {
        ref_texture user_tex = RImplementation.Resources->_CreateTexture("$user$interaction_atlas");
        user_tex->surface_set(interaction_atlas);
        Msg("* [DetailManager] Registered interaction atlas as $user$interaction_atlas for shader access");
    }

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

    // Phase 5 Persistence: Create dirty bits buffer for GPU to mark modified pages
    {
        D3D11_BUFFER_DESC dirty_desc = {};
        dirty_desc.ByteWidth = slot_count * sizeof(uint32_t);  // One uint per slot
        dirty_desc.Usage = D3D11_USAGE_DEFAULT;
        dirty_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        dirty_desc.CPUAccessFlags = 0;
        dirty_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        dirty_desc.StructureByteStride = sizeof(uint32_t);

        // Initialize to zeros (all clean)
        xr_vector<uint32_t> init_dirty(slot_count, 0);
        D3D11_SUBRESOURCE_DATA dirty_init = {};
        dirty_init.pSysMem = init_dirty.data();

        CHK_DX(HW.pDevice->CreateBuffer(&dirty_desc, &dirty_init, &page_dirty_bits_buffer));

        // Create UAV for compute shader writes
        D3D11_UNORDERED_ACCESS_VIEW_DESC dirty_uav_desc = {};
        dirty_uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        dirty_uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        dirty_uav_desc.Buffer.FirstElement = 0;
        dirty_uav_desc.Buffer.NumElements = slot_count;
        dirty_uav_desc.Buffer.Flags = 0;
        CHK_DX(HW.pDevice->CreateUnorderedAccessView(page_dirty_bits_buffer, &dirty_uav_desc, &page_dirty_bits_uav));

        // Create SRV for potential reads (debugging)
        D3D11_SHADER_RESOURCE_VIEW_DESC dirty_srv_desc = {};
        dirty_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        dirty_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        dirty_srv_desc.Buffer.FirstElement = 0;
        dirty_srv_desc.Buffer.NumElements = slot_count;
        CHK_DX(HW.pDevice->CreateShaderResourceView(page_dirty_bits_buffer, &dirty_srv_desc, &page_dirty_bits_srv));

        Msg("* [DetailManager] Created dirty bits buffer (%u slots, %.2f KB)",
            slot_count, (slot_count * sizeof(uint32_t)) / 1024.0f);
    }

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

    // Phase 3: Release interaction update resources
    interaction_update_cs.destroy();
    _RELEASE(interaction_update_cb);
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

    // Phase 1.5: Get entities from game logic collector
    xr_vector<GrassInteractionEntity> game_entities;
    g_GrassInteractionCollector.GetEntitiesForFrame(game_entities);

    // Convert to renderer format and add to buffer
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
    context->CSSetShaderResources(2, 1, &physical_to_logical_srv);  // NEW: physical→logical mapping
    context->CSSetUnorderedAccessViews(0, 1, &interaction_uav, nullptr);

    // Dispatch: Cover entire atlas texture with 32×32 thread groups
    u32 num_groups_x = (atlas_width + 31) / 32;
    u32 num_groups_y = (atlas_height + 31) / 32;

    if (frame_counter < 120)
        Msg("* [DetailManager] Dispatching interaction compute: groups=%ux%u, threads=%ux%u",
            num_groups_x, num_groups_y, num_groups_x * 32, num_groups_y * 32);

    context->Dispatch(num_groups_x, num_groups_y, 1);

    // Unbind resources
    ID3DShaderResourceView* nullSRV[3] = {nullptr, nullptr, nullptr};
    ID3DUnorderedAccessView* nullUAV[1] = {nullptr};
    context->CSSetShaderResources(0, 3, nullSRV);
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

    // 1. Allocate page table for actual slot count
    page_table.resize(slot_count);

    // 2. Initialize all entries to NOT RESIDENT
    for (uint32_t i = 0; i < slot_count; i++) {
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
    bufferDesc.ByteWidth = slot_count * sizeof(uint32_t);

    // Initialize with 0xFFFFFFFF (all invalid)
    xr_vector<uint32_t> initial_data(slot_count, 0xFFFFFFFF);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = initial_data.data();

    CHK_DX(HW.pDevice->CreateBuffer(&bufferDesc, &initData, &indirection_buffer));

    // 5. Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = slot_count;

    CHK_DX(HW.pDevice->CreateShaderResourceView(indirection_buffer, &srvDesc, &indirection_srv));

    // 6. Create reverse mapping buffer (physical page → logical slot) for compute shader
    D3D11_BUFFER_DESC revmapDesc = {};
    revmapDesc.Usage = D3D11_USAGE_DYNAMIC;
    revmapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    revmapDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    revmapDesc.MiscFlags = 0;
    revmapDesc.ByteWidth = PHYSICAL_PAGES * sizeof(uint32_t);

    // Initialize with 0xFFFFFFFF (invalid)
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

    // 7. Reset stats
    ResetPageTableStats();

    Msg("* [DetailManager] Page table initialized: %u logical slots -> %u physical pages",
        slot_count, PHYSICAL_PAGES);
}

void CDetailManager::ShutdownPageTable()
{
    Msg("* [DetailManager] Shutting down page table...");

    // Release D3D resources
    _RELEASE(indirection_srv);
    _RELEASE(indirection_buffer);
    _RELEASE(physical_to_logical_srv);
    _RELEASE(physical_to_logical_buffer);

    // Clear vectors
    page_table.clear();
    page_table.shrink_to_fit();

    Msg("* [DetailManager] Page table shutdown complete");
}

//-----------------------------------------------------------------------------
// Phase 6B: Visibility-Driven Page Management
//-----------------------------------------------------------------------------

void CDetailManager::InitializeVisibilityReadback()
{
    Msg("* [DetailManager] Initializing visibility readback...");

    // 1. Create readback buffer (staging buffer for visible slot IDs)
    D3D11_BUFFER_DESC readbackDesc = {};
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.BindFlags = 0;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackDesc.MiscFlags = 0;
    readbackDesc.ByteWidth = MAX_VISIBLE_SLOTS * sizeof(uint32_t);

    CHK_DX(HW.pDevice->CreateBuffer(&readbackDesc, nullptr, &visible_slots_readback));

    // 2. Create query for synchronization
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_EVENT;
    CHK_DX(HW.pDevice->CreateQuery(&queryDesc, &readback_query));

    // 3. Initialize cache
    visible_slots_cache.reserve(MAX_VISIBLE_SLOTS);
    visible_slot_count = 0;

    // 4. Set upload budget
    max_uploads_per_frame = 16;  // Tune based on profiling

    // 5. Create GPU-side visible slot ID buffer
    D3D11_BUFFER_DESC visibleSlotsDesc = {};
    visibleSlotsDesc.Usage = D3D11_USAGE_DEFAULT;
    visibleSlotsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    visibleSlotsDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    visibleSlotsDesc.StructureByteStride = sizeof(uint32_t);
    visibleSlotsDesc.ByteWidth = MAX_VISIBLE_SLOTS * sizeof(uint32_t);

    CHK_DX(HW.pDevice->CreateBuffer(&visibleSlotsDesc, nullptr, &visible_slot_ids_gpu));

    // 6. Create UAV for visible slots
    D3D11_UNORDERED_ACCESS_VIEW_DESC visibleSlotsUAVDesc = {};
    visibleSlotsUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    visibleSlotsUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    visibleSlotsUAVDesc.Buffer.FirstElement = 0;
    visibleSlotsUAVDesc.Buffer.NumElements = MAX_VISIBLE_SLOTS;

    CHK_DX(HW.pDevice->CreateUnorderedAccessView(
        visible_slot_ids_gpu, &visibleSlotsUAVDesc, &visible_slot_ids_uav));

    // 7. Create atomic counter buffer (single uint) - must be ByteAddressBuffer for InterlockedAdd
    D3D11_BUFFER_DESC counterDesc = {};
    counterDesc.Usage = D3D11_USAGE_DEFAULT;
    counterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    counterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;  // Raw buffer for ByteAddressBuffer
    counterDesc.ByteWidth = sizeof(uint32_t);

    uint32_t zero = 0;
    D3D11_SUBRESOURCE_DATA counterInitData = {};
    counterInitData.pSysMem = &zero;

    CHK_DX(HW.pDevice->CreateBuffer(&counterDesc, &counterInitData, &visible_slot_counter_gpu));

    // 8. Create UAV for counter (raw buffer view)
    D3D11_UNORDERED_ACCESS_VIEW_DESC counterUAVDesc = {};
    counterUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // Raw buffer
    counterUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    counterUAVDesc.Buffer.FirstElement = 0;
    counterUAVDesc.Buffer.NumElements = 1;
    counterUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;  // Raw buffer flag

    CHK_DX(HW.pDevice->CreateUnorderedAccessView(
        visible_slot_counter_gpu, &counterUAVDesc, &visible_slot_counter_uav));

    // 9. Create upload staging buffer (for future use in Week 5)
    // DYNAMIC buffers require at least one bind flag
    D3D11_BUFFER_DESC stagingDesc = {};
    stagingDesc.Usage = D3D11_USAGE_DYNAMIC;
    stagingDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;  // Required for DYNAMIC usage
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.ByteWidth = STAGING_BUFFER_SIZE;

    CHK_DX(HW.pDevice->CreateBuffer(&stagingDesc, nullptr, &upload_staging_buffer));

    // 10. Persistent map (keep mapped for entire session)
    D3D11_MAPPED_SUBRESOURCE mapped;
    CHK_DX(HW.get_context(CHW::IMM_CTX_ID)->Map(upload_staging_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    staging_buffer_mapped = (uint8_t*)mapped.pData;

    // Phase 3: Initialize A-Life integration
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

    // Unmap staging buffer
    if (upload_staging_buffer) {
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(upload_staging_buffer, 0);
        staging_buffer_mapped = nullptr;
    }

    // Release GPU resources
    _RELEASE(upload_staging_buffer);
    _RELEASE(visible_slot_counter_uav);
    _RELEASE(visible_slot_counter_gpu);
    _RELEASE(visible_slot_ids_uav);
    _RELEASE(visible_slot_ids_gpu);
    _RELEASE(readback_query);
    _RELEASE(visible_slots_readback);

    // Clear vectors
    visible_slots_cache.clear();
    visible_slots_cache.shrink_to_fit();

    // Clear upload queue
    while (!upload_queue.empty()) {
        upload_queue.pop();
    }

    // Phase 3: Clear pending updates
    pending_updates.clear();
    pending_updates.shrink_to_fit();

    Msg("* [DetailManager] Visibility readback shutdown complete");
    Msg("* [DetailManager] A-Life integration shutdown complete");
}

void CDetailManager::ReadVisibleSlotsFromGPU()
{
    // 1. Copy counter from GPU to readback buffer
    uint32_t counter_value = 0;
    D3D11_BOX counterBox = { 0, 0, 0, sizeof(uint32_t), 1, 1 };
    HW.get_context(CHW::IMM_CTX_ID)->CopySubresourceRegion(
        visible_slots_readback, 0, 0, 0, 0,
        visible_slot_counter_gpu, 0, &counterBox);

    // 2. Map and read counter
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = HW.get_context(CHW::IMM_CTX_ID)->Map(visible_slots_readback, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        counter_value = *(uint32_t*)mapped.pData;
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(visible_slots_readback, 0);
    }

    // 3. Clamp to MAX_VISIBLE_SLOTS
    visible_slot_count = std::min(counter_value, MAX_VISIBLE_SLOTS);

    if (visible_slot_count == 0) {
        return;  // Nothing visible
    }

    // 4. Copy visible slot IDs from GPU to readback buffer
    HW.get_context(CHW::IMM_CTX_ID)->CopyResource(visible_slots_readback, visible_slot_ids_gpu);

    // 5. Insert query and wait (non-blocking check)
    HW.get_context(CHW::IMM_CTX_ID)->End(readback_query);

    // 6. Check if data ready (non-blocking)
    BOOL data_ready = FALSE;
    hr = HW.get_context(CHW::IMM_CTX_ID)->GetData(readback_query, &data_ready, sizeof(BOOL), 0);
    if (SUCCEEDED(hr) && data_ready) {
        // 7. Map and read visible slot IDs
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
    // else: Data not ready yet, use cached data from previous frame
}

void CDetailManager::RequestVisiblePages()
{
    for (uint32_t slot_id : visible_slots_cache) {
        if (slot_id >= slot_count) continue;  // Safety check

        // High priority for visible slots
        RequestPageWithPriority(slot_id, 255);
    }
}

void CDetailManager::RequestPageWithPriority(uint32_t logical_slot, uint8_t priority)
{
    if (logical_slot >= slot_count) return;

    // Check if already resident
    if (IsPageResident(logical_slot)) {
        // Update reference bit (mark as recently used)
        PageTableEntry& entry = page_table[logical_slot];
        entry.reference_bit = 1;
        entry.last_access_frame = Device.dwFrame;

        PhysicalPageInfo& phys = physical_pages[entry.physical_page];
        phys.reference_bit = 1;

        page_table_stats.cache_hits++;
        return;  // Already loaded
    }

    // Not resident - add to upload queue
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

        // Double-check still not resident (might have been loaded since request)
        if (IsPageResident(logical_slot)) {
            continue;
        }

        // Find victim page if atlas full
        uint16_t physical_page;
        if (resident_page_count < PHYSICAL_PAGES) {
            // Atlas not full - use next free page
            for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
                if (physical_pages[i].logical_slot == UINT32_MAX) {
                    physical_page = i;
                    break;
                }
            }
        } else {
            // Atlas full - evict victim
            physical_page = FindVictimPage();
            EvictPage(physical_page);
        }

        // Promote page
        PromotePage(logical_slot, physical_page);

        // TODO Week 5: Upload actual data from warm cache/disk
        // For now, promoted pages start with zeros (no persistence yet)

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
    VERIFY(logical_slot < slot_count);

    // Phase 5: Read back if dirty
    if (page_table[logical_slot].dirty_bit) {
        RequestSlotReadback(logical_slot);
    }

    // Update page table entry (mark as not resident)
    page_table[logical_slot].physical_page = INVALID_PAGE;
    page_table[logical_slot].reference_bit = 0;
    // NOTE: dirty_bit is NOT cleared here - it will be cleared by ProcessReadbackQueue
    // after the GPU→CPU readback completes (1-2 frames later)

    // Mark physical page as free
    page.logical_slot = UINT32_MAX;
    page.reference_bit = 0;

    resident_page_count--;
    page_table_stats.evictions++;

    // Msg("* [DetailManager] Evicted slot %u from physical page %u", logical_slot, physical_page);
}

void CDetailManager::PromotePage(uint32_t logical_slot, uint16_t physical_page)
{
    VERIFY(logical_slot < slot_count);
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

    // Phase 5: Load data before applying pending updates
    bool data_loaded = false;

    // 1. Try warm cache first
    if (LoadSlotFromWarmCache(logical_slot, compression_work_buffer)) {
        data_loaded = true;
        Msg("* [PageTable] Loaded slot %u from warm cache", logical_slot);
    }
    // 2. Try disk if not in cache
    else {
        size_t loaded_size = 0;
        uint8_t compressed_buffer[1024];

        if (LoadSlotFromDisk(logical_slot, compressed_buffer, &loaded_size)) {
            // Decompress
            DecompressSlotData(compressed_buffer, loaded_size, compression_work_buffer);
            data_loaded = true;

            // Add to warm cache for future access
            SaveSlotToWarmCache(logical_slot, compression_work_buffer, UNCOMPRESSED_SLOT_SIZE);

            Msg("* [PageTable] Loaded slot %u from disk", logical_slot);
        }
    }

    // 3. If no data found, initialize with zeros (new slot)
    if (!data_loaded) {
        memset(compression_work_buffer, 0, UNCOMPRESSED_SLOT_SIZE);
        //Msg("* [PageTable] Initialized new slot %u (no saved data)", logical_slot);
    }

    // 4. Upload to GPU atlas
    UploadSlotDataToGPU(physical_page, compression_work_buffer);

    // Phase 3: Apply any pending A-Life updates for this slot
    ApplyPendingUpdatesToSlot(logical_slot);

    // Msg("* [DetailManager] Promoted slot %u to physical page %u", logical_slot, physical_page);
}

uint16_t CDetailManager::RequestPage(uint32_t logical_slot, uint8_t priority)
{
    VERIFY(logical_slot < slot_count);

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

    for (uint32_t i = 0; i < slot_count; i++) {
        // Pack: physical_page (16 bits) | mip_level (8 bits) | flags (8 bits)
        uint32_t packed = page_table[i].physical_page;  // Lower 16 bits
        packed |= (uint32_t(page_table[i].mip_level) << 16);  // Bits 16-23
        // Bits 24-31 reserved for flags

        data[i] = packed;
    }

    context->Unmap(indirection_buffer, 0);

    // Update reverse mapping buffer (physical → logical)
    if (physical_to_logical_buffer)
    {
        D3D11_MAPPED_SUBRESOURCE revmap_mapped;
        hr = context->Map(physical_to_logical_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &revmap_mapped);

        if (SUCCEEDED(hr)) {
            uint32_t* revmap_data = (uint32_t*)revmap_mapped.pData;

            // Initialize all as invalid
            for (uint16_t i = 0; i < PHYSICAL_PAGES; i++) {
                revmap_data[i] = 0xFFFFFFFF;
            }

            // Fill in resident mappings
            for (uint16_t phys_page = 0; phys_page < PHYSICAL_PAGES; phys_page++) {
                uint32_t logical_slot = physical_pages[phys_page].logical_slot;
                if (logical_slot != UINT32_MAX) {
                    revmap_data[phys_page] = logical_slot;
                }
            }

            context->Unmap(physical_to_logical_buffer, 0);
        }
    }

    // Msg("* [DetailManager] Updated indirection buffer (%u resident pages)", resident_page_count);
}

void CDetailManager::UpdatePageTable()
{
    // Phase 6B: Visibility-driven page management
    // This runs AFTER detail_cull.cs has identified visible slots
    // and BEFORE we render grass

    // 1. Read visible slots from GPU culling (non-blocking)
    ReadVisibleSlotsFromGPU();

    // 2. Request pages for visible slots (high priority)
    RequestVisiblePages();

    // 3. IMPORTANT: Also request pages for slots containing interactive entities
    // (entities may be behind camera, not visible, but still need interaction pages loaded)
    int x_offs = dtH.x_offs();
    int z_offs = dtH.z_offs();
    uint32_t x_size = dtH.x_size();
    uint32_t z_size = dtH.z_size();

    for (const auto& entity : interactive_entities) {
        // Calculate which slot this entity is in
        int slot_x = int(floorf(entity.position.x / DETAIL_SLOT_SIZE));
        int slot_z = int(floorf(entity.position.z / DETAIL_SLOT_SIZE));

        // Convert to slot array index
        int sx_local = slot_x + x_offs;
        int sz_local = slot_z + z_offs;

        // Bounds check
        if (sx_local >= 0 && sx_local < (int)x_size && sz_local >= 0 && sz_local < (int)z_size) {
            uint32_t slot_idx = sz_local * x_size + sx_local;
            if (slot_idx < slot_count) {
                RequestPageWithPriority(slot_idx, 255);  // High priority for interaction slots
            }
        }
    }

    // Phase 3: Process pending A-Life updates
    ProcessPendingUpdates();

    // 4. Process upload queue (promote up to N pages this frame)
    ProcessUploadQueue();

    // Phase 5: Process GPU→CPU readbacks (NEW)
    ProcessReadbackQueue();

    // 5. Update stats
    page_table_stats.total_requests += visible_slot_count;
    page_table_stats.current_resident = resident_page_count;

    // 6. Debug: Log culling effectiveness (every 60 frames)
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

// ===========================================================================================
// Phase 3 (Week 5-6): A-Life Integration with Deferred Updates
// ===========================================================================================

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

    // Check page table dirty bit
    if (IsPageResident(logical_slot) && page_table[logical_slot].dirty_bit) {
        return true;
    }

    // Check pending updates queue
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

    // 1. Compute which slot this position is in
    // Note: using your existing slot_size and slot grid setup
    const float slot_half = DETAIL_SLOT_SIZE_2;
    uint32_t slot_x = uint32_t((world_pos.x + slot_half) / DETAIL_SLOT_SIZE);
    uint32_t slot_z = uint32_t((world_pos.z + slot_half) / DETAIL_SLOT_SIZE);

    // Convert to slot index (Note: adapt this based on your actual slot indexing)
    // This assumes slots are indexed row-major: slot_idx = slot_z * width + slot_x
    // You may need to adjust based on how slot_aabbs are indexed
    uint32_t logical_slot = slot_z * 600 + slot_x;  // FIXME: Use actual world grid dimensions

    if (logical_slot >= slot_count) {
        return;  // Out of bounds
    }

    // 2. Check if slot is resident
    if (IsPageResident(logical_slot)) {
        // Immediate update - slot is in atlas
        alife_stats.immediate_updates++;

        // Get physical page
        uint16_t physical_page = page_table[logical_slot].physical_page;

        // Apply update via GPU compute shader
        ApplyInteractionUpdateGPU(physical_page, world_pos, radius, strength);

        // Mark page dirty (needs writeback eventually)
        page_table[logical_slot].dirty_bit = 1;

        Msg("* [A-Life] IMMEDIATE update at slot %u (phys page %u) - pos (%.1f, %.1f, %.1f) r=%.2f s=%.2f",
            logical_slot, physical_page, world_pos.x, world_pos.y, world_pos.z, radius, strength);

    } else {
        // Deferred update - slot not resident yet
        alife_stats.deferred_updates++;

        // Add to pending queue
        PendingInteractionUpdate pending;
        pending.logical_slot = logical_slot;
        pending.world_position = world_pos;
        pending.radius = radius;
        pending.strength = strength;
        pending.timestamp = Device.dwFrame;
        pending.interaction_type = type;

        pending_updates.push_back(pending);

        // Trim queue if too large (FIFO)
        if (pending_updates.size() > max_pending_updates) {
            pending_updates.erase(pending_updates.begin());
            alife_stats.expired_updates++;
        }

        // Request page with DIRTY priority (higher than normal, lower than visible)
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
            // Too old - discard
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
        return;  // Can't apply - slot not loaded yet
    }

    // Get physical page
    uint16_t physical_page = page_table[logical_slot].physical_page;

    // Find all pending updates for this slot
    uint32_t applied_count = 0;
    auto it = pending_updates.begin();
    while (it != pending_updates.end()) {
        if (it->logical_slot == logical_slot) {
            // Found matching update - apply it
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

            // Remove from pending queue
            it = pending_updates.erase(it);
        } else {
            ++it;
        }
    }

    // Mark page dirty if we applied anything
    if (applied_count > 0) {
        page_table[logical_slot].dirty_bit = 1;
        Msg("* [A-Life] Applied %u deferred updates to slot %u (queue remaining: %zu)",
            applied_count, logical_slot, pending_updates.size());
    }
}

void CDetailManager::ProcessPendingUpdates()
{
    // 1. Expire old pending updates (>30 seconds = ~1800 frames at 60fps)
    ExpireOldPendingUpdates(1800);

    // 2. Request pages for dirty slots (if not already in queue)
    // Use a set to get unique dirty slots
    xr_set<uint32_t> dirty_slots;
    for (const auto& update : pending_updates) {
        dirty_slots.insert(update.logical_slot);
    }

    for (uint32_t slot : dirty_slots) {
        if (!IsPageResident(slot)) {
            // Request with DIRTY priority (lower than visible, but still important)
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
        return;  // Shader not loaded
    }

    if (!interaction_atlas || !interaction_uav) {
        return;  // Atlas not created
    }

    // 1. Fill constant buffer
    InteractionUpdateCB cb_data;
    cb_data.world_center = Fvector2().set(world_center.x, world_center.z);
    cb_data.radius = radius;
    cb_data.strength = strength;
    cb_data.physical_page = physical_page;
    cb_data.slot_size = DETAIL_SLOT_SIZE;  // Your existing member (DETAIL_SLOT_SIZE = 2.0)
    cb_data.atlas_width = (float)atlas_width;  // 2048
    cb_data.slot_texture_size = (float)slot_texture_size;  // 32

    // 2. Map and update constant buffer
    auto* context = HW.get_context(CHW::IMM_CTX_ID);
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(interaction_update_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (FAILED(hr)) {
        Msg("! [A-Life] Failed to map interaction update CB");
        return;
    }

    memcpy(mapped.pData, &cb_data, sizeof(cb_data));
    context->Unmap(interaction_update_cb, 0);

    // 3. Bind resources
    ID3D11ComputeShader* cs = interaction_update_cs->sh;
    context->CSSetShader(cs, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &interaction_update_cb);
    context->CSSetUnorderedAccessViews(0, 1, &interaction_uav, nullptr);

    // 4. Dispatch (4×4 thread groups for 32×32 slot with 8×8 threads per group)
    context->Dispatch(4, 4, 1);

    // 5. Unbind UAV (important - grass shaders will sample this as SRV)
    ID3D11UnorderedAccessView* null_uav = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

    // Optional: Add debug logging (disabled by default for performance)
    // Msg("* [A-Life] Applied GPU interaction update to physical page %u", physical_page);
}

// ===========================================================================================
// Thread-Safe A-Life Integration (for calls from A-Life thread)
// ===========================================================================================

void CDetailManager::RequestInteractionUpdateThreadSafe(
    const Fvector& world_pos,
    float radius,
    float strength,
    uint8_t type)
{
    // Queue the request to be processed on the render thread
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
    // Called on render thread - process all queued requests
    if (threadsafe_request_queue.empty())
        return;

    threadsafe_queue_lock.Enter();

    // Copy and clear the queue
    xr_vector<ThreadSafeInteractionRequest> requests_copy;
    requests_copy.swap(threadsafe_request_queue);

    threadsafe_queue_lock.Leave();

    // Process all requests (now safe - we're on render thread)
    for (const auto& req : requests_copy)
    {
        RequestInteractionUpdate(req.world_position, req.radius, req.strength, req.type);
    }
}

// Phase 5: Warm Cache Implementation

void CDetailManager::InitializeWarmCache()
{
    Msg("* [DetailManager] Initializing warm cache...");

    // 1. Allocate warm cache (64K entries)
    warm_cache.resize(WARM_CACHE_SLOTS);

    // 2. Initialize all entries as free
    warm_cache_free_list.reserve(WARM_CACHE_SLOTS);
    for (uint32_t i = 0; i < WARM_CACHE_SLOTS; i++) {
        warm_cache[i].world_slot_id = UINT32_MAX;  // Invalid
        warm_cache[i].last_access_frame = 0;
        warm_cache[i].dirty_flag = 0;
        warm_cache[i].compressed_size = 0;

        warm_cache_free_list.push_back(i);
    }

    // 3. Clear lookup map
    world_to_warm_index.clear();

    // 4. Allocate compression work buffer
    compression_work_buffer = (uint8_t*)xr_malloc(UNCOMPRESSED_SLOT_SIZE);

    Msg("* [DetailManager] Warm cache initialized: %u slots (%.1f MB)",
        WARM_CACHE_SLOTS,
        (WARM_CACHE_SLOTS * sizeof(WarmCacheEntry)) / (1024.0f * 1024.0f));

    // 5. Initialize readback pipeline
    InitializeReadbackPipeline();

    // 6. Initialize disk persistence
    InitializePersistence();
}

void CDetailManager::ShutdownWarmCache()
{
    Msg("* [DetailManager] Shutting down warm cache...");

    // 1. Shutdown readback pipeline first
    ShutdownReadbackPipeline();

    // 2. Shutdown disk persistence (flushes pending saves)
    ShutdownPersistence();

    // 3. Free memory
    xr_free(compression_work_buffer);
    compression_work_buffer = nullptr;

    warm_cache.clear();
    warm_cache.shrink_to_fit();
    world_to_warm_index.clear();
    warm_cache_free_list.clear();

    Msg("* [DetailManager] Warm cache shutdown complete");
}

uint32_t CDetailManager::FindWarmCacheLRUVictim()
{
    uint32_t lru_idx = 0;
    uint64_t oldest_frame = UINT64_MAX;

    for (uint32_t i = 0; i < WARM_CACHE_SLOTS; i++) {
        if (warm_cache[i].world_slot_id == UINT32_MAX) {
            continue;  // Free slot
        }

        if (warm_cache[i].last_access_frame < oldest_frame) {
            oldest_frame = warm_cache[i].last_access_frame;
            lru_idx = i;
        }
    }

    return lru_idx;
}

void CDetailManager::EvictFromWarmCache(uint32_t logical_slot)
{
    auto it = world_to_warm_index.find(logical_slot);
    if (it == world_to_warm_index.end()) {
        return;  // Not in cache
    }

    uint32_t warm_idx = it->second;
    WarmCacheEntry& entry = warm_cache[warm_idx];

    // Writeback if dirty
    if (entry.dirty_flag) {
        QueueSlotForSave(logical_slot);
    }

    // Mark as free
    entry.world_slot_id = UINT32_MAX;
    entry.dirty_flag = 0;
    entry.compressed_size = 0;

    // Add to free list
    warm_cache_free_list.push_back(warm_idx);

    // Remove from lookup map
    world_to_warm_index.erase(it);
}

// Simple BC3/DXT5 compression helper - compresses a 4×4 block
static void CompressDXT5Block(const uint8_t* block, uint8_t* out)
{
    // DXT5 format: 8 bytes alpha + 8 bytes color
    // Alpha block: 2 alpha endpoints + 48 bits of 3-bit indices
    uint8_t alpha_min = 255, alpha_max = 0;

    // Find alpha range
    for (int i = 0; i < 16; i++) {
        uint8_t a = block[i * 4 + 3];
        if (a < alpha_min) alpha_min = a;
        if (a > alpha_max) alpha_max = a;
    }

    // Write alpha endpoints
    out[0] = alpha_max;
    out[1] = alpha_min;

    // Generate alpha indices (simple quantization)
    uint64_t alpha_bits = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t a = block[i * 4 + 3];
        int index;
        if (alpha_max > alpha_min) {
            index = (int)(((a - alpha_min) * 7.0f) / (alpha_max - alpha_min));
            index = std::min(std::max(index, 0), 7);
        } else {
            index = 0;
        }
        alpha_bits |= ((uint64_t)index << (i * 3));
    }

    // Write 48 bits of alpha indices
    for (int i = 0; i < 6; i++) {
        out[2 + i] = (uint8_t)((alpha_bits >> (i * 8)) & 0xFF);
    }

    // Color block: 2 RGB565 endpoints + 32 bits of 2-bit indices
    // Find color range (simple: use first and last pixel)
    uint8_t r_min = 255, r_max = 0, g_min = 255, g_max = 0, b_min = 255, b_max = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t r = block[i * 4 + 0];
        uint8_t g = block[i * 4 + 1];
        uint8_t b = block[i * 4 + 2];
        if (r < r_min) r_min = r;
        if (r > r_max) r_max = r;
        if (g < g_min) g_min = g;
        if (g > g_max) g_max = g;
        if (b < b_min) b_min = b;
        if (b > b_max) b_max = b;
    }

    // Convert to RGB565
    uint16_t color0 = ((r_max >> 3) << 11) | ((g_max >> 2) << 5) | (b_max >> 3);
    uint16_t color1 = ((r_min >> 3) << 11) | ((g_min >> 2) << 5) | (b_min >> 3);

    // Write color endpoints
    out[8] = (uint8_t)(color0 & 0xFF);
    out[9] = (uint8_t)((color0 >> 8) & 0xFF);
    out[10] = (uint8_t)(color1 & 0xFF);
    out[11] = (uint8_t)((color1 >> 8) & 0xFF);

    // Generate color indices (simple nearest-neighbor)
    uint32_t color_indices = 0;
    for (int i = 0; i < 16; i++) {
        // Simple: use index based on luminance
        uint8_t r = block[i * 4 + 0];
        uint8_t g = block[i * 4 + 1];
        uint8_t b = block[i * 4 + 2];
        int luma = (r + g + b) / 3;
        int index = (luma > 128) ? 0 : 1;
        color_indices |= (index << (i * 2));
    }

    // Write 32 bits of color indices
    out[12] = (uint8_t)(color_indices & 0xFF);
    out[13] = (uint8_t)((color_indices >> 8) & 0xFF);
    out[14] = (uint8_t)((color_indices >> 16) & 0xFF);
    out[15] = (uint8_t)((color_indices >> 24) & 0xFF);
}

void CDetailManager::CompressSlotData(
    const uint8_t* uncompressed,
    uint8_t* compressed,
    size_t* out_size)
{
    // BC3/DXT5: 4×4 blocks, 16 bytes per block
    // 32×32 = 64 blocks (8×8), 64 × 16 = 1024 bytes compressed

    const uint32_t block_size = 4;  // 4×4 pixels per block
    const uint32_t blocks_x = 32 / block_size;  // 8
    const uint32_t blocks_y = 32 / block_size;  // 8

    uint8_t* dst = compressed;

    // Compress each 4×4 block
    for (uint32_t by = 0; by < blocks_y; by++) {
        for (uint32_t bx = 0; bx < blocks_x; bx++) {
            // Extract 4×4 block from source
            uint8_t block[64];  // 4×4 × RGBA = 64 bytes

            for (uint32_t py = 0; py < block_size; py++) {
                for (uint32_t px = 0; px < block_size; px++) {
                    uint32_t sx = bx * block_size + px;
                    uint32_t sy = by * block_size + py;
                    uint32_t src_offset = (sy * 32 + sx) * 4;
                    uint32_t dst_offset = (py * block_size + px) * 4;

                    block[dst_offset + 0] = uncompressed[src_offset + 0];
                    block[dst_offset + 1] = uncompressed[src_offset + 1];
                    block[dst_offset + 2] = uncompressed[src_offset + 2];
                    block[dst_offset + 3] = uncompressed[src_offset + 3];
                }
            }

            // Compress block to BC3/DXT5
            CompressDXT5Block(block, dst);
            dst += 16;  // 16 bytes per compressed block
        }
    }

    *out_size = 1024;  // Always 1024 bytes for 32×32
}

// Simple BC3/DXT5 decompression helper - decompresses a 4×4 block
static void DecompressDXT5Block(const uint8_t* in, uint8_t* block)
{
    // Read alpha endpoints
    uint8_t alpha0 = in[0];
    uint8_t alpha1 = in[1];

    // Read 48 bits of alpha indices
    uint64_t alpha_bits = 0;
    for (int i = 0; i < 6; i++) {
        alpha_bits |= ((uint64_t)in[2 + i] << (i * 8));
    }

    // Interpolate alpha palette
    uint8_t alpha_palette[8];
    alpha_palette[0] = alpha0;
    alpha_palette[1] = alpha1;
    if (alpha0 > alpha1) {
        for (int i = 1; i < 7; i++) {
            alpha_palette[i + 1] = (uint8_t)(((7 - i) * alpha0 + i * alpha1) / 7);
        }
    } else {
        for (int i = 1; i < 5; i++) {
            alpha_palette[i + 1] = (uint8_t)(((5 - i) * alpha0 + i * alpha1) / 5);
        }
        alpha_palette[6] = 0;
        alpha_palette[7] = 255;
    }

    // Decode alpha values
    for (int i = 0; i < 16; i++) {
        int index = (int)((alpha_bits >> (i * 3)) & 0x7);
        block[i * 4 + 3] = alpha_palette[index];
    }

    // Read color endpoints (RGB565)
    uint16_t color0 = in[8] | (in[9] << 8);
    uint16_t color1 = in[10] | (in[11] << 8);

    // Extract RGB from RGB565
    uint8_t r0 = (uint8_t)(((color0 >> 11) & 0x1F) << 3);
    uint8_t g0 = (uint8_t)(((color0 >> 5) & 0x3F) << 2);
    uint8_t b0 = (uint8_t)((color0 & 0x1F) << 3);

    uint8_t r1 = (uint8_t)(((color1 >> 11) & 0x1F) << 3);
    uint8_t g1 = (uint8_t)(((color1 >> 5) & 0x3F) << 2);
    uint8_t b1 = (uint8_t)((color1 & 0x1F) << 3);

    // Interpolate color palette
    uint8_t color_palette[12];
    color_palette[0] = r0; color_palette[1] = g0; color_palette[2] = b0;
    color_palette[3] = r1; color_palette[4] = g1; color_palette[5] = b1;
    color_palette[6] = (2 * r0 + r1) / 3; color_palette[7] = (2 * g0 + g1) / 3; color_palette[8] = (2 * b0 + b1) / 3;
    color_palette[9] = (r0 + 2 * r1) / 3; color_palette[10] = (g0 + 2 * g1) / 3; color_palette[11] = (b0 + 2 * b1) / 3;

    // Read 32 bits of color indices
    uint32_t color_indices = in[12] | (in[13] << 8) | (in[14] << 16) | (in[15] << 24);

    // Decode color values
    for (int i = 0; i < 16; i++) {
        int index = (color_indices >> (i * 2)) & 0x3;
        block[i * 4 + 0] = color_palette[index * 3 + 0];
        block[i * 4 + 1] = color_palette[index * 3 + 1];
        block[i * 4 + 2] = color_palette[index * 3 + 2];
    }
}

void CDetailManager::DecompressSlotData(
    const uint8_t* compressed,
    size_t compressed_size,
    uint8_t* uncompressed)
{
    if (compressed_size != 1024) {
        Msg("! [Compression] Invalid compressed size: %zu (expected 1024)", compressed_size);
        memset(uncompressed, 0, UNCOMPRESSED_SLOT_SIZE);
        return;
    }

    const uint32_t block_size = 4;
    const uint32_t blocks_x = 8;
    const uint32_t blocks_y = 8;

    const uint8_t* src = compressed;

    // Decompress each block
    for (uint32_t by = 0; by < blocks_y; by++) {
        for (uint32_t bx = 0; bx < blocks_x; bx++) {
            // Decompress block from BC3/DXT5
            uint8_t block[64];
            DecompressDXT5Block(src, block);

            // Copy block to destination
            for (uint32_t py = 0; py < block_size; py++) {
                for (uint32_t px = 0; px < block_size; px++) {
                    uint32_t dx = bx * block_size + px;
                    uint32_t dy = by * block_size + py;
                    uint32_t dst_offset = (dy * 32 + dx) * 4;
                    uint32_t src_offset = (py * block_size + px) * 4;

                    uncompressed[dst_offset + 0] = block[src_offset + 0];
                    uncompressed[dst_offset + 1] = block[src_offset + 1];
                    uncompressed[dst_offset + 2] = block[src_offset + 2];
                    uncompressed[dst_offset + 3] = block[src_offset + 3];
                }
            }

            src += 16;
        }
    }
}

void CDetailManager::SaveSlotToWarmCache(
    uint32_t logical_slot,
    const uint8_t* data,
    size_t size)
{
    if (size != UNCOMPRESSED_SLOT_SIZE) {
        Msg("! [WarmCache] Invalid data size: %zu", size);
        return;
    }

    // 1. Check if already in warm cache
    auto it = world_to_warm_index.find(logical_slot);
    uint32_t warm_idx;

    if (it != world_to_warm_index.end()) {
        // Update existing entry
        warm_idx = it->second;
    } else {
        // Allocate new entry
        if (warm_cache_free_list.empty()) {
            // Warm cache full - evict LRU victim
            warm_idx = FindWarmCacheLRUVictim();
            EvictFromWarmCache(warm_cache[warm_idx].world_slot_id);
        } else {
            // Use free slot
            warm_idx = warm_cache_free_list.back();
            warm_cache_free_list.pop_back();
        }

        world_to_warm_index[logical_slot] = warm_idx;
    }

    // 2. Compress data
    WarmCacheEntry& entry = warm_cache[warm_idx];
    entry.world_slot_id = logical_slot;
    entry.last_access_frame = Device.dwFrame;

    size_t compressed_size = 0;
    CompressSlotData(data, entry.compressed_data, &compressed_size);
    entry.compressed_size = (uint16_t)compressed_size;

    entry.dirty_flag = 1;  // Needs writeback to disk

    Msg("* [WarmCache] Saved slot %u (compressed %u bytes)", logical_slot, entry.compressed_size);
}

bool CDetailManager::LoadSlotFromWarmCache(uint32_t logical_slot, uint8_t* out_data)
{
    // Check if in warm cache
    auto it = world_to_warm_index.find(logical_slot);
    if (it == world_to_warm_index.end()) {
        return false;  // Not in cache
    }

    uint32_t warm_idx = it->second;
    WarmCacheEntry& entry = warm_cache[warm_idx];

    // Update LRU timestamp
    entry.last_access_frame = Device.dwFrame;

    // Decompress data
    DecompressSlotData(entry.compressed_data, entry.compressed_size, out_data);

    Msg("* [WarmCache] Loaded slot %u from cache", logical_slot);
    return true;
}

// Phase 5B: GPU Readback Pipeline

void CDetailManager::InitializeReadbackPipeline()
{
    Msg("* [DetailManager] Initializing readback pipeline...");

    // Size for one 32×32 RGBA16F slot = 8KB
    const uint32_t slot_byte_size = 32 * 32 * 4 * sizeof(uint16_t);  // RGBA16F
    const uint32_t staging_size = slot_byte_size * MAX_READBACKS_PER_FRAME;

    // Create double-buffered staging buffers
    for (int i = 0; i < 2; i++) {
        D3D11_BUFFER_DESC stagingDesc = {};
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.ByteWidth = staging_size;

        CHK_DX(HW.pDevice->CreateBuffer(&stagingDesc, nullptr, &readback_staging_buffers[i]));

        // Create query for synchronization
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query = D3D11_QUERY_EVENT;
        CHK_DX(HW.pDevice->CreateQuery(&queryDesc, &readback_queries[i]));
    }

    readback_buffer_index = 0;

    // Create staging texture for intermediate copy
    D3D11_TEXTURE2D_DESC stagingTexDesc = {};
    stagingTexDesc.Width = 32;
    stagingTexDesc.Height = 32;
    stagingTexDesc.MipLevels = 1;
    stagingTexDesc.ArraySize = 1;
    stagingTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // Match atlas format
    stagingTexDesc.SampleDesc.Count = 1;
    stagingTexDesc.Usage = D3D11_USAGE_STAGING;
    stagingTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    CHK_DX(HW.pDevice->CreateTexture2D(&stagingTexDesc, nullptr, &readback_staging_texture));

    Msg("* [DetailManager] Readback pipeline initialized (%u KB per buffer, staging texture 32×32 RGBA16F)",
        staging_size / 1024);
}

void CDetailManager::ShutdownReadbackPipeline()
{
    Msg("* [DetailManager] Shutting down readback pipeline...");

    for (int i = 0; i < 2; i++) {
        _RELEASE(readback_queries[i]);
        _RELEASE(readback_staging_buffers[i]);
        pending_readbacks[i].clear();
    }

    _RELEASE(readback_staging_texture);
    readback_queue.clear();

    Msg("* [DetailManager] Readback pipeline shutdown complete");
}

void CDetailManager::RequestSlotReadback(uint32_t logical_slot)
{
    if (logical_slot >= slot_count) return;

    // Check if slot is resident and dirty
    if (!IsPageResident(logical_slot)) return;

    const PageTableEntry& entry = page_table[logical_slot];
    if (!entry.dirty_bit) return;  // Not dirty, no need to read back

    // Add to readback queue
    ReadbackRequest request;
    request.logical_slot = logical_slot;
    request.physical_page = entry.physical_page;
    request.request_frame = Device.dwFrame;

    readback_queue.push_back(request);
}

void CDetailManager::ReadbackSlotFromGPU(uint32_t logical_slot, uint16_t physical_page)
{
    auto context = HW.get_context(CHW::IMM_CTX_ID);

    // 1. Compute source region in atlas
    uint32_t page_x = physical_page % 64;
    uint32_t page_y = physical_page / 64;

    D3D11_BOX sourceBox = {};
    sourceBox.left = page_x * 32;
    sourceBox.top = page_y * 32;
    sourceBox.right = sourceBox.left + 32;
    sourceBox.bottom = sourceBox.top + 32;
    sourceBox.front = 0;
    sourceBox.back = 1;

    // 2. Copy from atlas texture to staging texture
    context->CopySubresourceRegion(
        readback_staging_texture,
        0,           // Dest subresource
        0, 0, 0,     // Dest x,y,z
        interaction_atlas,
        0,           // Source subresource
        &sourceBox);

    // 3. Track this pending readback
    uint32_t current_buffer = readback_buffer_index;
    PendingReadback pending;
    pending.logical_slot = logical_slot;
    pending.physical_page = physical_page;
    pending_readbacks[current_buffer].push_back(pending);

    // 4. Insert query
    context->End(readback_queries[current_buffer]);
}

// Helper: half-precision to float conversion
float CDetailManager::HalfToFloat(uint16_t h)
{
    uint32_t sign = (h >> 15) & 0x1;
    uint32_t exponent = (h >> 10) & 0x1F;
    uint32_t mantissa = h & 0x3FF;

    if (exponent == 0) {
        if (mantissa == 0) return sign ? -0.0f : 0.0f;
        // Denormalized
        exponent = 1;
    } else if (exponent == 31) {
        // Inf or NaN
        if (sign)
            return -std::numeric_limits<float>::infinity();
        else
            return std::numeric_limits<float>::infinity();
    }

    uint32_t f_exp = exponent - 15 + 127;
    uint32_t f_mantissa = mantissa << 13;
    uint32_t f_bits = (sign << 31) | (f_exp << 23) | f_mantissa;

    return *(float*)&f_bits;
}

// Helper: float to half-precision (FP16)
uint16_t CDetailManager::FloatToHalf(float f)
{
    uint32_t x = *(uint32_t*)&f;
    uint32_t sign = (x >> 31) & 0x1;
    uint32_t exponent = (x >> 23) & 0xFF;
    uint32_t mantissa = x & 0x7FFFFF;

    // Convert exponent
    int exp_half = (int)exponent - 127 + 15;
    if (exp_half <= 0) return (uint16_t)(sign << 15);  // Zero/denorm
    if (exp_half >= 31) return (uint16_t)((sign << 15) | 0x7C00);  // Infinity

    // Convert mantissa
    uint32_t mant_half = mantissa >> 13;

    return (uint16_t)((sign << 15) | ((uint32_t)exp_half << 10) | mant_half);
}

void CDetailManager::ProcessReadbackQueue()
{
    auto context = HW.get_context(CHW::IMM_CTX_ID);

    // 1. Check if previous frame's readback is ready
    uint32_t prev_buffer = (readback_buffer_index + 1) % 2;

    BOOL data_ready = FALSE;
    HRESULT hr = context->GetData(readback_queries[prev_buffer], &data_ready, sizeof(BOOL),
                                   D3D11_ASYNC_GETDATA_DONOTFLUSH);

    if (SUCCEEDED(hr) && data_ready && !pending_readbacks[prev_buffer].empty()) {
        // 2. Map staging texture and read data
        D3D11_MAPPED_SUBRESOURCE mapped;
        hr = context->Map(readback_staging_texture, 0, D3D11_MAP_READ, 0, &mapped);

        if (SUCCEEDED(hr)) {
            // 3. Process each slot from previous frame
            for (const auto& pending : pending_readbacks[prev_buffer]) {
                // Copy data to work buffer
                const uint8_t* src = (const uint8_t*)mapped.pData;

                // Convert RGBA16F → RGBA8 for compression
                uint16_t* src_16f = (uint16_t*)src;
                uint8_t* dst_8 = compression_work_buffer;

                for (uint32_t i = 0; i < 32 * 32; i++) {
                    // Convert FP16 to byte (simple: clamp and scale)
                    for (int c = 0; c < 4; c++) {
                        float val = HalfToFloat(src_16f[i * 4 + c]);
                        dst_8[i * 4 + c] = (uint8_t)std::min(std::max(val * 255.0f, 0.0f), 255.0f);
                    }
                }

                // Save to warm cache
                SaveSlotToWarmCache(pending.logical_slot, compression_work_buffer, UNCOMPRESSED_SLOT_SIZE);

                // Clear dirty bit
                page_table[pending.logical_slot].dirty_bit = 0;
            }

            context->Unmap(readback_staging_texture, 0);

            Msg("* [Readback] Processed %zu slot readbacks", pending_readbacks[prev_buffer].size());
        }

        pending_readbacks[prev_buffer].clear();
    }

    // 4. Process new readback requests (up to MAX_READBACKS_PER_FRAME)
    uint32_t readbacks_this_frame = 0;
    auto it = readback_queue.begin();

    while (it != readback_queue.end() && readbacks_this_frame < MAX_READBACKS_PER_FRAME) {
        ReadbackSlotFromGPU(it->logical_slot, it->physical_page);
        it = readback_queue.erase(it);
        readbacks_this_frame++;
    }

    // 5. Flip buffer
    readback_buffer_index = (readback_buffer_index + 1) % 2;
}

// Phase 5D: Disk Persistence

void CDetailManager::InitializePersistence()
{
    Msg("* [DetailManager] Initializing disk persistence...");

    // 1. Set database path (user save folder)
    FS.update_path(db_path, "$app_data_root$", "grass_interaction.dat");

    // 2. Check if file exists, load existing data if so
    if (FS.exist(db_path)) {
        Msg("* [Persistence] Found existing save file: %s", db_path);
        // Load will happen on-demand when slots requested
    } else {
        Msg("* [Persistence] No existing save file, will create new");
    }

    // 3. Start background save thread
    save_thread_running = true;
    save_thread = std::thread([this]() { this->BackgroundSaveThread(); });

    Msg("* [DetailManager] Disk persistence initialized: %s", db_path);
}

void CDetailManager::ShutdownPersistence()
{
    Msg("* [DetailManager] Shutting down disk persistence...");

    // 1. Stop background thread
    save_thread_running = false;
    if (save_thread.joinable()) {
        save_thread.join();
    }

    // 2. Flush pending saves
    FlushPendingSaves();

    Msg("* [DetailManager] Disk persistence shutdown complete");
}

void CDetailManager::SaveSlotToDisk(uint32_t logical_slot, const uint8_t* data, size_t size)
{
    // Simple format: append records to file
    // Record format: [slot_id:u32][size:u16][data:bytes]

    try {
        IWriter* W = FS.w_open_ex(db_path);  // w_open_ex opens in append mode
        if (W) {
            W->w_u32(logical_slot);
            W->w_u16((u16)size);
            W->w(data, size);
            FS.w_close(W);
        } else {
            Msg("! [Persistence] Failed to open file for writing: %s", db_path);
            // Disable further saves to avoid spam
            save_thread_running = false;
        }
    } catch (...) {
        Msg("! [Persistence] Exception while saving slot %u - disabling persistence", logical_slot);
        save_thread_running = false;
    }
}

bool CDetailManager::LoadSlotFromDisk(uint32_t logical_slot, uint8_t* out_data, size_t* out_size)
{
    if (!FS.exist(db_path)) {
        return false;
    }

    try {
        IReader* R = FS.r_open(db_path);
        if (!R) return false;

        // Linear search through file (simple, not optimal but works)
        while (!R->eof()) {
            u32 file_slot_id = R->r_u32();
            u16 data_size = R->r_u16();

            if (file_slot_id == logical_slot) {
                // Found it!
                *out_size = data_size;
                R->r(out_data, data_size);
                FS.r_close(R);
                return true;
            } else {
                // Skip this record
                R->advance(data_size);
            }
        }

        FS.r_close(R);
    } catch (...) {
        Msg("! [Persistence] Exception while loading slot %u", logical_slot);
    }

    return false;  // Not found
}

void CDetailManager::BackgroundSaveThread()
{
    Msg("* [Persistence] Background save thread started");

    while (save_thread_running) {
        // Sleep for 30 seconds between save batches
        std::this_thread::sleep_for(std::chrono::seconds(30));

        if (!save_thread_running) break;

        // Process pending saves
        xr_vector<uint32_t> slots_to_save;
        {
            std::lock_guard<std::mutex> lock(save_mutex);
            slots_to_save = slots_pending_save;
            slots_pending_save.clear();
        }

        for (uint32_t slot : slots_to_save) {
            // Load from warm cache
            auto it = world_to_warm_index.find(slot);
            if (it != world_to_warm_index.end()) {
                uint32_t warm_idx = it->second;
                WarmCacheEntry& entry = warm_cache[warm_idx];

                if (entry.dirty_flag) {
                    // Save to disk
                    SaveSlotToDisk(slot, entry.compressed_data, entry.compressed_size);
                    entry.dirty_flag = 0;  // Mark clean
                }
            }
        }

        if (!slots_to_save.empty()) {
            Msg("* [Persistence] Saved %zu slots to disk", slots_to_save.size());
        }
    }

    Msg("* [Persistence] Background save thread stopped");
}

void CDetailManager::QueueSlotForSave(uint32_t logical_slot)
{
    std::lock_guard<std::mutex> lock(save_mutex);

    // Avoid duplicates
    if (std::find(slots_pending_save.begin(), slots_pending_save.end(), logical_slot) == slots_pending_save.end()) {
        slots_pending_save.push_back(logical_slot);
    }
}

void CDetailManager::FlushPendingSaves()
{
    xr_vector<uint32_t> slots_to_save;
    {
        std::lock_guard<std::mutex> lock(save_mutex);
        slots_to_save = slots_pending_save;
        slots_pending_save.clear();
    }

    for (uint32_t slot : slots_to_save) {
        auto it = world_to_warm_index.find(slot);
        if (it != world_to_warm_index.end()) {
            uint32_t warm_idx = it->second;
            WarmCacheEntry& entry = warm_cache[warm_idx];

            if (entry.dirty_flag) {
                SaveSlotToDisk(slot, entry.compressed_data, entry.compressed_size);
                entry.dirty_flag = 0;
            }
        }
    }

    Msg("* [Persistence] Flushed %zu pending saves", slots_to_save.size());
}

void CDetailManager::UploadSlotDataToGPU(uint32_t physical_page, const uint8_t* data)
{
    auto context = HW.get_context(CHW::IMM_CTX_ID);

    // 1. Compute destination region in atlas
    uint32_t page_x = physical_page % 64;
    uint32_t page_y = physical_page / 64;

    D3D11_BOX destBox = {};
    destBox.left = page_x * 32;
    destBox.top = page_y * 32;
    destBox.right = destBox.left + 32;
    destBox.bottom = destBox.top + 32;
    destBox.front = 0;
    destBox.back = 1;

    // 2. Convert RGBA8 to RGBA16F (atlas format)
    const uint32_t upload_size = 32 * 32 * 4 * sizeof(uint16_t);  // RGBA16F
    uint16_t* upload_data_16f = (uint16_t*)xr_malloc(upload_size);
    const uint8_t* src_8 = data;

    for (uint32_t i = 0; i < 32 * 32; i++) {
        // Convert RGBA8 → RGBA16F (simple: divide by 255, convert to float16)
        float r = src_8[i * 4 + 0] / 255.0f;
        float g = src_8[i * 4 + 1] / 255.0f;
        float b = src_8[i * 4 + 2] / 255.0f;
        float a = src_8[i * 4 + 3] / 255.0f;

        upload_data_16f[i * 4 + 0] = FloatToHalf(r);
        upload_data_16f[i * 4 + 1] = FloatToHalf(g);
        upload_data_16f[i * 4 + 2] = FloatToHalf(b);
        upload_data_16f[i * 4 + 3] = FloatToHalf(a);
    }

    // 3. Copy from staging buffer to atlas
    context->UpdateSubresource(
        interaction_atlas,
        0,
        &destBox,
        upload_data_16f,
        32 * 4 * sizeof(uint16_t),  // Row pitch
        0);

    xr_free(upload_data_16f);

    //Msg("* [Upload] Uploaded slot data to physical page %u", physical_page);
}

} // namespace xray::render::RENDER_NAMESPACE
