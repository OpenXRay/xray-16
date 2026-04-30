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

void CRender::clearAllShaderOptions()                       { m_framegraphRenderer->clearAllShaderOptions(); }
void CRender::addShaderOption(pcstr name, pcstr value)      { m_framegraphRenderer->addShaderOption(name, value); }
void CRender::level_Load(IReader* fs)                       { m_framegraphRenderer->level_Load(fs); }
void CRender::level_Unload()                                { m_framegraphRenderer->level_Unload(); }
HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result)
                                                            { return m_framegraphRenderer->shader_compile(name, fs, pFunctionName, pTarget, Flags, result); }

u32 CRender::occq_begin(u32& ID) { return m_framegraphRenderer->m_HWOCC.occq_begin(ID); }
void CRender::occq_end(u32& ID) { m_framegraphRenderer->m_HWOCC.occq_end(ID); }
R_occlusion::occq_result CRender::occq_get(u32& ID) { return m_framegraphRenderer->m_HWOCC.occq_get(ID); }

void CRender::PrintFailedShadersSummary()
{
    if (::xray::render::g_failedShaders.empty())
        return;

    Msg("\n");
    Msg("╔══════════════════════════════════════════════════════════════");
    Msg("║ SHADER COMPILATION SUMMARY");
    Msg("╠══════════════════════════════════════════════════════════════");
    Msg("║ %zu shader(s) failed to compile:", ::xray::render::g_failedShaders.size());
    Msg("╠══════════════════════════════════════════════════════════════");
    for (const auto& failedShader : ::xray::render::g_failedShaders)
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

    m_framegraphRenderer->m_pTarget = xr_new<CRenderTarget>(); // Main target

    g_pModelPool = xr_new<CModelPool>();

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

    xr_delete(m_framegraphRenderer->m_pTarget);
    xr_delete(g_pModelPool);
    g_pModelPool = nullptr;

#if defined(USE_DX11) && RENDER == R_R4
    r5::ShutdownFrameGraph();

    if (m_backend)
    {
        GEnv.Backend = nullptr;
        xr_delete(m_backend);
        Msg("* Backend destroyed");
    }
#endif

    Device.seqFrame.Remove(this);
}

void CRender::reset_begin()
{
    ZoneScoped;
    // Wait for tasks to be done
    Resources->reset_begin();

    // Update incremental shadowmap-visibility solver
    // BUG-ID: 10646
    {
        u32 it = 0;
        for (it = 0; it < m_framegraphRenderer->m_Lights_LastFrame.size(); it++)
        {
            if (0 == m_framegraphRenderer->m_Lights_LastFrame[it])
                continue;
            try
            {
                for (int id = 0; id < 3; ++id)
                    m_framegraphRenderer->m_Lights_LastFrame[it]->svis[id].resetoccq();
            }
            catch (...)
            {
                Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&m_framegraphRenderer->m_Lights_LastFrame[it]));
            }
        }
        m_framegraphRenderer->m_Lights_LastFrame.clear();
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

    xr_delete(m_framegraphRenderer->m_pTarget);
    m_framegraphRenderer->m_HWOCC.occq_destroy();
}

void CRender::reset_end()
{
    ZoneScoped;
    m_framegraphRenderer->m_HWOCC.occq_create(occq_size);

    m_framegraphRenderer->m_pTarget = xr_new<CRenderTarget>();

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

void CRender::OnCameraUpdated() { m_framegraphRenderer->OnCameraUpdated(); }
void CRender::OnFrame()         { m_framegraphRenderer->OnFrame(); }

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
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent)              { return m_framegraphRenderer->ros_create(parent); }
void                    CRender::ros_destroy(IRender_ObjectSpecific*& p)      { m_framegraphRenderer->ros_destroy(p); }
IRenderVisual*          CRender::model_Create(LPCSTR name, IReader* data)     { return m_framegraphRenderer->model_Create(name, data); }
IRenderVisual*          CRender::model_CreateChild(LPCSTR name, IReader* data){ return m_framegraphRenderer->model_CreateChild(name, data); }
IRenderVisual*          CRender::model_Duplicate(IRenderVisual* V)            { return m_framegraphRenderer->model_Duplicate(V); }
void                    CRender::model_Delete(IRenderVisual*& V, bool bDiscard){ m_framegraphRenderer->model_Delete(V, bDiscard); }

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
    PS::CPEDef* SE = m_framegraphRenderer->m_PSLibrary.FindPED(name);
    R_ASSERT3(SE, "Particle effect doesn't exist", name);
    return g_pModelPool->CreatePE(SE);
}

IRenderVisual* CRender::model_CreateParticles(LPCSTR name)                    { return m_framegraphRenderer->model_CreateParticles(name); }
void           CRender::models_Prefetch()                                     { m_framegraphRenderer->models_Prefetch(); }
void           CRender::models_Clear(bool b_complete)                         { m_framegraphRenderer->models_Clear(b_complete); }

void CRender::Render()                                                        { if (m_framegraphRenderer) m_framegraphRenderer->Render(); }
void CRender::RenderMenu()                                                    { if (m_framegraphRenderer) m_framegraphRenderer->RenderMenu(); }
void CRender::Calculate()                                                     {}
void CRender::BeforeWorldRender()                                             {}
void CRender::AfterWorldRender()                                              {}
xray::render::CompiledLevelShader* CRender::getCompiledShader(int id)         { return m_framegraphRenderer->getCompiledShader(id); }
bool CRender::getShaderHandles(int id, nvrhi::ShaderHandle& outVS, nvrhi::ShaderHandle& outPS)
                                                                              { return m_framegraphRenderer->getShaderHandles(id, outVS, outPS); }

// Legacy D3D11 functions - removed for D3D12
// ref_shader CRender::getShader(int id) - REMOVED
// bool CRender::getShaderNames(...) - REMOVED
IRenderVisual* CRender::getVisual(int id) { return BufferPool.getVisual(id); }

IRender_Light* CRender::light_create()                  { return m_framegraphRenderer->light_create(); }
IRender_Glow*  CRender::glow_create()                   { return m_framegraphRenderer->glow_create(); }
bool           CRender::occ_visible(vis_data& P)        { return m_framegraphRenderer->occ_visible(P); }
bool           CRender::occ_visible(sPoly& P)           { return m_framegraphRenderer->occ_visible(P); }
bool           CRender::occ_visible(Fbox& P)            { return m_framegraphRenderer->occ_visible(P); }
void           CRender::add_Visual(u32 ctx, IRenderable* root, IRenderVisual* V, Fmatrix& m)
                                                        { m_framegraphRenderer->add_Visual(ctx, root, V, m); }
void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
    VERIFY2(T, "Invalid static wallmark triangle");
    if (T->suppress_wm)
        return;
    VERIFY2(_valid(P) && _valid(s) && verts && (s > EPS_L), "Invalid static wallmark params");
    m_framegraphRenderer->m_pWallmarksEngine->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    m_framegraphRenderer->add_StaticWallmark(pArray, P, s, T, V);
}

void CRender::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    m_framegraphRenderer->add_StaticWallmark(S, P, s, T, V);
}

void CRender::clear_static_wallmarks() { m_framegraphRenderer->clear_static_wallmarks(); }
void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm) { m_framegraphRenderer->m_pWallmarksEngine->AddSkeletonWallmark(wm); }

void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
    m_framegraphRenderer->m_pWallmarksEngine->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
void CRender::add_SkeletonWallmark(
    const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size)
{
    m_framegraphRenderer->add_SkeletonWallmark(xf, obj, pArray, start, dir, size);
}

void CRender::rmNear(CBackend& cmd_list)              { m_framegraphRenderer->rmNear(cmd_list); }
void CRender::rmFar(CBackend& cmd_list)               { m_framegraphRenderer->rmFar(cmd_list); }
void CRender::rmNormal(CBackend& cmd_list)            { m_framegraphRenderer->rmNormal(cmd_list); }
void CRender::SetPostProcessParams(const SPPInfo& ppi){ m_framegraphRenderer->SetPostProcessParams(ppi); }

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() = default;

CRender::~CRender() {}

void CRender::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    m_framegraphRenderer->DumpStatistics(font, alert);
}
} // namespace xray::render::fg
