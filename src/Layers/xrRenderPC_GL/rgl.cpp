#include "stdafx.h"
#include "rgl.h"
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
class CGlow : public IRender_Glow
{
public:
    bool bActive;
public:
    CGlow() : bActive(false) { }
    void set_active(bool b) override { bActive = b; }
    bool get_active() override { return bActive; }
    void set_position(const Fvector& /*P*/) override { }
    void set_direction(const Fvector& /*D*/) override { }
    void set_radius(float /*R*/) override { }
    void set_texture(LPCSTR /*name*/) override { }
    void set_color(const Fcolor& /*C*/) override { }
    void set_color(float /*r*/, float /*g*/, float /*b*/) override { }
};

float r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq)
{
    int id = SE_R2_SHADOW;
    if (PHASE_NORMAL == RImplementation.phase)
    {
        id = _sqrt(cdist_sq) - pVisual->vis.sphere.R < r_dtex_range ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}

//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq)
{
    int id = SE_R2_SHADOW;
    if (PHASE_NORMAL == RImplementation.phase)
    {
        id = _sqrt(cdist_sq) - pVisual->vis.sphere.R < r_dtex_range ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}

static class cl_parallax : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float h = ps_r2_df_parallax_h;
        RCache.set_c(C, h, -h / 2.f, 1.f / r_dtex_range, 1.f / r_dtex_range);
    }
} binder_parallax;

static class cl_pos_decompress_params : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
        float HorzTan = - VertTan / Device.fASPECT;

        RCache.set_c(C, HorzTan, VertTan, 2.0f * HorzTan / (float)Device.dwWidth,
                     2.0f * VertTan / (float)Device.dwHeight);
    }
} binder_pos_decompress_params;

static class cl_pos_decompress_params2 : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        RCache.set_c(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth,
                     1.0f / (float)Device.dwHeight);
    }
} binder_pos_decompress_params2;

static class cl_water_intensity : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        CEnvDescriptor& E = *g_pGamePersistent->Environment().CurrentEnv;
        float fValue = E.m_fWaterIntensity;
        RCache.set_c(C, fValue, fValue, fValue, 0);
    }
} binder_water_intensity;

static class cl_sun_shafts_intensity : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        CEnvDescriptor& E = *g_pGamePersistent->Environment().CurrentEnv;
        float fValue = E.m_fSunShaftsIntensity;
        RCache.set_c(C, fValue, fValue, fValue, 0);
    }
} binder_sun_shafts_intensity;

static class cl_alpha_ref : public R_constant_setup
{
    void setup(R_constant* /*C*/) override
    {
        // TODO: OGL: Implement AlphaRef.
        //StateManager.BindAlphaRef(C);
    }
} binder_alpha_ref;

extern ENGINE_API BOOL r2_sun_static;
extern ENGINE_API BOOL r2_advanced_pp; //	advanced post process and effects
//////////////////////////////////////////////////////////////////////////
// Just two static storage
void CRender::create()
{
    Device.seqFrame.Add(this,REG_PRIORITY_HIGH + 0x12345678);

    m_skinning = -1;
    m_MSAASample = -1;

    // hardware
    o.smapsize = ps_r2_smapsize;
    o.mrt = HW.Caps.raster.dwMRT_count >= 3;
    o.mrtmixdepth = HW.Caps.raster.b_MRT_mixdepth;

    // Check for NULL render target support
    //	DX10 disabled
    //D3DFORMAT	nullrt	= (D3DFORMAT)MAKEFOURCC('N','U','L','L');
    //o.nullrt			= HW.support	(nullrt,			D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET);
    o.nullrt = false;
    /*
    if (o.nullrt)		{
    Msg				("* NULLRT supported and used");
    };
    */
    if (o.nullrt)
    {
        Msg("* NULLRT supported");

        //.	    _tzset			();
        //.		??? _strdate	( date, 128 );	???
        //.		??? if (date < 22-march-07)
        if (false)
        {
            u32 device_id = HW.Caps.id_device;
            bool disable_nullrt = false;
            switch (device_id)
            {
            case 0x190:
            case 0x191:
            case 0x192:
            case 0x193:
            case 0x194:
            case 0x197:
            case 0x19D:
            case 0x19E:
            {
                disable_nullrt = true; //G80
                break;
            }
            case 0x400:
            case 0x401:
            case 0x402:
            case 0x403:
            case 0x404:
            case 0x405:
            case 0x40E:
            case 0x40F:
            {
                disable_nullrt = true; //G84
                break;
            }
            case 0x420:
            case 0x421:
            case 0x422:
            case 0x423:
            case 0x424:
            case 0x42D:
            case 0x42E:
            case 0x42F:
            {
                disable_nullrt = true; // G86
                break;
            }
            }
            if (disable_nullrt) o.nullrt = false;
        }
        if (o.nullrt) Msg("* ...and used");
    }


    // SMAP / DST
    o.HW_smap_FETCH4 = FALSE;
    o.HW_smap = true;
    o.HW_smap_PCF = o.HW_smap;
    o.HW_smap_FORMAT = D3DFMT_D24X8;

    o.fp16_filter = true;
    o.fp16_blend = true;

    VERIFY2 (o.mrt && (HW.Caps.raster.dwInstructions>=256),"Hardware doesn't meet minimum feature-level");
    if (o.mrtmixdepth) o.albedo_wo = FALSE;
    else if (o.fp16_blend) o.albedo_wo = FALSE;
    else o.albedo_wo = TRUE;

    // nvstencil on NV40 and up
    o.nvstencil = FALSE;
    //if ((HW.Caps.id_vendor==0x10DE)&&(HW.Caps.id_device>=0x40))	o.nvstencil = TRUE;
    if (strstr(Core.Params, "-nonvs")) o.nvstencil = FALSE;

    // nv-dbt
    //	DX10 disabled
    //o.nvdbt				= HW.support	((D3DFORMAT)MAKEFOURCC('N','V','D','B'), D3DRTYPE_SURFACE, 0);
    o.nvdbt = false;
    if (o.nvdbt) Msg("* NV-DBT supported and used");

    // options (smap-pool-size)
    if (strstr(Core.Params, "-smap1536")) o.smapsize = 1536;
    if (strstr(Core.Params, "-smap2048")) o.smapsize = 2048;
    if (strstr(Core.Params, "-smap2560")) o.smapsize = 2560;
    if (strstr(Core.Params, "-smap3072")) o.smapsize = 3072;
    if (strstr(Core.Params, "-smap4096")) o.smapsize = 4096;

    // gloss
    char* g = strstr(Core.Params, "-gloss ");
    o.forcegloss = g ? TRUE : FALSE;
    if (g)
    {
        o.forcegloss_v = float(atoi(g + xr_strlen("-gloss "))) / 255.f;
    }

    // options
    o.bug = strstr(Core.Params, "-bug") ? TRUE : FALSE;
    o.sunfilter = strstr(Core.Params, "-sunfilter") ? TRUE : FALSE;
    //.	o.sunstatic			= (strstr(Core.Params,"-sunstatic"))?	TRUE	:FALSE	;
    o.sunstatic = r2_sun_static;
    o.advancedpp = r2_advanced_pp;
    o.volumetricfog = ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE);
    o.sjitter = strstr(Core.Params, "-sjitter") ? TRUE : FALSE;
    o.depth16 = strstr(Core.Params, "-depth16") ? TRUE : FALSE;
    o.noshadows = strstr(Core.Params, "-noshadows") ? TRUE : FALSE;
    o.Tshadows = strstr(Core.Params, "-tsh") ? TRUE : FALSE;
    o.mblur = strstr(Core.Params, "-mblur") ? TRUE : FALSE;
    o.distortion_enabled = strstr(Core.Params, "-nodistort") ? FALSE : TRUE;
    o.distortion = o.distortion_enabled;
    o.disasm = strstr(Core.Params, "-disasm") ? TRUE : FALSE;
    o.forceskinw = strstr(Core.Params, "-skinw") ? TRUE : FALSE;

    o.ssao_blur_on = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) && ps_r_ssao != 0;
    o.ssao_opt_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_OPT_DATA) && ps_r_ssao != 0;
    o.ssao_half_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA) && o.ssao_opt_data && ps_r_ssao != 0;
    o.ssao_hdao = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HDAO) && ps_r_ssao != 0;
    o.ssao_hbao = !o.ssao_hdao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HBAO) && ps_r_ssao != 0;

    //	TODO: fix hbao shader to allow to perform per-subsample effect!
    o.hbao_vectorized = false;
    if (o.ssao_hdao)
        o.ssao_opt_data = false;
    else if (o.ssao_hbao)
    {
        if (HW.Caps.id_vendor == 0x1002)
            o.hbao_vectorized = true;
        o.ssao_opt_data = true;
    }

    o.dx10_sm4_1 = true;

    //	MSAA option dependencies
    o.dx10_msaa = !!ps_r3_msaa;
    o.dx10_msaa_samples = 1 << ps_r3_msaa;

    o.dx10_msaa_opt = o.dx10_msaa;

    o.dx10_msaa_hybrid = false;

    //	Allow alpha test MSAA for DX10.0

    //o.dx10_msaa_alphatest= ps_r2_ls_flags.test((u32)R3FLAG_MSAA_ALPHATEST);
    //o.dx10_msaa_alphatest= o.dx10_msaa_alphatest && o.dx10_msaa;

    //o.dx10_msaa_alphatest_atoc= (o.dx10_msaa_alphatest && !o.dx10_msaa_opt && !o.dx10_msaa_hybrid);

    o.dx10_msaa_alphatest = 0;
    if (o.dx10_msaa)
    {
        if (o.dx10_msaa_opt || o.dx10_msaa_hybrid)
        {
            if (ps_r3_msaa_atest == 1)
                o.dx10_msaa_alphatest = MSAA_ATEST_DX10_1_ATOC;
            else if (ps_r3_msaa_atest == 2)
                o.dx10_msaa_alphatest = MSAA_ATEST_DX10_1_NATIVE;
        }
        else
        {
            if (ps_r3_msaa_atest)
                o.dx10_msaa_alphatest = MSAA_ATEST_DX10_0_ATOC;
        }
    }

    o.dx10_gbuffer_opt = ps_r2_ls_flags.test(R3FLAG_GBUFFER_OPT);

    o.dx10_minmax_sm = ps_r3_minmax_sm;
    o.dx10_minmax_sm_screenarea_threshold = 1600 * 1200;

    if (o.dx10_minmax_sm == MMSM_AUTODETECT)
    {
        o.dx10_minmax_sm = MMSM_OFF;

        //	AMD device
        if (HW.Caps.id_vendor == 0x1002)
        {
            if (ps_r_sun_quality >= 3)
                o.dx10_minmax_sm = MMSM_AUTO;
            else if (ps_r_sun_shafts >= 2)
            {
                o.dx10_minmax_sm = MMSM_AUTODETECT;
                //	Check resolution in runtime in use_minmax_sm_this_frame
                o.dx10_minmax_sm_screenarea_threshold = 1600 * 1200;
            }
        }

        //	NVidia boards
        if (HW.Caps.id_vendor == 0x10DE)
        {
            if (ps_r_sun_shafts >= 2)
            {
                o.dx10_minmax_sm = MMSM_AUTODETECT;
                //	Check resolution in runtime in use_minmax_sm_this_frame
                o.dx10_minmax_sm_screenarea_threshold = 1280 * 1024;
            }
        }
    }

    // constants
    RImplementation.Resources->RegisterConstantSetup("parallax", &binder_parallax);
    RImplementation.Resources->RegisterConstantSetup("water_intensity", &binder_water_intensity);
    RImplementation.Resources->RegisterConstantSetup("sun_shafts_intensity", &binder_sun_shafts_intensity);
    RImplementation.Resources->RegisterConstantSetup("m_AlphaRef", &binder_alpha_ref);
    RImplementation.Resources->RegisterConstantSetup("pos_decompression_params", &binder_pos_decompress_params);
    RImplementation.Resources->RegisterConstantSetup("pos_decompression_params2", &binder_pos_decompress_params2);

    c_lmaterial = "L_material";
    c_sbase = "s_base";

    m_bMakeAsyncSS = false;

    Target = new CRenderTarget(); // Main target

    Models = new CModelPool();
    PSLibrary.OnCreate();
    HWOCC.occq_create(occq_size);

    rmNormal();
    marker = 0;
    // TODO: OGL: Implement SLI/Crossfire support.
    //R_CHK						(HW.pDevice->CreateQuery(&qdesc,&q_sync_point[0]));
    //R_CHK						(HW.pDevice->CreateQuery(&qdesc,&q_sync_point[1]));
    //	Prevent error on first get data
    //q_sync_point[0]->End();
    //q_sync_point[1]->End();
    //R_CHK						(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[0]));
    //R_CHK						(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[1]));

    PortalTraverser.initialize();
    //	TODO: OGL: Implement FluidManager.
    //	FluidManager.Initialize( 70, 70, 70 );
    //	FluidManager.Initialize( 100, 100, 100 );
    //	FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);
}

void CRender::destroy()
{
    m_bMakeAsyncSS = false;
    //FluidManager.Destroy();
    PortalTraverser.destroy();
    //_RELEASE					(q_sync_point[1]);
    //_RELEASE					(q_sync_point[0]);
    //for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
    //	_RELEASE				(q_sync_point[i]);

    HWOCC.occq_destroy();
    xr_delete(Models);
    xr_delete(Target);
    PSLibrary.OnDestroy();
    Device.seqFrame.Remove(this);
    r_dsgraph_destroy();
}

void CRender::reset_begin()
{
    // Update incremental shadowmap-visibility solver
    // BUG-ID: 10646
    {
        u32 it = 0;
        for (it = 0; it < Lights_LastFrame.size(); it++)
        {
            if (nullptr == Lights_LastFrame[it]) continue ;
            try
            {
                Lights_LastFrame[it]->svis.resetoccq();
            }
            catch (...)
            {
                Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)&Lights_LastFrame[it]);
            }
        }
        Lights_LastFrame.clear();
    }

    xr_delete(Target);
    HWOCC.occq_destroy();
    //_RELEASE					(q_sync_point[1]);
    //_RELEASE					(q_sync_point[0]);
    //for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
    //	_RELEASE				(q_sync_point[i]);
}

void CRender::reset_end()
{
    //for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
    //	R_CHK(HW.pDevice->CreateQuery(&qdesc,&q_sync_point[i]));
    HWOCC.occq_create(occq_size);

    Target = new CRenderTarget();

    //FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);

    // Set this flag true to skip the first render frame,
    // that some data is not ready in the first frame (for example device camera position)
    m_bFirstFrameAfterReset = true;
}

/*
void CRender::OnFrame()
{
    Models->DeleteQueue			();
    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))	{
        Device.seqParallel.insert	(Device.seqParallel.begin(),
            fastdelegate::FastDelegate0<>(&HOM,&CHOM::MT_RENDER));
    }
}*/
void CRender::OnFrame()
{
    Models->DeleteQueue();
    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
    {
        // MT-details (@front)
        Device.seqParallel.insert(Device.seqParallel.begin(),
                                  fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));

        // MT-HOM (@front)
        Device.seqParallel.insert(Device.seqParallel.begin(),
                                  fastdelegate::FastDelegate0<>(&HOM, &CHOM::MT_RENDER));
    }
}

void CRender::BeforeWorldRender() {}
void CRender::AfterWorldRender() {}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent) { return new CROS_impl(); }
void CRender::ros_destroy(IRender_ObjectSpecific* & p) { xr_delete(p); }
IRenderVisual* CRender::model_Create(LPCSTR name, IReader* data) { return Models->Create(name, data); }
IRenderVisual* CRender::model_CreateChild(LPCSTR name, IReader* data) { return Models->CreateChild(name, data); }
IRenderVisual* CRender::model_Duplicate(IRenderVisual* V) { return Models->Instance_Duplicate((dxRender_Visual*)V); }

void CRender::model_Delete(IRenderVisual* & V, BOOL bDiscard)
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

void CRender::model_Delete(IRender_DetailModel* & F)
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
    R_ASSERT3(SE,"Particle effect doesn't exist",name);
    return Models->CreatePE(SE);
}

IRenderVisual* CRender::model_CreateParticles(LPCSTR name)
{
    PS::CPEDef* SE = PSLibrary.FindPED(name);
    if (SE) return Models->CreatePE(SE);
    PS::CPGDef* SG = PSLibrary.FindPGD(name);
    R_ASSERT3(SG,"Particle effect or group doesn't exist",name);
    return Models->CreatePG(SG);
}

void CRender::models_Prefetch() { Models->Prefetch(); }
void CRender::models_Clear(BOOL b_complete) { Models->ClearPool(b_complete); }

ref_shader CRender::getShader(int id)
{
    VERIFY(id<int(Shaders.size()));
    return Shaders[id];
}

IRender_Portal* CRender::getPortal(int id)
{
    VERIFY(id<int(Portals.size()));
    return Portals[id];
}

IRender_Sector* CRender::getSector(int id)
{
    VERIFY(id<int(Sectors.size()));
    return Sectors[id];
}

IRender_Sector* CRender::getSectorActive() { return pLastSector; }

IRenderVisual* CRender::getVisual(int id)
{
    VERIFY(id<int(Visuals.size()));
    return Visuals[id];
}

D3DVERTEXELEMENT9* CRender::getVB_Format(int id, BOOL _alt)
{
    if (_alt)
    {
        VERIFY(id<int(xDC.size()));
        return xDC[id].begin();
    }
    VERIFY(id<int(nDC.size()));
    return nDC[id].begin();
}

GLuint CRender::getVB(int id, BOOL _alt)
{
    if (_alt)
    {
        VERIFY(id<int(xVB.size()));
        return xVB[id];
    }
    VERIFY(id<int(nVB.size()));
    return nVB[id];
}

GLuint CRender::getIB(int id, BOOL _alt)
{
    if (_alt)
    {
        VERIFY(id<int(xIB.size()));
        return xIB[id];
    }
    VERIFY(id<int(nIB.size()));
    return nIB[id];
}

FSlideWindowItem* CRender::getSWI(int id)
{
    VERIFY(id<int(SWIs.size()));
    return &SWIs[id];
}

IRender_Target* CRender::getTarget() { return Target; }

IRender_Light* CRender::light_create() { return Lights.Create(); }
IRender_Glow* CRender::glow_create() { return new CGlow(); }

void CRender::flush() { r_dsgraph_render_graph(0); }

BOOL CRender::occ_visible(vis_data& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(sPoly& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(Fbox& P) { return HOM.visible(P); }

void CRender::add_Visual(IRenderVisual* V) { add_leafs_Dynamic((dxRender_Visual*)V); }
void CRender::add_Geometry(IRenderVisual* V) { add_Static((dxRender_Visual*)V, View->getMask()); }

void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
    VERIFY2 (T, "Invalid static wallmark triangle");
    if (T->suppress_wm) return;
    VERIFY2 (_valid(P) && _valid(s) && verts && (s>EPS_L), "Invalid static wallmark params");
    Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    dxWallMarkArray* pWMA = (dxWallMarkArray *)pArray;
    ref_shader* pShader = pWMA->dxGenerateWallmark();
    if (pShader) add_StaticWallmark(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    dxUIShader* pShader = (dxUIShader*)&*S;
    add_StaticWallmark(pShader->hShader, P, s, T, V);
}

void CRender::clear_static_wallmarks()
{
    Wallmarks->clear();
}

void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm)
{
    Wallmarks->AddSkeletonWallmark(wm);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
                                   const Fvector& dir, float size)
{
    Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
                                   const Fvector& dir, float size)
{
    dxWallMarkArray* pWMA = (dxWallMarkArray *)pArray;
    ref_shader* pShader = pWMA->dxGenerateWallmark();
    if (pShader) add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}

void CRender::add_Occluder(Fbox2& bb_screenspace)
{
    HOM.occlude(bb_screenspace);
}

void CRender::set_Object(IRenderable* O)
{
    val_pObject = O;
}

void CRender::rmNear()
{
    IRender_Target* T = getTarget();
    CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
    CHK_GL(glDepthRangef(0.f, 0.02f));
    //CHK_DX				(HW.pDevice->SetViewport(&VP));
}

void CRender::rmFar()
{
    IRender_Target* T = getTarget();
    CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
    CHK_GL(glDepthRangef(0.99999f, 1.f));
    //CHK_DX				(HW.pDevice->SetViewport(&VP));
}

void CRender::rmNormal()
{
    IRender_Target* T = getTarget();
    CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
    CHK_GL(glDepthRangef(0.f, 1.f));
    //CHK_DX				(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender()
    : m_bFirstFrameAfterReset(false)
{
#if defined(WINDOWS) // remove this after port r2_R_sun.cpp
    init_cacades();
#endif
}

CRender::~CRender() {}

void CRender::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    D3DXRenderBase::DumpStatistics(font, alert);
    Stats.FrameEnd();
    font.OutNext("Lights:");
    font.OutNext("- total:      %u", Stats.l_total);
    font.OutNext("- visible:    %u", Stats.l_visible);
    font.OutNext("- shadowed:   %u", Stats.l_shadowed);
    font.OutNext("- unshadowed: %u", Stats.l_unshadowed);
    font.OutNext("Shadow maps:");
    font.OutNext("- used:       %d", Stats.s_used);
    font.OutNext("- merged:     %d", Stats.s_merged - Stats.s_used);
    font.OutNext("- finalclip:  %d", Stats.s_finalclip);
    u32 ict = Stats.ic_total + Stats.ic_culled;
    font.OutNext("ICULL:        %03.1f", 100.f * f32(Stats.ic_culled) / f32(ict ? ict : 1));
    font.OutNext("- visible:    %u", Stats.ic_total);
    font.OutNext("- culled:     %u", Stats.ic_culled);
    Stats.FrameStart();
    HOM.DumpStatistics(font, alert);
}
