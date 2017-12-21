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
void __fastcall sorted_L1(mapSorted_Node* N)
{
    VERIFY(N);
    dxRender_Visual* V = N->val.pVisual;
    VERIFY(V && V->shader._get());
    RCache.set_Element(N->val.se);
    RCache.set_xform_world(N->val.Matrix);
    RImplementation.apply_object(N->val.pObject);
    RImplementation.apply_lmaterial();
    V->Render(calcLOD(N->key, V->vis.sphere.R));
}

template <class T> IC bool cmp_val_ssa(const T &lhs, const T &rhs) { return (lhs->val.ssa > rhs->val.ssa); }
template <class T> IC bool cmp_ssa    (const T &lhs, const T &rhs) { return (lhs.ssa      > rhs.ssa     ); }

template <class T> IC bool cmp_ps_val_ssa(const T &lhs, const T &rhs)
{
#ifdef USE_DX11
    return (lhs->val.mapCS.ssa > rhs->val.mapCS.ssa);
#else
    return (lhs->val.ssa > rhs->val.ssa);
#endif
}

template <class T> IC bool cmp_textures_lex2(const T &lhs, const T &rhs)
{
    auto t1 = lhs->key;
    auto t2 = rhs->key;

    if ((*t1)[0] < (*t2)[0]) return true;
    if ((*t1)[0] > (*t2)[0]) return false;
    if ((*t1)[1] < (*t2)[1]) return true;
    else               return false;
}
template <class T> IC bool cmp_textures_lex3(const T &lhs, const T &rhs)
{
    auto t1 = lhs->key;
    auto t2 = rhs->key;

    if ((*t1)[0] < (*t2)[0]) return true;
    if ((*t1)[0] > (*t2)[0]) return false;
    if ((*t1)[1] < (*t2)[1]) return true;
    if ((*t1)[1] > (*t2)[1]) return false;
    if ((*t1)[2] < (*t2)[2]) return true;
    else               return false;
}
template <class T> IC bool cmp_textures_lexN(const T &lhs, const T &rhs)
{
    auto t1 = lhs->key;
    auto t2 = rhs->key;

    return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}

template <class T> void sort_tlist(xr_vector<T::template TNode*>& lst, T& textures)
{
    int amount = textures.begin()->key->size();

    if (amount <= 1)
    {
        // Just sort by SSA
        textures.getANY_P(lst);
        std::sort(lst.begin(), lst.end(), cmp_val_ssa<T::template TNode*>);
    }
    else
    {
        xr_vector<T::template TNode*> temp;

        // Split into 2 parts
        for (auto &it : textures)
        {
            if (it.val.ssa > r_ssaHZBvsTEX)
                lst.push_back(&it);
            else
                temp.push_back(&it);
        }

        // 1st - part - SSA, 2nd - lexicographically
        std::sort(lst.begin(), lst.end(), cmp_val_ssa<T::template TNode*>);
        if (2 == amount)
            std::sort(temp.begin(), temp.end(), cmp_textures_lex2<T::template TNode*>);
        else if (3 == amount)
            std::sort(temp.begin(), temp.end(), cmp_textures_lex3<T::template TNode*>);
        else
            std::sort(temp.begin(), temp.end(), cmp_textures_lexN<T::template TNode*>);

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

            xr_vector<mapNormalVS::TNode*> nrmVS;
            vs.getANY_P(nrmVS);
            std::sort(nrmVS.begin(), nrmVS.end(), cmp_val_ssa<mapNormalVS::TNode*>);
            for (auto & vs_it : nrmVS)
            {
                RCache.set_VS(vs_it->key);

#if defined(USE_DX10) || defined(USE_DX11)
                //	GS setup
                mapNormalGS& gs = vs_it->val;

                xr_vector<mapNormalGS::TNode*> nrmGS;
                gs.getANY_P(nrmGS);
                std::sort(nrmGS.begin(), nrmGS.end(), cmp_val_ssa<mapNormalGS::TNode*>);
                for (auto & gs_it : nrmGS)
                {
                    RCache.set_GS(gs_it->key);

                    mapNormalPS& ps = gs_it->val;
#else // USE_DX10
                    mapNormalPS& ps = vs_it->val;
#endif // USE_DX10
                    xr_vector<mapNormalPS::TNode*> nrmPS;
                    ps.getANY_P(nrmPS);
                    std::sort(nrmPS.begin(), nrmPS.end(), cmp_ps_val_ssa<mapNormalPS::TNode*>);
                    for (auto &ps_it : nrmPS)
                    {
                        RCache.set_PS(ps_it->key);
#ifdef USE_DX11
                        RCache.set_HS(ps_it->val.hs);
                        RCache.set_DS(ps_it->val.ds);

                        mapNormalCS& cs = ps_it->val.mapCS;
#else
                        mapNormalCS& cs = ps_it->val;
#endif
                        xr_vector<mapNormalCS::TNode*> nrmCS;
                        cs.getANY_P(nrmCS);
                        std::sort(nrmCS.begin(), nrmCS.end(), cmp_val_ssa<mapNormalCS::TNode*>);
                        for (auto &cs_it : nrmCS)
                        {
                            RCache.set_Constants(cs_it->key);

                            mapNormalStates& states = cs_it->val;

                            xr_vector<mapNormalStates::TNode*> nrmStates;
                            states.getANY_P(nrmStates);
                            std::sort(nrmStates.begin(), nrmStates.end(), cmp_val_ssa<mapNormalStates::TNode*>);
                            for (auto &state_it : nrmStates)
                            {
                                RCache.set_States(state_it->key);

                                mapNormalTextures& tex = state_it->val;

                                xr_vector<mapNormalTextures::TNode*> nrmTextures;
                                sort_tlist<mapNormalTextures>(nrmTextures, tex);
                                for (auto &tex_it : nrmTextures)
                                {
                                    RCache.set_Textures(tex_it->key);
                                    RImplementation.apply_lmaterial();

                                    mapNormalItems& items = tex_it->val;

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
                                tex.clear();
                            }
                            states.clear();
                        }
                        cs.clear();
                    }
                    ps.clear();
#if defined(USE_DX10) || defined(USE_DX11)
                }
                gs.clear();
#endif // USE_DX10
            }
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

        xr_vector<mapMatrixVS::TNode*> matVS;
        vs.getANY_P(matVS);
        std::sort(matVS.begin(), matVS.end(), cmp_val_ssa<mapMatrixVS::TNode*>);
        for (auto &vs_id : matVS)
        {
            RCache.set_VS(vs_id->key);

#if defined(USE_DX10) || defined(USE_DX11)
            mapMatrixGS& gs = vs_id->val;

            xr_vector<mapMatrixGS::TNode*> matGS;
            gs.getANY_P(matGS);
            std::sort(matGS.begin(), matGS.end(), cmp_val_ssa<mapMatrixGS::TNode*>);
            for (auto &gs_it : matGS)
            {
                RCache.set_GS(gs_it->key);

                mapMatrixPS& ps = gs_it->val;
#else // USE_DX10
                mapMatrixPS& ps = vs_id->val;
#endif // USE_DX10
                xr_vector<mapMatrixPS::TNode *> matPS;
                ps.getANY_P(matPS);
                std::sort(matPS.begin(), matPS.end(), cmp_ps_val_ssa<mapMatrixPS::TNode *>);
                for (auto &ps_it : matPS)
                {
                    RCache.set_PS(ps_it->key);
#ifdef USE_DX11
                    RCache.set_HS(ps_it->val.hs);
                    RCache.set_DS(ps_it->val.ds);

                    mapMatrixCS& cs = ps_it->val.mapCS;
#else
                    mapMatrixCS& cs = ps_it->val;
#endif
                    xr_vector<mapMatrixCS::TNode*> matCS;
                    cs.getANY_P(matCS);
                    std::sort(matCS.begin(), matCS.end(), cmp_val_ssa<mapMatrixCS::TNode*>);
                    for (auto &cs_it : matCS)
                    {
                        RCache.set_Constants(cs_it->key);

                        mapMatrixStates& states = cs_it->val;

                        xr_vector<mapMatrixStates::TNode*> matStates;
                        states.getANY_P(matStates);
                        std::sort(matStates.begin(), matStates.end(), cmp_val_ssa<mapMatrixStates::TNode*>);
                        for (auto &state_it : matStates)
                        {
                            RCache.set_States(state_it->key);

                            mapMatrixTextures& tex = state_it->val;

                            xr_vector<mapMatrixTextures::TNode*> matTextures;
                            sort_tlist<mapMatrixTextures>(matTextures, tex);
                            for (auto &tex_it : matTextures)
                            {
                                RCache.set_Textures(tex_it->key);
                                RImplementation.apply_lmaterial();

                                mapMatrixItems& items = tex_it->val;

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
                            tex.clear();
                        }
                        states.clear();
                    }
                    cs.clear();
                }
                ps.clear();
#if defined(USE_DX10) || defined(USE_DX11)
            }
            gs.clear();
#endif // USE_DX10
        }
        vs.clear();
    }

    BasicStats.Primitives.End();
}

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
    mapHUD.traverseLR(sorted_L1);
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
#if (RENDER == R_R3) || (RENDER == R_R4)
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
    // Sorted (back to front)
    mapSorted.traverseRL(sorted_L1);
    mapSorted.clear();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_emissive()
{
#if RENDER != R_R1
    // Sorted (back to front)
    mapEmissive.traverseLR(sorted_L1);
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
    // Sorted (back to front)
    mapHUDEmissive.traverseLR(sorted_L1);
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
    // Sorted (back to front)
    mapWmark.traverseLR(sorted_L1);
    mapWmark.clear();
#endif
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void D3DXRenderBase::r_dsgraph_render_distort()
{
    // Sorted (back to front)
    mapDistort.traverseRL(sorted_L1);
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
    xr_vector<dxRender_Visual*> lstVisuals;
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
