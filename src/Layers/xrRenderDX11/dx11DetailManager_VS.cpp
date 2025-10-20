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
    const u32 initialBufferSize = 256 * 256;

    // Instance data structure (must match shader)
    struct InstanceData
    {
        Fvector hpb;    // Heading, pitch, bank rotation
        float scale;    // Scale factor
        Fvector pos;    // Position
        float hemi;     // Hemisphere lighting
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

void CDetailManager::hw_Render(CBackend& cmd_list)
{
    ZoneScoped;
    using namespace detail_manager;

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

    // Instance data structure (must match shader)
    struct InstanceData
    {
        Fvector hpb;    // Heading, pitch, bank rotation
        float scale;    // Scale factor
        Fvector pos;    // Position
        float hemi;     // Hemisphere lighting
    };

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");

    RImplementation.BasicStats.DetailCount = 0;

    vis_list& list = m_visibles[var_id];

    // Set shader constants (only once per pass, not per object)
    cmd_list.set_CullMode(CULL_NONE);
    cmd_list.set_xform_world(Fidentity);

    // Phase 1, Milestone 1.1: Iterate vis_id FIRST, then objects
    // This prepares us for Phase 1, Milestone 1.2 where we'll merge objects per vis_id

    // NOTE: var_id maps to animation type passed from hw_Render:
    //   var_id=0 -> still grass (lod_id=1)
    //   var_id=1 -> wave1 grass (lod_id=0)
    //   var_id=2 -> wave2 grass (lod_id=0)
    // But within each call, all grass has same vis_ID

    // Process each object type
    for (u32 O = 0; O < objects.size(); O++)
    {
        CDetail& Object = *objects[O];
        xr_vector<SlotItemVec*>& vis = list[O];
        if (vis.empty())
            continue;

        // Count instances for this object
        u32 totalInstances = 0;
        for (SlotItemVec* items : vis)
            totalInstances += items->size();

        if (totalInstances == 0)
            continue;

        // Check buffer capacity - reallocate if needed
        u32 bufferSize = detailBufferSize_vis[var_id];
        if (totalInstances > bufferSize)
        {
            Msg("! [DetailManager] Buffer too small for vis_id=%u: need %u, have %u. Clamping to buffer size.",
                var_id, totalInstances, bufferSize);
            totalInstances = bufferSize;
        }

        // Get buffer for this vis_id
        ID3DBuffer* currentBuffer = detailBuffer_vis[var_id];
        ID3DShaderResourceView* currentSRV = detailSRV_vis[var_id];

        // Bind the structured buffer SRV to slot 0
        cmd_list.SRVSManager.SetVSResource(0, currentSRV);

        // Set geometry for this object
        cmd_list.set_Geometry(Object.hw_Geom);

        // Set render states and shaders
        cmd_list.set_Element(Object.shader->E[lod_id], 0);
        cmd_list.apply_lmaterial();

        // Bind CBuffers
        cmd_list.set_c(strConsts, consts);
        cmd_list.set_c(strWave, wave);
        cmd_list.set_c(strDir2D, wind);

        // Fill instance buffer
        u32 instanceCount = 0;
        InstanceData* c_storage = nullptr;

        // Map buffer once per object
        D3D11_MAPPED_SUBRESOURCE pSubRes;
        CHK_DX(HW.get_context(cmd_list.context_id)->Map(currentBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pSubRes));
        c_storage = reinterpret_cast<InstanceData*>(pSubRes.pData);

        for (SlotItemVec* items : vis)
        {
            for (SlotItem* item : *items)
            {
                if (instanceCount >= bufferSize)
                    break; // Buffer full

                SlotItem& Instance = *item;

                // Extract data from matrix
                Fmatrix& M = Instance.mRotY;

                // Extract heading from Y-rotation matrix (rotation around Y axis)
                // For a Y-rotation matrix: M._11 = cos(h), M._13 = sin(h)
                Fvector3 hpb;
                hpb.x = atan2f(M._13, M._11);  // heading (rotation around Y)
                hpb.y = 0.0f;                   // pitch
                hpb.z = 0.0f;                   // bank

                c_storage[instanceCount].hpb = hpb;
                c_storage[instanceCount].scale = Instance.scale_calculated;
                c_storage[instanceCount].pos = M.c;  // Position from matrix
                c_storage[instanceCount].hemi = Instance.c_hemi;

                instanceCount++;
            }
        }

        // Unmap and draw all instances for this object in ONE call
        HW.get_context(cmd_list.context_id)->Unmap(currentBuffer, 0);

        if (instanceCount > 0)
        {
            cmd_list.RenderInstancedIndexed(
                D3DPT_TRIANGLELIST,
                0, 0,
                Object.number_vertices,
                0,
                Object.number_indices / 3,
                instanceCount,
                0);

            cmd_list.stat.r.s_details.add(Object.number_vertices * instanceCount);
            RImplementation.BasicStats.DetailCount += instanceCount;
        }
    }

    cmd_list.set_CullMode(CULL_CCW);
}
} // namespace xray::render::RENDER_NAMESPACE
