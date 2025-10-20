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
    hwc_wind = T0.get("dir2D");
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

// Phase 2.0.4: Render all instances from full level decompression (no culling)
void CDetailManager::hw_Render_FullLevel(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

    // Reset detail count
    RImplementation.BasicStats.DetailCount = 0;

    // Update animation timers (same as old code)
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

    // Instance data structure
    struct InstanceData
    {
        Fvector hpb;
        float scale;
        Fvector pos;
        float hemi;
        u32 vis_id;
        u32 object_id;
    };

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");

    // Render each object by gathering all its instances from all_level_instances
    for (u32 O = 0; O < objects.size(); O++)
    {
        CDetail& Object = *objects[O];

        // Count instances for this object
        u32 instanceCount = 0;
        for (u32 i = 0; i < total_instance_count; i++)
        {
            if (all_level_instances[i].object_id == O)
                instanceCount++;
        }

        if (instanceCount == 0)
            continue;

        // Use first buffer for rendering
        ID3DBuffer* currentBuffer = detailBuffer_vis[0];
        ID3DShaderResourceView* currentSRV = detailSRV_vis[0];
        u32 bufferSize = detailBufferSize_vis[0];

        if (instanceCount > bufferSize)
        {
            Msg("! [DetailManager] Too many instances for object=%u: need %u, have %u. Clamping.",
                O, instanceCount, bufferSize);
            instanceCount = bufferSize;
        }

        // Map and fill buffer
        D3D11_MAPPED_SUBRESOURCE pSubRes;
        CHK_DX(HW.get_context(cmd_list.context_id)->Map(currentBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pSubRes));
        InstanceData* c_storage = reinterpret_cast<InstanceData*>(pSubRes.pData);

        u32 written = 0;
        for (u32 i = 0; i < total_instance_count && written < instanceCount; i++)
        {
            const SlotItemWithObject& src = all_level_instances[i];
            if (src.object_id != O)
                continue;

            const Fmatrix& M = src.item.mRotY;

            c_storage[written].hpb.x = atan2f(M._13, M._11);
            c_storage[written].hpb.y = 0.0f;
            c_storage[written].hpb.z = 0.0f;
            c_storage[written].scale = src.item.scale;
            c_storage[written].pos = M.c;
            c_storage[written].hemi = src.item.c_hemi;
            c_storage[written].vis_id = src.item.vis_ID;
            c_storage[written].object_id = O;

            written++;
            RImplementation.BasicStats.DetailCount++;
        }

        HW.get_context(cmd_list.context_id)->Unmap(currentBuffer, 0);

        // Set shader state and draw
        cmd_list.set_CullMode(CULL_NONE);
        cmd_list.set_xform_world(Fidentity);
        cmd_list.SRVSManager.SetVSResource(0, currentSRV);
        cmd_list.set_Geometry(vis_unified_geom[0]);
        cmd_list.set_c(strConsts, consts);
        cmd_list.set_c(strWave, wave.div(PI_MUL_2));
        cmd_list.set_c(strDir2D, dir1);

        cmd_list.set_Element(Object.shader->E[0], 0);
        cmd_list.apply_lmaterial();

        u32 baseIndex = vis_geometry_index_offsets[0][O];
        u32 numVertices = Object.number_vertices;
        u32 numIndices = Object.number_indices;

        cmd_list.RenderInstancedIndexed(
            D3DPT_TRIANGLELIST,
            0, 0,
            numVertices,
            baseIndex,
            numIndices / 3,
            written,
            0);

        cmd_list.stat.r.s_details.add(numVertices * written);
        cmd_list.set_CullMode(CULL_CCW);
    }
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
