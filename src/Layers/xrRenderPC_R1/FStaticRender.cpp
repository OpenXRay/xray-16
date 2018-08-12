#include "stdafx.h"
#include "FStaticRender.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/LightTrack.h"
#include "Layers/xrRender/dxWallMarkArray.h"
#include "Layers/xrRender/dxUIShader.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

CRender RImplementation;

//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(dxRender_Visual* pVisual, float /*cdist_sq*/)
{
    switch (phase)
    {
    case PHASE_NORMAL:
        return (RImplementation.L_Projector->shadowing() ?
            pVisual->shader->E[SE_R1_NORMAL_HQ] : pVisual->shader->E[SE_R1_NORMAL_LQ])._get();
    case PHASE_POINT: return pVisual->shader->E[SE_R1_LPOINT]._get();
    case PHASE_SPOT: return pVisual->shader->E[SE_R1_LSPOT]._get();
    default: NODEFAULT;
    }
#ifdef DEBUG
    return nullptr;
#endif
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq)
{
    switch (phase)
    {
    case PHASE_NORMAL:
        return (((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < 44) ? pVisual->shader->E[SE_R1_NORMAL_HQ] :
                                                                   pVisual->shader->E[SE_R1_NORMAL_LQ])
            ._get();
    case PHASE_POINT: return pVisual->shader->E[SE_R1_LPOINT]._get();
    case PHASE_SPOT: return pVisual->shader->E[SE_R1_LSPOT]._get();
    default: NODEFAULT;
    }
#ifdef DEBUG
    return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CRender::create()
{
    L_DB = nullptr;
    L_Shadows = nullptr;
    L_Projector = nullptr;

    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

    // c-setup
    Resources->RegisterConstantSetup("L_dynamic_pos", &r1_dlight_binder_PR);
    Resources->RegisterConstantSetup("L_dynamic_color", &r1_dlight_binder_color);
    Resources->RegisterConstantSetup("L_dynamic_xform", &r1_dlight_binder_xform);

    // distortion
    u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
    u32 v_need = CAP_VERSION(1, 4);
    if (v_dev >= v_need)
        o.distortion = TRUE;
    else
        o.distortion = FALSE;
    if (strstr(Core.Params, "-nodistort"))
        o.distortion = FALSE;
    Msg("* distortion: %s, dev(%d),need(%d)", o.distortion ? "used" : "unavailable", v_dev, v_need);

    //	Color mapping
    if (v_dev >= v_need)
        o.color_mapping = TRUE;
    else
        o.color_mapping = FALSE;
    if (strstr(Core.Params, "-nocolormap"))
        o.color_mapping = FALSE;
    Msg("* color_mapping: %s, dev(%d),need(%d)", o.color_mapping ? "used" : "unavailable", v_dev, v_need);

    m_skinning = -1;

    // disasm
    o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
    o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;
    o.no_detail_textures = !ps_r2_ls_flags.test(R1FLAG_DETAIL_TEXTURES);
    c_ldynamic_props = "L_dynamic_props";

    m_bMakeAsyncSS = false;

    Target = new CRenderTarget(); // Main target

    Models = new CModelPool();
    L_Dynamic = new CLightR_Manager();
    PSLibrary.OnCreate();
    //.	HWOCC.occq_create			(occq_size);

    ::PortalTraverser.initialize();
}

void CRender::destroy()
{
    m_bMakeAsyncSS = false;
    ::PortalTraverser.destroy();
    //.	HWOCC.occq_destroy			();
    PSLibrary.OnDestroy();

    xr_delete(L_Dynamic);
    xr_delete(Models);

    //*** Components
    xr_delete(Target);
    Device.seqFrame.Remove(this);
    r_dsgraph_destroy();
}

void CRender::reset_begin()
{
    //AVO: let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size || ps_r__Detail_density != ps_current_detail_density))
    {
        Details->Unload();
        xr_delete(Details);
    }
    xr_delete(Target);
    //HWOCC.occq_destroy();
}

void CRender::reset_end()
{
    //.	HWOCC.occq_create			(occq_size);
    Target = new CRenderTarget();
    if (L_Projector)
        L_Projector->invalidate();

    // let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size || ps_r__Detail_density != ps_current_detail_density))
    {
        Details = new CDetailManager();
        Details->Load();
    }

    // Set this flag true to skip the first render frame,
    // that some data is not ready in the first frame (for example device camera position)
    m_bFirstFrameAfterReset = true;
}

void CRender::OnFrame()
{
    Models->DeleteQueue();

    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
    {
        // MT-details (@front)
        Device.seqParallel.insert(
            Device.seqParallel.begin(), fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));

        // MT-HOM (@front)
        Device.seqParallel.insert(Device.seqParallel.begin(), fastdelegate::FastDelegate0<>(&HOM, &CHOM::MT_RENDER));
    }
}

// Перед началом рендера мира --#SM+#-- +SecondVP+
void CRender::BeforeWorldRender() {}

// После рендера мира и пост-эффектов --#SM+#-- +SecondVP+
void CRender::AfterWorldRender()
{
    if (Device.m_SecondViewport.IsSVPFrame())
    {
        // Делает копию бэкбуфера (текущего экрана) в рендер-таргет второго вьюпорта
        IRender_Target* T = getTarget();
        IDirect3DSurface9* pBackBuffer = nullptr;
        HW.pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer); // Получаем ссылку на бэкбуфер
        D3DXLoadSurfaceFromSurface(Target->RT_SecondVP->pRT, 0, 0, pBackBuffer, 0, 0, D3DX_DEFAULT, 0);
        pBackBuffer->Release(); // Корректно очищаем ссылку на бэкбуфер (иначе игра зависнет в опциях)
    }
}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent) { return new CROS_impl(); }
void CRender::ros_destroy(IRender_ObjectSpecific*& p) { xr_delete(p); }
IRenderVisual* CRender::model_Create(LPCSTR name, IReader* data) { return Models->Create(name, data); }
IRenderVisual* CRender::model_CreateChild(LPCSTR name, IReader* data) { return Models->CreateChild(name, data); }
IRenderVisual* CRender::model_Duplicate(IRenderVisual* V) { return Models->Instance_Duplicate((dxRender_Visual*)V); }
void CRender::model_Delete(IRenderVisual*& V, BOOL bDiscard)
{
    dxRender_Visual* pVisual = (dxRender_Visual*)V;
    Models->Delete(pVisual, bDiscard);
    V = nullptr;
}
IRender_DetailModel* CRender::model_CreateDM(IReader* F)
{
    CDetail* D = new CDetail();
    D->Load(F);
    return D;
}
void CRender::model_Delete(IRender_DetailModel*& F)
{
    if (F)
    {
        CDetail* D = (CDetail*)F;
        D->Unload();
        xr_delete(D);
        F = nullptr;
    }
}
IRenderVisual* CRender::model_CreatePE(LPCSTR name)
{
    PS::CPEDef* SE = PSLibrary.FindPED(name);
    R_ASSERT3(SE, "Particle effect doesn't exist", name);
    return Models->CreatePE(SE);
}
IRenderVisual* CRender::model_CreateParticles(LPCSTR name)
{
    PS::CPEDef* SE = PSLibrary.FindPED(name);
    if (SE)
        return Models->CreatePE(SE);
    else
    {
        PS::CPGDef* SG = PSLibrary.FindPGD(name);
        R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
        return Models->CreatePG(SG);
    }
}
void CRender::models_Prefetch() { Models->Prefetch(); }
void CRender::models_Clear(BOOL b_complete) { Models->ClearPool(b_complete); }
ref_shader CRender::getShader(int id)
{
    VERIFY(id < int(Shaders.size()));
    return Shaders[id];
}
IRender_Portal* CRender::getPortal(int id)
{
    VERIFY(id < int(Portals.size()));
    return Portals[id];
}
IRender_Sector* CRender::getSector(int id)
{
    VERIFY(id < int(Sectors.size()));
    return Sectors[id];
}
IRender_Sector* CRender::getSectorActive() { return pLastSector; }
IRenderVisual* CRender::getVisual(int id)
{
    VERIFY(id < int(Visuals.size()));
    return Visuals[id];
}
D3DVERTEXELEMENT9* CRender::getVB_Format(int id)
{
    VERIFY(id < int(DCL.size()));
    return DCL[id].begin();
}
IDirect3DVertexBuffer9* CRender::getVB(int id)
{
    VERIFY(id < int(VB.size()));
    return VB[id];
}
IDirect3DIndexBuffer9* CRender::getIB(int id)
{
    VERIFY(id < int(IB.size()));
    return IB[id];
}
FSlideWindowItem* CRender::getSWI(int id)
{
    VERIFY(id < int(SWIs.size()));
    return &SWIs[id];
}
IRender_Target* CRender::getTarget() { return Target; }
IRender_Light* CRender::light_create() { return L_DB->Create(); }
IRender_Glow* CRender::glow_create() { return new CGlow(); }
void CRender::flush() { r_dsgraph_render_graph(0); }
BOOL CRender::occ_visible(vis_data& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(sPoly& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(Fbox& P) { return HOM.visible(P); }
void CRender::add_Visual(IRenderVisual* V) { add_leafs_Dynamic((dxRender_Visual*)V); }
void CRender::add_Geometry(IRenderVisual* V) { add_Static((dxRender_Visual*)V, View->getMask()); }
void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
    if (T->suppress_wm)
        return;
    VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
    Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    dxWallMarkArray* pWMA = (dxWallMarkArray*)pArray;
    ref_shader* pShader = pWMA->dxGenerateWallmark();
    if (pShader)
        add_StaticWallmark(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    dxUIShader* pShader = (dxUIShader*)&*S;
    add_StaticWallmark(pShader->hShader, P, s, T, V);
}

void CRender::clear_static_wallmarks() { Wallmarks->clear(); }
void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm) { Wallmarks->AddSkeletonWallmark(wm); }
void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
    Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size)
{
    dxWallMarkArray* pWMA = (dxWallMarkArray*)pArray;
    ref_shader* pShader = pWMA->dxGenerateWallmark();
    if (pShader)
        add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}
void CRender::add_Occluder(Fbox2& bb_screenspace) { HOM.occlude(bb_screenspace); }

#include "xrEngine/PS_instance.h"
void CRender::set_Object(IRenderable* O)
{
    val_pObject = O; // NULL is OK, trust me :)
    if (val_pObject)
    {
        VERIFY(dynamic_cast<IGameObject*>(O) || dynamic_cast<CPS_Instance*>(O));
        if (O->GetRenderData().pROS)
        {
            VERIFY(dynamic_cast<CROS_impl*>(O->GetRenderData().pROS));
        }
    }
    if (PHASE_NORMAL == phase)
    {
        if (L_Shadows)
            L_Shadows->set_object(O);

        if (L_Projector)
            L_Projector->set_object(O);
    }
    else
    {
        if (L_Shadows)
            L_Shadows->set_object(nullptr);

        if (L_Projector)
            L_Projector->set_object(nullptr);
    }
}
void CRender::apply_object(IRenderable* O)
{
    if (nullptr == O)
        return;
    if (PHASE_NORMAL == phase && O->renderable_ROS())
    {
        CROS_impl& LT = *((CROS_impl*)O->GetRenderData().pROS);
        VERIFY(dynamic_cast<IGameObject*>(O) || dynamic_cast<CPS_Instance*>(O));
        VERIFY(dynamic_cast<CROS_impl*>(O->GetRenderData().pROS));
        float o_hemi = 0.5f * LT.get_hemi();
        float o_sun = 0.5f * LT.get_sun();
        RCache.set_c(c_ldynamic_props, o_sun, o_sun, o_sun, o_hemi);
        // shadowing
        if ((LT.shadow_recv_frame == Device.dwFrame) && O->renderable_ShadowReceive())
            RImplementation.L_Projector->setup(LT.shadow_recv_slot);
    }
}

// Misc
float g_fSCREEN;
static BOOL gm_Nearer = 0;

IC void gm_SetNearer(BOOL bNearer)
{
    if (bNearer != gm_Nearer)
    {
        gm_Nearer = bNearer;
        if (gm_Nearer)
            RImplementation.rmNear();
        else
            RImplementation.rmNormal();
    }
}

void CRender::rmNear()
{
    IRender_Target* T = getTarget();
    D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0, 0.02f};
    CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmFar()
{
    IRender_Target* T = getTarget();
    D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0.99999f, 1.f};
    CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmNormal()
{
    IRender_Target* T = getTarget();
    D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0, 1.f};
    CHK_DX(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() : m_bFirstFrameAfterReset(false), Sectors_xrc("render") {}
CRender::~CRender() {}
extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaLOD_A, r_ssaLOD_B;
extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float r_ssaHZBvsTEX;

ICF bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
    float d1 = _1->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
    float d2 = _2->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
    return d1 < d2;
}

void CRender::Calculate()
{
#ifdef _GPA_ENABLED
    TAL_SCOPED_TASK_NAMED("CRender::Calculate()");
#endif // _GPA_ENABLED

    BasicStats.Culling.Begin();

    // Transfer to global space to avoid deep pointer access
    IRender_Target* T = getTarget();
    float fov_factor = _sqr(90.f / Device.fFOV);
    g_fSCREEN = float(T->get_width() * T->get_height()) * fov_factor * (EPS_S + ps_r__LOD);
    r_ssaDISCARD = _sqr(ps_r__ssaDISCARD) / g_fSCREEN;
    r_ssaDONTSORT = _sqr(ps_r__ssaDONTSORT / 3) / g_fSCREEN;
    r_ssaLOD_A = _sqr(ps_r1_ssaLOD_A / 3) / g_fSCREEN;
    r_ssaLOD_B = _sqr(ps_r1_ssaLOD_B / 3) / g_fSCREEN;
    r_ssaGLOD_start = _sqr(ps_r__GLOD_ssa_start / 3) / g_fSCREEN;
    r_ssaGLOD_end = _sqr(ps_r__GLOD_ssa_end / 3) / g_fSCREEN;
    r_ssaHZBvsTEX = _sqr(ps_r__ssaHZBvsTEX / 3) / g_fSCREEN;

    // Frustum & HOM rendering
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    View = nullptr;
    if (!ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
    {
        HOM.Enable();
        HOM.Render(ViewBase);
    }
    gm_SetNearer(FALSE);
    phase = PHASE_NORMAL;

    // Detect camera-sector
    if (!vLastCameraPos.similar(Device.vCameraPosition, EPS_S))
    {
        CSector* pSector = (CSector*)detectSector(Device.vCameraPosition);
        if (pSector && (pSector != pLastSector))
            g_pGamePersistent->OnSectorChanged(translateSector(pSector));

        if (nullptr == pSector)
            pSector = pLastSector;
        pLastSector = pSector;
        vLastCameraPos.set(Device.vCameraPosition);
    }

    // Check if camera is too near to some portal - if so force DualRender
    if (rmPortals)
    {
        Fvector box_radius;
        box_radius.set(EPS_L * 2, EPS_L * 2, EPS_L * 2);
        Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
        Sectors_xrc.box_query(rmPortals, Device.vCameraPosition, box_radius);
        for (int K = 0; K < Sectors_xrc.r_count(); K++)
        {
            CPortal* pPortal = (CPortal*)Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
            pPortal->bDualRender = TRUE;
        }
    }
    //
    if (L_DB)
        L_DB->Update();

    // Main process
    marker++;
    if (pLastSector)
    {
        // Traverse sector/portal structure
        PortalTraverser.traverse(pLastSector, ViewBase, Device.vCameraPosition, Device.mFullTransform,
            CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);

        // Determine visibility for static geometry hierarchy
        if (psDeviceFlags.test(rsDrawStatic))
        {
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
        }

        // Traverse object database
        if (psDeviceFlags.test(rsDrawDynamic))
        {
            g_SpatialSpace->q_frustum(
                lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE, ViewBase);

            // Exact sorting order (front-to-back)
            std::sort(lstRenderables.begin(), lstRenderables.end(), pred_sp_sort);

            // Determine visibility for dynamic part of scene
            set_Object(nullptr);
            g_hud->Render_First(); // R1 shadows
            g_hud->Render_Last();
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
                if (nullptr == sector)
                    continue; // disassociated from S/P structure

                // Filter only not light spatial
                if (PortalTraverser.i_marker != sector->r_marker && (spatial->GetSpatialData().type & STYPE_RENDERABLE))
                    continue; // inactive (untouched) sector

                if (spatial->GetSpatialData().type & STYPE_RENDERABLE)
                {
                    for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
                    {
                        set_Frustum(&(sector->r_frustums[v_it]));

                        if (!View->testSphere_dirty(spatial->GetSpatialData().sphere.P,
                                spatial->GetSpatialData().sphere.R) /*&& (spatial->spatial.type & STYPE_RENDERABLE)*/)
                            continue;
                        // renderable
                        IRenderable* renderable = spatial->dcast_Renderable();
                        if (nullptr == renderable)
                        {
                            // It may be an glow
                            CGlow* glow = dynamic_cast<CGlow*>(spatial);
                            VERIFY(glow);
#ifdef DEBUG
                            BasicStats.Glows.Begin();
#endif
                            L_Glows->add(glow);
#ifdef DEBUG
                            BasicStats.Glows.End();
#endif
                        }
                        else
                        {
                            // Occlusiond
                            vis_data& v_orig = renderable->GetRenderData().visual->getVisData();
                            vis_data v_copy = v_orig;
                            v_copy.box.xform(renderable->GetRenderData().xform);
                            BOOL bVisible = HOM.visible(v_copy);
                            v_orig.accept_frame = v_copy.accept_frame;
                            v_orig.marker = v_copy.marker;
                            v_orig.hom_frame = v_copy.hom_frame;
                            v_orig.hom_tested = v_copy.hom_tested;
                            if (!bVisible)
                                break; // exit loop on frustums

                            // rendering
                            if (o_it == uID_LTRACK && renderable->renderable_ROS())
                            {
                                // track lighting environment
                                CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
                                T->update(renderable);
                            }
                            set_Object(renderable);
                            renderable->renderable_Render();
                            set_Object(nullptr); //? is it needed at all
                        }
                        break; // exit loop on frustums
                    }
                }
                else
                {
                    if (ViewBase.testSphere_dirty(
                            spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
                    {
                        VERIFY(spatial->GetSpatialData().type & STYPE_LIGHTSOURCE);
                        // lightsource
                        light* L = (light*)spatial->dcast_Light();
                        VERIFY(L);
                        if (L->spatial.sector)
                        {
                            vis_data& vis = L->get_homdata();
                            if (HOM.visible(vis))
                                L_DB->add_light(L);
                        }
                    }
                }
            }
        }

        // Calculate miscellaneous stuff
        BasicStats.ShadowsCalc.Begin();
        L_Shadows->calculate();
        BasicStats.ShadowsCalc.End();
        BasicStats.Projectors.Begin();
        L_Projector->calculate();
        BasicStats.Projectors.End();
    }
    else
    {
        set_Object(nullptr);
    }

    // End calc
    BasicStats.Culling.End();
}

extern u32 g_r;
void CRender::Render()
{
#ifdef _GPA_ENABLED
    TAL_SCOPED_TASK_NAMED("CRender::Render()");
#endif // _GPA_ENABLED

    if (m_bFirstFrameAfterReset)
    {
        m_bFirstFrameAfterReset = false;
        return;
    }

    g_r = 1;
    BasicStats.Primitives.Begin();
    // Begin
    Target->Begin();
    o.vis_intersect = FALSE;
    phase = PHASE_NORMAL;
    r_dsgraph_render_hud(); // hud
    r_dsgraph_render_graph(0); // normal level
    if (Details)
        Details->Render(); // grass / details
    r_dsgraph_render_lods(true, false); // lods - FB

    g_pGamePersistent->Environment().RenderSky(); // sky / sun
    g_pGamePersistent->Environment().RenderClouds(); // clouds

    r_pmask(true, false); // disable priority "1"
    o.vis_intersect = TRUE;
    HOM.Disable();
    L_Dynamic->render(0); // additional light sources
    if (Wallmarks)
    {
        g_r = 0;
        Wallmarks->Render(); // wallmarks has priority as normal geometry
    }
    HOM.Enable();
    o.vis_intersect = FALSE;
    phase = PHASE_NORMAL;
    r_pmask(true, true); // enable priority "0" and "1"
    BasicStats.ShadowsRender.Begin();
    if (L_Shadows)
        L_Shadows->render(); // ... and shadows
    BasicStats.ShadowsRender.End();
    r_dsgraph_render_lods(false, true); // lods - FB
    r_dsgraph_render_graph(1); // normal level, secondary priority
    L_Dynamic->render(1); // additional light sources, secondary priority
    PortalTraverser.fade_render(); // faded-portals
    r_dsgraph_render_sorted(); // strict-sorted geoms
    BasicStats.Glows.Begin();
    if (L_Glows)
        L_Glows->Render(); // glows
    BasicStats.Glows.End();
    g_pGamePersistent->Environment().RenderFlares(); // lens-flares
    g_pGamePersistent->Environment().RenderLast(); // rain/thunder-bolts

#ifdef DEBUG
    for (int _priority = 0; _priority < 2; ++_priority)
    {
        for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
        {
            R_ASSERT(mapNormalPasses[_priority][iPass].size() == 0);
            R_ASSERT(mapMatrixPasses[_priority][iPass].size() == 0);
        }
    }

#endif
    // Postprocess, if necessary
    Target->End();
    if (L_Projector)
        L_Projector->finalize();

    // HUD
    BasicStats.Primitives.End();
}

void CRender::ApplyBlur4(FVF::TL4uv* pv, u32 w, u32 h, float k)
{
    float _w = float(w);
    float _h = float(h);
    float kw = (1.f / _w) * k;
    float kh = (1.f / _h) * k;
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    u32 _c = 0xffffffff;

    // Fill vertex buffer
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f);
    pv->color = _c;
    pv->uv[0].set(p0.x - kw, p1.y - kh);
    pv->uv[1].set(p0.x + kw, p1.y + kh);
    pv->uv[2].set(p0.x + kw, p1.y - kh);
    pv->uv[3].set(p0.x - kw, p1.y + kh);
    pv++;
    pv->p.set(EPS, EPS, EPS, 1.f);
    pv->color = _c;
    pv->uv[0].set(p0.x - kw, p0.y - kh);
    pv->uv[1].set(p0.x + kw, p0.y + kh);
    pv->uv[2].set(p0.x + kw, p0.y - kh);
    pv->uv[3].set(p0.x - kw, p0.y + kh);
    pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f);
    pv->color = _c;
    pv->uv[0].set(p1.x - kw, p1.y - kh);
    pv->uv[1].set(p1.x + kw, p1.y + kh);
    pv->uv[2].set(p1.x + kw, p1.y - kh);
    pv->uv[3].set(p1.x - kw, p1.y + kh);
    pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f);
    pv->color = _c;
    pv->uv[0].set(p1.x - kw, p0.y - kh);
    pv->uv[1].set(p1.x + kw, p0.y + kh);
    pv->uv[2].set(p1.x + kw, p0.y - kh);
    pv->uv[3].set(p1.x - kw, p0.y + kh);
    pv++;
}

void CRender::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    D3DXRenderBase::DumpStatistics(font, alert);
    HOM.DumpStatistics(font, alert);
    Sectors_xrc.DumpStatistics(font, alert);
}

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    T*& result, bool const disasm)
{
    result->sh = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size);

    LPCVOID data = nullptr;

    HRESULT const _hr = D3DXFindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, nullptr);

    if (SUCCEEDED(_hr) && data)
    {
        // Parse constant table data
        LPD3DXSHADER_CONSTANTTABLE pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
        result->constants.parse(pConstants, ShaderTypeTraits<T>::GetShaderDest());
    }
    else
        Msg("! D3DXFindShaderComment %s hr == 0x%08x", file_name, _hr);

    if (disasm)
    {
        ID3DXBuffer* disasm = nullptr;
        D3DXDisassembleShader(LPDWORD(buffer), FALSE, nullptr, &disasm);
        string_path dname;
        strconcat(sizeof(dname), dname, "disasm\\", file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
        IWriter* W = FS.w_open("$logs$", dname);
        W->w(disasm->GetBufferPointer(), disasm->GetBufferSize());
        FS.w_close(W);
        _RELEASE(disasm);
    }

    return _hr;
}

inline HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name, void*& result, bool const disasm)
{
    if (pTarget[0] == 'p')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SPS*&)result, disasm);
    
    if (pTarget[0] == 'v')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SVS*&)result, disasm);
    
    NODEFAULT;
    return E_FAIL;
}

class includer : public ID3DXInclude
{
public:
    HRESULT __stdcall Open(
        D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        string_path pname;
        strconcat(sizeof(pname), pname, GEnv.Render->getShaderPath(), pFileName);
        IReader* R = FS.r_open("$game_shaders$", pname);
        if (nullptr == R)
        {
            // possibly in shared directory or somewhere else - open directly
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        // duplicate and zero-terminate
        u32 size = R->length();
        u8* data = xr_alloc<u8>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        *ppData = data;
        *pBytes = size;
        return D3D_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData)
    {
        xr_free(pData);
        return D3D_OK;
    }
};

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);

HRESULT CRender::shader_compile(LPCSTR name, DWORD const* pSrcData, UINT SrcDataLen, LPCSTR pFunctionName,
    LPCSTR pTarget, DWORD Flags, void*& result)
{
    D3DXMACRO defines[128];
    int def_it = 0;

    char sh_name[MAX_PATH] = "";
    u32 len = 0;
    // options
    if (o.forceskinw)
    {
        defines[def_it].Name = "SKIN_COLOR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.forceskinw);
    ++len;

    // skinning
    if (m_skinning < 0)
    {
        defines[def_it].Name = "SKIN_NONE";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (0 == m_skinning)
    {
        defines[def_it].Name = "SKIN_0";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(0 == m_skinning);
    ++len;

    if (1 == m_skinning)
    {
        defines[def_it].Name = "SKIN_1";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(1 == m_skinning);
    ++len;

    if (2 == m_skinning)
    {
        defines[def_it].Name = "SKIN_2";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(2 == m_skinning);
    ++len;

    if (3 == m_skinning)
    {
        defines[def_it].Name = "SKIN_3";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(3 == m_skinning);
    ++len;

    if (4 == m_skinning)
    {
        defines[def_it].Name = "SKIN_4";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(4 == m_skinning);
    ++len;

    sh_name[len] = '\0';

    // finish
    defines[def_it].Name = nullptr;
    defines[def_it].Definition = nullptr;
    def_it++;
    R_ASSERT(def_it < 128);

    HRESULT _result = E_FAIL;

    string_path folder_name, folder;
    xr_strcpy(folder, "r1\\objects\\r1\\");
    xr_strcat(folder, name);
    xr_strcat(folder, ".");

    char extension[3];
    strncpy_s(extension, pTarget, 2);
    xr_strcat(folder, extension);

    FS.update_path(folder_name, "$game_shaders$", folder);
    xr_strcat(folder_name, "\\");

    m_file_set.clear();
    FS.file_list(m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

    string_path temp_file_name, file_name;
    if (!match_shader_id(name, sh_name, m_file_set, temp_file_name))
    {
        string_path file;
        xr_strcpy(file, "shaders_cache\\r1\\");
        xr_strcat(file, name);
        xr_strcat(file, ".");
        xr_strcat(file, extension);
        xr_strcat(file, "\\");
        xr_strcat(file, sh_name);
        FS.update_path(file_name, "$app_data_root$", file);
    }
    else
    {
        xr_strcpy(file_name, folder_name);
        xr_strcat(file_name, temp_file_name);
    }

    if (FS.exist(file_name))
    {
        IReader* file = FS.r_open(file_name);
        if (file->length() > 4)
        {
            u32 crc = file->r_u32();
            u32 crcComp = crc32(file->pointer(), file->elapsed());
            if (crcComp == crc)
                _result = create_shader(pTarget, (DWORD*)file->pointer(), file->elapsed(), file_name, result, o.disasm);
        }
        file->close();
    }

    if (FAILED(_result))
    {
        includer Includer;
        LPD3DXBUFFER pShaderBuf = nullptr;
        LPD3DXBUFFER pErrorBuf = nullptr;
        LPD3DXCONSTANTTABLE pConstants = nullptr;
        LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)&Includer;

        _result = D3DXCompileShader((LPCSTR)pSrcData, SrcDataLen, defines, pInclude, pFunctionName, pTarget,
            Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &pShaderBuf, &pErrorBuf, &pConstants);
        if (SUCCEEDED(_result))
        {
            IWriter* file = FS.w_open(file_name);
            u32 crc = crc32(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());
            file->w_u32(crc);
            file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
            FS.w_close(file);

            _result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(),
                file_name, result, o.disasm);
        }
        else
        {
            Log("! ", file_name);
            if (pErrorBuf)
                Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
            else
                Msg("Can't compile shader hr=0x%08x", _result);
        }
    }

    return _result;
}

static inline bool match_shader(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length)
{
    u32 const full_shader_id_length = xr_strlen(full_shader_id);
    R_ASSERT2(full_shader_id_length == mask_length,
        make_string("bad cache for shader %s, [%s], [%s]", debug_shader_id, mask, full_shader_id));
    char const* i = full_shader_id;
    char const* const e = full_shader_id + full_shader_id_length;
    char const* j = mask;
    for (; i != e; ++i, ++j)
    {
        if (*i == *j)
            continue;

        if (*j == '_')
            continue;

        return false;
    }

    return true;
}

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result)
{
#if 1
	strcpy_s					( result, "" );
	return						false;
#else // #if 1
#ifdef DEBUG
    LPCSTR temp = "";
    bool found = false;
    FS_FileSet::const_iterator i = file_set.begin();
    FS_FileSet::const_iterator const e = file_set.end();
    for (; i != e; ++i)
    {
        if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
        {
            VERIFY(!found);
            found = true;
            temp = (*i).name.c_str();
        }
    }

    xr_strcpy(result, temp);
    return found;
#else // #ifdef DEBUG
    FS_FileSet::const_iterator i = file_set.begin();
    FS_FileSet::const_iterator const e = file_set.end();
    for (; i != e; ++i)
    {
        if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
        {
            xr_strcpy(result, (*i).name.c_str());
            return true;
        }
    }

    return false;
#endif // #ifdef DEBUG
#endif // #if 1
}
