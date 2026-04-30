#include "stdafx.h"

#include "xrCore/PostProcess/PPInfo.hpp"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/dxWallMarkArray.h"
#include "Layers/xrRender/dxUIShader.h"
#include "Layers/xrRender/Decals/fgWallMarkArray.h"
#include "Layers/xrRender/Decals/DecalManager.h"
#include "Layers/xrRender/Decals/MeshPicker.h"
#include "Layers/xrRender/Decals/OverlayManager.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/3DFluid/dx113DFluidManager.h"
#endif

#if defined(USE_DX11) && RENDER == R_R4
#include "Layers/xrRender/r5.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#endif

namespace xray::render::fg
{
CRender RImplementation;

// Global storage for failed shader compilation tracking
xr_vector<xr_string> g_failedShaders;

framegraph::ShaderLoader* CRender::GetShaderLoader() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetShaderLoader() : nullptr;
}

void CRender::RenderStatsOverlay()
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->RenderStatsOverlay();
}

void CRender::SetEnabled(bool enabled)
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->SetEnabled(enabled);
}

bool CRender::IsEnabled() const
{
    return m_framegraphRenderer && m_framegraphRenderer->IsEnabled();
}

xray::render::fg::RenderDevice* CRender::GetRenderDevice() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetRenderDevice() : nullptr;
}

xray::render::fg::ImGuiRendererNVRHI* CRender::GetImGuiRendererNVRHI() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetImGuiRendererNVRHI() : nullptr;
}

xray::render::MaterialCache* CRender::GetMaterialCache() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetMaterialCache() : nullptr;
}

xray::render::MaterialCache* CRender::GetUIMaterialCache() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetUIMaterialCache() : nullptr;
}

xray::render::ui::UIRenderCollector* CRender::GetUICollector() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetUICollector() : nullptr;
}

xray::render::ui::NVRHIUIRenderer* CRender::GetUIRenderer() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetUIRenderer() : nullptr;
}

xray::render::MaterialCache* CRender::GetTextMaterialCache() const
{
    return m_framegraphRenderer ? m_framegraphRenderer->GetTextMaterialCache() : nullptr;
}

void CRender::UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode)
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->UpdateSmokeTrail(muzzlePos, muzzleDir, dt, isHUDMode);
}

void CRender::NotifySmokeShot()
{
    if (m_framegraphRenderer)
        m_framegraphRenderer->NotifySmokeShot();
}

void CRender::RequestGrassInteraction(const Fvector& world_pos, float radius, float strength, uint8_t type)
{
    if (m_framegraphRenderer && m_framegraphRenderer->m_pDetailManager)
        m_framegraphRenderer->m_pDetailManager->RequestInteractionUpdateThreadSafe(world_pos, radius, strength, type);
}

void CRender::PrintFailedShadersSummary()
{
    if (g_failedShaders.empty())
        return;

    Msg("\n");
    Msg("╔══════════════════════════════════════════════════════════════");
    Msg("║ SHADER COMPILATION SUMMARY");
    Msg("╠══════════════════════════════════════════════════════════════");
    Msg("║ %zu shader(s) failed to compile:", g_failedShaders.size());
    Msg("╠══════════════════════════════════════════════════════════════");
    for (const auto& failedShader : g_failedShaders)
    {
        Msg("%s", failedShader.c_str());
    }
    Msg("╚══════════════════════════════════════════════════════════════");
    Msg("\n");
}

//////////////////////////////////////////////////////////////////////////
class CGlow : public IRender_Glow
{
public:
    bool bActive;

public:
    CGlow() : bActive(false) {}
    virtual void set_active(bool b) { bActive = b; }
    virtual bool get_active() { return bActive; }
    virtual void set_position(const Fvector& P) {}
    virtual void set_direction(const Fvector& D) {}
    virtual void set_radius(float R) {}
    virtual void set_texture(LPCSTR name) {}
    virtual void set_color(const Fcolor& C) {}
    virtual void set_color(float r, float g, float b) {}
};

float r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq, u32 phase)
{
    int id = SE_R2_SHADOW;
    if (CRender::PHASE_NORMAL == phase)
    {
        id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq, u32 phase)
{
    if (!pVisual->shader)
        return nullptr;
    int id = SE_R2_SHADOW;
    if (CRender::PHASE_NORMAL == phase)
    {
        id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}
static class cl_parallax : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        float h = ps_r2_df_parallax_h;
        cmd_list.set_c(C, h, -h / 2.f, 1.f / r_dtex_range, 1.f / r_dtex_range);
    }
} binder_parallax;

#if defined(USE_DX11)
static class cl_LOD : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.LOD.set_LOD(C); }
} binder_LOD;
#endif

static class cl_pos_decompress_params : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
#if defined(USE_DX11)
        const float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
        const float HorzTan = -VertTan / Device.fASPECT;
#elif defined(USE_OGL)
        const float VertTan = tanf(deg2rad(Device.fFOV / 2.0f));
        const float HorzTan = VertTan / Device.fASPECT;
#else
#   error No graphics API selected or enabled!
#endif
        cmd_list.set_c(
            C, HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
    }
} binder_pos_decompress_params;

static class cl_pos_decompress_params2 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth,
            1.0f / (float)Device.dwHeight);
    }
} binder_pos_decompress_params2;

static class cl_water_intensity : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float fValue = env.m_fWaterIntensity;
        cmd_list.set_c(C, fValue, fValue, fValue, 0.f);
    }
} binder_water_intensity;

static class cl_sun_shafts_intensity : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float fValue = env.m_fSunShaftsIntensity;
        cmd_list.set_c(C, fValue, fValue, fValue, 0.f);
    }
} binder_sun_shafts_intensity;

static class cl_alpha_ref : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        // TODO: OGL: Implement AlphaRef.
#   if defined(USE_DX11)
        cmd_list.StateManager.BindAlphaRef(C);
#   endif
    }
} binder_alpha_ref;

// Defined in ResourceManager.cpp
IReader* open_shader(pcstr shader);

// Check shadow cascades type (old SOC/CS or new COP)
static bool must_enable_old_cascades()
{
    return false;
}

// Returns true if compute shaders for HDAO Ultra exist
[[maybe_unused]] static bool ssao_hdao_cs_shaders_exist()
{
    IReader* hdao_cs      = open_shader("ssao_hdao.cs");
    IReader* hdao_cs_msaa = open_shader("ssao_hdao_msaa.cs");

    const bool exist      = hdao_cs && hdao_cs_msaa;

    FS.r_close(hdao_cs);
    FS.r_close(hdao_cs_msaa);

    return exist;
}

//////////////////////////////////////////////////////////////////////////
// Just two static storage
void CRender::create()
{
    ZoneScoped;

    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

    m_skinning = -1;
    m_MSAASample = -1;

    // hardware
    const auto& caps = GEnv.Backend->GetCapabilities();
    o.mrt = (caps.raster.dwMRT_count >= 3);
    o.mrtmixdepth = (caps.raster.b_MRT_mixdepth);

    // Check for NULL render target support
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
        if (0)
        {
            u32 device_id = caps.id_device;
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
                disable_nullrt = true; // G80
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
                disable_nullrt = true; // G84
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
            if (disable_nullrt)
                o.nullrt = false;
        }
        if (o.nullrt)
            Msg("* ...and used");
    }

    // SMAP / DST
    o.HW_smap_FETCH4 = FALSE;
    o.HW_smap = true;
    o.HW_smap_PCF = o.HW_smap;

    if (o.HW_smap)
    {
#if defined(USE_DX11)
        //	For ATI it's much faster on DX11 to use D32F format
        if (caps.id_vendor == 0x1002)
            o.HW_smap_FORMAT = D3DFMT_D32F_LOCKABLE;
        else
#endif
        {
            o.HW_smap_FORMAT = D3DFMT_D24X8;
        }
        Msg("* HWDST/PCF supported and used");
    }

    o.fp16_filter = true;
    o.fp16_blend = true;

    // emulate ATI-R4xx series
    if (strstr(Core.Params, "-r4xx"))
    {
        o.mrtmixdepth = FALSE;
        o.HW_smap = FALSE;
        o.HW_smap_PCF = FALSE;
        o.fp16_filter = FALSE;
        o.fp16_blend = FALSE;
    }

    VERIFY2(o.mrt && (caps.raster.dwInstructions >= 256), "Hardware doesn't meet minimum feature-level");
    if (o.mrtmixdepth)
        o.albedo_wo = FALSE;
    else if (o.fp16_blend)
        o.albedo_wo = FALSE;
    else
        o.albedo_wo = TRUE;

    // nvstencil on NV40 and up
    // nvstencil should be enabled only for GF 6xxx and GF 7xxx
    // if hardware support early stencil (>= GF 8xxx) stencil reset trick only
    // slows down.
    o.nvstencil = FALSE;
    if (strstr(Core.Params, "-nonvs"))
        o.nvstencil = FALSE;

    // nv-dbt
    o.nvdbt = false;

    if (o.nvdbt)
        Msg("* NV-DBT supported and used");

    o.ffp = false;

    // options (smap-pool-size)
    if (strstr(Core.Params, "-smap1024"))
        o.smapsize = 1024;
    else if (strstr(Core.Params, "-smap1536"))
        o.smapsize = 1536;
    else if (strstr(Core.Params, "-smap2048"))
        o.smapsize = 2048;
    else if (strstr(Core.Params, "-smap2560"))
        o.smapsize = 2560;
    else if (strstr(Core.Params, "-smap3072"))
        o.smapsize = 3072;
    else if (strstr(Core.Params, "-smap4096"))
        o.smapsize = 4096;
    else if (strstr(Core.Params, "-smap8192"))
        o.smapsize = 8192;
    else
        o.smapsize = ps_r2_smapsize;

    // gloss
    cpcstr g = strstr(Core.Params, "-gloss ");
    o.forcegloss = g ? TRUE : FALSE;
    if (g)
    {
        o.forcegloss_v = float(atoi(g + xr_strlen("-gloss "))) / 255.f;
    }

    // options
    o.bug = (strstr(Core.Params, "-bug")) ? TRUE : FALSE;
    o.sunfilter = (strstr(Core.Params, "-sunfilter")) ? TRUE : FALSE;
    //.	o.sunstatic			= (strstr(Core.Params,"-sunstatic"))?	TRUE	:FALSE	;
    o.sunstatic = ps_r2_sun_static;
    o.advancedpp = ps_r2_advanced_pp;
#if defined(USE_DX11)
    o.volumetricfog = ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE);
#elif defined(USE_OGL)
    // TODO: OGL: temporary disabled, need to fix it
    o.volumetricfog = false;
#endif
    o.sjitter = (strstr(Core.Params, "-sjitter")) ? TRUE : FALSE;
    o.depth16 = (strstr(Core.Params, "-depth16")) ? TRUE : FALSE;
    o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
    o.Tshadows = (strstr(Core.Params, "-tsh")) ? TRUE : FALSE;
    o.oldshadowcascades = must_enable_old_cascades() || ps_r2_ls_flags_ext.test(R2FLAGEXT_SUN_OLD);
    o.mblur = (strstr(Core.Params, "-mblur")) ? TRUE : FALSE;
    o.distortion_enabled = (strstr(Core.Params, "-nodistort")) ? FALSE : TRUE;
    o.distortion = o.distortion_enabled;
    o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
    o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

    o.ssao_blur_on = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) && (ps_r_ssao != 0);
    o.ssao_opt_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_OPT_DATA) && (ps_r_ssao != 0);
    o.ssao_half_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA) && o.ssao_opt_data && (ps_r_ssao != 0);
#if defined(USE_DX11)
    o.ssao_hdao = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HDAO) && (ps_r_ssao != 0);
    o.ssao_ultra = HW.ComputeShadersSupported && ssao_hdao_cs_shaders_exist();
    o.ssao_hbao = !o.ssao_hdao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HBAO) && (ps_r_ssao != 0);
#elif defined(USE_OGL)
    // TODO: OGL: temporary disabled HBAO/HDAO, need to fix it
    o.ssao_hbao = false;
    o.ssao_hdao = false;
#else
#   error No graphics API selected or enabled!
#endif

    //	TODO: fix hbao shader to allow to perform per-subsample effect!
    if (o.ssao_hbao && caps.id_vendor == 0x1002)
        o.hbao_vectorized = true;
    else
        o.hbao_vectorized = false;

#if defined(USE_DX11)
    o.dx11_sm4_1 = ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);
    o.dx11_sm4_1 = o.dx11_sm4_1 && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1);
#elif defined(USE_OGL)
    o.dx11_sm4_1 = true;
#else
#   error No graphics API selected or enabled!
#endif

    //	MSAA option dependencies
#if defined(USE_DX11)
    o.msaa = !!ps_r3_msaa;
    o.msaa_samples = (1 << ps_r3_msaa);

    o.msaa_opt = ps_r2_ls_flags.test(R3FLAG_MSAA_OPT);
    o.msaa_opt = o.msaa_opt && o.msaa && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1) ||
        o.msaa && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0);

    // o.msaa_hybrid	= ps_r2_ls_flags.test(R3FLAG_MSAA_HYBRID);
    o.msaa_hybrid = ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);
    o.msaa_hybrid &= !o.msaa_opt && o.msaa && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1);
#elif defined(USE_OGL)
    // TODO: OGL: temporary disabled, need to fix it
    o.msaa = false;
    o.msaa_samples = 0;
    o.msaa_opt = o.msaa;
    o.msaa_hybrid = false;
#else
#   error No graphics API selected or enabled!
#endif
    //	Allow alpha test MSAA for DX10.0

    // o.msaa_alphatest= ps_r2_ls_flags.test((u32)R3FLAG_MSAA_ALPHATEST);
    // o.msaa_alphatest= o.msaa_alphatest && o.msaa;

    // o.msaa_alphatest_atoc= (o.msaa_alphatest && !o.msaa_opt && !o.msaa_hybrid);

    o.msaa_alphatest = 0;
    if (o.msaa)
    {
        if (o.msaa_opt || o.msaa_hybrid)
        {
            if (ps_r3_msaa_atest == 1)
                o.msaa_alphatest = MSAA_ATEST_DX10_1_ATOC;
            else if (ps_r3_msaa_atest == 2)
                o.msaa_alphatest = MSAA_ATEST_DX10_1_NATIVE;
        }
        else
        {
            if (ps_r3_msaa_atest)
                o.msaa_alphatest = MSAA_ATEST_DX10_0_ATOC;
        }
    }

    o.gbuffer_opt = ps_r2_ls_flags.test(R3FLAG_GBUFFER_OPT);

    o.minmax_sm = ps_r3_minmax_sm;
    o.minmax_sm_screenarea_threshold = 1600 * 1200;

#if defined(USE_DX11)
    o.tessellation =
        HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);
    o.support_rt_arrays = true;
#else
    o.support_rt_arrays = false;
#endif

    if (o.minmax_sm == MMSM_AUTODETECT)
    {
        o.minmax_sm = MMSM_OFF;

        //	AMD device
        if (caps.id_vendor == 0x1002)
        {
            if (ps_r_sun_quality >= 3)
                o.minmax_sm = MMSM_AUTO;
            else if (ps_r_sun_shafts >= 2)
            {
                o.minmax_sm = MMSM_AUTODETECT;
                //	Check resolution in runtime in use_minmax_sm_this_frame
                o.minmax_sm_screenarea_threshold = 1600 * 1200;
            }
        }

        //	NVidia boards
        if (caps.id_vendor == 0x10DE)
        {
            if (ps_r_sun_shafts >= 2)
            {
                o.minmax_sm = MMSM_AUTODETECT;
                //	Check resolution in runtime in use_minmax_sm_this_frame
                o.minmax_sm_screenarea_threshold = 1280 * 1024;
            }
        }
    }

    // constants
    Resources->RegisterConstantSetup("parallax", &binder_parallax);
    Resources->RegisterConstantSetup("water_intensity", &binder_water_intensity);
    Resources->RegisterConstantSetup("sun_shafts_intensity", &binder_sun_shafts_intensity);
    Resources->RegisterConstantSetup("pos_decompression_params", &binder_pos_decompress_params);
    Resources->RegisterConstantSetup("pos_decompression_params2", &binder_pos_decompress_params2);
    Resources->RegisterConstantSetup("m_AlphaRef", &binder_alpha_ref);
#if defined(USE_DX11)
    Resources->RegisterConstantSetup("triLOD", &binder_LOD);
#endif

#if RENDER == R_R4
    r5::InitializeFrameGraph();
#endif

    Target = xr_new<CRenderTarget>(); // Main target

    g_pModelPool = xr_new<CModelPool>();
    g_pPSLibrary = &PSLibrary;
    PSLibrary.OnCreate();
    HWOCC.occq_create(occq_size);

    rmNormal(RCache);

    //	TODO: OGL: Implement FluidManager.
#if defined(USE_DX11)
    // FluidManager uses D3D11 geometry shaders - skip for D3D12/FrameGraph
    if (!GEnv.Backend || !GEnv.Backend->IsFrameGraph())
    {
        FluidManager.Initialize(70, 70, 70);
        //	FluidManager.Initialize( 100, 100, 100 );
        FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);
    }
    else
    {
        Msg("* [FluidManager] Disabled for D3D12 - uses legacy geometry shaders");
    }
#endif

    // Print summary of any failed shader compilations
    PrintFailedShadersSummary();
}

void CRender::destroy()
{
#if defined(USE_DX11)
    // FluidManager only initialized for D3D11, not D3D12
    if (!GEnv.Backend || !GEnv.Backend->IsFrameGraph())
    {
        FluidManager.Destroy();
    }
#endif

#if defined(USE_DX11) && RENDER == R_R4
    r5::ShutdownFrameGraph();

    if (m_backend)
    {
        GEnv.Backend = nullptr;
        xr_delete(m_backend);
        Msg("* Backend destroyed");
    }
#endif

    HWOCC.occq_destroy();
    xr_delete(g_pModelPool);
    g_pModelPool = nullptr;
    g_pPSLibrary = nullptr;
    xr_delete(Target);
    PSLibrary.OnDestroy();
    Device.seqFrame.Remove(this);
}

void CRender::reset_begin()
{
    ZoneScoped;
    // Wait for tasks to be done
    r_main.sync();
    r_sun.sync();
    r_sun_old.sync();
#if RENDER != R_R2
    r_rain.sync();
#endif

    Resources->reset_begin();

    // Update incremental shadowmap-visibility solver
    // BUG-ID: 10646
    {
        u32 it = 0;
        for (it = 0; it < Lights_LastFrame.size(); it++)
        {
            if (0 == Lights_LastFrame[it])
                continue;
            try
            {
                for (int id = 0; id < 3; ++id)
                    Lights_LastFrame[it]->svis[id].resetoccq();
            }
            catch (...)
            {
                Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
            }
        }
        Lights_LastFrame.clear();
    }

    //AVO: let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size ||
        !fsimilar(ps_r__Detail_density, ps_current_detail_density) ||
        !fsimilar(ps_r__Detail_height, ps_current_detail_height)))
    {
        m_framegraphRenderer->m_pDetailManager->Unload();
        xr_delete(m_framegraphRenderer->m_pDetailManager);
    }
    //-AVO

    xr_delete(Target);
    HWOCC.occq_destroy();
}

void CRender::reset_end()
{
    ZoneScoped;
    HWOCC.occq_create(occq_size);

    Target = xr_new<CRenderTarget>();

    //AVO: let's reload details while changed details options on vid_restart
    if (b_loaded && (dm_current_size != dm_size ||
        !fsimilar(ps_r__Detail_density, ps_current_detail_density) ||
        !fsimilar(ps_r__Detail_height, ps_current_detail_height)))
    {
        m_framegraphRenderer->m_pDetailManager = xr_new<CDetailManager>();
        m_framegraphRenderer->m_pDetailManager->Load();
    }
    //-AVO

#if defined(USE_DX11)
    FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);
#endif

    cleanup_contexts();

    // Set this flag true to skip the first render frame,
    // that some data is not ready in the first frame (for example device camera position)
    m_framegraphRenderer->m_bFirstFrameAfterReset = true;
}

void CRender::OnCameraUpdated()
{
    ZoneScoped;

    // Frustum
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

    if (g_pGamePersistent->MainMenuActiveOrLevelNotExist())
        return;

    m_framegraphRenderer->m_pProcessHOMTask = &HOM.DispatchMTRender();
    if (m_framegraphRenderer->m_pDetailManager)
        m_framegraphRenderer->m_pDetailManager->DispatchMTCalc();
}

void CRender::OnFrame()
{
    ZoneScoped;

    g_pModelPool->DeleteQueue();

    if (g_pGamePersistent->MainMenuActiveOrLevelNotExist())
        return;
}

#ifdef USE_OGL
IRender::RenderContext CRender::GetCurrentContext() const
{
    return HW.GetCurrentContext();
}

void CRender::MakeContextCurrent(RenderContext context)
{
    R_ASSERT3(HW.MakeContextCurrent(context) == 0,
        "Failed to switch OpenGL context", SDL_GetError());
}
#endif

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent) { return xr_new<CROS_impl>(); }
void CRender::ros_destroy(IRender_ObjectSpecific*& p) { xr_delete(p); }
IRenderVisual* CRender::model_Create(LPCSTR name, IReader* data) { return g_pModelPool->Create(name, data); }
IRenderVisual* CRender::model_CreateChild(LPCSTR name, IReader* data) { return g_pModelPool->CreateChild(name, data); }
IRenderVisual* CRender::model_Duplicate(IRenderVisual* V) { return g_pModelPool->Instance_Duplicate((dxRender_Visual*)V); }

void CRender::model_Delete(IRenderVisual*& V, bool bDiscard)
{
    dxRender_Visual* pVisual = (dxRender_Visual*)V;
    g_pModelPool->Delete(pVisual, bDiscard);
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
    return g_pModelPool->CreatePE(SE);
}

IRenderVisual* CRender::model_CreateParticles(LPCSTR name)
{
    PS::CPEDef* SE = PSLibrary.FindPED(name);
    if (SE)
        return g_pModelPool->CreatePE(SE);

    PS::CPGDef* SG = PSLibrary.FindPGD(name);
    R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
    return g_pModelPool->CreatePG(SG);
}
void CRender::models_Prefetch() { g_pModelPool->Prefetch(); }
void CRender::models_Clear(bool b_complete) { g_pModelPool->ClearPool(b_complete); }
// D3D12: New shader access using CompiledLevelShader
CRender::CompiledLevelShader* CRender::getCompiledShader(int id)
{
    if (id < 0 || id >= int(CompiledLevelShaders.size()))
        return nullptr;
    return &CompiledLevelShaders[id];
}

bool CRender::getShaderHandles(int id, nvrhi::ShaderHandle& outVS, nvrhi::ShaderHandle& outPS)
{
    auto* compiled = getCompiledShader(id);
    if (!compiled || !compiled->vsHandle || !compiled->psHandle)
        return false;

    outVS = compiled->vsHandle;
    outPS = compiled->psHandle;
    return true;
}

// Legacy D3D11 functions - removed for D3D12
// ref_shader CRender::getShader(int id) - REMOVED
// bool CRender::getShaderNames(...) - REMOVED
IRenderVisual* CRender::getVisual(int id) { return BufferPool.getVisual(id); }

// ═══════════════════════════════════════════════════
//  D3D12: PSO PRECOMPILATION HELPER FUNCTIONS
// ═══════════════════════════════════════════════════

u32 CRender::GetVertexStride(u32 vertexFormatID)
{
    if (vertexFormatID >= nDC.size())
        return 0;
    const VertexDeclarator& decl = nDC[vertexFormatID];
    return GetDeclVertexSize(decl.begin(), 0);  // Stream 0
}

bool CRender::IsFormatCompatible(u8 d3dFormat, nvrhi::Format nvrhiFormat)
{
    // TODO: Implement proper D3D format → NVRHI format conversion check
    // For now, assume compatible (vertex layout matching will catch real issues)
    return true;
}

bool CRender::MatchesSemanticName(const VertexElement& elem, const xr_string& semanticName)
{
    // Map D3D11_DECL_USAGE to HLSL semantic names
    static const char* semanticNames[] = {
        "POSITION",     // D3DDECLUSAGE_POSITION = 0
        "BLENDWEIGHT",  // D3DDECLUSAGE_BLENDWEIGHT = 1
        "BLENDINDICES", // D3DDECLUSAGE_BLENDINDICES = 2
        "NORMAL",       // D3DDECLUSAGE_NORMAL = 3
        "PSIZE",        // D3DDECLUSAGE_PSIZE = 4
        "TEXCOORD",     // D3DDECLUSAGE_TEXCOORD = 5
        "TANGENT",      // D3DDECLUSAGE_TANGENT = 6
        "BINORMAL",     // D3DDECLUSAGE_BINORMAL = 7
        "TESSFACTOR",   // D3DDECLUSAGE_TESSFACTOR = 8
        "POSITIONT",    // D3DDECLUSAGE_POSITIONT = 9
        "COLOR",        // D3DDECLUSAGE_COLOR = 10
        "FOG",          // D3DDECLUSAGE_FOG = 11
        "DEPTH",        // D3DDECLUSAGE_DEPTH = 12
        "SAMPLE",       // D3DDECLUSAGE_SAMPLE = 13
    };

    if (elem.Usage >= sizeof(semanticNames) / sizeof(semanticNames[0]))
        return false;

    const char* declSemantic = semanticNames[elem.Usage];

    // Match semantic name + index (e.g., "TEXCOORD0")
    char expected[64];
    xr_sprintf(expected, "%s%u", declSemantic, elem.UsageIndex);

    return semanticName == expected || semanticName == declSemantic;
}

bool CRender::IsVertexFormatCompatible(const VertexDeclarator& decl, const framegraph::ExtractedReflection* vsReflection)
{
    if (!vsReflection)
        return false;

    // Check if vertex declaration provides all inputs required by shader
    for (const auto& input : vsReflection->vertexInputSignature.elements) {
        bool found = false;

        for (u32 i = 0; decl[i].Stream != 0xFF; ++i) {
            if (MatchesSemanticName(decl[i], input.semanticName.c_str())) {
                // Check format compatibility
                if (IsFormatCompatible(decl[i].Type, input.format)) {
                    found = true;
                    break;
                }
            }
        }

        // If required input not found, formats are incompatible
        if (!found) {
            return false;
        }
    }

    return true;
}

void CRender::SetupDepthState(RenderPassType passType, const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc)
{
    using RenderPassType = RenderPassType;

    switch (passType) {
    case RenderPassType::DepthPrepass:
        // Depth prepass: write depth, test with Less
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::ForwardColor:
        // Forward color: read depth (early-Z), no write, test with Equal
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = false;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::HUD:
        // HUD: test and write with LessEqual (renders in front)
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::UI:
        // UI: depth disabled
        psoDesc.renderState.depthStencilState.depthTestEnable = false;
        psoDesc.renderState.depthStencilState.depthWriteEnable = false;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    default:
        // Default: standard depth test
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;
    }
}

void CRender::SetupBlendState(const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc)
{
    if (materialInfo.transparent) {
        // Transparent: alpha blending enabled
        psoDesc.renderState.blendState.targets[0].blendEnable = true;
        psoDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        psoDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
        psoDesc.renderState.blendState.targets[0].blendOp = nvrhi::BlendOp::Add;
        psoDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        psoDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
        psoDesc.renderState.blendState.targets[0].blendOpAlpha = nvrhi::BlendOp::Add;
    } else {
        // Opaque: no blending
        psoDesc.renderState.blendState.targets[0].blendEnable = false;
    }

    // Alpha-to-coverage for alpha test materials (optional quality improvement)
    psoDesc.renderState.blendState.alphaToCoverageEnable = materialInfo.alphaTest;
}

bool CRender::CreatePrecompiledPSO(
    u32 shaderID,
    u32 vertexFormatID,
    RenderPassType passType,
    nvrhi::Format colorFormat,
    nvrhi::Format depthFormat,
    MaterialCache* materialCache)
{
    auto& compiled = CompiledLevelShaders[shaderID];

    // ═══════════════════════════════════════════════════
    //  CREATE PSO DESCRIPTOR
    // ═══════════════════════════════════════════════════
    nvrhi::GraphicsPipelineDesc psoDesc;
    psoDesc.VS = compiled.vsHandle;
    psoDesc.PS = compiled.psHandle;

    // Setup vertex input layout
    const VertexDeclarator& decl = nDC[vertexFormatID];
    u32 vb_stride = GetVertexStride(vertexFormatID);

    xr_vector<nvrhi::VertexAttributeDesc> attributes;
    for (const auto& input : compiled.vsReflection->vertexInputSignature.elements) {
        // Find matching element in vertex declaration
        for (u32 i = 0; decl[i].Stream != 0xFF; ++i) {
            if (MatchesSemanticName(decl[i], input.semanticName.c_str())) {
                nvrhi::VertexAttributeDesc attr;
                attr.name = input.semanticName.c_str();
                attr.format = input.format;  // Use format from reflection
                attr.offset = decl[i].Offset;
                attr.bufferIndex = 0;
                attr.elementStride = vb_stride;
                attr.isInstanced = false;
                attributes.push_back(attr);
                break;
            }
        }
    }

    auto* nvrhiDevice = m_renderDevice->GetNVRHIDevice();
    if (!nvrhiDevice) {
        Msg("! NVRHI device not available");
        return false;
    }

    if (!attributes.empty()) {
        psoDesc.inputLayout = nvrhiDevice->createInputLayout(
            attributes.data(),
            (uint32_t)attributes.size(),
            compiled.vsHandle);
    }

    // Setup depth/blend states
    SetupDepthState(passType, compiled.materialInfo, psoDesc);
    SetupBlendState(compiled.materialInfo, psoDesc);

    // Set topology (always triangles for level geometry)
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // ═══════════════════════════════════════════════════
    //  CREATE FRAMEBUFFER INFO (required for PSO creation)
    // ═══════════════════════════════════════════════════
    nvrhi::FramebufferInfoEx fbInfo;
    if (passType == RenderPassType::ForwardColor) {
        fbInfo.addColorFormat(colorFormat);
        fbInfo.setDepthFormat(depthFormat);
    } else if (passType == RenderPassType::DepthPrepass) {
        // Depth-only: no color output
        fbInfo.setDepthFormat(depthFormat);
    }

    // ═══════════════════════════════════════════════════
    //  CREATE PSO
    // ═══════════════════════════════════════════════════
    nvrhi::GraphicsPipelineHandle pso = nvrhiDevice->createGraphicsPipeline(psoDesc, fbInfo);

    if (!pso) {
        Msg("! Failed to create PSO for shader %u (%s), format %u, pass %u",
            shaderID, compiled.shaderName.c_str(), vertexFormatID, (u32)passType);
        return false;
    }

    // Store NVRHI handle in precompiled PSO cache (MaterialCache will wrap it in MaterialPSO later)
    u64 cacheKey = ((u64)vertexFormatID << 32) | (u64)passType;

    CompiledLevelShader::PrecompiledPSOs::PSOVariant variant;
    variant.vertexFormatID = vertexFormatID;
    variant.passType = passType;
    variant.pso = nullptr;  // fg::PipelineState not used here - we store nvrhi handle directly
    variant.materialPSO = nullptr;  // MaterialPSO creation deferred until first use

    compiled.precompiledPSOs.variants.push_back(variant);
    // Store the NVRHI handle as a MaterialPSO (temporary - will be wrapped properly on first use)
    compiled.precompiledPSOs.psoCache[cacheKey] = reinterpret_cast<MaterialPSO*>(pso.Get());

    Msg("* Precompiled PSO for shader %u (%s), format %u, pass %u",
        shaderID, compiled.shaderName.c_str(), vertexFormatID, (u32)passType);

    return true;
}

IRender_Light* CRender::light_create() { return Lights.Create(); }
IRender_Glow* CRender::glow_create() { return xr_new<CGlow>(); }
bool CRender::occ_visible(vis_data& P) { return HOM.visible(P); }
bool CRender::occ_visible(sPoly& P) { return HOM.visible(P); }
bool CRender::occ_visible(Fbox& P) { return HOM.visible(P); }
void CRender::add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m)
{
#if defined(USE_DX11) && RENDER == R_R4
    // ═══════════════════════════════════════════════════════
    //  FRAMEGRAPH INTEGRATION
    // ═══════════════════════════════════════════════════════
    // When FrameGraph is active, route geometry collection through FrameGraph renderer
    // This allows game objects to add their visuals via renderable_Render() callbacks
    // without going through legacy dsgraph
    //
    // This is CRITICAL for rendering:
    // - NPCs with their weapons/equipment
    // - Player weapons and attachments
    // - HUD items
    // - Dynamic objects with sub-objects
    // ═══════════════════════════════════════════════════════

    if (m_framegraphRenderer && m_framegraphRenderer->IsEnabled())
    {
        m_framegraphRenderer->add_Visual(root, V, m);
        return;
    }
#endif
}
void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
#if RENDER == R_R4
    if (m_framegraphRenderer && m_framegraphRenderer->IsEnabled())
        return;
#endif
    VERIFY2(T, "Invalid static wallmark triangle");
    if (T->suppress_wm)
        return;
    VERIFY2(_valid(P) && _valid(s) && verts && (s > EPS_L), "Invalid static wallmark params");
    m_framegraphRenderer->m_pWallmarksEngine->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
#if RENDER == R_R4
    if (m_framegraphRenderer && m_framegraphRenderer->IsEnabled()) {
        if (!T || T->suppress_wm || !V || s <= EPS_L)
            return;
        auto* fgArray = static_cast<decals::fgWallMarkArray*>(pArray);
        u32 matID = fgArray->GenerateBindlessMaterialID();
        if (matID == UINT32_MAX)
            return;
        Fvector N;
        N.mknormal(V[T->verts[0]], V[T->verts[1]], V[T->verts[2]]);
        float decalSize = s * 2.0f;
        m_framegraphRenderer->GetDecalManager()->AddStaticDecal(P, N, decalSize, matID);
        Msg("[Decal] STATIC pos=(%.1f,%.1f,%.1f) size=%.3f matID=%u count=%u",
            P.x, P.y, P.z, decalSize, matID, m_framegraphRenderer->GetDecalManager()->GetActiveCount());
        return;
    }
#endif
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

void CRender::clear_static_wallmarks() { m_framegraphRenderer->m_pWallmarksEngine->clear(); }
void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm) { m_framegraphRenderer->m_pWallmarksEngine->AddSkeletonWallmark(wm); }

static float EstimateSplatUVRadius(const decals::MeshPickResult& pickResult, float worldRadius)
{
    const float du1 = pickResult.triUV[1].x - pickResult.triUV[0].x;
    const float dv1 = pickResult.triUV[1].y - pickResult.triUV[0].y;
    const float du2 = pickResult.triUV[2].x - pickResult.triUV[0].x;
    const float dv2 = pickResult.triUV[2].y - pickResult.triUV[0].y;

    const float det = du1 * dv2 - dv1 * du2;
    if (_abs(det) > EPS_S)
    {
        const float invDet = 1.f / det;

        Fvector dpdu;
        dpdu.set(
            (pickResult.triWorldEdge1.x * dv2 - pickResult.triWorldEdge2.x * dv1) * invDet,
            (pickResult.triWorldEdge1.y * dv2 - pickResult.triWorldEdge2.y * dv1) * invDet,
            (pickResult.triWorldEdge1.z * dv2 - pickResult.triWorldEdge2.z * dv1) * invDet);

        Fvector dpdv;
        dpdv.set(
            (pickResult.triWorldEdge2.x * du1 - pickResult.triWorldEdge1.x * du2) * invDet,
            (pickResult.triWorldEdge2.y * du1 - pickResult.triWorldEdge1.y * du2) * invDet,
            (pickResult.triWorldEdge2.z * du1 - pickResult.triWorldEdge1.z * du2) * invDet);

        const float worldPerUV = 0.5f * (dpdu.magnitude() + dpdv.magnitude());
        if (worldPerUV > EPS_S)
            return _max(worldRadius / worldPerUV, 1e-4f);
    }

    const float worldEdge = _max(pickResult.triWorldEdge1.magnitude(), pickResult.triWorldEdge2.magnitude());
    const float uvEdge1 = _sqrt(_sqr(du1) + _sqr(dv1));
    const float uvEdge2 = _sqrt(_sqr(du2) + _sqr(dv2));
    const float uvEdge = _max(uvEdge1, uvEdge2);
    if (worldEdge > EPS_S && uvEdge > EPS_S)
        return _max(worldRadius * (uvEdge / worldEdge), 1e-4f);

    return 0.02f;
}

void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
    m_framegraphRenderer->m_pWallmarksEngine->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size)
{
#if RENDER == R_R4
    if (m_framegraphRenderer && m_framegraphRenderer->IsEnabled()) {
        if (!xf || !obj || size <= EPS_L) {
            Msg("[Splat] SKIP: bad params xf=%p obj=%p size=%.3f", xf, obj, size);
            return;
        }
        float distSq = xf->c.distance_to_sqr(Device.vCameraPosition);
        if (distSq > _sqr(50.f)) {
            Msg("[Splat] SKIP: too far dist=%.1f", _sqrt(distSq));
            return;
        }
        const u32 splatMode = ps_r4_skeleton_wallmark_mode > 0
            ? decals::SPLAT_MODE_PROCEDURAL_BLOOD
            : decals::SPLAT_MODE_DECAL;

        shared_str wallmarkTexture;
        u32 matID = UINT32_MAX;
        if (splatMode == decals::SPLAT_MODE_DECAL)
        {
            if (!pArray) {
                Msg("[Splat] SKIP: missing wallmark array for decal mode");
                return;
            }

            auto* fgArray = static_cast<decals::fgWallMarkArray*>(pArray);
            matID = fgArray->GenerateBindlessMaterialID(&wallmarkTexture);
            if (matID == UINT32_MAX) {
                Msg("[Splat] SKIP: matID invalid");
                return;
            }
        }
        float decalSize = size * 2.0f;
        decals::MeshPickResult entryPick;
        Msg("[Splat] Picking obj=%p dir=(%.2f,%.2f,%.2f)", obj, dir.x, dir.y, dir.z);
        if (decals::PickMeshDirect((CKinematics*)obj, *xf, start, dir, 100.f, entryPick)) {
            auto* overlayMgr = m_framegraphRenderer->GetOverlayManager();
            float worldRadius = decalSize * 0.5f;
            Fvector bloodColor = { 0.4f, 0.02f, 0.02f };
            const float lifetime = splatMode == decals::SPLAT_MODE_PROCEDURAL_BLOOD ? 18.f : 12.f;

            auto queueSplat = [&](const decals::MeshPickResult& pick, const char* hitKind)
            {
                const float uvRadius = EstimateSplatUVRadius(pick, worldRadius);
                overlayMgr->AddSplat((CKinematics*)obj, pick.triVerts,
                                      pick.baryU, pick.baryV,
                                      worldRadius, bloodColor, 0.8f,
                                      pick.uv, uvRadius, matID,
                                      splatMode, lifetime,
                                      pick.hitTextureName.c_str(),
                                      wallmarkTexture.c_str());
                Msg("[Splat] %s: mode=%s r=%.3f uv=(%.3f,%.3f) uvR=%.4f bary=(%.3f,%.3f) obj=%p wm=%s",
                    hitKind,
                    splatMode == decals::SPLAT_MODE_PROCEDURAL_BLOOD ? "proc" : "decal",
                    worldRadius, pick.uv.x, pick.uv.y,
                    uvRadius, pick.baryU, pick.baryV, obj,
                    wallmarkTexture.size() ? wallmarkTexture.c_str() : "<none>");
            };

            queueSplat(entryPick, "ENTRY");

            Fvector shotDir = dir;
            shotDir.normalize_safe();
            const float entryAdvance = entryPick.dist + 0.01f;
            const float remaining = 100.f - entryAdvance;
            if (remaining > 0.01f)
            {
                Fvector exitStart;
                exitStart.mad(start, shotDir, entryAdvance);

                decals::MeshPickResult exitPick;
                if (decals::PickMeshDirect((CKinematics*)obj, *xf, exitStart, shotDir, remaining, exitPick))
                {
                    const float minSeparation = _max(0.02f, worldRadius * 0.25f);
                    if (entryPick.worldPos.distance_to(exitPick.worldPos) > minSeparation)
                        queueSplat(exitPick, "EXIT");
                }
            }
        } else {
            Msg("[Splat] MISS: no triangle hit for obj=%p", obj);
        }
        return;
    }
#endif
    dxWallMarkArray* pWMA = (dxWallMarkArray*)pArray;
    ref_shader* pShader = pWMA->dxGenerateWallmark();
    if (pShader)
        add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}

void CRender::rmNear(CBackend& cmd_list)
{
    const D3D_VIEWPORT viewport = { 0, 0, Target->get_width(cmd_list), Target->get_height(cmd_list), 0.f, 0.02f };
    cmd_list.SetViewport(viewport);
}

void CRender::rmFar(CBackend& cmd_list)
{
    const D3D_VIEWPORT viewport = { 0, 0, Target->get_width(cmd_list), Target->get_height(cmd_list), 0.99999f, 1.f };
    cmd_list.SetViewport(viewport);
}

void CRender::rmNormal(CBackend& cmd_list)
{
    const D3D_VIEWPORT viewport = { 0, 0, Target->get_width(cmd_list), Target->get_height(cmd_list), 0.f, 1.f };
    cmd_list.SetViewport(viewport);
}

void CRender::SetPostProcessParams(const SPPInfo& ppi)
{
    Target->set_blur(ppi.blur);
    Target->set_gray(ppi.gray);

    Target->set_duality_h(ppi.duality.h);
    Target->set_duality_v(ppi.duality.v);

    Target->set_noise(ppi.noise.intensity);
    Target->set_noise_scale(ppi.noise.grain);
    Target->set_noise_fps(ppi.noise.fps);

    Target->set_color_base(ppi.color_base);
    Target->set_color_gray(ppi.color_gray);
    Target->set_color_add(ppi.color_add);

    Target->set_cm_imfluence(ppi.cm_influence);
    Target->set_cm_interpolate(ppi.cm_interpolate);
    Target->set_cm_textures(ppi.cm_tex1, ppi.cm_tex2);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() = default;

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
    m_framegraphRenderer->m_Sectors_xrc.DumpStatistics(font, alert);
}
} // namespace xray::render::fg
