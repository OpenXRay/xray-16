#include "stdafx.h"

#include "xrEngine/IRenderable.h"
#include "xrEngine/CustomHUD.h"

#include "FBasicVisual.h"

using namespace R_dsgraph;

extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

ICF float calcLOD(float ssa /*fDistSq*/, float /*R*/)
{
    return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// ALPHA
void __fastcall sorted_L1(mapSorted_T::value_type &N)
{
    dxRender_Visual* V = N.second.pVisual;
    VERIFY(V && V->shader._get());
    RCache.set_Element(N.second.se);
    RCache.set_xform_world(N.second.Matrix);
    RImplementation.apply_object(N.second.pObject);
    RImplementation.apply_lmaterial();
    V->Render(calcLOD(N.first, V->vis.sphere.R));
}

template <class T> IC bool cmp_second_ssa(const T &lhs, const T &rhs) { return (lhs->second.ssa > rhs->second.ssa); }
template <class T> IC bool cmp_ssa       (const T &lhs, const T &rhs) { return (lhs.ssa         > rhs.ssa        ); }

template <class T> IC bool cmp_ps_second_ssa(const T &lhs, const T &rhs)
{
#ifdef USE_DX11
    return (lhs->second.mapCS.ssa > rhs->second.mapCS.ssa);
#else
    return (lhs->second.ssa > rhs->second.ssa);
#endif
}

template <class T> IC bool cmp_textures_lex2(const T &lhs, const T &rhs)
{
    auto t1 = lhs->first;
    auto t2 = rhs->first;

    if ((*t1)[0] < (*t2)[0]) return true;
    if ((*t1)[0] > (*t2)[0]) return false;
    if ((*t1)[1] < (*t2)[1]) return true;
    else               return false;
}
template <class T> IC bool cmp_textures_lex3(const T &lhs, const T &rhs)
{
    auto t1 = lhs->first;
    auto t2 = rhs->first;

    if ((*t1)[0] < (*t2)[0]) return true;
    if ((*t1)[0] > (*t2)[0]) return false;
    if ((*t1)[1] < (*t2)[1]) return true;
    if ((*t1)[1] > (*t2)[1]) return false;
    if ((*t1)[2] < (*t2)[2]) return true;
    else               return false;
}
template <class T> IC bool cmp_textures_lexN(const T &lhs, const T &rhs)
{
    auto t1 = lhs->first;
    auto t2 = rhs->first;

    return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}

template <class T> void sort_tlist(xr_vector<T::template value_type *>& lst, xr_vector<T::template value_type *>& temp, T& textures)
{
    int amount = textures.begin()->first->size();

    if (amount <= 1)
    {
        // Just sort by SSA
        lst.reserve(textures.size());
        for (auto &i : textures) lst.push_back(&i);
        std::sort(lst.begin(), lst.end(), cmp_second_ssa<T::template value_type *>);
    }
    else
    {
        // Split into 2 parts
        for (auto &it : textures)
        {
            if (it.second.ssa > r_ssaHZBvsTEX)
                lst.push_back(&it);
            else
                temp.push_back(&it);
        }

        // 1st - part - SSA, 2nd - lexicographically
        std::sort(lst.begin(), lst.end(), cmp_second_ssa<T::template value_type *>);
        if (2 == amount)
            std::sort(temp.begin(), temp.end(), cmp_textures_lex2<T::template value_type *>);
        else if (3 == amount)
            std::sort(temp.begin(), temp.end(), cmp_textures_lex3<T::template value_type *>);
        else
            std::sort(temp.begin(), temp.end(), cmp_textures_lexN<T::template value_type *>);

        // merge lists
        lst.insert(lst.end(), temp.begin(), temp.end());
    }
}

void D3DXRenderBase::r_dsgraph_render_graph(u32 _priority)
{
    // PIX_EVENT(r_dsgraph_render_graph);
    BasicStats.Primitives.Begin();

    // **************************************************** NORMAL
    // Perform sorting based on ScreenSpaceArea
    // Sorting by SSA and changes minimizations
    {
        RCache.set_xform_world(Fidentity);

        // Render several passes
        for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
        {
            mapNormalVS& vs = mapNormalPasses[_priority][iPass];

            nrmVS.reserve(vs.size());
            for (auto &i : vs) nrmVS.push_back(&i);
            std::sort(nrmVS.begin(), nrmVS.end(), cmp_second_ssa<mapNormalVS::value_type *>);
            for (auto & vs_it : nrmVS)
            {
                RCache.set_VS(vs_it->first);

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
                //	GS setup
                mapNormalGS& gs = vs_it->second;
                gs.ssa = 0;

                nrmGS.reserve(gs.size());
                for (auto &i : gs) nrmGS.push_back(&i);
                std::sort(nrmGS.begin(), nrmGS.end(), cmp_second_ssa<mapNormalGS::value_type *>);
                for (auto & gs_it : nrmGS)
                {
                    RCache.set_GS(gs_it->first);

                    mapNormalPS& ps = gs_it->second;
#else
                    mapNormalPS& ps = vs_it->second;
#endif
                    ps.ssa = 0;

                    nrmPS.reserve(ps.size());
                    for (auto &i : ps) nrmPS.push_back(&i);
                    std::sort(nrmPS.begin(), nrmPS.end(), cmp_ps_second_ssa<mapNormalPS::value_type *>);
                    for (auto &ps_it : nrmPS)
                    {
                        RCache.set_PS(ps_it->first);
#ifdef USE_DX11
                        RCache.set_HS(ps_it->second.hs);
                        RCache.set_DS(ps_it->second.ds);

                        mapNormalCS& cs = ps_it->second.mapCS;
#else
                        mapNormalCS& cs = ps_it->second;
#endif
                        cs.ssa = 0;

                        nrmCS.reserve(cs.size());
                        for (auto &i : cs) nrmCS.push_back(&i);
                        std::sort(nrmCS.begin(), nrmCS.end(), cmp_second_ssa<mapNormalCS::value_type *>);
                        for (auto &cs_it : nrmCS)
                        {
                            RCache.set_Constants(cs_it->first);

                            mapNormalStates& states = cs_it->second;
                            states.ssa = 0;

                            nrmStates.reserve(states.size());
                            for (auto &i : states) nrmStates.push_back(&i);
                            std::sort(nrmStates.begin(), nrmStates.end(), cmp_second_ssa<mapNormalStates::value_type *>);
                            for (auto &state_it : nrmStates)
                            {
                                RCache.set_States(state_it->first);

                                mapNormalTextures& tex = state_it->second;
                                tex.ssa = 0;

                                sort_tlist<mapNormalTextures>(nrmTextures, nrmTexturesTemp, tex);
                                for (auto &tex_it : nrmTextures)
                                {
                                    RCache.set_Textures(tex_it->first);
                                    RImplementation.apply_lmaterial();

                                    mapNormalItems& items = tex_it->second;
                                    items.ssa = 0;

                                    std::sort(items.begin(), items.end(), cmp_ssa<_NormalItem>);
                                    for (auto &it_it : items)
                                    {
                                        float LOD = calcLOD(it_it.ssa, it_it.pVisual->vis.sphere.R);
#ifdef USE_DX11
                                        RCache.LOD.set_LOD(LOD);
#endif
                                        it_it.pVisual->Render(LOD);
                                    }
                                    items.clear();
                                }
                                nrmTexturesTemp.clear();
                                nrmTextures.clear();
                                tex.clear();
                            }
                            nrmStates.clear();
                            states.clear();
                        }
                        nrmCS.clear();
                        cs.clear();
                    }
                    nrmPS.clear();
                    ps.clear();
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
                }
                nrmGS.clear();
                gs.clear();
#endif
            }
            nrmVS.clear();
            vs.clear();
        }
    }

    // **************************************************** MATRIX
    // Perform sorting based on ScreenSpaceArea
    // Sorting by SSA and changes minimizations
    // Render several passes
    for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
    {
        mapMatrixVS& vs = mapMatrixPasses[_priority][iPass];

        matVS.reserve(vs.size());
        for (auto &i : vs) matVS.push_back(&i);
        std::sort(matVS.begin(), matVS.end(), cmp_second_ssa<mapMatrixVS::value_type *>);
        for (auto &vs_id : matVS)
        {
            RCache.set_VS(vs_id->first);

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
            mapMatrixGS& gs = vs_id->second;
            gs.ssa = 0;

            matGS.reserve(gs.size());
            for (auto &i : gs) matGS.push_back(&i);
            std::sort(matGS.begin(), matGS.end(), cmp_second_ssa<mapMatrixGS::value_type *>);
            for (auto &gs_it : matGS)
            {
                RCache.set_GS(gs_it->first);

                mapMatrixPS& ps = gs_it->second;
#else
                mapMatrixPS& ps = vs_id->second;
#endif
                ps.ssa = 0;

                matPS.reserve(ps.size());
                for (auto &i : ps) matPS.push_back(&i);
                std::sort(matPS.begin(), matPS.end(), cmp_ps_second_ssa<mapMatrixPS::value_type *>);
                for (auto &ps_it : matPS)
                {
                    RCache.set_PS(ps_it->first);
#ifdef USE_DX11
                    RCache.set_HS(ps_it->second.hs);
                    RCache.set_DS(ps_it->second.ds);

                    mapMatrixCS& cs = ps_it->second.mapCS;
#else
                    mapMatrixCS& cs = ps_it->second;
#endif
                    cs.ssa = 0;

                    matCS.reserve(cs.size());
                    for (auto &i : cs) matCS.push_back(&i);
                    std::sort(matCS.begin(), matCS.end(), cmp_second_ssa<mapMatrixCS::value_type *>);
                    for (auto &cs_it : matCS)
                    {
                        RCache.set_Constants(cs_it->first);

                        mapMatrixStates& states = cs_it->second;
                        states.ssa = 0;

                        matStates.reserve(states.size());
                        for (auto &i : states) matStates.push_back(&i);
                        std::sort(matStates.begin(), matStates.end(), cmp_second_ssa<mapMatrixStates::value_type *>);
                        for (auto &state_it : matStates)
                        {
                            RCache.set_States(state_it->first);

                            mapMatrixTextures& tex = state_it->second;
                            tex.ssa = 0;

                            sort_tlist<mapMatrixTextures>(matTextures, matTexturesTemp, tex);
                            for (auto &tex_it : matTextures)
                            {
                                RCache.set_Textures(tex_it->first);
                                RImplementation.apply_lmaterial();

                                mapMatrixItems& items = tex_it->second;
                                items.ssa = 0;

                                std::sort(items.begin(), items.end(), cmp_ssa<_MatrixItem>);
                                for (auto &ni_it : items)
                                {
                                    RCache.set_xform_world(ni_it.Matrix);
                                    RImplementation.apply_object(ni_it.pObject);
                                    RImplementation.apply_lmaterial();

                                    float LOD = calcLOD(ni_it.ssa, ni_it.pVisual->vis.sphere.R);
#ifdef USE_DX11
                                    RCache.LOD.set_LOD(LOD);
#endif
                                    ni_it.pVisual->Render(LOD);
                                }
                                items.clear();
                            }
                            matTexturesTemp.clear();
                            matTextures.clear();
                            tex.clear();
                        }
                        matStates.clear();
                        states.clear();
                    }
                    matCS.clear();
                    cs.clear();
                }
                matPS.clear();
                ps.clear();
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
            }
            matGS.clear();
            gs.clear();
#endif
        }
        matVS.clear();
        vs.clear();
    }

    BasicStats.Primitives.End();
}


template <class T> IC bool cmp_first_l(const T &lhs, const T &rhs) { return (lhs.first < rhs.first); }
template <class T> IC bool cmp_first_h(const T &lhs, const T &rhs) { return (lhs.first > rhs.first); }
//////////////////////////////////////////////////////////////////////////
// HUD render
void D3DXRenderBase::r_dsgraph_render_hud()
{
    extern ENGINE_API float psHUD_FOV;

    // PIX_EVENT(r_dsgraph_render_hud);

    // Change projection
    Fmatrix Pold = Device.mProject;
    Fmatrix FTold = Device.mFullTransform;
    Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV /* *Device.fASPECT*/), Device.fASPECT,
        VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv->far_plane);

    Device.mFullTransform.mul(Device.mProject, Device.mView);
    RCache.set_xform_project(Device.mProject);

    // Rendering
    rmNear();
    std::sort(mapHUD.begin(), mapHUD.end(), cmp_first_l<R_dsgraph::mapHUD_T::value_type>); // front-to-back
    for (auto &i : mapHUD)
        sorted_L1(i);
    mapHUD.clear();

#if RENDER == R_R1
    if (g_hud && g_hud->RenderActiveItemUIQuery())
        r_dsgraph_render_hud_ui(); // hud ui
#endif
    /*
    if(g_hud && g_hud->RenderActiveItemUIQuery())
    {
#if	RENDER!=R_R1
        // Targets, use accumulator for temporary storage
        const ref_rt	rt_null;
        //	Reset all rt.
        //RCache.set_RT(0,	0);
        RCache.set_RT(0,	1);
        RCache.set_RT(0,	2);
        //if (RImplementation.o.albedo_wo)	RCache.set_RT(RImplementation.Target->rt_Accumulator->pRT,	0);
        //else								RCache.set_RT(RImplementation.Target->rt_Color->pRT,	0);
        if (RImplementation.o.albedo_wo)	RImplementation.Target->u_setrt
(RImplementation.Target->rt_Accumulator,	rt_null,	rt_null,	HW.pBaseZB);
        else								RImplementation.Target->u_setrt		(RImplementation.Target->rt_Color,
rt_null,	rt_null,	HW.pBaseZB);
        //	View port is reset in DX9 when you change rt
        rmNear						();
#endif
        g_hud->RenderActiveItemUI	();

#if	RENDER!=R_R1
        //RCache.set_RT(0,	0);
        // Targets, use accumulator for temporary storage
        if (RImplementation.o.albedo_wo)	RImplementation.Target->u_setrt		(RImplementation.Target->rt_Position,
RImplementation.Target->rt_Normal,	RImplementation.Target->rt_Accumulator,	HW.pBaseZB);
        else								RImplementation.Target->u_setrt		(RImplementation.Target->rt_Position,
RImplementation.Target->rt_Normal,	RImplementation.Target->rt_Color,		HW.pBaseZB);
#endif
    }
    */

    rmNormal();

    // Restore projection
    Device.mProject = Pold;
    Device.mFullTransform = FTold;
    RCache.set_xform_project(Device.mProject);
}

void D3DXRenderBase::r_dsgraph_render_hud_ui()
{
    VERIFY(g_hud && g_hud->RenderActiveItemUIQuery());

    extern ENGINE_API float psHUD_FOV;

    // Change projection
    Fmatrix Pold = Device.mProject;
    Fmatrix FTold = Device.mFullTransform;
    Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV /* *Device.fASPECT*/), Device.fASPECT,
        VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv->far_plane);

    Device.mFullTransform.mul(Device.mProject, Device.mView);
    RCache.set_xform_project(Device.mProject);

#if RENDER != R_R1
    // Targets, use accumulator for temporary storage
    const ref_rt rt_null;
    RCache.set_RT(0, 1);
    RCache.set_RT(0, 2);
#if (RENDER == R_R3) || (RENDER == R_R4) || (RENDER==R_GL)
    if (!RImplementation.o.dx10_msaa)
    {
        if (RImplementation.o.albedo_wo)
            RImplementation.Target->u_setrt(RImplementation.Target->rt_Accumulator, rt_null, rt_null, HW.pBaseZB);
        else
            RImplementation.Target->u_setrt(RImplementation.Target->rt_Color, rt_null, rt_null, HW.pBaseZB);
    }
    else
    {
        if (RImplementation.o.albedo_wo)
            RImplementation.Target->u_setrt(
                RImplementation.Target->rt_Accumulator, rt_null, rt_null, RImplementation.Target->rt_MSAADepth->pZRT);
        else
            RImplementation.Target->u_setrt(
                RImplementation.Target->rt_Color, rt_null, rt_null, RImplementation.Target->rt_MSAADepth->pZRT);
    }
#else // (RENDER==R_R3) || (RENDER==R_R4)
    if (RImplementation.o.albedo_wo)
        RImplementation.Target->u_setrt(RImplementation.Target->rt_Accumulator, rt_null, rt_null, HW.pBaseZB);
    else
        RImplementation.Target->u_setrt(RImplementation.Target->rt_Color, rt_null, rt_null, HW.pBaseZB);
#endif // (RENDER==R_R3) || (RENDER==R_R4)
#endif // RENDER!=R_R1

    rmNear();
    g_hud->RenderActiveItemUI();
    rmNormal();

    // Restore projection
    Device.mProject = Pold;
    Device.mFullTransform = FTold;
    RCache.set_xform_project(Device.mProject);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_sorted()
{
    std::sort(mapSorted.begin(), mapSorted.end(), cmp_first_h<R_dsgraph::mapSorted_T::value_type>); // back-to-front
    for (auto &i : mapSorted)
        sorted_L1(i);
    mapSorted.clear();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_emissive()
{
#if RENDER != R_R1
    std::sort(mapEmissive.begin(), mapEmissive.end(), cmp_first_l<R_dsgraph::mapSorted_T::value_type>); // front-to-back
    for (auto &i : mapEmissive)
        sorted_L1(i);
    mapEmissive.clear();

    //	HACK: Calculate this only once

    extern ENGINE_API float psHUD_FOV;

    // Change projection
    Fmatrix Pold = Device.mProject;
    Fmatrix FTold = Device.mFullTransform;
    Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV /* *Device.fASPECT*/), Device.fASPECT,
        VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv->far_plane);

    Device.mFullTransform.mul(Device.mProject, Device.mView);
    RCache.set_xform_project(Device.mProject);

    // Rendering
    rmNear();
    std::sort(mapHUDEmissive.begin(), mapHUDEmissive.end(), cmp_first_l<R_dsgraph::mapSorted_T::value_type>); // front-to-back
    for (auto &i : mapHUDEmissive)
        sorted_L1(i);
    mapHUDEmissive.clear();

    rmNormal();

    // Restore projection
    Device.mProject = Pold;
    Device.mFullTransform = FTold;
    RCache.set_xform_project(Device.mProject);
#endif
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_wmarks()
{
#if RENDER != R_R1
    std::sort(mapWmark.begin(), mapWmark.end(), cmp_first_l<R_dsgraph::mapSorted_T::value_type>); // front-to-back
    for (auto &i : mapWmark)
        sorted_L1(i);
    mapWmark.clear();
#endif
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_distort()
{
    std::sort(mapDistort.begin(), mapDistort.end(), cmp_first_h<R_dsgraph::mapSorted_T::value_type>); // back-to-front
    for (auto &i : mapDistort)
        sorted_L1(i);
    mapDistort.clear();
}

//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void D3DXRenderBase::r_dsgraph_render_subspace(
    IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
    CFrustum temp;
    temp.CreateFromMatrix(mCombined, FRUSTUM_P_ALL & (~FRUSTUM_P_NEAR));
    r_dsgraph_render_subspace(_sector, &temp, mCombined, _cop, _dynamic, _precise_portals);
}

// sub-space rendering - main procedure
void D3DXRenderBase::r_dsgraph_render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined,
    Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
    VERIFY(_sector);
    RImplementation.marker++; // !!! critical here

    // Save and build new frustum, disable HOM
    CFrustum ViewSave = ViewBase;
    ViewBase = *_frustum;
    View = &ViewBase;

    if (_precise_portals && RImplementation.rmPortals)
    {
        // Check if camera is too near to some portal - if so force DualRender
        Fvector box_radius;
        box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
        RImplementation.Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
        RImplementation.Sectors_xrc.box_query(RImplementation.rmPortals, _cop, box_radius);
        for (int K = 0; K < RImplementation.Sectors_xrc.r_count(); K++)
        {
            CPortal* pPortal =
                (CPortal*)RImplementation
                    .Portals[RImplementation.rmPortals->get_tris()[RImplementation.Sectors_xrc.r_begin()[K].id].dummy];
            pPortal->bDualRender = TRUE;
        }
    }

    // Traverse sector/portal structure
    PortalTraverser.traverse(_sector, ViewBase, _cop, mCombined, 0);

    // Determine visibility for static geometry hierrarhy
    for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
    {
        CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
        dxRender_Visual* root = sector->root();
        for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
        {
            set_Frustum(&(sector->r_frustums[v_it]));
            add_Geometry(root);
        }
    }

    if (_dynamic)
    {
        set_Object(nullptr);

        // Traverse object database
        g_SpatialSpace->q_frustum(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE, ViewBase);

        // Determine visibility for dynamic part of scene
        for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
        {
            ISpatial* spatial = lstRenderables[o_it];
            CSector* sector = (CSector*)spatial->GetSpatialData().sector;
            if (nullptr == sector)
                continue; // disassociated from S/P structure
            if (PortalTraverser.i_marker != sector->r_marker)
                continue; // inactive (untouched) sector
            for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
            {
                set_Frustum(&(sector->r_frustums[v_it]));
                if (!View->testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
                    continue;

                // renderable
                IRenderable* renderable = spatial->dcast_Renderable();
                if (nullptr == renderable)
                    continue; // unknown, but renderable object (r1_glow???)

                renderable->renderable_Render();
            }
        }
    }

#if RENDER != R_R1
    if (g_pGameLevel && (phase == RImplementation.PHASE_SMAP) && ps_actor_shadow_flags.test(RFLAG_ACTOR_SHADOW))
        g_hud->Render_Actor_Shadow(); // Actor Shadow
#endif

    // Restore
    ViewBase = ViewSave;
    View = nullptr;
}

#include "SkeletonCustom.h"
#include "FLOD.h"

void D3DXRenderBase::r_dsgraph_render_R1_box(IRender_Sector* S, Fbox& BB, int sh)
{
    lstVisuals.clear();
    lstVisuals.push_back(((CSector*)S)->root());

    for (auto &it : lstVisuals)
    {
        // Visual is 100% visible - simply add it
        switch (it->Type)
        {
        case MT_HIERRARHY:
        {
            // Add all children
            FHierrarhyVisual* pV = (FHierrarhyVisual*)it;
            for (auto &i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID:
        {
            // Add all children	(s)
            CKinematics* pV = (CKinematics*)it;
            pV->CalculateBones(TRUE);
            for (auto &i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        case MT_LOD:
        {
            FLOD* pV = (FLOD*)it;
            for (auto &i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        default:
        {
            // Renderable visual
            ShaderElement* E2 = it->shader->E[sh]._get();
            if (E2 && !(E2->flags.bDistort))
            {
                for (u32 pass = 0; pass < E2->passes.size(); pass++)
                {
                    RCache.set_Element(E2, pass);
                    it->Render(-1.f);
                }
            }
        }
        break;
        }
    }
}
