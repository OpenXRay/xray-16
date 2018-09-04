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
    if (T->suppress_wm) return;
    VERIFY2 (_valid(P) && _valid(s) && T && verts && (s>EPS_L), "Invalid static wallmark params");
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
    init_cacades();
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

void CRender::addShaderOption(const char* name, const char* value)
{
    m_ShaderOptions += "#define ";
    m_ShaderOptions += name;
    m_ShaderOptions += " ";
    m_ShaderOptions += value;
    m_ShaderOptions += "\n";
}

static inline bool match_shader_id(LPCSTR const debug_shader_id, LPCSTR const full_shader_id,
                                   FS_FileSet const& file_set, string_path& result);

/////////

static inline void load_includes(LPCSTR pSrcData, UINT SrcDataLen, xr_vector<char*>& source, xr_vector<char*>& includes)
{
    // Copy source file data into a null-terminated buffer
    char* srcData = xr_alloc<char>(SrcDataLen + 2);
    memcpy(srcData, pSrcData, SrcDataLen);
    srcData[SrcDataLen] = '\n';
    srcData[SrcDataLen + 1] = '\0';
    includes.push_back(srcData);
    source.push_back(srcData);

    string_path path;
    char* str = srcData;
    while (strstr(str, "#include") != nullptr)
    {
        // Get filename of include directive
        str = strstr(str, "#include"); // Find the include directive
        char* fn = strchr(str, '"') + 1; // Get filename, skip quotation
        *str = '\0'; // Terminate previous source
        str = strchr(fn, '"'); // Get end of filename path
        *str = '\0'; // Terminate filename path

        // Create path to included shader
        strconcat(sizeof path, path, GEnv.Render->getShaderPath(), fn);
        FS.update_path(path, "$game_shaders$", path);
        while (char* sep = strchr(path, '/')) *sep = '\\';

        // Open and read file, recursively load includes
        IReader* R = FS.r_open(path);
        R_ASSERT2(R, path);
        load_includes((char*)R->pointer(), R->length(), source, includes);
        FS.r_close(R);

        // Add next source, skip quotation
        str++;
        source.push_back(str);
    }
}

struct SHADER_MACRO
{
    char *Define = "#define ", *Name = "\n", *Sep = "\t", *Definition = "\n", *EOL = "\n";
};

HRESULT CRender::shader_compile(
    LPCSTR name,
    DWORD const* pSrcData,
    UINT SrcDataLen,
    LPCSTR pFunctionName,
    LPCSTR pTarget,
    DWORD Flags,
    void*& result)
{
    xr_vector<char*> source, includes;
    SHADER_MACRO defines[128];
    int def_it = 0;
    char c_smapsize[32];
    char c_gloss[32];
    char c_sun_shafts[32];
    char c_ssao[32];
    char c_sun_quality[32];

    // TODO: OGL: Implement these parameters.
    VERIFY(!pFunctionName);
    VERIFY(!pTarget);
    VERIFY(!Flags);

    // open included files
    load_includes((LPCSTR)pSrcData, SrcDataLen, source, includes);

    char sh_name[MAX_PATH] = "";
    u32 len = 0;
    // options
    {
        xr_sprintf(c_smapsize, "%04d", u32(o.smapsize));
        defines[def_it].Name = "SMAP_size";
        defines[def_it].Definition = c_smapsize;
        def_it++;
        VERIFY(xr_strlen(c_smapsize) == 4);
        xr_strcat(sh_name, c_smapsize);
        len += 4;
    }

    if (o.fp16_filter)
    {
        defines[def_it].Name = "FP16_FILTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.fp16_filter);
    ++len;

    if (o.fp16_blend)
    {
        defines[def_it].Name = "FP16_BLEND";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.fp16_blend);
    ++len;

    if (o.HW_smap)
    {
        defines[def_it].Name = "USE_HWSMAP";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap);
    ++len;

    if (o.HW_smap_PCF)
    {
        defines[def_it].Name = "USE_HWSMAP_PCF";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap_PCF);
    ++len;

    if (o.HW_smap_FETCH4)
    {
        defines[def_it].Name = "USE_FETCH4";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap_FETCH4);
    ++len;

    if (o.sjitter)
    {
        defines[def_it].Name = "USE_SJITTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sjitter);
    ++len;

    if (HW.Caps.raster_major >= 3)
    {
        defines[def_it].Name = "USE_BRANCHING";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(HW.Caps.raster_major >= 3);
    ++len;

    if (HW.Caps.geometry.bVTF)
    {
        defines[def_it].Name = "USE_VTF";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(HW.Caps.geometry.bVTF);
    ++len;

    if (o.Tshadows)
    {
        defines[def_it].Name = "USE_TSHADOWS";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.Tshadows);
    ++len;

    if (o.mblur)
    {
        defines[def_it].Name = "USE_MBLUR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.mblur);
    ++len;

    if (o.sunfilter)
    {
        defines[def_it].Name = "USE_SUNFILTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sunfilter);
    ++len;

    if (o.sunstatic)
    {
        defines[def_it].Name = "USE_R2_STATIC_SUN";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sunstatic);
    ++len;

    if (o.forcegloss)
    {
        xr_sprintf(c_gloss, "%f", o.forcegloss_v);
        defines[def_it].Name = "FORCE_GLOSS";
        defines[def_it].Definition = c_gloss;
        def_it++;
    }
    sh_name[len] = '0' + char(o.forcegloss);
    ++len;

    if (o.forceskinw)
    {
        defines[def_it].Name = "SKIN_COLOR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.forceskinw);
    ++len;

    if (o.ssao_blur_on)
    {
        defines[def_it].Name = "USE_SSAO_BLUR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_blur_on);
    ++len;

    if (o.ssao_hdao)
    {
        defines[def_it].Name = "HDAO";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_hdao);
    ++len;

    if (o.ssao_hbao)
    {
        defines[def_it].Name = "USE_HBAO";
        defines[def_it].Definition = "1";
        def_it++;
        if (o.hbao_vectorized)
        {
            defines[def_it].Name = "VECTORIZED_CODE";
            defines[def_it].Definition = "1";
            def_it++;
        }
    }
    sh_name[len] = '0' + char(o.ssao_hbao);
    ++len;
    sh_name[len] = '0' + char(o.ssao_hbao ? o.hbao_vectorized : 0);
    ++len;

    if (o.ssao_opt_data)
    {
        defines[def_it].Name = "SSAO_OPT_DATA";
        if (o.ssao_half_data)
            defines[def_it].Definition = "2";
        else
            defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_opt_data ? (o.ssao_half_data ? 2 : 1) : 0);
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

    //	Igor: need restart options
    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER))
    {
        defines[def_it].Name = "USE_SOFT_WATER";
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

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES))
    {
        defines[def_it].Name = "USE_SOFT_PARTICLES";
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

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF))
    {
        defines[def_it].Name = "USE_DOF";
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

    if (RImplementation.o.advancedpp && ps_r_sun_shafts)
    {
        xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);
        defines[def_it].Name = "SUN_SHAFTS_QUALITY";
        defines[def_it].Definition = c_sun_shafts;
        def_it++;
        sh_name[len] = '0' + char(ps_r_sun_shafts);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r_ssao)
    {
        xr_sprintf(c_ssao, "%d", ps_r_ssao);
        defines[def_it].Name = "SSAO_QUALITY";
        defines[def_it].Definition = c_ssao;
        def_it++;
        sh_name[len] = '0' + char(ps_r_ssao);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r_sun_quality)
    {
        xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
        defines[def_it].Name = "SUN_QUALITY";
        defines[def_it].Definition = c_sun_quality;
        def_it++;
        sh_name[len] = '0' + char(ps_r_sun_quality);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX))
    {
        defines[def_it].Name = "ALLOW_STEEPPARALLAX";
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

    if (o.dx10_gbuffer_opt)
    {
        defines[def_it].Name = "GBUFFER_OPTIMIZATION";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_gbuffer_opt);
    ++len;

    if (o.dx10_sm4_1)
    {
        defines[def_it].Name = "SM_4_1";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_sm4_1);
    ++len;

    if (o.dx10_minmax_sm)
    {
        defines[def_it].Name = "USE_MINMAX_SM";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_minmax_sm != 0);
    ++len;

    // add a #define for DX10_1 MSAA support
    if (o.dx10_msaa)
    {
        static char samples[2];

        defines[def_it].Name = "USE_MSAA";
        defines[def_it].Definition = "1";
        def_it++;

        defines[def_it].Name = "MSAA_SAMPLES";
        samples[0] = char(o.dx10_msaa_samples) + '0';
        samples[1] = 0;
        defines[def_it].Definition = samples;
        def_it++;

        static char def[256];
        if (m_MSAASample < 0)
            def[0] = '0';
        else
            def[0] = '0' + char(m_MSAASample);

        def[1] = 0;
        defines[def_it].Name = "ISAMPLE";
        defines[def_it].Definition = def;
        def_it++;


        if (o.dx10_msaa_opt)
        {
            defines[def_it].Name = "MSAA_OPTIMIZATION";
            defines[def_it].Definition = "1";
            def_it++;
        }

        sh_name[len] = '1';
        ++len;
        sh_name[len] = '0' + char(o.dx10_msaa_samples);
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0' + char(o.dx10_msaa_opt);
        ++len;

        switch (o.dx10_msaa_alphatest)
        {
        case MSAA_ATEST_DX10_0_ATOC:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_0_ATOC";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '1';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            break;
        case MSAA_ATEST_DX10_1_ATOC:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_1_ATOC";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '1';
            ++len;
            sh_name[len] = '0';
            ++len;
            break;
        case MSAA_ATEST_DX10_1_NATIVE:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_1";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '1';
            ++len;
            break;
        default:
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
        }
    }
    else
    {
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
    }

    // Compile sources list
    size_t def_len = def_it * 5;
    size_t sources_len = source.size() + def_len + 2;
    string256 name_comment;
    sprintf_s(name_comment, "// %s\n", name);
    const char** sources = xr_alloc<const char*>(sources_len);
#ifdef DEBUG
	sources[0] = "#version 450\n#pragma optimize (off)\n";
#else
    sources[0] = "#version 450\n";
#endif
    sources[1] = name_comment;
    memcpy(sources + 2, defines, def_len * sizeof(char*));
    memcpy(sources + def_len + 2, source.data(), source.size() * sizeof(char*));

    // Compile the shader
    GLuint shader = *(GLuint*)result;
    R_ASSERT(shader);
    CHK_GL(glShaderSource(shader, sources_len, sources, nullptr));
    CHK_GL(glCompileShader(shader));

    // Create the shader program
    GLuint program = glCreateProgram();
    R_ASSERT(program);
    CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));
    *(GLuint*)result = program;

    // Free string resources
    xr_free(sources);
    for (xr_vector<char*>::iterator it = includes.begin(); it != includes.end(); ++it)
        xr_free(*it);

    // Get the compilation result
    GLint status;
    CHK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));

    // Link program if compilation succeeded
    GLchar* _pErrorMsgs = nullptr;
    if ((GLboolean)status == GL_TRUE)
    {
        CHK_GL(glAttachShader(program, shader));
        CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
        CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
        CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
        CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));
        CHK_GL(glLinkProgram(program));
        CHK_GL(glDetachShader(program, shader));
        CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));

        if ((GLboolean)status == GL_FALSE)
        {
            GLint length;
            CHK_GL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
            _pErrorMsgs = xr_alloc<GLchar>(length);
            CHK_GL(glGetProgramInfoLog(program, length, nullptr, _pErrorMsgs));
        }
    }
    else
    {
        GLint length;
        CHK_GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
        _pErrorMsgs = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderInfoLog(shader, length, nullptr, _pErrorMsgs));
    }

    if ((GLboolean)status == GL_FALSE)
    {
#ifdef DEBUG
        GLint srcLen;
        CHK_GL(glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srcLen));
        GLchar* shaderSrc = xr_alloc<GLchar>(srcLen);
        CHK_GL(glGetShaderSource(shader, srcLen, nullptr, shaderSrc));
#endif

        Msg("! shader compilation failed");
        Log("! ", name);
        if (_pErrorMsgs)
            Log("! error: ", _pErrorMsgs);

#ifdef DEBUG
        if (shaderSrc)
        {
            Log("Shader source:");
            Log(shaderSrc);
            Log("Shader source end.");
        }
        xr_free(shaderSrc);
#endif

        xr_free(_pErrorMsgs);
        CHK_GL(glDeleteShader(shader));
        return E_FAIL;
    }

    xr_free(_pErrorMsgs);
    CHK_GL(glDeleteShader(shader));
    return S_OK;
}
