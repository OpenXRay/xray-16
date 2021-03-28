#include "stdafx.h"

#include "xrCore/Threading/TaskManager.hpp"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/xr_object.h"

#include "Layers/xrRender/FBasicVisual.h"

IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
    float d1 = _1->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
    float d2 = _2->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
    return d1 < d2;
}

void CRender::render_main(Fmatrix& m_ViewProjection, bool _fportals)
{
    PIX_EVENT(render_main);
    //	Msg						("---begin");
    marker++;

    // Calculate sector(s) and their objects
    if (pLastSector)
    {
        // Traverse sector/portal structure
        PortalTraverser.traverse(pLastSector, ViewBase, Device.vCameraPosition, m_ViewProjection,
            CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
            //. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0)	// generate scissoring info
            );

        // Determine visibility for static geometry hierrarhy
        if (psDeviceFlags.test(rsDrawStatic))
        {
            for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
            {
                CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
                dxRender_Visual* root = sector->root();
                for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
                {
                    add_Geometry(root, sector->r_frustums[v_it]);
                }
            }
        }

        // Traverse frustums
        if (psDeviceFlags.test(rsDrawDynamic))
        {
            // Traverse object database
            g_SpatialSpace->q_frustum(
                lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE, ViewBase);

            // Exact sorting order (front-to-back)
            std::sort(lstRenderables.begin(), lstRenderables.end(), pred_sp_sort);

            // Determine visibility for dynamic part of scene
            u32 uID_LTRACK = 0xffffffff;
            if (phase == PHASE_NORMAL)
            {
                uLastLTRACK++;
                if (lstRenderables.size())
                    uID_LTRACK = uLastLTRACK % lstRenderables.size();

                // update light-vis for current entity / actor
                IGameObject* O = g_pGameLevel->CurrentViewEntity();
                if (O)
                {
                    CROS_impl* R = (CROS_impl*)O->ROS();
                    if (R)
                        R->update(O);
                }
            }

            for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
            {
                ISpatial* spatial = lstRenderables[o_it];
                spatial->spatial_updatesector();
                CSector* sector = (CSector*)spatial->GetSpatialData().sector;
                if (0 == sector)
                    continue; // disassociated from S/P structure

                if (spatial->GetSpatialData().type & STYPE_LIGHTSOURCE)
                {
                    // lightsource
                    light* L = (light*)spatial->dcast_Light();
                    VERIFY(L);
                    float lod = L->get_LOD();
                    if (lod > EPS_L)
                    {
                        vis_data& vis = L->get_homdata();
                        if (HOM.visible(vis))
                            Lights.add_light(L);
                    }
                    continue;
                }

                if (PortalTraverser.i_marker != sector->r_marker)
                    continue; // inactive (untouched) sector
                for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
                {
                    CFrustum& view = sector->r_frustums[v_it];
                    if (!view.testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
                        continue;

                    if (spatial->GetSpatialData().type & STYPE_RENDERABLE)
                    {
                        // renderable
                        IRenderable* renderable = spatial->dcast_Renderable();
                        VERIFY(renderable);

                        // Occlusion
                        //	casting is faster then using getVis method
                        vis_data& v_orig = ((dxRender_Visual*)renderable->GetRenderData().visual)->vis;
                        vis_data v_copy = v_orig;
                        v_copy.box.xform(renderable->GetRenderData().xform);
                        BOOL bVisible = HOM.visible(v_copy);
                        v_orig.marker = v_copy.marker;
                        v_orig.accept_frame = v_copy.accept_frame;
                        v_orig.hom_frame = v_copy.hom_frame;
                        v_orig.hom_tested = v_copy.hom_tested;
                        if (!bVisible)
                            break; // exit loop on frustums

                        // update light-vis for selected entity
                        if (o_it == uID_LTRACK && renderable->renderable_ROS())
                        {
                            // track lighting environment
                            CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
                            T->update(renderable);
                        }

                        // Rendering
                        renderable->renderable_Render(renderable);
                    }
                    break; // exit loop on frustums
                }
            }
            if (g_pGameLevel && (phase == PHASE_NORMAL))
                g_hud->Render_Last(); // HUD
        }
    }
    else
    {
        if (g_pGameLevel && (phase == PHASE_NORMAL))
            g_hud->Render_Last(); // HUD
    }
}

void CRender::render_menu()
{
    PIX_EVENT(render_menu);
    //	Globals
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(FALSE);
    RCache.set_ColorWriteEnable();

    // Main Render
    {
        Target->u_setrt(Target->rt_Generic_0, nullptr, nullptr, Target->get_base_zb()); // LDR RT
        g_pGamePersistent->OnRenderPPUI_main(); // PP-UI
    }
    // Distort
    {
        Target->u_setrt(Target->rt_Generic_1, nullptr, nullptr, Target->get_base_zb()); // Now RT is a distortion mask
        RCache.ClearRT(Target->rt_Generic_1, color_rgba(127, 127, 0, 127));
        g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
    }

    // Actual Display
    Target->u_setrt(Device.dwWidth, Device.dwHeight, Target->get_base_rt(), 0, 0, Target->get_base_zb());
    RCache.set_Shader(Target->s_menu);
    RCache.set_Geometry(Target->g_menu);

    Fvector2 p0, p1;
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    float d_Z = EPS_S;
    float d_W = 1.f;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);

    FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, Target->g_menu->vb_stride, Offset);
#ifdef USE_OGL
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
    pv++;
#else
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
    pv++;
#endif // USE_OGL
    RCache.Vertex.Unlock(4, Target->g_menu->vb_stride);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

extern u32 g_r;
void CRender::Render()
{
    PIX_EVENT(CRender_Render);

    g_r = 1;
    VERIFY(0 == mapDistort.size());

#ifndef USE_DX9
    rmNormal();
#endif

    bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;
    if (_menu_pp)
    {
        render_menu();
        return;
    }

    IMainMenu* pMainMenu = g_pGamePersistent ? g_pGamePersistent->m_pMainMenu : 0;
    bool bMenu = pMainMenu ? pMainMenu->CanSkipSceneRendering() : false;

    if (!(g_pGameLevel && g_hud) || bMenu)
    {
#ifndef USE_DX9 // XXX: probably we can just enable this on DX9 too
        Target->u_setrt(Device.dwWidth, Device.dwHeight, Target->get_base_rt(), 0, 0, Target->get_base_zb());
#endif
        return;
    }

    if (m_bFirstFrameAfterReset)
    {
        m_bFirstFrameAfterReset = false;
        return;
    }

    //.	VERIFY					(g_pGameLevel && g_pGameLevel->pHUD);

    // Configure
    o.distortion = FALSE; // disable distorion
    Fcolor sun_color = ((light*)Lights.sun._get())->color;
    bool bSUN = ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
    if (o.sunstatic)
        bSUN = false;
    // Msg						("sstatic: %s, sun: %s",o.sunstatic?;"true":"false", bSUN?"true":"false");

    // Frustum
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

    //******* Z-prefill calc - DEFERRER RENDERER
    if (ps_r2_ls_flags.test(R2FLAG_ZFILL))
    {
        PIX_EVENT(DEFER_Z_FILL);
        BasicStats.Culling.Begin();
        float z_distance = ps_r2_zfill;
        Fmatrix m_zfill, m_project;
        m_project.build_projection(
            deg2rad(Device.fFOV/* *Device.fASPECT*/),
            Device.fASPECT, VIEWPORT_NEAR,
            z_distance * g_pGamePersistent->Environment().CurrentEnv->far_plane);
        m_zfill.mul(m_project, Device.mView);
        r_pmask(true, false); // enable priority "0"
        set_Recorder(nullptr);
        phase = PHASE_SMAP;
        TaskScheduler->Wait(*ProcessHOMTask);
        render_main(m_zfill, false);
        r_pmask(true, false); // disable priority "1"
        BasicStats.Culling.End();

        // flush
        Target->phase_scene_prepare();
        RCache.set_ColorWriteEnable(FALSE);
        r_dsgraph_render_graph(0);
        RCache.set_ColorWriteEnable();
    }
    else
    {
        Target->phase_scene_prepare();
    }

    //*******
    // Sync point
    BasicStats.WaitS.Begin();
    {
        q_sync_point.Wait(ps_r2_wait_sleep, ps_r2_wait_timeout);
    }
    BasicStats.WaitS.End();
    q_sync_point.End();

    //******* Main calc - DEFERRER RENDERER
    // Main calc
    BasicStats.Culling.Begin();
    r_pmask(true, false, true); // enable priority "0",+ capture wmarks
    if (bSUN)
        set_Recorder(&main_coarse_structure);
    else
        set_Recorder(nullptr);
    phase = PHASE_NORMAL;
    if (!ps_r2_ls_flags.test(R2FLAG_ZFILL))
        TaskScheduler->Wait(*ProcessHOMTask);
    render_main(Device.mFullTransform, true);
    set_Recorder(nullptr);
    r_pmask(true, false); // disable priority "1"
    BasicStats.Culling.End();

    BOOL split_the_scene_to_minimize_wait = FALSE;
    if (ps_r2_ls_flags.test(R2FLAG_EXP_SPLIT_SCENE))
        split_the_scene_to_minimize_wait = TRUE;

    //******* Main render :: PART-0	-- first
#ifdef USE_OGL
    if (psDeviceFlags.test(rsWireframe))
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    if (!split_the_scene_to_minimize_wait)
    {
        PIX_EVENT(DEFER_PART0_NO_SPLIT);
        // level, DO NOT SPLIT
        Target->phase_scene_begin();
        r_dsgraph_render_hud();
        r_dsgraph_render_graph(0);
        r_dsgraph_render_lods(true, true);
        if (Details)
            Details->Render();
        Target->phase_scene_end();
    }
    else
    {
        PIX_EVENT(DEFER_PART0_SPLIT);
        // level, SPLIT
        Target->phase_scene_begin();
        r_dsgraph_render_graph(0);
        Target->disable_aniso();
    }
#ifdef USE_OGL
    if (psDeviceFlags.test(rsWireframe))
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    //******* Occlusion testing of volume-limited light-sources
    Target->phase_occq();
    LP_normal.clear();
    LP_pending.clear();
#ifndef USE_DX9
    if (o.dx10_msaa)
    {
        RCache.set_ZB(Target->rt_MSAADepth->pZRT);
    }
#endif
    {
        PIX_EVENT(DEFER_TEST_LIGHT_VIS);
        // perform tests
        auto count = 0;
        light_Package& LP = Lights.package;

        // stats
        Stats.l_shadowed = LP.v_shadowed.size();
        Stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
        Stats.l_total = Stats.l_shadowed + Stats.l_unshadowed;

        // perform tests
        count = _max(count, LP.v_point.size());
        count = _max(count, LP.v_spot.size());
        count = _max(count, LP.v_shadowed.size());
        for (auto it = 0; it < count; it++)
        {
            if (it < LP.v_point.size())
            {
                light* L = LP.v_point[it];
                L->vis_prepare();
                if (L->vis.pending)
                    LP_pending.v_point.push_back(L);
                else
                    LP_normal.v_point.push_back(L);
            }
            if (it < LP.v_spot.size())
            {
                light* L = LP.v_spot[it];
                L->vis_prepare();
                if (L->vis.pending)
                    LP_pending.v_spot.push_back(L);
                else
                    LP_normal.v_spot.push_back(L);
            }
            if (it < LP.v_shadowed.size())
            {
                light* L = LP.v_shadowed[it];
                L->vis_prepare();
                if (L->vis.pending)
                    LP_pending.v_shadowed.push_back(L);
                else
                    LP_normal.v_shadowed.push_back(L);
            }
        }
    }
    LP_normal.sort();
    LP_pending.sort();

    //******* Main render :: PART-1 (second)
    if (split_the_scene_to_minimize_wait)
    {
        PIX_EVENT(DEFER_PART1_SPLIT);
        // skybox can be drawn here
        if (false)
        {
            Target->u_setrt(Target->rt_Generic_0_r, Target->rt_Generic_1_r, nullptr, Target->rt_MSAADepth);
            RCache.set_CullMode(CULL_NONE);
            RCache.set_Stencil(FALSE);

            // draw skybox
            RCache.set_ColorWriteEnable();
            RCache.set_Z(false);
            g_pGamePersistent->Environment().RenderSky();
            RCache.set_Z(true);
        }

        // level
        Target->phase_scene_begin();
        r_dsgraph_render_hud();
        r_dsgraph_render_lods(true, true);
        if (Details)
            Details->Render();
        Target->phase_scene_end();
    }

    if (g_hud && g_hud->RenderActiveItemUIQuery())
    {
        Target->phase_wallmarks();
        r_dsgraph_render_hud_ui();
    }

    // Wall marks
    if (Wallmarks)
    {
        PIX_EVENT(DEFER_WALLMARKS);
        Target->phase_wallmarks();
        g_r = 0;
        Wallmarks->Render(); // wallmarks has priority as normal geometry
    }

    // Update incremental shadowmap-visibility solver
    {
        PIX_EVENT(DEFER_FLUSH_OCCLUSION);
        u32 it = 0;
        for (it = 0; it < Lights_LastFrame.size(); it++)
        {
            if (0 == Lights_LastFrame[it])
                continue;
            try
            {
                Lights_LastFrame[it]->svis.flushoccq();
            }
            catch (...)
            {
                Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
            }
        }
        Lights_LastFrame.clear();
    }

#ifndef USE_DX9
    // full screen pass to mark msaa-edge pixels in highest stencil bit
    if (o.dx10_msaa)
    {
        PIX_EVENT(MARK_MSAA_EDGES);
        Target->mark_msaa_edges();
    }

    //	TODO: DX10: Implement DX10 rain.
    if (ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF))
    {
        PIX_EVENT(DEFER_RAIN);
        render_rain();
    }
#endif // !USE_DX9

    // Directional light - fucking sun
    if (bSUN)
    {
        PIX_EVENT(DEFER_SUN);
        Stats.l_visible++;
        if (!RImplementation.o.oldshadowcascades)
            render_sun_cascades();
        else
        {
            render_sun_near();
            render_sun();
            render_sun_filtered();
        }
        Target->accum_direct_blend();
    }

    {
        PIX_EVENT(DEFER_SELF_ILLUM);
        Target->phase_accumulator();
        // Render emissive geometry, stencil - write 0x0 at pixel pos
        RCache.set_xform_project(Device.mProject);
        RCache.set_xform_view(Device.mView);
        // Stencil - write 0x1 at pixel pos -
#ifdef USE_DX9
        RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff,
            D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
#else
        if (!o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
        else
        {
            RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
#endif // USE_DX9
        RCache.set_CullMode(CULL_CCW);
        RCache.set_ColorWriteEnable();
        r_dsgraph_render_emissive();
    }

    // Lighting, non dependant on OCCQ
    {
        PIX_EVENT(DEFER_LIGHT_NO_OCCQ);
        Target->phase_accumulator();
        HOM.Disable();
        render_lights(LP_normal);
    }

    // Lighting, dependant on OCCQ
    {
        PIX_EVENT(DEFER_LIGHT_OCCQ);
        render_lights(LP_pending);
    }

    // Postprocess
    {
        PIX_EVENT(DEFER_LIGHT_COMBINE);
        Target->phase_combine();
    }

    VERIFY(0 == mapDistort.size());
}

void CRender::render_forward()
{
    VERIFY(0 == mapDistort.size());
    o.distortion = o.distortion_enabled; // enable distorion

    //******* Main render - second order geometry (the one, that doesn't support deffering)
    //.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
    {
        // level
        r_pmask(false, true); // enable priority "1"
        phase = PHASE_NORMAL;
        render_main(Device.mFullTransform, false); //
        //	Igor: we don't want to render old lods on next frame.
        mapLOD.clear();
        r_dsgraph_render_graph(1); // normal level, secondary priority
        PortalTraverser.fade_render(); // faded-portals
        r_dsgraph_render_sorted(); // strict-sorted geoms
        g_pGamePersistent->Environment().RenderLast(); // rain/thunder-bolts
    }

    o.distortion = FALSE; // disable distorion
}

// Перед началом рендера мира --#SM+#--
void CRender::BeforeWorldRender() {}

// После рендера мира и пост-эффектов --#SM+#--
void CRender::AfterWorldRender() {}
