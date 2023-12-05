#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"

namespace detail_manager
{
extern const int quant;
//extern const int c_hdr;
}

static const shared_str strConsts("consts");
static const shared_str strWave("wave");
static const shared_str strDir2D("dir2D");
static const shared_str strArray("array");
static const shared_str strXForm("xform");

static constexpr size_t BATCH_DATA_ENTRIES_NUM = 2u;
static constexpr size_t MAX_ENTRIES_NUM = 4096u; // max 1D texture size
static constexpr size_t BATCH_INSTANCES_MAX = MAX_ENTRIES_NUM / BATCH_DATA_ENTRIES_NUM;

void CDetailManager::hw_Load_Shaders()
{
    if (RImplementation.o.instanced_details)
    {
        t_draw_matrices.create("$details$array");
        ID3D11Texture1D* array_texture;
        D3D11_TEXTURE1D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ArraySize = 1;
        desc.MipLevels = 1;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.Width = MAX_ENTRIES_NUM;
        CHK_DX(HW.pDevice->CreateTexture1D(&desc, nullptr, &array_texture));
        t_draw_matrices->surface_set(array_texture);
        for (int idx = 0; idx < R__NUM_CONTEXTS; ++idx)
        {
            upload_buffer[idx] = (Fvector4*)xr_malloc(sizeof(Fvector4) * MAX_ENTRIES_NUM);
        }
    }

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
}

void CDetailManager::hw_Unload()
{
    if (RImplementation.o.instanced_details)
    {
        t_draw_matrices.destroy();
        for (int idx = 0; idx < R__NUM_CONTEXTS; ++idx)
        {
            xr_free(upload_buffer[idx]);
        }
    }

    // Destroy VS/VB/IB
    if (hw_Geom)
        hw_Geom.destroy();
    if (hw_IB)
        hw_IB.Release();
    if (hw_VB)
        hw_VB.Release();
}

void CDetailManager::hw_Render(CBackend& cmd_list)
{
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

    // float		tm_rot1		= (PI_MUL_2*Device.fTimeGlobal/swing_current.rot1);
    // float		tm_rot2		= (PI_MUL_2*Device.fTimeGlobal/swing_current.rot2);
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
    // wave.set				(1.f/5.f,		1.f/7.f,	1.f/3.f,	Device.fTimeGlobal*swing_current.speed);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);
    // RCache.set_c			(&*hwc_consts,	scale,		scale,		ps_r__Detail_l_aniso,	ps_r__Detail_l_ambient);
    // //
    // consts
    // RCache.set_c			(&*hwc_wave,	wave.div(PI_MUL_2));	// wave
    // RCache.set_c			(&*hwc_wind,	dir1); //
    // wind-dir
    // hw_Render_dump			(&*hwc_array,	1, 0, c_hdr );
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir1, 1, 0);

    // Wave1
    // wave.set				(1.f/3.f,		1.f/7.f,	1.f/5.f,	Device.fTimeGlobal*swing_current.speed);
    wave.set(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos);
    // RCache.set_c			(&*hwc_wave,	wave.div(PI_MUL_2));	// wave
    // RCache.set_c			(&*hwc_wind,	dir2); //
    // wind-dir
    // hw_Render_dump			(&*hwc_array,	2, 0, c_hdr );
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir2, 2, 0);

    // Still
    consts.set(scale, scale, scale, 1.f);
    // RCache.set_c			(&*hwc_s_consts,scale,		scale,		scale,				1.f);
    // RCache.set_c			(&*hwc_s_xform,	Device.mFullTransform);
    // hw_Render_dump			(&*hwc_s_array,	0, 1, c_hdr );
    hw_Render_dump(cmd_list, consts, wave.div(PI_MUL_2), dir2, 0, 1);
}

void CDetailManager::hw_Render_dump(CBackend& cmd_list,
    const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id)
{
    if (RImplementation.o.instanced_details)
        draw_instances(cmd_list, consts, wave, wind, var_id, lod_id);
    else
        draw(cmd_list, consts, wave, wind, var_id, lod_id);
}

void CDetailManager::draw_instances(
    CBackend& cmd_list,
    const Fvector4& consts,
    const Fvector4& wave,
    const Fvector4& wind,
    u32 var_id,
    u32 lod_id)
{
    PIX_EVENT_CTX(cmd_list, Details_draw_instances);

    u32 vOffset = 0;
    u32 iOffset = 0;

    vis_list& list = m_visibles[var_id];

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
    Fvector c_sun, c_ambient, c_hemi;
    c_sun.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z);
    c_sun.mul(.5f);
    c_ambient.set(desc.ambient.x, desc.ambient.y, desc.ambient.z);
    c_hemi.set(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z);

    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");
    static shared_str strArray("array");
    static shared_str strXForm("xform");
    static shared_str strPos("benders_pos");
    static shared_str strGrassSetup("benders_setup");

    // Grass benders data
    IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;
    Fvector4 player_pos = { 0, 0, 0, 0 };
    int BendersQty = _min(16, (int)(ps_ssfx_grass_interactive.y + 1));

    // Add Player?
    if (ps_ssfx_grass_interactive.x > 0)
        player_pos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);

    // Iterate
    for (u32 O = 0; O < objects.size(); O++)
    {
        PIX_EVENT_CTX(cmd_list, Object_draw);

        CDetail& Object = *objects[O];
        xr_vector<SlotItemVec*>& vis = list[O];
        if (!vis.empty())
        {
            for (u32 iPass = 0; iPass < Object.shader->E[lod_id]->passes.size(); ++iPass)
            {
                cmd_list.set_Element(Object.shader->E[lod_id], iPass);
                cmd_list.apply_lmaterial();

                cmd_list.set_c(strConsts, consts);
                cmd_list.set_c(strWave, wave);
                cmd_list.set_c(strDir2D, wind);
                cmd_list.set_c(strXForm, Device.mFullTransform);

                if (ps_ssfx_grass_interactive.y > 0)
                {
                    cmd_list.set_c(strGrassSetup, ps_ssfx_int_grass_params_1);

                    Fvector4* c_grass{};
                    {
                        void* GrassData;
                        cmd_list.get_ConstantDirect(strPos, BendersQty * sizeof(Fvector4), &GrassData, 0, 0);
                        c_grass = (Fvector4*)GrassData;
                    }

                    if (c_grass)
                    {
                        c_grass[0].set(player_pos);
                        c_grass[16].set(0.0f, -99.0f, 0.0f, 1.0f);

                        for (int Bend = 1; Bend < BendersQty; Bend++)
                        {
                            c_grass[Bend].set(GData.pos[Bend].x, GData.pos[Bend].y, GData.pos[Bend].z, GData.radius_curr[Bend]);
                            c_grass[Bend + 16].set(GData.dir[Bend].x, GData.dir[Bend].y, GData.dir[Bend].z, GData.str[Bend]);
                        }
                    }
                }

                Fvector4* c_storage = upload_buffer[cmd_list.context_id];

                u32 dwBatch = 0;

                xr_vector<SlotItemVec*>::iterator _vI = vis.begin();
                xr_vector<SlotItemVec*>::iterator _vE = vis.end();
                for (; _vI != _vE; ++_vI)
                {
                    SlotItemVec* items = *_vI;

                    auto _iI = items->begin();
                    auto _iE = items->end();
                    for (; _iI != _iE; ++_iI)
                    {
                        SlotItem& Instance = **_iI;
                        u32 base = dwBatch * 2;

                        float scale = Instance.scale_calculated;

                        // Sort of fade using the scale
                        // fade_distance == -1 use light_position to define "fade", anything else uses fade_distance
                        if (fade_distance <= -1)
                            scale *= 1.0f - Instance.position.distance_to_xz_sqr(light_position) * 0.005f;
                        else if (Instance.distance > fade_distance)
                            scale *= 1.0f - abs(Instance.distance - fade_distance) * 0.005f;

                        if (scale <= 0)
                            break;

                        Fmatrix& M = Instance.mRotY;

                        c_storage[base + 0].set(M._13 * scale, M._11 * scale, Instance.c_hemi, Instance.c_sun);
                        c_storage[base + 1].set(M._41, M._42, M._43, scale);

                        dwBatch++;
                        if (dwBatch == BATCH_INSTANCES_MAX)
                        {
                            u32 dwCNT_verts = dwBatch * Object.number_vertices;
                            u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;

                            {
                                HW.get_context(cmd_list.context_id)->UpdateSubresource(
                                    t_draw_matrices->surface_get(),
                                    0,
                                    nullptr,
                                    upload_buffer[cmd_list.context_id],
                                    MAX_ENTRIES_NUM * sizeof(Fvector4),
                                    1
                                );

                                cmd_list.render_instanced(D3DPT_TRIANGLELIST, vOffset, iOffset, Object.number_indices, BATCH_INSTANCES_MAX);
                                cmd_list.stat.r.s_details.add(dwCNT_verts);
                            }

                            // restart
                            dwBatch = 0;
                        }
                    }
                }

                // flush if necessary
                if (dwBatch)
                {
                    HW.get_context(cmd_list.context_id)->UpdateSubresource(
                        t_draw_matrices->surface_get(),
                        0,
                        nullptr,
                        upload_buffer[cmd_list.context_id],
                        dwBatch * BATCH_DATA_ENTRIES_NUM * sizeof(Fvector4),
                        1
                    );

                    u32 dwCNT_verts = dwBatch * Object.number_vertices;
                    u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;
                    cmd_list.render_instanced(D3DPT_TRIANGLELIST, vOffset, iOffset, Object.number_indices, dwBatch);
                    cmd_list.stat.r.s_details.add(dwCNT_verts);
                }
            }
        }
        vOffset += hw_BatchSize * Object.number_vertices;
        iOffset += hw_BatchSize * Object.number_indices;
    }
}

void CDetailManager::draw(
    CBackend& cmd_list,
    const Fvector4& consts,
    const Fvector4& wave,
    const Fvector4& wind,
    u32 var_id,
    u32 lod_id)
{
    static shared_str strConsts("consts");
    static shared_str strWave("wave");
    static shared_str strDir2D("dir2D");
    static shared_str strArray("array");
    static shared_str strXForm("xform");
    static shared_str strPos("benders_pos");
    static shared_str strGrassSetup("benders_setup");

    // Grass benders data
    IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;
    Fvector4 player_pos = { 0, 0, 0, 0 };
    int BendersQty = _min(16, (int)(ps_ssfx_grass_interactive.y + 1));

    // Add Player?
    if (ps_ssfx_grass_interactive.x > 0)
        player_pos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);

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

                if (ps_ssfx_grass_interactive.y > 0)
                {
                    cmd_list.set_c(strGrassSetup, ps_ssfx_int_grass_params_1);

                    Fvector4* c_grass{};
                    {
                        void* GrassData;
                        cmd_list.get_ConstantDirect(strPos, BendersQty * sizeof(Fvector4), &GrassData, 0, 0);
                        c_grass = (Fvector4*)GrassData;
                    }

                    if (c_grass)
                    {
                        c_grass[0].set(player_pos);
                        c_grass[16].set(0.0f, -99.0f, 0.0f, 1.0f);

                        for (int Bend = 1; Bend < BendersQty; Bend++)
                        {
                            c_grass[Bend].set(GData.pos[Bend].x, GData.pos[Bend].y, GData.pos[Bend].z, GData.radius_curr[Bend]);
                            c_grass[Bend + 16].set(GData.dir[Bend].x, GData.dir[Bend].y, GData.dir[Bend].z, GData.str[Bend]);
                        }
                    }
                }

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

                xr_vector<SlotItemVec*>::iterator _vI = vis.begin();
                xr_vector<SlotItemVec*>::iterator _vE = vis.end();
                for (; _vI != _vE; ++_vI)
                {
                    SlotItemVec* items = *_vI;

                    auto _iI = items->begin();
                    auto _iE = items->end();
                    for (; _iI != _iE; ++_iI)
                    {
                        SlotItem& Instance = **_iI;
                        u32 base = dwBatch * 4;

                        // Build matrix ( 3x4 matrix, last row - color )
                        float scale = Instance.scale_calculated;

                        // Sort of fade using the scale
                        // fade_distance == -1 use light_position to define "fade", anything else uses fade_distance
                        if (fade_distance <= -1)
                            scale *= 1.0f - Instance.position.distance_to_xz_sqr(light_position) * 0.005f;
                        else if (Instance.distance > fade_distance)
                            scale *= 1.0f - abs(Instance.distance - fade_distance) * 0.005f;

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
