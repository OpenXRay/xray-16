#include "stdafx.h"

#include "xrCore/Threading/TaskManager.hpp"

#include "FStaticRender.h"

#include "xrEngine/xr_object.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/LightTrack.h"
#include "Layers/xrRender/dxWallMarkArray.h"
#include "Layers/xrRender/dxUIShader.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

CRender RImplementation;

//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(dxRender_Visual* pVisual, float /*cdist_sq*/, u32 phase)
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
ShaderElement* CRender::rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq, u32 phase)
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

void CRender::OnDeviceCreate(pcstr shName)
{
    o.new_shader_support = 0;
    D3DXRenderBase::OnDeviceCreate(shName);
}

//////////////////////////////////////////////////////////////////////////
void CRender::create()
{
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

    // Fixed-function pipeline
    o.ffp = HW.Caps.hasFixedPipeline && ps_r1_flags.test(R1FLAG_FFP);

    // disasm
    o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
    o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;
    o.no_detail_textures = !ps_r2_ls_flags.test(R1FLAG_DETAIL_TEXTURES);

    m_bMakeAsyncSS = false;

    Target = xr_new<CRenderTarget>(); // Main target

    Models = xr_new<CModelPool>();
    L_Dynamic = xr_new<CLightR_Manager>();
    PSLibrary.OnCreate();
    //.	HWOCC.occq_create			(occq_size);
}

void CRender::destroy()
{
    m_bMakeAsyncSS = false;
    //.	HWOCC.occq_destroy			();
    PSLibrary.OnDestroy();

    xr_delete(L_Dynamic);
    xr_delete(Models);

    //*** Components
    xr_delete(Target);
    Device.seqFrame.Remove(this);
}

void CRender::reset_begin()
{
    Resources->reset_begin();

    //AVO: let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size ||
        !fsimilar(ps_r__Detail_density, ps_current_detail_density) ||
        !fsimilar(ps_r__Detail_height, ps_current_detail_height)))
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
    Target = xr_new<CRenderTarget>();
    if (L_Projector)
        L_Projector->invalidate();

    // let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size ||
        !fsimilar(ps_r__Detail_density, ps_current_detail_density) ||
        !fsimilar(ps_r__Detail_height, ps_current_detail_height)))
    {
        Details = xr_new<CDetailManager>();
        Details->Load();
    }

    // Set this flag true to skip the first render frame,
    // that some data is not ready in the first frame (for example device camera position)
    m_bFirstFrameAfterReset = true;
}

void CRender::BeforeRender()
{
    if (IGame_Persistent::MainMenuActiveOrLevelNotExist())
        return;

    ProcessHOMTask = &TaskScheduler->AddTask("MT-HOM", { &HOM, &CHOM::MT_RENDER });
}

void CRender::OnFrame()
{
    Models->DeleteQueue();
    if (IGame_Persistent::MainMenuActiveOrLevelNotExist())
        return;
    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
    {
        // MT-details (@front)
        Device.seqParallel.insert(
            Device.seqParallel.begin(), fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));
    }
}

// Перед началом рендера мира --#SM+#--
void CRender::BeforeWorldRender() {}

// После рендера мира и пост-эффектов --#SM+#--
void CRender::AfterWorldRender() {}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent) { return xr_new<CROS_impl>(); }
void CRender::ros_destroy(IRender_ObjectSpecific*& p) { xr_delete(p); }
IRenderVisual* CRender::model_Create(LPCSTR name, IReader* data) { return Models->Create(name, data); }
IRenderVisual* CRender::model_CreateChild(LPCSTR name, IReader* data) { return Models->CreateChild(name, data); }
IRenderVisual* CRender::model_Duplicate(IRenderVisual* V) { return Models->Instance_Duplicate((dxRender_Visual*)V); }
void CRender::model_Delete(IRenderVisual*& V, bool bDiscard)
{
    dxRender_Visual* pVisual = (dxRender_Visual*)V;
    Models->Delete(pVisual, bDiscard);
    V = nullptr;
}
IRender_DetailModel* CRender::model_CreateDM(IReader* F)
{
    CDetail* D = xr_new<CDetail>();
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
void CRender::models_Clear(bool b_complete) { Models->ClearPool(b_complete); }
ref_shader CRender::getShader(int id)
{
    VERIFY(id < int(Shaders.size()));
    return Shaders[id];
}
IRenderVisual* CRender::getVisual(int id)
{
    VERIFY(id < int(Visuals.size()));
    return Visuals[id];
}
VertexElement* CRender::getVB_Format(int id, bool alternative)
{
    if (alternative)
    {
        VERIFY(id < int(xDC.size()));
        return xDC[id].begin();
    }
    else
    {
        VERIFY(id < int(nDC.size()));
        return nDC[id].begin();
    }
}
VertexStagingBuffer* CRender::getVB(int id, bool alternative)
{
    if (alternative)
    {
        VERIFY(id < int(xVB.size()));
        return &xVB[id];
    }
    else
    {
        VERIFY(id < int(nVB.size()));
        return &nVB[id];
    }
}
IndexStagingBuffer* CRender::getIB(int id, bool alternative)
{
    if (alternative)
    {
        VERIFY(id < int(xIB.size()));
        return &xIB[id];
    }
    else
    {
        VERIFY(id < int(nIB.size()));
        return &nIB[id];
    }
}
FSlideWindowItem* CRender::getSWI(int id)
{
    VERIFY(id < int(SWIs.size()));
    return &SWIs[id];
}
IRender_Target* CRender::getTarget() { return Target; }
IRender_Light* CRender::light_create() { return Lights.Create(); }
IRender_Glow* CRender::glow_create() { return xr_new<CGlow>(); }
bool CRender::occ_visible(vis_data& P) { return HOM.visible(P); }
bool CRender::occ_visible(sPoly& P) { return HOM.visible(P); }
bool CRender::occ_visible(Fbox& P) { return HOM.visible(P); }
void CRender::add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m)
{
    // TODO: this whole function should be replaced by a list of renderables+xforms returned from `renderable_Render` call
    auto& dsgraph = get_context(context_id);
    set_Object(root, dsgraph.o.phase);
    dsgraph.add_leafs_dynamic(root, (dxRender_Visual*)V, m);
}
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

#include "xrEngine/PS_instance.h"
void CRender::set_Object(IRenderable* O, u32 phase)
{
    if (O)
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

static u32 gm_Ambient = 0;
IC void gm_SetAmbient(u32 C)
{
    if (C != gm_Ambient)
    {
        gm_Ambient = C;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_AMBIENT, color_xrgb(C, C, C)));
    }
}

void CRender::apply_object(CBackend& cmd_list, IRenderable* O)
{
    if (nullptr == O)
        return;
    if (O->renderable_ROS())
    {
        CROS_impl& LT = *((CROS_impl*)O->GetRenderData().pROS);
        VERIFY(dynamic_cast<IGameObject*>(O) || dynamic_cast<CPS_Instance*>(O));
        VERIFY(dynamic_cast<CROS_impl*>(O->GetRenderData().pROS));
        float o_hemi = 0.5f * LT.get_hemi();
        float o_sun = 0.5f * LT.get_sun();
        RCache.set_c(c_ldynamic_props, o_sun, o_sun, o_sun, o_hemi);
        // shadowing
        if ((LT.shadow_recv_frame == Device.dwFrame) && O->renderable_ShadowReceive())
        {
            gm_SetAmbient(0);
            RImplementation.L_Projector->setup(LT.shadow_recv_slot);
        }
        else
        {
            //gm_SetAmbient(iFloor(LT.ambient) / 2);
        }

        // ambience
        //gm_SetAmbient(iFloor(LT.ambient) / 2);

        // set up to 8 lights to device
        const int max = _min(int(LT.lights.size()), HW.Caps.max_ffp_lights);
        for (int L = 0; L < max; L++)
        {
            CHK_DX(HW.pDevice->SetLight(L, (D3DLIGHT9*)&LT.lights[L].source->ldata));
        }

        // enable them, disable others
        static int gm_Lcount = 0;
        for (int L = gm_Lcount; L < max; L++)
        {
            CHK_DX(HW.pDevice->LightEnable(L, TRUE));
        }
        for (int L = max; L < gm_Lcount; L++)
        {
            CHK_DX(HW.pDevice->LightEnable(L, FALSE));
        }
        gm_Lcount = max;
    }
}

// Misc
float g_fSCREEN;

void CRender::rmNear(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0, 0.02f });
}

void CRender::rmFar(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0.99999f, 1.f });
}

void CRender::rmNormal(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0, 1.f });
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

void CRender::Calculate()
{
#ifdef _GPA_ENABLED
    TAL_SCOPED_TASK_NAMED("CRender::Calculate()");
#endif // _GPA_ENABLED

    BasicStats.Culling.Begin();

    // Transfer to global space to avoid deep pointer access
    IRender_Target* T = getTarget();
    float fov_factor = _sqr(90.f / Device.fFOV);
    g_fSCREEN = float(T->get_width(RCache) * T->get_height(RCache)) * fov_factor * (EPS_S + ps_r__LOD);
    r_ssaDISCARD = _sqr(ps_r__ssaDISCARD) / g_fSCREEN;
    r_ssaDONTSORT = _sqr(ps_r__ssaDONTSORT / 3) / g_fSCREEN;
    r_ssaLOD_A = _sqr(ps_r1_ssaLOD_A / 3) / g_fSCREEN;
    r_ssaLOD_B = _sqr(ps_r1_ssaLOD_B / 3) / g_fSCREEN;
    r_ssaGLOD_start = _sqr(ps_r__GLOD_ssa_start / 3) / g_fSCREEN;
    r_ssaGLOD_end = _sqr(ps_r__GLOD_ssa_end / 3) / g_fSCREEN;
    r_ssaHZBvsTEX = _sqr(ps_r__ssaHZBvsTEX / 3) / g_fSCREEN;

    // Frustum
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    // Build L_DB visibility & perform basic initialization
    gm_Ambient = 0xFFFFFFFF;
    gm_SetAmbient(0);

    if (!ps_r1_flags.is_any(R1FLAG_FFP_LIGHTMAPS | R1FLAG_DLIGHTS))
        HW.pDevice->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF);
    else
        HW.pDevice->SetRenderState(D3DRS_AMBIENT, 0x00000000);

    rmNormal(RCache);
    auto& dsgraph = get_imm_context();
    dsgraph.o.use_hom = true;
    dsgraph.o.phase = PHASE_NORMAL;

    // Detect camera-sector
    if (!vLastCameraPos.similar(Device.vCameraPosition, EPS_S))
    {
        const auto sector_id = dsgraph.detect_sector(Device.vCameraPosition);
        if (sector_id != IRender_Sector::INVALID_SECTOR_ID)
        {
            if (sector_id != last_sector_id)
                g_pGamePersistent->OnSectorChanged(sector_id);

            last_sector_id = sector_id;
        }
        vLastCameraPos.set(Device.vCameraPosition);

        // Check if camera is too near to some portal - if so force DualRender
        if (rmPortals)
        {
            Fvector box_radius;
            box_radius.set(EPS_L * 2, EPS_L * 2, EPS_L * 2);
            dsgraph.Sectors_xrc.box_query(CDB::OPT_FULL_TEST, rmPortals, Device.vCameraPosition, box_radius);
            for (size_t K = 0; K < dsgraph.Sectors_xrc.r_count(); K++)
            {
                CPortal* pPortal = dsgraph.Portals[rmPortals->get_tris()[dsgraph.Sectors_xrc.r_begin()[K].id].dummy];
                pPortal->bDualRender = TRUE;
            }
        }
    }

    //
    Lights.Update();

    // Main process
    dsgraph.marker++;
    set_Object(nullptr, dsgraph.o.phase);
    TaskScheduler->Wait(*ProcessHOMTask);
    if (last_sector_id != IRender_Sector::INVALID_SECTOR_ID)
    {
        // Traverse sector/portal structure
        dsgraph.PortalTraverser.traverse(dsgraph.Sectors[last_sector_id], ViewBase, Device.vCameraPosition,
            Device.mFullTransform,
            CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);

        // Determine visibility for static geometry hierarchy
        if (psDeviceFlags.test(rsDrawStatic))
        {
            for (u32 s_it = 0; s_it < dsgraph.PortalTraverser.r_sectors.size(); s_it++)
            {
                CSector* sector = (CSector*)dsgraph.PortalTraverser.r_sectors[s_it];
                dxRender_Visual* root = sector->root();
                for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
                {
                    dsgraph.add_static(root, sector->r_frustums[v_it], sector->r_frustums[v_it].getMask());
                }
            }
        }

        // Traverse object database
        if (psDeviceFlags.test(rsDrawDynamic))
        {
            g_pGamePersistent->SpatialSpace.q_frustum(
                dsgraph.lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE, ViewBase);

            // Exact sorting order (front-to-back)
            std::sort(dsgraph.lstRenderables.begin(), dsgraph.lstRenderables.end(), [](ISpatial* s1, ISpatial* s2)
            {
                const float d1 = s1->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
                const float d2 = s2->GetSpatialData().sphere.P.distance_to_sqr(Device.vCameraPosition);
                return d1 < d2;
            });

            if (ps_r__common_flags.test(RFLAG_ACTOR_SHADOW)) // Actor Shadow (Sun + Light)
                g_pGameLevel->pHUD->Render_First(dsgraph.context_id); // R1 shadows

            g_pGameLevel->pHUD->Render_Last(dsgraph.context_id);

            // Determine visibility for dynamic part of scene
            u32 uID_LTRACK = 0xffffffff;
            if (dsgraph.o.phase == PHASE_NORMAL)
            {
                uLastLTRACK++;
                if (dsgraph.lstRenderables.size())
                    uID_LTRACK = uLastLTRACK % dsgraph.lstRenderables.size();

                // update light-vis for current entity / actor
                IGameObject* O = g_pGameLevel->CurrentViewEntity();
                if (O)
                {
                    CROS_impl* R = (CROS_impl*)O->ROS();
                    if (R)
                        R->update(O);
                }
            }
            for (u32 o_it = 0; o_it < dsgraph.lstRenderables.size(); o_it++)
            {
                ISpatial* spatial = dsgraph.lstRenderables[o_it];
                const auto& entity_pos = spatial->spatial_sector_point();
                spatial->spatial_updatesector(dsgraph.detect_sector(entity_pos));
                const auto sector_id = spatial->GetSpatialData().sector_id;
                if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                    continue; // disassociated from S/P structure
                CSector* sector = dsgraph.Sectors[sector_id];

                // Filter only not light spatial
                if (dsgraph.PortalTraverser.i_marker != sector->r_marker && (spatial->GetSpatialData().type & STYPE_RENDERABLE))
                    continue; // inactive (untouched) sector

                if (spatial->GetSpatialData().type & STYPE_RENDERABLE)
                {
                    for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
                    {
                        const CFrustum& view = sector->r_frustums[v_it];

                        if (!view.testSphere_dirty(spatial->GetSpatialData().sphere.P,
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
                            memcpy(v_orig.marker, v_copy.marker, sizeof(v_copy.marker));
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
                            renderable->renderable_Render(dsgraph.context_id, renderable);
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
                        if (L->spatial.sector_id != IRender_Sector::INVALID_SECTOR_ID)
                        {
                            vis_data& vis = L->get_homdata();
                            if (HOM.visible(vis))
                                Lights.add_light(L);
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

    // End calc
    BasicStats.Culling.End();
}

void CRender::RenderMenu()
{
    Target->Begin();

    if (g_pGamePersistent)
        g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

    // find if distortion is needed at all
    const bool bPerform = Target->Perform();
    const bool _menu_pp = o.distortion && (g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false);
    if (bPerform || _menu_pp)
    {
        Target->phase_distortion();

        if (g_pGamePersistent)
            g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

        // combination/postprocess
        Target->phase_combine(_menu_pp, false);
    }
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
    auto& dsgraph = get_imm_context();
    dsgraph.o.phase = PHASE_NORMAL;
    dsgraph.render_hud(); // hud
    dsgraph.render_graph(0); // normal level
    if (Details)
        Details->Render(RCache); // grass / details
    dsgraph.render_lods(true, false); // lods - FB

    g_pGamePersistent->Environment().RenderSky(); // sky / sun
    g_pGamePersistent->Environment().RenderClouds(); // clouds

    dsgraph.r_pmask(true, false); // disable priority "1"
    o.vis_intersect = TRUE;
    dsgraph.o.use_hom = false;
    L_Dynamic->render(0); // additional light sources
    if (Wallmarks)
    {
        g_r = 0;
        Wallmarks->Render(); // wallmarks has priority as normal geometry
    }
    dsgraph.o.use_hom = true;
    o.vis_intersect = FALSE;
    dsgraph.o.phase = PHASE_NORMAL;
    dsgraph.r_pmask(true, true); // enable priority "0" and "1"
    BasicStats.ShadowsRender.Begin();
    if (L_Shadows)
        L_Shadows->render(); // ... and shadows
    BasicStats.ShadowsRender.End();
    dsgraph.render_lods(false, true); // lods - FB
    dsgraph.render_graph(1); // normal level, secondary priority
    L_Dynamic->render(1); // additional light sources, secondary priority
    dsgraph.PortalTraverser.fade_render(); // faded-portals
    dsgraph.render_sorted(); // strict-sorted geoms
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
            R_ASSERT(dsgraph.mapNormalPasses[_priority][iPass].size() == 0);
            R_ASSERT(dsgraph.mapMatrixPasses[_priority][iPass].size() == 0);
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

void CRender::ApplyBlur2(FVF::TL2uv* pv, u32 size) const
{
    const float dim = float(size);
    Fvector2 shift, p0, p1, a0, a1, b0, b1, c0, c1, d0, d1;
    p0.set(.5f / dim, .5f / dim);
    p1.set((dim + .5f) / dim, (dim + .5f) / dim);
    shift.set(.5f / dim, .5f / dim);
    a0.add(p0, shift);
    a1.add(p1, shift);
    b0.sub(p0, shift);
    b1.sub(p1, shift);
    shift.set(.5f / dim, -.5f / dim);
    c0.add(p0, shift);
    c1.add(p1, shift);
    d0.sub(p0, shift);
    d1.sub(p1, shift);

    constexpr u32 C = 0xffffffff;

    // Fill VB
    pv->set(0.f, dim, C, a0.x, a1.y, b0.x, b1.y);
    pv++;
    pv->set(0.f, 0.f, C, a0.x, a0.y, b0.x, b0.y);
    pv++;
    pv->set(dim, dim, C, a1.x, a1.y, b1.x, b1.y);
    pv++;
    pv->set(dim, 0.f, C, a1.x, a0.y, b1.x, b0.y);
    pv++;

    pv->set(0.f, dim, C, c0.x, c1.y, d0.x, d1.y);
    pv++;
    pv->set(0.f, 0.f, C, c0.x, c0.y, d0.x, d0.y);
    pv++;
    pv->set(dim, dim, C, c1.x, c1.y, d1.x, d1.y);
    pv++;
    pv->set(dim, 0.f, C, c1.x, c0.y, d1.x, d0.y);
    pv++;
}

void CRender::ApplyBlur4(FVF::TL4uv* pv, u32 w, u32 h, float k) const
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
