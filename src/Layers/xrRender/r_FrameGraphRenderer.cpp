// xrRender/r_FrameGraphRenderer.cpp
#include "stdafx.h"
#include "r_FrameGraphRenderer.h"
#include "xrEngine/IRenderable.h"
#include "Layers/xrRender/DetailModel.h"
#include "Layers/xrRender/LightTrack.h"
#include "xrCore/FMesh.hpp"
#include "FHierrarhyVisual.h"
#include "SkeletonAnimated.h"
#include "FVisual.h"
#include "FProgressive.h"
#include "FSkinned.h"
#include "FLOD.h"
#include "FTreeVisual.h"
#include "ParticleGroup.h"
#include "ParticleEffect.h"
#include "ParticleEffectDef.h"
#include "Shader.h"
#include "r__scene.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Materials/ShaderInfo.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/fgUIRender.h"
#include "Layers/xrRender/ShaderKey.h"
#include "xrEngine/CustomHUD.h"
#include "ImGuiRendererNVRHI.h"
#include "xrEngine/device.h"
#include "xrCore/Threading/TaskManager.hpp"
#include <imgui.h>

// Lambda-based pass setup functions
#include "FrameGraphPasses/DebugDrawPassSetup.h"
#include "FrameGraphPasses/HiZBuildPassSetup.h"      // Phase 3.5: Hi-Z pyramid for GPU culling
#include "FrameGraphPasses/ForwardColorPassSetup.h"  // Phase 1: Single-RT forward rendering + pipeline init
#include "FrameGraphPasses/DepthPrepassSetup.h"
#include "FrameGraphPasses/PassCommon.h"
#include "GPUCullingManager.h"                       // Phase 3.5: GPU frustum/occlusion culling
#include "FGDetailManager.h"                         // Detail system (grass/vegetation)
#include "FrameGraphPasses/DetailCullPassSetup.h"    // Detail culling (async compute)
#include "FrameGraphPasses/DetailPassSetup.h"        // Detail rendering pass
#include "FrameGraphPasses/TransparentPassSetup.h"   // Transparent alpha-blended geometry (after detail)
// SM6 bindless: Textures registered directly with D3D12Backend via RegisterBindlessTexture()
#include "Bindless/MaterialBuffer.h"                 // Bindless material buffer
#include "Bindless/TerrainMaterialBuffer.h"          // Terrain material buffer
#include "Bindless/VariantTextureBuffer.h"           // Variant texture buffer
#include "FrameGraphPasses/SkyPassSetup.h"           // Sky dome rendering
#include "FrameGraphPasses/SunPassSetup.h"           // Sun disc rendering
#include "FrameGraphPasses/SkinningPassSetup.h"
#include "FrameGraphPasses/ParticlePassSetup.h"      // Particle rendering (billboards/sprites)
#include "FrameGraphPasses/GlowPassSetup.h"
#include "FrameGraphPasses/DistortionApplyPassSetup.h" // Distortion post-process
#include "FrameGraphPasses/DecalPassSetup.h"          // Screen-space box decals
#include "Decals/DecalManager.h"                      // Decal manager
#include "Decals/OverlayManager.h"                    // Per-NPC overlay textures
#include "FrameGraphPasses/ExposurePassSetup.h"      // Auto-exposure from histogram
#include "FrameGraphPasses/UIPassSetup.h"
#include "FrameGraphPasses/FontPassSetup.h"
#include "FrameGraphPasses/TonemapPassSetup.h"       // Tonemap pass: HDR→LDR conversion
#include "FrameGraphPasses/VolumetricPassSetup.h"
#include "Volumetrics/VolumetricRenderer.h"
#include "FrameGraphPasses/ShadowPassSetup.h"
#include "FrameGraphPasses/LocalShadowPassSetup.h"
#include "FrameGraphPasses/SmokeTrailPassSetup.h"
#include "FrameGraphPasses/ClusterLightPassSetup.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "FrameGraphPasses/MotionVectorPassSetup.h"
#include "FrameGraphPasses/ReSTIRGIPassSetup.h"
#include "Upscaling/UpscaleState.h"
#include "Upscaling/IUpscaleBackend.h"
#include "Upscaling/UpscalePassSetup.h"
#include "Upscaling/StreamlineDLSS.h"
#include "Upscaling/DlssFgPassSetup.h"
#include "Denoising/IDenoiseBackend.h"
#include "FrameGraphPasses/RibbonPassSetup.h"
#include "FrameGraphPasses/TrailPassSetup.h"
#include "Layers/xrRender/FrameGraph/Blackboard.h"
#include "FrameGraphPasses/ImGuiPassSetup.h"
#include "FrameGraphPasses/RainPassSetup.h"
#include "FrameGraphPasses/WetSurfacesPassSetup.h"
#include "FrameGraphPasses/SceneReflectionPassSetup.h"
#include "FrameGraphPasses/RainShadowPassSetup.h"
#include "FrameGraphPasses/AmbientOcclusionPassSetup.h"
#include "FrameGraphPasses/SunShaftsPassSetup.h"
#include "FrameGraphPasses/ThunderboltPassSetup.h"
#include "FrameGraphPasses/LensFlarePassSetup.h"
#include "FrameGraphPasses/PathTracerPassSetup.h"
#include "FrameGraphPasses/SSRPassSetup.h"
#include "FrameGraphPasses/SSGIPassSetup.h"
#include "FrameGraphPasses/SSSSSPassSetup.h"
#include "FrameGraphPasses/IBLPrefilterPassSetup.h"
#include "FrameGraphPasses/DofPassSetup.h"
#include "FrameGraphPasses/TAAPassSetup.h"
#include "FrameGraphPasses/ContactShadowsPassSetup.h"
#include "FrameGraphPasses/ShadowHZBPassSetup.h"
#include "FrameGraphPasses/ShadowMaskPassSetup.h"
#include "FrameGraphPasses/BloomPassSetup.h"
#include "FrameGraphPasses/CASPassSetup.h"
#include "FrameGraphPasses/PostProcessPassSetup.h"
#include "FrameGraphPasses/CameraModelPassSetup.h"
#include "FrameGraphPasses/LodPassSetup.h"
#include "Layers/xrRender/FLOD.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/fgRainRender.h"
#include "Layers/xrRender/fgThunderboltRender.h"
#include "Layers/xrRender/fgLensFlareRender.h"
#include "Layers/xrRender/fgEnvironmentRender.h"
#include "xrEngine/Environment.h"
#include "xrEngine/Rain.h"
#include "xrEngine/thunderbolt.h"
#include "xrEngine/xr_efflensflare.h"
#include "xrEngine/IGame_Persistent.h"
#include "RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "FrameGraphPasses/ShaderConstants.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrParticles/psystem.h"
#include "Layers/xrRender/blenders/Blender_Particle.h"

#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/LightTrack.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/r__buffer_pool.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/Decals/fgWallMarkArray.h"
#include "Layers/xrRender/Decals/MeshPicker.h"
#include "xrEngine/IGameFont.hpp"
#include "xrEngine/IPerformanceAlert.hpp"
#include "xrCore/PostProcess/PPInfo.hpp"

namespace xray::render::fg { xray::render::FrameGraphRenderer RImplementation; }

namespace xray::render
{
fg::ShaderElement* FrameGraphRenderer::rimp_select_sh_static(fg::dxRender_Visual* pVisual, float cdist_sq, u32 phase)
{
    if (!pVisual->shader)
        return nullptr;
    int id = SE_R2_SHADOW;
    if (FrameGraphRenderer::PHASE_NORMAL == phase)
    {
        id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < fg::r__dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}

fg::ShaderElement* FrameGraphRenderer::rimp_select_sh_dynamic(fg::dxRender_Visual* pVisual, float cdist_sq, u32 phase)
{
    int id = SE_R2_SHADOW;
    if (FrameGraphRenderer::PHASE_NORMAL == phase)
    {
        id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < fg::r__dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
    }
    return pVisual->shader->E[id]._get();
}

fg::IRender_DetailModel* FrameGraphRenderer::model_CreateDM(IReader* F)
{
    fg::CDetail* D = xr_new<fg::CDetail>();
    D->Load(F);
    return D;
}

void FrameGraphRenderer::model_Delete(fg::IRender_DetailModel*& F)
{
    if (F)
    {
        fg::CDetail* D = (fg::CDetail*)F;
        D->Unload();
        xr_delete(D);
        F = nullptr;
    }
}

void FrameGraphRenderer::CreateQuadIB()
{
    constexpr auto triCount = 4 * 1024;
    constexpr auto idxCount = triCount * 2 * 3;
    constexpr auto idxSize = idxCount * sizeof(u16);

    QuadIB.Create(idxSize, false, false);

    u16* Indices = static_cast<u16*>(QuadIB.Map());
    int Cnt = 0;
    int ICnt = 0;
    for (int i = 0; i < triCount; i++)
    {
        Indices[ICnt++] = u16(Cnt + 0);
        Indices[ICnt++] = u16(Cnt + 1);
        Indices[ICnt++] = u16(Cnt + 2);
        Indices[ICnt++] = u16(Cnt + 3);
        Indices[ICnt++] = u16(Cnt + 2);
        Indices[ICnt++] = u16(Cnt + 1);
        Cnt += 4;
    }
    QuadIB.Unmap(true);
}
}
namespace xray::render { void InitializeImGuiRenderer(fg::RenderDevice* renderDevice); void ShutdownImGuiRenderer(); }

extern ENGINE_API float psHUD_FOV;
extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API float ps_r_rt_gi_intensity;
extern ENGINE_API int ps_r_path_tracer;
extern ENGINE_API int ps_r_path_tracer_bounces;
extern ENGINE_API int ps_r_taa;
extern ENGINE_API int ps_r_bloom;
extern ENGINE_API int ps_r_cas;
extern ENGINE_API int ps_r_ssr;
extern ENGINE_API int ps_r_ssgi;
extern ENGINE_API int ps_r_sssss;
extern ENGINE_API int ps_r_contact_shadows;
extern ENGINE_API int ps_r_upscale;
extern ENGINE_API int ps_r_denoise;
extern ENGINE_API int ps_r_nrd_method;
extern ENGINE_API int ps_r_nrd_apply;
extern ENGINE_API int ps_r_dlss;
extern ENGINE_API int ps_r_dlss_rr;
extern ENGINE_API int ps_r_dlss_fg;

namespace xray::render {

using namespace fg;

bool FrameGraphRenderer::IsRTGIActive() const
{
    return passes::IsRTGIActive(m_rtAccelMgr.get());
}

static fg::UpscaleState g_upscaleState;
static xr_unique_ptr<fg::IUpscaleBackend> g_upscaleBackend;
static xr_unique_ptr<fg::IDenoiseBackend> g_denoiseBackend;

// Forward declaration and extern for accessing RImplementation
namespace fg {
    extern xray::render::FrameGraphRenderer RImplementation;
}

static Fmatrix BuildHUDFOVMatrix()
{
    const float fovScale = 1.0f / psHUD_FOV;
    Fmatrix invView;
    invView.invert(Device.mView);

    Fmatrix fovScaleMat;
    fovScaleMat.identity();
    fovScaleMat._11 = fovScale;
    fovScaleMat._22 = fovScale;

    Fmatrix t1, result;
    t1.mul(fovScaleMat, Device.mView);
    result.mul(invView, t1);
    return result;
}

FrameGraphRenderer::FrameGraphRenderer() {
    // Msg("* [FrameGraphRenderer] Created");
}

FrameGraphRenderer::~FrameGraphRenderer() {
    Shutdown();
}

bool FrameGraphRenderer::Initialize(fg::RenderDevice* device) {
    VERIFY(device != nullptr);
    m_device = device;

    Msg("* [FrameGraphRenderer] Initializing...");

    m_shaderLoader = xr_new<framegraph::ShaderLoader>(device->GetSlangCompiler());
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
        m_shaderLoader->SetTarget(SlangCompiler::Target::SPIRV);
    else
        m_shaderLoader->SetTarget(SlangCompiler::Target::DXIL);
    Msg("* [FrameGraphRenderer] ShaderLoader initialized (target: %s)",
        m_shaderLoader->GetTarget() == SlangCompiler::Target::SPIRV ? "SPIRV" : "DXIL");

    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device);
    m_shaderPhaseCache = xr_make_unique<framegraph::ShaderPhaseCache>();
    m_geometryVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_geometryVCBPool.get()
    );
    m_uiVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_uiMaterialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_uiVCBPool.get()
    );
    m_uiRender = xr_make_unique<fg::FGUIRender>();
    m_geometryCollector = xr_make_unique<GeometryCollector>();
    g_geometryCollector = m_geometryCollector.get();


    m_gpuCullingManager = xr_make_unique<fg::GPUCullingManager>();
    m_detailManager = xr_make_unique<fg::FGDetailManager>();
    m_decalManager = xr_make_unique<fg::decals::DecalManager>();
    m_overlayManager = xr_make_unique<fg::decals::OverlayManager>();
    m_rtAccelMgr = xr_make_unique<fg::RTAccelStructManager>();
    m_smokeTrailManager = xr_make_unique<fg::passes::SmokeTrailManager>();
    m_volumetricRenderer = xr_make_unique<fg::VolumetricRenderer>();


    bindless::MaterialBuffer::Instance().Initialize(m_device);
    bindless::TerrainMaterialBuffer::Instance().Initialize(m_device);
    bindless::VariantTextureBuffer::Instance().Initialize(m_device);
    bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);
    fg::ClusteredLightManager::Instance().Initialize(m_device);
    Msg("* [FrameGraphRenderer] Bindless material buffers initialized (early)");

    m_uiRender->Initialize(device, m_uiMaterialCache.get());
    m_decalManager->Initialize(device);
    m_overlayManager->Initialize(device);
    m_rtAccelMgr->Initialize(device);
    m_smokeTrailManager->Initialize(device);
    m_volumetricRenderer->Initialize(device);

    m_gpuCullingManager->Initialize(m_device);
    if (m_rtAccelMgr && m_rtAccelMgr->IsSupported())
        m_gpuCullingManager->SetRTAccelStructManager(m_rtAccelMgr.get());

    // Create RenderContext for execution
    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    m_blackboard = xr_make_unique<framegraph::Blackboard>();
    if (m_volumetricRenderer->IsReady())
    {
        passes::InitializeVolumetricPass(
            device->GetNVRHIDevice(),
            m_blackboard->get_or_add<passes::VolumetricPassState>());
    }
    m_gpuProfiler = xr_make_unique<xray::profiler::GPUProfiler>();
    m_statsOverlay = xr_make_unique<xray::profiler::StatsOverlay>();
    
    m_gpuProfiler->Initialize(device->GetNVRHIDevice());
    m_statsOverlay->SetGPUProfiler(m_gpuProfiler.get());
    
    Msg("* [FrameGraphRenderer] Profiler initialized");

    {
        nvrhi::TextureDesc previewDesc;
        previewDesc.debugName = "InspectorPreview";
        previewDesc.width = 512;
        previewDesc.height = 512;
        previewDesc.format = nvrhi::Format::RGBA16_FLOAT;
        previewDesc.isUAV = true;
        previewDesc.isRenderTarget = false;
        previewDesc.keepInitialState = true;
        previewDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        m_inspectorPreview = device->GetNVRHIDevice()->createTexture(previewDesc);
    }

    m_PSLibrary.OnCreate();
    m_HWOCC.occq_create(occq_size);

    Msg("* [FrameGraphRenderer] initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    m_HWOCC.occq_destroy();
    m_PSLibrary.OnDestroy();

    if (m_shaderLoader) {
        xr_delete(m_shaderLoader);
        Msg("* [FrameGraphRenderer] ShaderLoader destroyed");
    }

    g_geometryCollector = nullptr;
    m_statsOverlay = nullptr;
    if (m_gpuProfiler) {
        m_gpuProfiler->Shutdown();
        m_gpuProfiler = nullptr;
    }

    m_inspectorPreview = nullptr;
    m_renderContext = nullptr;
    m_geometryCollector = nullptr;
    m_geometryVCBPool = nullptr;
    m_materialCache = nullptr;
    m_uiRender = nullptr;
    m_uiMaterialCache = nullptr;
    m_uiVCBPool = nullptr;
    m_cachedStaticBatches.clear();
    m_sectorStaticBatchIds.clear();
    m_sectorCacheReady.clear();
    m_visualCacheBatchIds.clear();
    m_staticCacheInitialized = false;
    m_portalTraverseActive = false;
    if (m_gpuCullingManager) {
        m_gpuCullingManager->InvalidateStaticCullingData();
        m_gpuCullingManager = nullptr;
    }
    m_detailManager = nullptr;

    if (m_decalManager) {
        m_decalManager->Shutdown();
        m_decalManager = nullptr;
    }

    if (m_overlayManager) {
        m_overlayManager->Shutdown();
        m_overlayManager = nullptr;
    }

    if (m_rtAccelMgr) {
        m_rtAccelMgr->Shutdown();
        m_rtAccelMgr = nullptr;
    }

    if (m_smokeTrailManager) {
        m_smokeTrailManager->Shutdown();
        m_smokeTrailManager = nullptr;
    }

    if (m_volumetricRenderer) {
        m_volumetricRenderer->Shutdown();
        m_volumetricRenderer = nullptr;
    }

    fg::ClusteredLightManager::Instance().Shutdown();

    passes::ShutdownPathTracer();

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    if (m_blackboard) {
        if (auto* tonemap = m_blackboard->try_get<passes::TonemapPassState>())
            passes::ShutdownTonemapPass(*tonemap);
        if (auto* vol = m_blackboard->try_get<passes::VolumetricPassState>())
            passes::ShutdownVolumetricPass(*vol);
        if (auto* shadow = m_blackboard->try_get<passes::ShadowPassState>())
            passes::ShutdownShadowPass(m_device, *shadow);
        if (auto* localShadow = m_blackboard->try_get<passes::LocalShadowPassState>())
            passes::ShutdownLocalShadowPass(m_device, *localShadow);
        if (auto* taa = m_blackboard->try_get<passes::TAAPassState>())
            passes::ShutdownTAAPass(*taa);
        m_blackboard.reset();
    }

    m_prevFrameDepth = nullptr;
    m_normals[0] = nullptr;
    m_normals[1] = nullptr;
    m_worldPos[0] = nullptr;
    m_worldPos[1] = nullptr;
    m_inspectorPreview = nullptr;
    old_QuadIB = nullptr;
}

void FrameGraphRenderer::Render() {
    ZoneScopedN("FrameGraphRenderer::Render");

    if (!m_enabled) return;
    if (g_pGamePersistent &&
        (g_pGamePersistent->MainMenuActiveOrLevelNotExist() || g_pGamePersistent->IsLoadingScreenShown()))
    {
        RenderMenu();
        return;
    }

    VERIFY(m_framegraph != nullptr);

    if (m_gpuProfiler)
    {
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }

    auto frameStart = std::chrono::high_resolution_clock::now();
    
    Lights.Update();

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAME (PER-FRAME: Collect geometry)
    // ═══════════════════════════════════════════════════════

    {
        ZoneScopedN("FG::SetupFrame");
        SetupFrame();
    }

    // ═══════════════════════════════════════════════════════
    //  RESET FRAMEGRAPH FOR NEW FRAME
    // ═══════════════════════════════════════════════════════
    m_framegraph->ResetForNextFrame();

    // Shader hot-reload check (throttled to avoid per-frame filesystem polling)
    if (ps_fg_hot_reload_shaders)
    {
        static u32 frameCounter = 0;
        if (++frameCounter >= 60)
        {
            frameCounter = 0;
            if (GEnv.Render->GetShaderLoader() && GEnv.Render->GetShaderLoader()->CheckForChangedFiles())
            {
                Msg("* Shader hot-reload detected, validating changed shaders...");
                if (!GEnv.Render->GetShaderLoader()->ValidateChangedFiles())
                {
                    Msg("! Shader hot-reload skipped: validation failed, keeping current shaders");
                }
                else
                {
                    Msg("* Hot-reloading shaders...");

                    // Ensure no in-flight work references old pipelines/shaders.
                    m_device->GetNVRHIDevice()->waitForIdle();

                    GEnv.Render->GetShaderLoader()->ClearAllCaches();
                    framegraph::GetPassResourceCache().Clear();
                    framegraph::BindingSetBuilder::InvalidateReflectionCache();
                    if (m_blackboard)
                    {
                        m_blackboard->clear();
                    }

                    if (m_detailManager)
                        m_detailManager->InvalidateShadersAndPipelines();

                    if (m_gpuCullingManager)
                        m_gpuCullingManager->InvalidateShadersAndPipelines();

                    if (m_rtAccelMgr)
                        fg::RTAccelStructManager::InvalidateShaderPipelines();

                    fg::ImGuiRendererNVRHI* imguiReload = GEnv.Render->GetImGuiRendererNVRHI();
                    if (imguiReload)
                        imguiReload->InvalidateShadersAndPipeline();

                    Msg("* Shader hot-reload complete");
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  SETUP PASSES (PER-FRAME: Route geometry to passes)
    // ═══════════════════════════════════════════════════════
    {
        ZoneScopedN("FG::SetupPasses");
        auto t0 = std::chrono::high_resolution_clock::now();
        SetupFrameGraphPasses();
        m_stats.fgSetupPassesMs = std::chrono::duration<float, std::milli>(
            std::chrono::high_resolution_clock::now() - t0).count();
    }

    // ═══════════════════════════════════════════════════════
    //  COMPILE & EXECUTE
    // ═══════════════════════════════════════════════════════
    m_renderContext->SetCommandList(GEnv.Backend->GetCommandList());
    m_framegraph->SetRenderContext(m_renderContext.get());
    m_framegraph->SetGPUProfiler(m_gpuProfiler.get());
    
    // Wire async compute (Vulkan only for now — D3D12 triggers device removed)
    if (ps_fg_render_mode == FG_RENDER_VULKAN && GEnv.Backend->HasAsyncCompute())
        m_framegraph->SetAsyncCompute(GEnv.Backend->GetComputeCommandList(), GEnv.Backend);
    else
        m_framegraph->SetAsyncCompute(nullptr, nullptr);

    {
        ZoneScopedN("FG::Compile");
        auto t0 = std::chrono::high_resolution_clock::now();
        m_framegraph->Compile();
        m_stats.fgCompileMs = std::chrono::duration<float, std::milli>(
            std::chrono::high_resolution_clock::now() - t0).count();
    }

    auto& cache = framegraph::GetPassResourceCache();
    auto* cmdList = m_renderContext->GetCommandList();
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(passes::StaticGlobals), m_device);
    auto staticGlobalsData = passes::BuildStaticGlobals();

    auto& clm = fg::ClusteredLightManager::Instance();
    if (clm.IsReady() && clm.GetLightCount() > 0) {
        float zNear = VIEWPORT_NEAR;
        float zFar = g_pGamePersistent->Environment().CurrentEnv.far_plane;
        auto ccb = clm.BuildClusterCB(Device.dwWidth, Device.dwHeight, zNear, zFar);
        staticGlobalsData.cluster_params.set(ccb.gridDims.x, ccb.gridDims.y, ccb.gridDims.z, ccb.gridDims.w);
        staticGlobalsData.cluster_scales.set(ccb.depthParams.x, ccb.depthParams.y, ccb.depthParams.z, ccb.depthParams.w);
    }

    cmdList->writeBuffer(staticGlobalsCB, &staticGlobalsData, sizeof(staticGlobalsData));

    auto dynamicTransformsCB = cache.GetOrCreateVolatileCB("Frame", "DynamicTransforms",
        sizeof(passes::DynamicTransforms), m_device);
    passes::DynamicTransforms dynamicTransformsData = {};
    passes::FillDynamicTransforms(dynamicTransformsData);
    cmdList->writeBuffer(dynamicTransformsCB, &dynamicTransformsData, sizeof(dynamicTransformsData));

    {
        ZoneScopedN("FG::Execute");
        m_framegraph->Execute();
    }

    if (m_gpuCullingManager && psDeviceFlags.test(rsStatistic))
    {
        m_gpuCullingManager->ScheduleStatsReadback(m_renderContext->GetCommandList());
    }

    m_hasPrevFrameData = true;
    m_prevViewProj = passes::g_taa_unjittered_full_transform;
    m_prevCameraPos = Device.vCameraPosition;
    m_pingPongIndex = 1 - m_pingPongIndex;

    if (m_gpuProfiler)
    {
        m_gpuProfiler->FrameEnd();
    }

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_stats.totalFrameMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart
    ).count();

    // Old pass statistics (disabled - passes are now lambda-based)
    // TODO: Get statistics from FrameGraph itself
    m_stats.gbufferMs = 0.0f;
    m_stats.lightingMs = 0.0f;
    m_stats.tonemapMs = 0.0f;
    m_stats.numDrawCalls = 0;
    m_stats.numTriangles = 0;
}

void FrameGraphRenderer::RenderMenu() {
    ZoneScopedN("FrameGraphRenderer::RenderMenu");

    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    if (m_gpuProfiler)
    {
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }
    
    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }
    
    m_framegraph->ResetForNextFrame();

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    nvrhi::ITexture* backbufferTexture = GEnv.Backend->GetBackBuffer();
    framegraph::VirtualResourceHandle backbufferHandle;

    if (backbufferTexture) {
        framegraph::ResourceDesc backbufferDesc;
        backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        backbufferDesc.width = width;
        backbufferDesc.height = height;
        backbufferDesc.format = backbufferTexture->getDesc().format;
        backbufferDesc.isRenderTarget = true;
        backbufferDesc.isImported = true;
        backbufferDesc.isTransient = false;
        backbufferDesc.debugName = "Backbuffer";

        backbufferHandle = m_framegraph->ImportTexture("Backbuffer", backbufferTexture, backbufferDesc);
    }

    if (!backbufferHandle.is_valid())
        return;

    // Menu / loading-only path: draw UI straight to backbuffer (no ACES/post)
    framegraph::PassHandle clearPass = m_framegraph->AddPass("ClearBackground");
    m_framegraph->PassWrite(clearPass, backbufferHandle, framegraph::ResourceState::RenderTarget);
    m_framegraph->SetPassCallback(clearPass,
        [backbufferHandle](fg::RenderContext& ctx, const framegraph::FrameGraph& fg) {
            auto* bb = fg.GetPhysicalTexture(backbufferHandle);
            if (bb) {
                nvrhi::ICommandList* cmdList = ctx.GetCommandList();
                cmdList->clearTextureFloat(bb, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            }
        }
    );

    auto sceneWithUI = passes::setupUIPass(*m_framegraph, backbufferHandle, width, height);
    sceneWithUI = passes::setupFontPass(*m_framegraph, sceneWithUI);
    sceneWithUI = passes::setupCursorPass(*m_framegraph, sceneWithUI, width, height);
    sceneWithUI = passes::setupDebugDrawPass(*m_framegraph, sceneWithUI, width, height);

    fg::ImGuiRendererNVRHI* imguiRenderer = GEnv.Render->GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        sceneWithUI,
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

    m_renderContext->SetCommandList(GEnv.Backend->GetCommandList());
    m_framegraph->SetRenderContext(m_renderContext.get());
    m_framegraph->SetGPUProfiler(m_gpuProfiler.get());
    m_framegraph->SetAsyncCompute(nullptr, nullptr);
    m_framegraph->Compile();
    m_framegraph->Execute();

    if (m_gpuProfiler)
        m_gpuProfiler->FrameEnd();
}

void FrameGraphRenderer::RenderStatsOverlay()
{
    if (m_statsOverlay && psDeviceFlags.test(rsStatistic))
    {
        xray::profiler::RenderStats stats;
        stats.Reset();

        if (m_geometryCollector)
        {
            const auto& batches = m_geometryCollector->GetBatches();
            stats.totalBatches = static_cast<u32>(batches.size());

            xr_set<IRenderVisual*> uniqueSkeletons;

            for (const auto& batch : batches)
            {
                u32 triangles = batch.indexCount / 3;
                stats.totalTriangles += triangles;

                if (batch.isSkinned)
                {
                    stats.skinnedBatches++;
                    stats.skinnedTriangles += triangles;

                    if (batch.renderable)
                    {
                        IRenderVisual* rootVisual = batch.renderable->GetRenderData().visual;
                        if (rootVisual && uniqueSkeletons.find(rootVisual) == uniqueSkeletons.end())
                        {
                            uniqueSkeletons.insert(rootVisual);
                            stats.skinnedMeshes++;

                            // Get bone count from kinematics
                            IKinematics* K = rootVisual->dcast_PKinematics();
                            if (K)
                            {
                                u32 boneCount = K->LL_BoneCount();
                                stats.totalBones += boneCount;
                                if (boneCount > stats.maxBonesPerMesh)
                                    stats.maxBonesPerMesh = boneCount;
                            }
                        }
                    }
                }
                else if (batch.isTerrain)
                {
                    stats.terrainBatches++;
                    stats.terrainTriangles += triangles;
                }
                else if (batch.isStatic)
                {
                    stats.staticBatches++;
                    stats.staticTriangles += triangles;
                }
                else
                {
                    stats.dynamicBatches++;
                    stats.dynamicTriangles += triangles;
                }
            }
        }

        // Collect particle stats
        stats.particleBatches = static_cast<u32>(m_worldParticleBatches.size() + m_hudParticleBatches.size());

        if (m_blackboard)
        {
            const auto& particleCull = m_blackboard->get_or_add<passes::ParticlePassState>().cullStats;
            if (particleCull.active)
            {
                stats.particleCullSubmitted = particleCull.submittedBatches;
                stats.particleCullVisible = particleCull.visibleBatches;
                stats.particleQuadsSubmitted = particleCull.submittedQuads;
                stats.particleQuadsVisible = particleCull.visibleQuads;
            }
        }

        // Collect GPU culling stats
        if (m_gpuCullingManager)
        {
            stats.objectsSubmitted = m_gpuCullingManager->GetStaticObjectCount() +
                                     m_gpuCullingManager->GetDynamicObjectCount() +
                                     m_gpuCullingManager->GetTerrainObjectCount();

            // Use readback data from previous frame (1-frame latency)
            const auto& cullStats = m_gpuCullingManager->GetCullingStats();
            stats.objectsVisible = cullStats.totalVisible();
            stats.objectsCulled = (stats.objectsSubmitted > stats.objectsVisible)
                                  ? (stats.objectsSubmitted - stats.objectsVisible)
                                  : 0;

            // Mega-buffer stats
            stats.megaBufferVertices = m_gpuCullingManager->GetTotalVertexCount();
            stats.megaBufferIndices = m_gpuCullingManager->GetTotalIndexCount();

            // Skinned Hi-Z culling stats
            const auto& skinnedCullStats = m_gpuCullingManager->GetSkinnedCullingStats();
            stats.skinnedSubmitted = skinnedCullStats.submitted;
            stats.skinnedVisible = skinnedCullStats.visible;
            stats.skinnedCulled = skinnedCullStats.culled;
        }

        // Collect detail/grass stats
        {
            auto& clmStats = fg::ClusteredLightManager::Instance();
            stats.lightsClustered = clmStats.GetLightCount();
            stats.lightsPoint = clmStats.GetPointCount();
            stats.lightsSpot = clmStats.GetSpotCount();
            stats.lightsOmni = clmStats.GetOmniCount();
            u32 visCount = clmStats.GetVisibleLightCount();
            stats.lightsHiZVisible = (visCount > 0) ? visCount : stats.lightsClustered;
            stats.localShadowTiles = static_cast<u32>(clmStats.GetLocalShadowTiles().size());
            stats.localShadowCandidates = clmStats.GetLocalShadowCandidateCount();
            stats.localShadowDropped = clmStats.GetLocalShadowDroppedCount();
            stats.localShadowRedraw = clmStats.GetLocalShadowRedrawCount();
        }

        stats.mdiDrawCalls = passes::GetMdiDrawCallCount();
        stats.mdiMaxDrawCountSum = passes::GetMdiMaxDrawCountSum();

        if (m_detailManager)
        {
            stats.detailSlots = m_detailManager->slot_count;
            for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++)
                stats.detailTrisPerBlade[lod] = m_detailManager->bladeIndexCount[lod] / 3;

            const auto& cullStats = m_detailManager->GetCullingStats();
            stats.detailVisibleSlots = cullStats.visibleSlotsCount;
            stats.detailVisibleLOD0 = cullStats.visibleLOD0Count;
            stats.detailVisibleLOD1 = cullStats.visibleLOD1Count;
            stats.detailVisibleLOD2 = cullStats.visibleLOD2Count;
            stats.detailVisibleDecals = cullStats.visibleDecalCount;
            stats.detailGeneratedInstances = m_detailManager->totalGeneratedInstances;
            stats.detailVisibleCapacity = m_detailManager->visibleBufferCapacity;
            stats.detailDecalCapacity = std::max(m_detailManager->visibleBufferCapacity / 4, 10000u);
        }

        if (m_framegraph) {
            const auto& arenaStats = m_framegraph->GetFrameArenaStats();
            stats.fgArenaUsed = arenaStats.lastUsed;
            stats.fgArenaPeak = arenaStats.peak;
            stats.fgArenaCapacity = arenaStats.capacity;
            stats.fgArenaFallbacks = arenaStats.lastFallbackAllocs;
        }
        stats.fgSetupPassesMs = m_stats.fgSetupPassesMs;
        stats.fgCompileMs = m_stats.fgCompileMs;

        m_statsOverlay->SetRenderStats(stats);
        m_statsOverlay->SetInspectorPreview(m_inspectorPreview ? m_inspectorPreview.Get() : nullptr);

        if (m_overlayManager)
        {
            xr_vector<xray::profiler::StatsOverlay::WallmarkObjectData> wmData;
            for (const auto& [obj, data] : m_overlayManager->GetDebugObjects())
            {
                xray::profiler::StatsOverlay::WallmarkObjectData od;
                od.objKey = obj;

                xr_map<xr_string, int> groupIndex;
                xr_map<xr_string, nvrhi::ITexture*> texCache;
                auto resolveTexture = [&](const xr_string& texName) -> nvrhi::ITexture*
                {
                    if (!m_materialCache || texName.empty())
                        return nullptr;

                    auto it = texCache.find(texName);
                    if (it != texCache.end())
                        return it->second;

                    nvrhi::ITexture* tex = m_materialCache->GetNVRHITextureByName(texName.c_str());
                    texCache[texName] = tex;
                    return tex;
                };

                for (u32 i = 0; i < (u32)data.splats.size(); i++)
                {
                    const auto& gpu   = data.splats[i];
                    const auto& dbg   = data.splatDebug[i];
                    const xr_string& targetTexName = dbg.targetTextureName;

                    auto git = groupIndex.find(targetTexName);
                    if (git == groupIndex.end())
                    {
                        xray::profiler::StatsOverlay::WallmarkTexGroup g;
                        g.texName = targetTexName;
                        g.diffuseTex = resolveTexture(targetTexName);
                        groupIndex[targetTexName] = (int)od.groups.size();
                        od.groups.push_back(std::move(g));
                        git = groupIndex.find(targetTexName);
                    }

                    nvrhi::ITexture* stampTex = resolveTexture(dbg.wallmarkTextureName);
                    u32 mode = (u32)(gpu.evolution.w + 0.5f);
                    od.groups[git->second].splats.push_back(
                        { dbg.uv.x, dbg.uv.y, gpu.uvRadius,
                          gpu.color.x, gpu.color.y, gpu.color.z, gpu.color.w,
                          mode, gpu.evolution.z,
                          stampTex });
                }

                wmData.push_back(std::move(od));
            }
            m_statsOverlay->SetWallmarkData(std::move(wmData));
        }

        m_statsOverlay->SetVisible(true);
        m_statsOverlay->Render();
    }
}

void FrameGraphRenderer::SetupFrame() {
    const bool levelLoaded = g_pGamePersistent && g_pGameLevel;

    if (m_gpuCullingManager) {
        {
            ZoneScopedN("Readback::CullStats");
            m_gpuCullingManager->ProcessStatsReadback();
        }
        m_gpuCullingManager->BeginSkinnedFrame();
    }

    if (m_detailManager && m_device) {
        ZoneScopedN("Readback::DetailStats");
        m_detailManager->ProcessStatsReadback(m_device->GetNVRHIDevice());
    }

    if (psDeviceFlags.test(rsStatistic)) {
        ZoneScopedN("Readback::LightStats");
        fg::ClusteredLightManager::Instance().ProcessStatsReadback();
    }

    m_lstRenderables.clear();

    if (levelLoaded)
    {
        if (levelLoaded && !g_pGamePersistent->IsLoadingScreenShown())
        {
            ZoneScopedN("SetupFrame::FrustumQuery");

            CFrustum view_frustum;
            view_frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

            // ref: src/Layers/xrRender_R2/r2_R_calculate.cpp lines 54-58
            u32 spatial_traverse_flags = ISpatial_DB::O_ORDERED;  // Front-to-back ordering
            u32 spatial_types = STYPE_RENDERABLE | STYPE_LIGHTSOURCE;  // Both renderables AND lights

            g_pGamePersistent->SpatialSpace.q_frustum(
                m_lstRenderables,
                spatial_traverse_flags,
                spatial_types,
                view_frustum
            );
        }
    }

    {
        ZoneScopedN("SetupFrame::CollectorBegin");
        m_geometryCollector->BeginFrame();
        m_hudBatches.clear();
        m_lodImpostors.clear();
        m_worldParticleBatches.clear();
        m_hudParticleBatches.clear();

        fg::ClusteredLightManager::Instance().BeginFrame();
    }

    if (levelLoaded && !g_pGamePersistent->IsLoadingScreenShown()) {
        ZoneScopedN("SetupFrame::CollectVisibleGeometry");
        CollectVisibleGeometry();
    }

    {
        ZoneScopedN("SetupFrame::CollectorEnd");
        m_geometryCollector->Sort(); // opaque → aref → transparent (front-to-back)
        m_geometryCollector->EndFrame();
    }
}

framegraph::VirtualResourceHandle FrameGraphRenderer::CreateRT(
    const char* name,
    u32 width,
    u32 height,
    nvrhi::Format format,
    bool isDepthStencil)
{
    framegraph::ResourceDesc desc;
    desc.type = framegraph::ResourceDesc::Type::Texture2D;
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.isRenderTarget = !isDepthStencil;
    desc.isDepthStencil = isDepthStencil;
    desc.isTransient = true;
    desc.debugName = name;

    return m_framegraph->CreateTexture(name, desc);
}

void FrameGraphRenderer::SetupFrameGraphPasses() {
    // Subpixel Halton jitter for TAA (must run after camera projection is final)
    passes::ApplyTAAJitter();
    passes::ResetMdiDrawCounters();

    if ((ps_r_upscale != 0 || ps_r_dlss != 0) && !g_upscaleBackend)
        g_upscaleBackend.reset(fg::CreateUpscaleBackendAuto());

    {
        static int s_denoiseCvar = -1;
        static int s_nrdMethodCvar = -1;
        static int s_dlssRrCvar = -1;
        static int s_upscaleCvar = -1;
        static int s_rrAvail = -1;
        static u32 s_denoiseW = 0;
        static u32 s_denoiseH = 0;
        UpdateUpscaleState(g_upscaleState, Device.dwWidth, Device.dwHeight);
        const u32 denoiseW = g_upscaleState.renderWidth ? g_upscaleState.renderWidth : Device.dwWidth;
        const u32 denoiseH = g_upscaleState.renderHeight ? g_upscaleState.renderHeight : Device.dwHeight;
        const int rrAvail = fg::Streamline_IsRRAvailable() ? 1 : 0;
        const bool denoiseDirty =
            s_denoiseCvar != ps_r_denoise ||
            s_nrdMethodCvar != ps_r_nrd_method ||
            s_dlssRrCvar != ps_r_dlss_rr ||
            s_upscaleCvar != ps_r_upscale ||
            s_rrAvail != rrAvail ||
            (g_denoiseBackend && (s_denoiseW != denoiseW || s_denoiseH != denoiseH));
        if (denoiseDirty && g_denoiseBackend) {
            g_denoiseBackend->Shutdown();
            g_denoiseBackend.reset();
        }
        if (ps_r_denoise != 0 && !g_denoiseBackend)
            g_denoiseBackend.reset(fg::CreateDenoiseBackendAuto(ps_r_dlss_rr != 0 && ps_r_upscale == 2));
        if (g_denoiseBackend && g_denoiseBackend->IsAvailable())
            g_denoiseBackend->Resize(denoiseW, denoiseH);
        s_denoiseCvar = ps_r_denoise;
        s_nrdMethodCvar = ps_r_nrd_method;
        s_dlssRrCvar = ps_r_dlss_rr;
        s_upscaleCvar = ps_r_upscale;
        s_rrAvail = rrAvail;
        s_denoiseW = denoiseW;
        s_denoiseH = denoiseH;
    }

    // Recompute after backend auto-fallback may clear r_upscale
    UpdateUpscaleState(g_upscaleState, Device.dwWidth, Device.dwHeight);
    g_upscaleState.jitterX = passes::g_taa_jitter_px;
    g_upscaleState.jitterY = passes::g_taa_jitter_py;
    g_upscaleState.prevJitterX = passes::g_taa_jitter_prev_px;
    g_upscaleState.prevJitterY = passes::g_taa_jitter_prev_py;
    g_upscaleState.resetHistory = !m_hasPrevFrameData;

    const u32 displayWidth = g_upscaleState.displayWidth;
    const u32 displayHeight = g_upscaleState.displayHeight;
    const u32 width = g_upscaleState.renderWidth;
    const u32 height = g_upscaleState.renderHeight;
    const bool rtLightingWanted =
        (ps_r_rt_gi || ps_r_path_tracer) && m_rtAccelMgr && m_rtAccelMgr->IsSupported();
    const bool rtgiActive = IsRTGIActive();
    const bool wantSunShaftCsm = ps_r_sun_shafts != 0;
    if (rtLightingWanted)
    {
        auto& localShadow = m_blackboard->get_or_add<passes::LocalShadowPassState>();
        if (localShadow.initialized)
            passes::ShutdownLocalShadowPass(m_device, localShadow);
        if (!wantSunShaftCsm)
        {
            auto& shadowState = m_blackboard->get_or_add<passes::ShadowPassState>();
            if (shadowState.initialized)
                passes::ShutdownShadowPass(m_device, shadowState);
        }
    }
    const bool upscaleResolved = g_upscaleState.upscaleActive &&
        g_upscaleBackend && g_upscaleBackend->IsAvailable();

    nvrhi::ITexture* backbufferTexture = GEnv.Backend->GetBackBuffer();
    framegraph::VirtualResourceHandle backbufferHandle;

    if (backbufferTexture) {
        framegraph::ResourceDesc backbufferDesc;
        backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        backbufferDesc.width = displayWidth;
        backbufferDesc.height = displayHeight;
        backbufferDesc.format = backbufferTexture->getDesc().format;
        backbufferDesc.isRenderTarget = true;
        backbufferDesc.isImported = true;
        backbufferDesc.isTransient = false;  // External resource - don't manage lifetime
        backbufferDesc.debugName = "Backbuffer";

        backbufferHandle = m_framegraph->ImportTexture("Backbuffer", backbufferTexture, backbufferDesc);
    }

    // ═══════════════════════════════════════════════════════
    //  TEMPORAL HI-Z (No Depth Prepass)
    // ═══════════════════════════════════════════════════════
    framegraph::ResourceDesc depthDesc;
    depthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    depthDesc.debugName = "rt_Depth";
    depthDesc.width = width;
    depthDesc.height = height;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;

    framegraph::VirtualResourceHandle depthBuffer = m_framegraph->CreateTexture("rt_Depth", depthDesc);

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    u32 writeIdx = m_pingPongIndex;
    u32 readIdx = 1 - m_pingPongIndex;

    if (!m_normals[0] || m_prevFrameWidth != width || m_prevFrameHeight != height) {
        nvrhi::TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isShaderResource = true;
        desc.isRenderTarget = true;
        desc.initialState = nvrhi::ResourceStates::RenderTarget;
        desc.keepInitialState = true;
        for (int i = 0; i < 2; i++) {
            desc.debugName = (i == 0) ? "Normals_A" : "Normals_B";
            m_normals[i] = nvDevice->createTexture(desc);
        }
    }

    if (!m_worldPos[0] || m_prevFrameWidth != width || m_prevFrameHeight != height) {
        nvrhi::TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = nvrhi::Format::RGBA32_FLOAT;
        desc.isShaderResource = true;
        desc.isRenderTarget = true;
        desc.initialState = nvrhi::ResourceStates::RenderTarget;
        desc.keepInitialState = true;
        for (int i = 0; i < 2; i++) {
            desc.debugName = (i == 0) ? "WorldPos_A" : "WorldPos_B";
            m_worldPos[i] = nvDevice->createTexture(desc);
        }
    }

    framegraph::ResourceDesc normalImportDesc;
    normalImportDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    normalImportDesc.width = width;
    normalImportDesc.height = height;
    normalImportDesc.format = nvrhi::Format::RGBA16_FLOAT;
    normalImportDesc.isRenderTarget = true;
    normalImportDesc.isImported = true;
    normalImportDesc.isTransient = false;
    normalImportDesc.debugName = "rt_Normal";
    framegraph::VirtualResourceHandle normalBuffer = m_framegraph->ImportTexture("rt_Normal", m_normals[writeIdx], normalImportDesc);

    framegraph::ResourceDesc baseColorDesc;
    baseColorDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    baseColorDesc.debugName = "rt_BaseColor";
    baseColorDesc.width = width;
    baseColorDesc.height = height;
    baseColorDesc.format = nvrhi::Format::RGBA8_UNORM;
    baseColorDesc.isRenderTarget = true;
    baseColorDesc.isTransient = true;
    framegraph::VirtualResourceHandle baseColorBuffer = m_framegraph->CreateTexture("rt_BaseColor", baseColorDesc);

    framegraph::ResourceDesc worldPosImportDesc;
    worldPosImportDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    worldPosImportDesc.width = width;
    worldPosImportDesc.height = height;
    worldPosImportDesc.format = nvrhi::Format::RGBA32_FLOAT;
    worldPosImportDesc.isRenderTarget = true;
    worldPosImportDesc.isImported = true;
    worldPosImportDesc.isTransient = false;
    worldPosImportDesc.debugName = "rt_WorldPos";
    framegraph::VirtualResourceHandle worldPosBuffer = m_framegraph->ImportTexture("rt_WorldPos", m_worldPos[writeIdx], worldPosImportDesc);

    // ═══════════════════════════════════════════════════════
    //  HI-Z + GPU CULL
    //  Linux + temporal depth: one cull with prev-frame Hi-Z; depth prepass
    //    still fills early-Z; same-frame Hi-Z for consumers; no cull#2.
    //  Otherwise (or no prev depth): frustum cull → depth → same-frame Hi-Z →
    //    occlusion compact when r_hiz_occlusion.
    //  Detail/skinned/particle never use Hi-Z occlusion (flicker / self-occ).
    // ═══════════════════════════════════════════════════════
    const bool useDepthPrepass = ps_r_depth_prepass != 0;
    const bool useHizOcclusion = ps_r_hiz_occlusion != 0;

    passes::HiZPyramidOutput hizOutput;
    hizOutput.pyramid = framegraph::VirtualResourceHandle();
    hizOutput.mipLevels = 0;
    hizOutput.width = width / 2;
    hizOutput.height = height / 2;

    bool hasPrevDepth = m_hasPrevFrameData && m_prevFrameDepth &&
                        m_prevFrameWidth == width && m_prevFrameHeight == height;

#if !defined(XR_PLATFORM_APPLE)
    const bool useTemporalHizCull = hasPrevDepth && (!useDepthPrepass || useHizOcclusion);
#else
    const bool useTemporalHizCull = hasPrevDepth && !useDepthPrepass;
#endif

    if (useTemporalHizCull) {
        framegraph::ResourceDesc prevDepthDesc;
        prevDepthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevDepthDesc.debugName = "rt_PrevDepth";
        prevDepthDesc.width = width;
        prevDepthDesc.height = height;
        prevDepthDesc.format = nvrhi::Format::D32;
        prevDepthDesc.isDepthStencil = true;
        prevDepthDesc.isImported = true;
        prevDepthDesc.isTransient = false;

        auto prevDepthHandle = m_framegraph->ImportTexture("rt_PrevDepth", m_prevFrameDepth, prevDepthDesc);

        hizOutput = passes::setupHiZBuildPass(
            *m_framegraph,
            m_device,
            prevDepthHandle,
            width,
            height,
            m_blackboard->get_or_add<passes::HiZBuildPassState>()
        );
    }

    m_hizPyramid = hizOutput.pyramid;
    if (m_hizPyramid.is_valid())
        m_framegraph->GetRTRegistry().RegisterRT("rt_HiZ", m_hizPyramid);

    framegraph::VirtualResourceHandle prevNormalsHandle;
    if (m_hasPrevFrameData && m_normals[readIdx]) {
        framegraph::ResourceDesc prevNormalsDesc;
        prevNormalsDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevNormalsDesc.debugName = "rt_PrevNormals";
        prevNormalsDesc.width = width;
        prevNormalsDesc.height = height;
        prevNormalsDesc.format = nvrhi::Format::RGBA16_FLOAT;
        prevNormalsDesc.isRenderTarget = true;
        prevNormalsDesc.isImported = true;
        prevNormalsDesc.isTransient = false;
        prevNormalsHandle = m_framegraph->ImportTexture("rt_PrevNormals", m_normals[readIdx], prevNormalsDesc);
        m_framegraph->GetRTRegistry().RegisterRT("rt_PrevNormals", prevNormalsHandle);
    }

    framegraph::VirtualResourceHandle prevWorldPosHandle;
    if (m_hasPrevFrameData && m_worldPos[readIdx]) {
        framegraph::ResourceDesc prevWorldPosDesc;
        prevWorldPosDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevWorldPosDesc.debugName = "rt_PrevWorldPos";
        prevWorldPosDesc.width = width;
        prevWorldPosDesc.height = height;
        prevWorldPosDesc.format = nvrhi::Format::RGBA32_FLOAT;
        prevWorldPosDesc.isRenderTarget = true;
        prevWorldPosDesc.isImported = true;
        prevWorldPosDesc.isTransient = false;
        prevWorldPosHandle = m_framegraph->ImportTexture("rt_PrevWorldPos", m_worldPos[readIdx], prevWorldPosDesc);
        m_framegraph->GetRTRegistry().RegisterRT("rt_PrevWorldPos", prevWorldPosHandle);
    }

    // ═══════════════════════════════════════════════════════
    //  GPU CULLING — pass 1: frustum (+ temporal Hi-Z if depth prepass OFF)
    // ═══════════════════════════════════════════════════════

    framegraph::VirtualResourceHandle drawArgsBuffer;
    framegraph::VirtualResourceHandle skinnedDrawArgsBuffer;

    if (m_gpuCullingManager) {
        m_gpuCullingManager->Initialize(m_device);

        if (m_detailManager && !m_detailManager->computePipeline) {
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            m_detailManager->LoadCullComputeShader(shaderLoader);
            m_detailManager->LoadInstanceGenShader(shaderLoader);
            m_detailManager->LoadPrefixSumShaders(shaderLoader);
            m_detailManager->LoadGraphicsShaders(shaderLoader);

            m_detailManager->CreateComputePipeline(m_device);
            m_detailManager->CreateInstanceGenPipeline(m_device);
            m_detailManager->CreatePrefixSumPipeline(m_device);

            if (!m_detailManager->perlin4dTexture) {
                m_detailManager->CreatePerlin4DTexture(m_device->GetNVRHIDevice());
            }
            m_detailManager->LoadPerlin4DComputeShader(shaderLoader);
            if (m_detailManager->perlin4dComputeShader) {
                m_detailManager->CreatePerlin4DPipeline(m_device->GetNVRHIDevice());
            }
        }

        if (m_rtAccelMgr && m_rtAccelMgr->IsSupported())
            m_gpuCullingManager->SetRTAccelStructManager(m_rtAccelMgr.get());

#if !defined(XR_PLATFORM_APPLE)
        const bool forceFrustumOnly = !hizOutput.pyramid.is_valid();
#else
        const bool forceFrustumOnly = useDepthPrepass || !hizOutput.pyramid.is_valid();
#endif

        if (m_gpuCullingManager->IsEnabled()) {
            auto cullOutput = m_gpuCullingManager->SetupCullingPass(
                *m_framegraph,
                forceFrustumOnly ? framegraph::VirtualResourceHandle() : m_hizPyramid,
                forceFrustumOnly ? 1u : hizOutput.width,
                forceFrustumOnly ? 1u : hizOutput.height,
                forceFrustumOnly ? 1u : hizOutput.mipLevels,
                m_geometryCollector.get(),
                m_prevViewProj,
                forceFrustumOnly,
                false
            );

            drawArgsBuffer = cullOutput.drawArgsBuffer;
        }

        // Skinned/particle always take the dummy Hi-Z (all-visible) path: NPCs and
        // effects move between frames, so Hi-Z occlusion makes them flicker.
        const framegraph::VirtualResourceHandle noHiz;
        if (m_gpuCullingManager->IsParticleCullingEnabled() && !m_worldParticleBatches.empty()) {
            m_gpuCullingManager->SetupParticleCullingPass(
                *m_framegraph,
                noHiz,
                1,
                1,
                1,
                &m_worldParticleBatches
            );
        }

        if (m_gpuCullingManager->IsSkinnedCullingEnabled()) {
            skinnedDrawArgsBuffer = m_gpuCullingManager->SetupSkinnedCullingPass(
                *m_framegraph,
                noHiz,
                1,
                1,
                1,
                m_geometryCollector.get(),
                m_prevViewProj,
                m_overlayManager.get()
            );
        }
    }

    // ═══════════════════════════════════════════════════════
    //  SKY PASS (Renders sky dome behind everything)
    // ═══════════════════════════════════════════════════════
    framegraph::ResourceDesc colorDesc;
    colorDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    colorDesc.width = width;
    colorDesc.height = height;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.debugName = "rt_SceneColor";

    auto skyColorHandle = m_framegraph->CreateTexture("rt_SceneColor", colorDesc);

    FGEnvironmentRender* fgEnv = nullptr;
    if (g_pGamePersistent)
        fgEnv = dynamic_cast<FGEnvironmentRender*>(&*g_pGamePersistent->Environment().m_pRender);

    auto skyOutput = passes::setupSkyPass(
        *m_framegraph,
        skyColorHandle,
        depthBuffer,
        fgEnv,
        width,
        height
    );

    // Sun is drawn AFTER opaque forward (with depth test) — see below.

    // ═══════════════════════════════════════════════════════
    //  FORWARD COLOR PASS (Single-RT, Reuses Depth)
    // ═══════════════════════════════════════════════════════
    passes::BindlessForwardConfig bindlessConfig;
    if (m_gpuCullingManager && m_gpuCullingManager->IsCompactionEnabled()) {
        bindlessConfig.enabled = true;  // TODO: Add console var to toggle bindless mode

        bindlessConfig.staticSet.compactDrawArgsBuffer = m_gpuCullingManager->GetStaticCompactDrawArgsBuffer();
        bindlessConfig.staticSet.compactMaterialIDBuffer = m_gpuCullingManager->GetStaticCompactMaterialIDBuffer();
        bindlessConfig.staticSet.compactBatchIndicesBuffer = m_gpuCullingManager->GetStaticCompactBatchIndicesBuffer();
        bindlessConfig.staticSet.compactCountBuffer = m_gpuCullingManager->GetStaticCompactCountBuffer();
        bindlessConfig.staticSet.instanceBuffer = m_gpuCullingManager->GetStaticInstanceBuffer();
        bindlessConfig.staticSet.totalObjectCount = m_gpuCullingManager->GetStaticObjectCount();
        bindlessConfig.staticSet.castAllDrawArgsBuffer = m_gpuCullingManager->GetStaticDrawArgsBuffer();
        bindlessConfig.staticSet.castAllBatchIndicesBuffer = m_gpuCullingManager->GetStaticShadowIndicesBuffer();
        bindlessConfig.staticSet.castAllMaterialIDBuffer = m_gpuCullingManager->GetStaticFullMaterialIDBuffer();
        bindlessConfig.staticSet.castAllCountBuffer = m_gpuCullingManager->GetStaticShadowCountBuffer();
        bindlessConfig.staticSet.lightCullDrawArgsBuffer = m_gpuCullingManager->GetStaticShadowCompactDrawArgsBuffer();
        bindlessConfig.staticSet.lightCullBatchIndicesBuffer = m_gpuCullingManager->GetStaticShadowIndicesBuffer();
        bindlessConfig.staticSet.lightCullMaterialIDBuffer = m_gpuCullingManager->GetStaticShadowCompactMaterialIDBuffer();
        bindlessConfig.staticSet.lightCullCountBuffer = m_gpuCullingManager->GetStaticShadowCountBuffer();

        bindlessConfig.dynamicSet.compactDrawArgsBuffer = m_gpuCullingManager->GetDynamicCompactDrawArgsBuffer();
        bindlessConfig.dynamicSet.compactMaterialIDBuffer = m_gpuCullingManager->GetDynamicCompactMaterialIDBuffer();
        bindlessConfig.dynamicSet.compactBatchIndicesBuffer = m_gpuCullingManager->GetDynamicCompactBatchIndicesBuffer();
        bindlessConfig.dynamicSet.compactCountBuffer = m_gpuCullingManager->GetDynamicCompactCountBuffer();
        bindlessConfig.dynamicSet.instanceBuffer = m_gpuCullingManager->GetDynamicInstanceBuffer();
        bindlessConfig.dynamicSet.totalObjectCount = m_gpuCullingManager->GetDynamicObjectCount();
        bindlessConfig.dynamicSet.castAllDrawArgsBuffer = m_gpuCullingManager->GetDynamicDrawArgsBuffer();
        bindlessConfig.dynamicSet.castAllBatchIndicesBuffer = m_gpuCullingManager->GetDynamicShadowIndicesBuffer();
        bindlessConfig.dynamicSet.castAllMaterialIDBuffer = m_gpuCullingManager->GetDynamicFullMaterialIDBuffer();
        bindlessConfig.dynamicSet.castAllCountBuffer = m_gpuCullingManager->GetDynamicShadowCountBuffer();
        bindlessConfig.dynamicSet.lightCullDrawArgsBuffer = m_gpuCullingManager->GetDynamicShadowCompactDrawArgsBuffer();
        bindlessConfig.dynamicSet.lightCullBatchIndicesBuffer = m_gpuCullingManager->GetDynamicShadowIndicesBuffer();
        bindlessConfig.dynamicSet.lightCullMaterialIDBuffer = m_gpuCullingManager->GetDynamicShadowCompactMaterialIDBuffer();
        bindlessConfig.dynamicSet.lightCullCountBuffer = m_gpuCullingManager->GetDynamicShadowCountBuffer();

        // Transparent set as foliage shadow caster (trees/bushes). Only cast-all buffers
        // are needed here; the transparent forward render uses its own compact config.
        bindlessConfig.transparentCasterSet.instanceBuffer = m_gpuCullingManager->GetTransparentInstanceBuffer();
        bindlessConfig.transparentCasterSet.totalObjectCount = m_gpuCullingManager->GetTransparentObjectCount();
        bindlessConfig.transparentCasterSet.castAllDrawArgsBuffer = m_gpuCullingManager->GetTransparentDrawArgsBuffer();
        bindlessConfig.transparentCasterSet.castAllBatchIndicesBuffer = m_gpuCullingManager->GetTransparentShadowIndicesBuffer();
        bindlessConfig.transparentCasterSet.castAllMaterialIDBuffer = m_gpuCullingManager->GetTransparentFullMaterialIDBuffer();
        bindlessConfig.transparentCasterSet.castAllCountBuffer = m_gpuCullingManager->GetTransparentShadowCountBuffer();
        bindlessConfig.transparentCasterSet.lightCullDrawArgsBuffer = m_gpuCullingManager->GetTransparentShadowCompactDrawArgsBuffer();
        bindlessConfig.transparentCasterSet.lightCullBatchIndicesBuffer = m_gpuCullingManager->GetTransparentShadowIndicesBuffer();
        bindlessConfig.transparentCasterSet.lightCullMaterialIDBuffer = m_gpuCullingManager->GetTransparentShadowCompactMaterialIDBuffer();
        bindlessConfig.transparentCasterSet.lightCullCountBuffer = m_gpuCullingManager->GetTransparentShadowCountBuffer();

        if (m_gpuCullingManager->GetTessObjectCount() > 0) {
            bindlessConfig.tessMaterialIDBuffer = m_gpuCullingManager->GetTessMaterialIDBuffer();
            bindlessConfig.tessBatchIndicesBuffer = m_gpuCullingManager->GetTessBatchIndicesBuffer();
            bindlessConfig.tessInstanceBuffer = m_gpuCullingManager->GetTessInstanceBuffer();
            bindlessConfig.tessDrawArgs = m_gpuCullingManager->GetTessDrawArgsData().data();
            bindlessConfig.tessObjects = m_gpuCullingManager->GetTessObjectData().data();
            bindlessConfig.tessObjectCount = m_gpuCullingManager->GetTessObjectCount();
        }

        // ═══════════════════════════════════════════════════════
        //  MEGA-BUFFER CONFIGURATION (GPU-Driven Rendering)
        // ═══════════════════════════════════════════════════════
        if (m_gpuCullingManager->AreMegaBuffersReady()) {
            bindlessConfig.megaVertexBuffer = m_gpuCullingManager->GetMegaVertexBuffer();
            bindlessConfig.megaIndexBuffer = m_gpuCullingManager->GetMegaIndexBuffer();
            bindlessConfig.megaBuffersReady = true;
        }

        // ═══════════════════════════════════════════════════════
        //  TERRAIN CONFIGURATION (4-layer detail blending)
        // ═══════════════════════════════════════════════════════
        if (m_gpuCullingManager->GetTerrainObjectCount() > 0) {
            bindlessConfig.terrainDrawArgsBuffer = m_gpuCullingManager->GetTerrainDrawArgsBuffer();
            bindlessConfig.terrainMaterialIDBuffer = m_gpuCullingManager->GetTerrainMaterialIDBuffer();
            bindlessConfig.terrainInstanceBuffer = m_gpuCullingManager->GetTerrainInstanceBuffer();
            bindlessConfig.terrainBatchIndicesBuffer = m_gpuCullingManager->GetTerrainBatchIndicesBuffer();
            bindlessConfig.terrainCompactDrawArgsBuffer = m_gpuCullingManager->GetTerrainCompactDrawArgsBuffer();
            bindlessConfig.terrainCompactBatchIndicesBuffer = m_gpuCullingManager->GetTerrainCompactBatchIndicesBuffer();
            bindlessConfig.terrainCompactMaterialIDBuffer = m_gpuCullingManager->GetTerrainCompactMaterialIDBuffer();
            bindlessConfig.terrainCompactCountBuffer = m_gpuCullingManager->GetTerrainCompactCountBuffer();
            bindlessConfig.terrainObjectCount = m_gpuCullingManager->GetTerrainObjectCount();
        }

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
        {
            auto vp = m_gpuCullingManager->GetStaticPartition().ToConfig();
            // Cap rare variant bins — too many PSO switches hurts more than uber path
            if (vp.variantCount > 8)
                vp.variantCount = 0;
            bindlessConfig.variantPartition = vp;
        }
    }

#if !defined(XR_PLATFORM_APPLE)
    bindlessConfig.useEqualDepth = useDepthPrepass;
#endif

    // Cluster assign runs after Hi-Z is final (see below, post DepthPrepass).

    // ═══════════════════════════════════════════════════════
    //  PERLIN4D NOISE (async compute — overlaps CSM / Forward)
    // ═══════════════════════════════════════════════════════
    framegraph::VirtualResourceHandle perlinReadyHandle;
    if (m_detailManager && m_detailManager->perlin4dPipeline)
    {
        framegraph::ResourceDesc perlinSyncDesc;
        perlinSyncDesc.type = framegraph::ResourceDesc::Type::Buffer;
        perlinSyncDesc.bufferSize = 16;
        perlinSyncDesc.isUAV = true;
        perlinSyncDesc.allowUAV = true;
        perlinSyncDesc.debugName = "buf_PerlinReady";
        perlinReadyHandle = m_framegraph->CreateBuffer("buf_PerlinReady", perlinSyncDesc);

        struct Perlin4DGenData {
            FGDetailManager* dm = nullptr;
            framegraph::VirtualResourceHandle ready;
        };
        m_framegraph->addCallbackPass<Perlin4DGenData>(
            "Perlin4DGen",
            [&, perlinReadyHandle](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, Perlin4DGenData& data)
            {
                framegraph::RenderPassBuilder passBuilder(builder, passHandle);
                data.ready = passBuilder.write(perlinReadyHandle, framegraph::ResourceState::UnorderedAccess);
                passBuilder.asyncCompute();
                data.dm = m_detailManager.get();
            },
            [](const Perlin4DGenData& data, const framegraph::FrameGraph&, fg::RenderContext* ctx)
            {
                auto* cmdList = ctx->GetCommandList();
                auto* device  = cmdList->getDevice();
                data.dm->DispatchPerlin4DCompute(cmdList, device, Device.fTimeGlobal);
            }
        );
    }

    // DetailCull: prefer prev-frame Hi-Z when available; dummy only as last resort.
    // Same-frame pyramid is built after DepthPrepass — too late for grass CSM.
    if (!m_hizPyramid.is_valid() && m_gpuCullingManager && m_gpuCullingManager->GetDummyHiZTexture())
    {
        framegraph::ResourceDesc dummyDesc;
        dummyDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        dummyDesc.debugName = "rt_DummyHiZ";
        dummyDesc.width = 1;
        dummyDesc.height = 1;
        dummyDesc.format = nvrhi::Format::R32_FLOAT;
        dummyDesc.isImported = true;
        dummyDesc.isTransient = false;
        m_hizPyramid = m_framegraph->ImportTexture(
            "rt_DummyHiZ_Early", m_gpuCullingManager->GetDummyHiZTexture(), dummyDesc);
        hizOutput.pyramid = m_hizPyramid;
        hizOutput.width = 1;
        hizOutput.height = 1;
        hizOutput.mipLevels = 1;
    }

    framegraph::VirtualResourceHandle detailHiz = m_hizPyramid;
    u32 detailHizW = hizOutput.width;
    u32 detailHizH = hizOutput.height;
    u32 detailHizMips = hizOutput.mipLevels;
    const Fmatrix* detailPrevVP =
        (useTemporalHizCull || (!useDepthPrepass && m_hasPrevFrameData)) ? &m_prevViewProj : nullptr;

#if defined(XR_PLATFORM_APPLE)
    if (useDepthPrepass && hasPrevDepth)
    {
        framegraph::ResourceDesc prevDepthDesc;
        prevDepthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevDepthDesc.debugName = "rt_PrevDepth_Detail";
        prevDepthDesc.width = width;
        prevDepthDesc.height = height;
        prevDepthDesc.format = nvrhi::Format::D32;
        prevDepthDesc.isDepthStencil = true;
        prevDepthDesc.isImported = true;
        prevDepthDesc.isTransient = false;
        auto prevDepthHandle = m_framegraph->ImportTexture(
            "rt_PrevDepth_Detail", m_prevFrameDepth, prevDepthDesc);
        auto detailHizOut = passes::setupHiZBuildPass(
            *m_framegraph,
            m_device,
            prevDepthHandle,
            width,
            height,
            m_blackboard->get_or_add<passes::HiZBuildPassState>(),
            "rt_HiZPyramid_Detail",
            "Hi-Z Build Detail"
        );
        detailHiz = detailHizOut.pyramid;
        detailHizW = detailHizOut.width;
        detailHizH = detailHizOut.height;
        detailHizMips = detailHizOut.mipLevels;
        detailPrevVP = &m_prevViewProj;
    }
    else
#endif
    if (useDepthPrepass && !useTemporalHizCull && m_gpuCullingManager &&
        m_gpuCullingManager->GetDummyHiZTexture())
    {
        framegraph::ResourceDesc dummyDesc;
        dummyDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        dummyDesc.debugName = "rt_DummyHiZ_Detail";
        dummyDesc.width = 1;
        dummyDesc.height = 1;
        dummyDesc.format = nvrhi::Format::R32_FLOAT;
        dummyDesc.isImported = true;
        dummyDesc.isTransient = false;
        detailHiz = m_framegraph->ImportTexture(
            "rt_DummyHiZ_Detail", m_gpuCullingManager->GetDummyHiZTexture(), dummyDesc);
        detailHizW = 1;
        detailHizH = 1;
        detailHizMips = 1;
    }

    // ═══════════════════════════════════════════════════════
    //  DETAIL CULL PASS (before CSM so grass can cast with r2_sun_details)
    // ═══════════════════════════════════════════════════════
    passes::setupDetailCullPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        detailHiz,
        detailHizW,
        detailHizH,
        detailHizMips,
        detailPrevVP,
        m_gpuProfiler.get(),
        &m_blackboard->get_or_add<passes::DetailPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  CASCADED SHADOW MAPS (sun CSM, before Forward)
    // ═══════════════════════════════════════════════════════
    if (!rtLightingWanted)
    {
        Fvector sunDir(0.3f, 0.8f, 0.2f);
        if (g_pGamePersistent)
        {
            const auto& env = g_pGamePersistent->Environment().CurrentEnv;
            sunDir.set(-env.sun_dir.x, -env.sun_dir.y, -env.sun_dir.z);
        }
        auto shadowOut = passes::setupCascadedShadowPass(
            *m_framegraph,
            m_device,
            bindlessConfig,
            m_materialCache.get(),
            m_detailManager.get(),
            sunDir,
            &m_hudBatches,
            &m_geometryCollector->GetBatches(),
            m_gpuCullingManager.get(),
            m_blackboard->get_or_add<passes::ShadowPassState>(),
            skinnedDrawArgsBuffer);
        if (shadowOut.valid)
        {
            auto& shadowState = m_blackboard->get_or_add<passes::ShadowPassState>();
            bindlessConfig.shadowMapArray = shadowState.shadowCascades[0];
            for (u32 i = 0; i < 3; ++i)
                bindlessConfig.shadowCascades[i] = shadowState.shadowCascades[i];
            bindlessConfig.shadowMapHandle = shadowOut.shadowArray;
            bindlessConfig.hudShadowMap = shadowState.hudShadowMap;
            m_framegraph->GetRTRegistry().RegisterRT("rt_ShadowMap", shadowOut.shadowArray);
        }
    }

    if (!rtLightingWanted)
    {
        auto& clmLocal = fg::ClusteredLightManager::Instance();
        clmLocal.AssignLocalShadowTiles(Device.vCameraPosition);
        if (!clmLocal.GetLocalShadowTiles().empty())
        {
            auto localOut = passes::setupLocalShadowPass(
                *m_framegraph,
                m_device,
                bindlessConfig,
                m_blackboard->get_or_add<passes::ShadowPassState>(),
                m_blackboard->get_or_add<passes::LocalShadowPassState>(),
                &m_geometryCollector->GetBatches(),
                m_gpuCullingManager.get(),
                drawArgsBuffer);
            if (localOut.valid)
            {
                bindlessConfig.localShadowAtlas = localOut.atlasTex;
                bindlessConfig.localShadowESM = localOut.esmTex;
                bindlessConfig.localShadowHandle = localOut.atlas;
                m_framegraph->GetRTRegistry().RegisterRT("rt_LocalShadowAtlas", localOut.atlas);
            }
        }
    }

    passes::ShadowHZBOutput shadowHzbOut{};
    if (!rtLightingWanted)
    {
        shadowHzbOut = passes::setupShadowHZBPass(
            *m_framegraph,
            m_device,
            m_blackboard->get_or_add<passes::ShadowPassState>(),
            m_blackboard->get_or_add<passes::ShadowHZBPassState>(),
            bindlessConfig.shadowMapHandle);
        if (shadowHzbOut.valid)
        {
            for (u32 i = 0; i < 3; ++i)
            {
                bindlessConfig.shadowHZB[i] = shadowHzbOut.hzb[i];
                bindlessConfig.shadowHZBHandles[i] = shadowHzbOut.handles[i];
            }
        }
    }

    passes::ResolveEnvSkyCubes(m_device, bindlessConfig.envSky0, bindlessConfig.envSky1);
    nvrhi::ITexture* rawEnvSky0 = bindlessConfig.envSky0;
    nvrhi::ITexture* rawEnvSky1 = bindlessConfig.envSky1;
    {
        auto& iblState = m_blackboard->get_or_add<passes::IBLPrefilterPassState>();
        auto ibl = passes::setupIBLPrefilterPass(
            *m_framegraph, m_device, bindlessConfig.envSky0, bindlessConfig.envSky1, iblState);
        if (ibl.spec0)
            bindlessConfig.envSky0 = ibl.spec0;
        if (ibl.spec1)
            bindlessConfig.envSky1 = ibl.spec1;
        bindlessConfig.envBrdfLut = ibl.brdfLut;
        bindlessConfig.envSkySH = ibl.shBuffer;
        bindlessConfig.envProbes = ibl.probeBuffer;
        bindlessConfig.envProbeCubes = ibl.probeCubeArray;
    }

    if (!rtLightingWanted && ps_r_contact_shadows && m_hasPrevFrameData && m_prevFrameDepth)
        bindlessConfig.contactDepth = m_prevFrameDepth;
    if (!rtLightingWanted)
    {
        auto& contactState = m_blackboard->get_or_add<passes::ContactShadowsPassState>();
        bindlessConfig.contactHistory = passes::GetContactShadowHistory(contactState);
    }

    // Opaque depth fill → early-Z for Forward (LEQ, no depth clear).
    // Same-frame Hi-Z is built from this depth when r_depth_prepass is on.
    // Cull1→Depth edge is required: Cull1 is otherwise orphaned when Cull2
    // replaces drawArgsBuffer, and Depth/Forward sample Cull1 compact buffers.
    const framegraph::VirtualResourceHandle cull1DrawArgs = drawArgsBuffer;
    if (useDepthPrepass)
    {
        depthBuffer = passes::setupDepthPrepass(
            *m_framegraph,
            m_device,
            depthBuffer,
            m_geometryCollector.get(),
            m_materialCache.get(),
            width,
            height,
            bindlessConfig,
            &m_blackboard->get_or_add<passes::DepthPrepassState>(),
            cull1DrawArgs);

        hizOutput = passes::setupHiZBuildPass(
            *m_framegraph,
            m_device,
            depthBuffer,
            width,
            height,
            m_blackboard->get_or_add<passes::HiZBuildPassState>()
        );
        m_hizPyramid = hizOutput.pyramid;
        if (m_hizPyramid.is_valid())
            m_framegraph->GetRTRegistry().RegisterRT("rt_HiZ", m_hizPyramid);

#if !defined(XR_PLATFORM_APPLE)
        const bool needSameFrameOcclusionCull = useHizOcclusion && !useTemporalHizCull;
#else
        const bool needSameFrameOcclusionCull = useHizOcclusion;
#endif
        if (needSameFrameOcclusionCull && m_gpuCullingManager && m_gpuCullingManager->IsEnabled() &&
            hizOutput.pyramid.is_valid())
        {
            auto cullHiz = m_gpuCullingManager->SetupCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                m_geometryCollector.get(),
                Device.mFullTransform,
                false,
                true,
                cull1DrawArgs);
            if (cullHiz.drawArgsBuffer.is_valid())
                drawArgsBuffer = cullHiz.drawArgsBuffer;
        }
    }

    // Cluster assign: Hi-Z cull with inflated light radius (light_hiz_cull.cs)
    // and previous-frame pyramid to avoid same-frame false cull flicker.
    {
        auto& clmSetup = fg::ClusteredLightManager::Instance();
        if (clmSetup.IsReady() && clmSetup.GetLightCount() > 0) {
            const bool useClusterHiZ =
                !rtLightingWanted && m_hasPrevFrameData && hizOutput.pyramid.is_valid();
            passes::setupClusterLightPass(
                *m_framegraph,
                m_device,
                &clmSetup,
                width,
                height,
                &m_blackboard->get_or_add<passes::ClusterLightPassState>(),
                useClusterHiZ ? hizOutput.pyramid : framegraph::VirtualResourceHandle(),
                useClusterHiZ ? hizOutput.width : 0,
                useClusterHiZ ? hizOutput.height : 0,
                useClusterHiZ ? hizOutput.mipLevels : 0,
                m_prevViewProj,
                useClusterHiZ
            );
        }
    }

    if (!rtLightingWanted)
    {
        auto maskOut = passes::setupShadowMaskPass(
            *m_framegraph,
            m_device,
            depthBuffer,
            width,
            height,
            bindlessConfig,
            shadowHzbOut,
            m_blackboard->get_or_add<passes::ShadowMaskPassState>());
        if (maskOut.valid)
        {
            bindlessConfig.shadowMask = maskOut.mask;
            bindlessConfig.shadowMaskHandle = maskOut.handle;
            if (maskOut.handle.is_valid())
                m_framegraph->GetRTRegistry().RegisterRT("rt_ShadowMask", maskOut.handle);
        }
    }

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        skyOutput,
        normalBuffer,
        baseColorBuffer,
        worldPosBuffer,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height,
        drawArgsBuffer,
        bindlessConfig,
        &m_blackboard->get_or_add<passes::ForwardColorPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  GPU CULLING DEBUG VISUALIZATION (Optional overlay)
    // ═══════════════════════════════════════════════════════
    if (m_gpuCullingManager && m_gpuCullingManager->IsDebugEnabled() && hizOutput.pyramid.is_valid()) {
        m_gpuCullingManager->SetupDebugVisualizationPass(
            *m_framegraph,
            m_hizPyramid,
            forwardOutputs.albedo,
            depthBuffer,
            hizOutput.width,
            hizOutput.height,
            hizOutput.mipLevels,
            &m_worldParticleBatches
        );
    }

    // 2. Skinning Pass - world skinned only (HUD deferred past RTGI/NRD)
    auto hudOutputs = passes::setupSkinningPass(
        *m_framegraph,
        m_device,
        forwardOutputs,
        m_geometryCollector.get(),
        nullptr,
        m_materialCache.get(),
        width,
        height,
        m_gpuCullingManager.get(),
        skinnedDrawArgsBuffer,
        &m_blackboard->get_or_add<passes::SkinningPassState>(),
        m_overlayManager.get(),
        bindlessConfig.shadowMapArray,
        bindlessConfig.shadowMapHandle,
        bindlessConfig.contactDepth,
        bindlessConfig.contactHistory,
        bindlessConfig.envSky0,
        bindlessConfig.envSky1,
        bindlessConfig.hudShadowMap,
        bindlessConfig.shadowCascades,
        bindlessConfig.localShadowAtlas,
        bindlessConfig.shadowHZB,
        bindlessConfig.shadowMask,
        bindlessConfig.localShadowESM
    );

    auto lodOutputs = passes::setupLodPass(
        *m_framegraph,
        m_device,
        hudOutputs,
        &m_lodImpostors,
        width,
        height,
        m_blackboard->get_or_add<passes::LodPassState>());

    // ═══════════════════════════════════════════════════════
    //  DETAIL DRAW PASS (Graphics)
    // ═══════════════════════════════════════════════════════
    auto detailOutputs = passes::setupDetailPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        lodOutputs,
        width,
        height,
        m_gpuProfiler.get(),
        bindlessConfig.shadowMapArray,
        bindlessConfig.shadowMapHandle,
        bindlessConfig.contactDepth,
        bindlessConfig.contactHistory,
        perlinReadyHandle,
        bindlessConfig.shadowCascades,
        bindlessConfig.localShadowAtlas,
        bindlessConfig.localShadowESM
    );

    auto opaqueLit = detailOutputs;
    if (!rtLightingWanted)
    {
        auto aoDepth = detailOutputs.depth.is_valid() ? detailOutputs.depth : depthBuffer;
        auto aoNormal = detailOutputs.normal.is_valid() ? detailOutputs.normal : normalBuffer;
        auto aoWorldPos = detailOutputs.worldPos.is_valid() ? detailOutputs.worldPos : worldPosBuffer;
        auto aoColor = passes::setupAmbientOcclusionPass(
            *m_framegraph,
            m_device,
            detailOutputs.albedo,
            aoDepth,
            aoNormal,
            aoWorldPos,
            width,
            height,
            m_blackboard->get_or_add<passes::AmbientOcclusionPassState>());
        if (aoColor.is_valid())
            opaqueLit.albedo = aoColor;
    }

    // ═══════════════════════════════════════════════════════
    //  TRANSPARENT PASS (alpha-blended geometry)
    // ═══════════════════════════════════════════════════════
    passes::TransparentPassConfig transparentConfig;
    if (m_gpuCullingManager && m_gpuCullingManager->GetTransparentObjectCount() > 0) {
        transparentConfig.megaVertexBuffer = bindlessConfig.megaVertexBuffer;
        transparentConfig.megaIndexBuffer = bindlessConfig.megaIndexBuffer;
        transparentConfig.instanceBuffer = m_gpuCullingManager->GetTransparentInstanceBuffer();
        transparentConfig.compactDrawArgsBuffer = m_gpuCullingManager->GetTransparentCompactDrawArgsBuffer();
        transparentConfig.compactBatchIndicesBuffer = m_gpuCullingManager->GetTransparentCompactBatchIndicesBuffer();
        transparentConfig.compactMaterialIDBuffer = m_gpuCullingManager->GetTransparentCompactMaterialIDBuffer();
        transparentConfig.compactCountBuffer = m_gpuCullingManager->GetTransparentCompactCountBuffer();
        transparentConfig.objectCount = m_gpuCullingManager->GetTransparentObjectCount();
        transparentConfig.shadowMapArray = bindlessConfig.shadowMapArray;
        for (int i = 0; i < 3; ++i)
            transparentConfig.shadowCascades[i] = bindlessConfig.shadowCascades[i];
        transparentConfig.localShadowAtlas = bindlessConfig.localShadowAtlas;
        transparentConfig.localShadowESM = bindlessConfig.localShadowESM;
        transparentConfig.envSky0 = rawEnvSky0 ? rawEnvSky0 : bindlessConfig.envSky0;
        transparentConfig.envSky1 = rawEnvSky1 ? rawEnvSky1 : bindlessConfig.envSky1;
        transparentConfig.contactDepth = bindlessConfig.contactDepth;
        transparentConfig.contactHistory = bindlessConfig.contactHistory;
        for (u32 i = 0; i < 3; ++i)
        {
            transparentConfig.shadowHZB[i] = bindlessConfig.shadowHZB[i];
            transparentConfig.shadowHZBHandles[i] = bindlessConfig.shadowHZBHandles[i];
        }
        transparentConfig.shadowMask = bindlessConfig.shadowMask;
        transparentConfig.shadowMaskHandle = bindlessConfig.shadowMaskHandle;

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
            transparentConfig.variantPartition = m_gpuCullingManager->GetTransparentPartition().ToConfig();
    }

    auto transparentOutputs = passes::setupTransparentPass(
        *m_framegraph,
        m_device,
        opaqueLit,
        transparentConfig,
        width, height,
        m_blackboard->get_or_add<passes::TransparentPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  DECAL PASS (screen-space box decals on surfaces)
    // ═══════════════════════════════════════════════════════
    if (m_decalManager) {
        m_decalManager->Update(Device.fTimeDelta, Device.fTimeGlobal);
        if (m_decalManager->GetActiveCount() > 0) {
            transparentOutputs = passes::setupDecalPass(
                *m_framegraph, m_device,
                transparentOutputs, m_decalManager.get(),
                width, height,
                m_blackboard->get_or_add<passes::DecalPassState>()
            );
        }
    }

    framegraph::VirtualResourceHandle rtgiGuideDepth = transparentOutputs.depth;
    framegraph::VirtualResourceHandle rtgiGuideNormal = transparentOutputs.normal;
    framegraph::VirtualResourceHandle rtgiGuideWorldPos = transparentOutputs.worldPos;
    framegraph::VirtualResourceHandle rtgiGuideBaseColor = transparentOutputs.baseColor;
    if (rtgiActive)
    {
        framegraph::ResourceDesc gd;
        gd.type = framegraph::ResourceDesc::Type::Texture2D;
        gd.width = width;
        gd.height = height;
        gd.isTransient = true;

        gd.debugName = "rtgi_GuideDepth";
        gd.format = nvrhi::Format::D32;
        gd.isDepthStencil = true;
        auto guideDepth = m_framegraph->CreateTexture("rtgi_GuideDepth", gd);

        gd.isDepthStencil = false;
        gd.isRenderTarget = true;
        gd.debugName = "rtgi_GuideNormal";
        gd.format = nvrhi::Format::RGBA16_FLOAT;
        auto guideNormal = m_framegraph->CreateTexture("rtgi_GuideNormal", gd);

        gd.debugName = "rtgi_GuideWorldPos";
        gd.format = nvrhi::Format::RGBA32_FLOAT;
        auto guideWorldPos = m_framegraph->CreateTexture("rtgi_GuideWorldPos", gd);

        gd.debugName = "rtgi_GuideBaseColor";
        gd.format = nvrhi::Format::RGBA8_UNORM;
        auto guideBaseColor = m_framegraph->CreateTexture("rtgi_GuideBaseColor", gd);

        struct GuideCopyData {
            framegraph::VirtualResourceHandle srcDepth, srcNormal, srcWorldPos, srcBase;
            framegraph::VirtualResourceHandle dstDepth, dstNormal, dstWorldPos, dstBase;
        };
        m_framegraph->addCallbackPass<GuideCopyData>(
            "RTGI Guide Copy",
            [&](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, GuideCopyData& data) {
                framegraph::RenderPassBuilder pb(builder, passHandle);
                data.srcDepth = pb.read(transparentOutputs.depth, framegraph::ResourceState::CopySource);
                data.srcNormal = pb.read(transparentOutputs.normal, framegraph::ResourceState::CopySource);
                data.srcWorldPos = pb.read(transparentOutputs.worldPos, framegraph::ResourceState::CopySource);
                data.srcBase = pb.read(transparentOutputs.baseColor, framegraph::ResourceState::CopySource);
                data.dstDepth = pb.write(guideDepth, framegraph::ResourceState::CopyDest);
                data.dstNormal = pb.write(guideNormal, framegraph::ResourceState::CopyDest);
                data.dstWorldPos = pb.write(guideWorldPos, framegraph::ResourceState::CopyDest);
                data.dstBase = pb.write(guideBaseColor, framegraph::ResourceState::CopyDest);
            },
            [](const GuideCopyData& data, const framegraph::FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto* cmd = ctx->GetCommandList();
                auto* sd = fgGraph.GetPhysicalTexture(data.srcDepth);
                auto* sn = fgGraph.GetPhysicalTexture(data.srcNormal);
                auto* sw = fgGraph.GetPhysicalTexture(data.srcWorldPos);
                auto* sb = fgGraph.GetPhysicalTexture(data.srcBase);
                auto* dd = fgGraph.GetPhysicalTexture(data.dstDepth);
                auto* dn = fgGraph.GetPhysicalTexture(data.dstNormal);
                auto* dw = fgGraph.GetPhysicalTexture(data.dstWorldPos);
                auto* db = fgGraph.GetPhysicalTexture(data.dstBase);
                if (!cmd || !sd || !sn || !sw || !sb || !dd || !dn || !dw || !db)
                    return;
                cmd->copyTexture(dd, nvrhi::TextureSlice(), sd, nvrhi::TextureSlice());
                cmd->copyTexture(dn, nvrhi::TextureSlice(), sn, nvrhi::TextureSlice());
                cmd->copyTexture(dw, nvrhi::TextureSlice(), sw, nvrhi::TextureSlice());
                cmd->copyTexture(db, nvrhi::TextureSlice(), sb, nvrhi::TextureSlice());
            });

        rtgiGuideDepth = guideDepth;
        rtgiGuideNormal = guideNormal;
        rtgiGuideWorldPos = guideWorldPos;
        rtgiGuideBaseColor = guideBaseColor;

        if (!m_hudBatches.empty())
        {
            passes::setupHudOverlayPass(
                *m_framegraph,
                m_device,
                transparentOutputs.albedo,
                rtgiGuideDepth,
                rtgiGuideNormal,
                rtgiGuideBaseColor,
                rtgiGuideWorldPos,
                &m_hudBatches,
                m_materialCache.get(),
                width,
                height,
                m_gpuCullingManager.get(),
                &m_blackboard->get_or_add<passes::SkinningPassState>(),
                bindlessConfig.shadowMapArray,
                bindlessConfig.contactDepth,
                bindlessConfig.contactHistory,
                bindlessConfig.envSky0,
                bindlessConfig.envSky1,
                bindlessConfig.hudShadowMap,
                bindlessConfig.shadowCascades,
                bindlessConfig.localShadowAtlas,
                bindlessConfig.shadowHZB,
                bindlessConfig.shadowMask,
                bindlessConfig.localShadowESM,
                true
            );
        }
    }

    // ═══════════════════════════════════════════════════════
    //  MOTION VECTOR PASS (worldPos camera reprojection)
    // ═══════════════════════════════════════════════════════
    passes::MotionVectorOutput motionOutput;
    if (m_hasPrevFrameData) {
        motionOutput = passes::setupMotionVectorPass(
            *m_framegraph, m_device,
            rtgiGuideDepth,
            rtgiGuideWorldPos,
            passes::g_taa_unjittered_full_transform,
            m_prevViewProj,
            width, height,
            m_blackboard->get_or_add<passes::MotionVectorPassState>()
        );
    }

    // Wet G-buffer snapshot before RTGI (particles deferred until after denoise).
    auto gbufferForWet = transparentOutputs;
    auto sceneColor = transparentOutputs.albedo;
    framegraph::VirtualResourceHandle pendingDistortRT{};
    framegraph::VirtualResourceHandle pendingDistortWorldPos = transparentOutputs.worldPos;
    framegraph::VirtualResourceHandle pendingDistortBaseColor = transparentOutputs.baseColor;

    // ═══════════════════════════════════════════════════════
    //  DYNAMIC BLAS BUILD (before ReSTIR / path tracer)
    // ═══════════════════════════════════════════════════════
    bool needsRT = (ps_r_path_tracer || ps_r_rt_gi) && m_rtAccelMgr && m_rtAccelMgr->IsSupported();

    if (needsRT && m_rtAccelMgr->IsReady()) {
        bool ptNeedsBLAS = ps_r_path_tracer && m_ptSampleIndex == 0;
        bool giNeedsBLAS = ps_r_rt_gi && !ps_r_path_tracer;

        if (ptNeedsBLAS || giNeedsBLAS) {
            struct DynamicBLASData {
                RTAccelStructManager* accelMgr;
                GPUCullingManager* gpuCulling;
                FGDetailManager* detailMgr;
                const GeometryCollector* geometry;
                const xr_vector<GeometryBatch>* hudBatches;
            };

            m_framegraph->addCallbackPass<DynamicBLASData>(
                "Dynamic BLAS Build",
                [&](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, DynamicBLASData& data) {
                    framegraph::RenderPassBuilder pb(builder, passHandle);
                    pb.sideEffects();
                    data.accelMgr = m_rtAccelMgr.get();
                    data.gpuCulling = m_gpuCullingManager.get();
                    data.detailMgr = m_detailManager.get();
                    data.geometry = m_geometryCollector.get();
                    data.hudBatches = &m_hudBatches;
                },
                [](const DynamicBLASData& data, const framegraph::FrameGraph&, fg::RenderContext* ctx) {
                    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                    xr_vector<GeometryBatch> worldSkinned;
                    for (const auto& b : data.geometry->GetBatches()) {
                        if (b.isSkinned && b.visual && b.indexCount > 0)
                            worldSkinned.push_back(b);
                    }

                    float fovScale = 1.0f / psHUD_FOV;
                    Fmatrix viewMatrix = Device.mView;
                    Fmatrix invView;
                    invView.invert(viewMatrix);
                    Fmatrix fovScaleMat;
                    fovScaleMat.identity();
                    fovScaleMat._11 = fovScale;
                    fovScaleMat._22 = fovScale;

                    xr_vector<GeometryBatch> hudSkinned;
                    for (const auto& b : *data.hudBatches) {
                        if (b.isSkinned && b.visual && b.indexCount > 0) {
                            auto adjusted = b;
                            Fmatrix t1, t2;
                            t1.mul(viewMatrix, b.worldMatrix);
                            t2.mul(fovScaleMat, t1);
                            adjusted.worldMatrix.mul(invView, t2);
                            hudSkinned.push_back(adjusted);
                        }
                    }

                    data.accelMgr->BuildSkinnedBLAS(cmdList, data.gpuCulling, worldSkinned, hudSkinned);
                    data.accelMgr->BuildGrassBLAS(cmdList, data.detailMgr);
                    data.accelMgr->RebuildDynamic(cmdList, data.gpuCulling);
                }
            );
        }
    }

    // ═══════════════════════════════════════════════════════
    //  ReSTIR GI (RT Shadows + Indirect Lighting)
    // ═══════════════════════════════════════════════════════

    if (rtgiActive) {
        const bool useDlssRR = ps_r_dlss_rr != 0 && ps_r_upscale == 2 && fg::Streamline_IsRRAvailable();
        const bool vendorDenoise = fg::IsVendorDenoiseActive(g_denoiseBackend.get());
        const bool useVendorDenoise = vendorDenoise && ps_r_nrd_apply != 0 && !useDlssRR;
        const bool skipInTreeDenoise = useVendorDenoise || useDlssRR;
        auto sceneColorBeforeRtgi = sceneColor;
        auto& rtgiState = m_blackboard->get_or_add<passes::ReSTIRGIPassState>();
        {
            auto& tpState = m_blackboard->get_or_add<passes::TransparentPassState>();
            rtgiState.waterUnderWorldPos = tpState.waterSceneWorldPos.Get();
        }
        auto rtgiOutput = passes::setupReSTIRGIPass(
            *m_framegraph, m_device, m_rtAccelMgr.get(),
            rtgiGuideDepth, rtgiGuideNormal,
            rtgiGuideBaseColor, rtgiGuideWorldPos,
            prevNormalsHandle, prevWorldPosHandle,
            motionOutput.motionVectors,
            sceneColor,
            passes::g_taa_unjittered_inv_full_transform, m_prevViewProj,
            Device.vCameraPosition, ps_r_rt_gi_intensity,
            width, height,
            rtgiState, m_hasPrevFrameData,
            skipInTreeDenoise,
            transparentOutputs.worldPos,
            useDlssRR
        );
        sceneColor = rtgiOutput.sceneColor;

        if (useVendorDenoise)
        {
            struct DenoisePassData {
                fg::IDenoiseBackend* backend = nullptr;
                passes::ReSTIRGIPassState* state = nullptr;
                framegraph::VirtualResourceHandle depth, normal, worldPos, baseColor, classifyWorldPos, motion, sceneIn, sceneOut;
                bool reset = false;
            };
            auto& rtgiState = m_blackboard->get_or_add<passes::ReSTIRGIPassState>();
            const bool denoiseReset = !m_hasPrevFrameData;
            m_framegraph->addCallbackPass<DenoisePassData>(
                "RT Denoise",
                [&, sceneColorBeforeRtgi, denoiseReset](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, DenoisePassData& data) {
                    framegraph::RenderPassBuilder pb(builder, passHandle);
                    data.depth = pb.read(rtgiGuideDepth, framegraph::ResourceState::ShaderResource);
                    data.normal = pb.read(rtgiGuideNormal, framegraph::ResourceState::ShaderResource);
                    data.worldPos = pb.read(rtgiGuideWorldPos, framegraph::ResourceState::ShaderResource);
                    data.baseColor = pb.read(rtgiGuideBaseColor, framegraph::ResourceState::ShaderResource);
                    data.classifyWorldPos = pb.read(transparentOutputs.worldPos, framegraph::ResourceState::ShaderResource);
                    if (motionOutput.motionVectors.is_valid())
                        data.motion = pb.read(motionOutput.motionVectors, framegraph::ResourceState::ShaderResource);
                    data.sceneIn = pb.read(sceneColorBeforeRtgi, framegraph::ResourceState::ShaderResource);
                    data.sceneOut = pb.write(sceneColor, framegraph::ResourceState::UnorderedAccess);
                    pb.sideEffects();
                    data.backend = g_denoiseBackend.get();
                    data.state = &rtgiState;
                    data.reset = denoiseReset;
                },
                [](const DenoisePassData& data, const framegraph::FrameGraph& fgGraph, fg::RenderContext* ctx) {
                    if (!data.backend || !data.state) return;
                    auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                    auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                    auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
                    auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                    auto* classifyTex = fgGraph.GetPhysicalTexture(data.classifyWorldPos);
                    auto* motionTex = data.motion.is_valid() ? fgGraph.GetPhysicalTexture(data.motion) : nullptr;
                    auto* sceneInTex = fgGraph.GetPhysicalTexture(data.sceneIn);
                    auto* sceneOutTex = fgGraph.GetPhysicalTexture(data.sceneOut);
                    if (!depthTex || !normalTex || !worldPosTex || !sceneInTex || !sceneOutTex)
                        return;
                    if (!data.state->noisyDiffuse || !data.state->noisySpecular || !data.state->hitDistance ||
                        !data.state->directLighting)
                        return;

                    auto copyMat = [](float dst[16], const Fmatrix& m) {
                        dst[0] = m._11; dst[1] = m._12; dst[2] = m._13; dst[3] = m._14;
                        dst[4] = m._21; dst[5] = m._22; dst[6] = m._23; dst[7] = m._24;
                        dst[8] = m._31; dst[9] = m._32; dst[10] = m._33; dst[11] = m._34;
                        dst[12] = m._41; dst[13] = m._42; dst[14] = m._43; dst[15] = m._44;
                    };

                    fg::DenoiseInputs in;
                    in.noisyDiffuse = data.state->noisyDiffuse;
                    in.noisySpecular = data.state->noisySpecular;
                    in.hitDistance = data.state->hitDistance;
                    in.normals = normalTex;
                    in.depth = depthTex;
                    in.worldPos = worldPosTex;
                    in.baseColor = baseColorTex;
                    in.classifyWorldPos = classifyTex;
                    in.motionVectors = motionTex;
                    in.directLighting = data.state->directLighting;
                    in.sceneColorIn = sceneInTex;
                    in.outDiffuse = data.state->noisyDiffuse;
                    in.outSpecular = data.state->noisySpecular;
                    in.outSceneColor = sceneOutTex;
                    in.width = data.state->texWidth;
                    in.height = data.state->texHeight;
                    in.jitterX = passes::g_taa_jitter_px;
                    in.jitterY = passes::g_taa_jitter_py;
                    in.jitterPrevX = passes::g_taa_jitter_prev_px;
                    in.jitterPrevY = passes::g_taa_jitter_prev_py;
                    in.nearZ = 0.2f;
                    in.farZ = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 500.f;
                    copyMat(in.viewToClip, Device.mProject);
                    copyMat(in.viewToClipPrev, Device.mProjectSaved);
                    copyMat(in.worldToView, Device.mView);
                    copyMat(in.worldToViewPrev, Device.mViewSaved);
                    in.frameIndex = Device.dwFrame;
                    in.reset = data.reset;
                    data.backend->Evaluate(ctx->GetCommandList(), in);
                });
        }
    }

    // ═══════════════════════════════════════════════════════
    //  PATH TRACER (Reference / Ground-Truth Mode)
    // ═══════════════════════════════════════════════════════
    if (ps_r_path_tracer && m_rtAccelMgr && m_rtAccelMgr->IsSupported()) {
        bool justEnabled = !m_ptWasEnabled;
        m_ptWasEnabled = true;

        bool posChanged = !Device.vCameraPosition.similar(m_ptPrevCameraPos, 0.01f);
        bool dirChanged = !Device.vCameraDirection.similar(m_ptPrevCameraDir, 0.001f);
        bool bouncesChanged = m_ptPrevBounces != ps_r_path_tracer_bounces;

        if (justEnabled || posChanged || dirChanged || bouncesChanged)
            m_ptSampleIndex = 0;

        m_ptPrevCameraPos = Device.vCameraPosition;
        m_ptPrevCameraDir = Device.vCameraDirection;
        m_ptPrevBounces = ps_r_path_tracer_bounces;

        passes::PathTracerConfig ptConfig;
        ptConfig.maxBounces = static_cast<u32>(ps_r_path_tracer_bounces);
        ptConfig.sampleIndex = m_ptSampleIndex;

        auto ptOutput = passes::setupPathTracerPass(
            *m_framegraph,
            m_device,
            m_rtAccelMgr.get(),
            ptConfig,
            Device.mInvFullTransform,
            Device.vCameraPosition,
            width, height
        );

        sceneColor = ptOutput.composited;
        m_ptSampleIndex++;
    } else {
        if (m_ptWasEnabled) {
            m_ptSampleIndex = 0;
            m_ptWasEnabled = false;
            if (m_rtAccelMgr) {
                m_rtAccelMgr->InvalidateSkinned();
                m_rtAccelMgr->InvalidateGrass();
            }
        }
    }

    {
        framegraph::DefaultOutputLayout fxLayout = transparentOutputs;
        fxLayout.albedo = sceneColor;
        if (rtgiActive && rtgiGuideDepth.is_valid())
            fxLayout.depth = rtgiGuideDepth;
        if (rtgiActive && rtgiGuideWorldPos.is_valid())
            fxLayout.worldPos = rtgiGuideWorldPos;
        if (rtgiActive && rtgiGuideNormal.is_valid())
            fxLayout.normal = rtgiGuideNormal;
        if (rtgiActive && rtgiGuideBaseColor.is_valid())
            fxLayout.baseColor = rtgiGuideBaseColor;
        framegraph::VirtualResourceHandle particlePrevDepth;
        if (hasPrevDepth && m_prevFrameDepth)
        {
            framegraph::ResourceDesc particlePrevDepthDesc;
            particlePrevDepthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            particlePrevDepthDesc.debugName = "rt_ParticlePrevDepth";
            particlePrevDepthDesc.width = width;
            particlePrevDepthDesc.height = height;
            particlePrevDepthDesc.format = nvrhi::Format::D32;
            particlePrevDepthDesc.isDepthStencil = true;
            particlePrevDepthDesc.isImported = true;
            particlePrevDepthDesc.isTransient = false;
            particlePrevDepth = m_framegraph->ImportTexture(
                "rt_ParticlePrevDepth", m_prevFrameDepth, particlePrevDepthDesc);
        }
        auto particleOutputs = passes::setupParticlePass(
            *m_framegraph,
            m_device,
            fxLayout,
            &m_worldParticleBatches,
            &m_hudParticleBatches,
            m_materialCache.get(),
            width,
            height,
            hizOutput.pyramid,
            hizOutput.width,
            hizOutput.height,
            hizOutput.mipLevels,
            m_hasPrevFrameData ? &m_prevViewProj : nullptr,
            particlePrevDepth,
            &m_blackboard->get_or_add<passes::ParticlePassState>()
        );

        auto trailChainLayout = particleOutputs.layout;
        if (ps_r_test_trails)
        {
            auto ribbonOutputs = passes::setupRibbonPass(
                *m_framegraph,
                m_device,
                particleOutputs.layout,
                width,
                height,
                &m_blackboard->get_or_add<passes::RibbonPassState>()
            );
            auto trailOutputs = passes::setupTrailPass(
                *m_framegraph,
                m_device,
                ribbonOutputs.layout,
                width,
                height,
                &m_blackboard->get_or_add<passes::TrailPassState>()
            );
            trailChainLayout = trailOutputs.layout;
        }

        auto smokeOutputs = trailChainLayout;
        if (ps_r_smoke_trail_enabled && m_smokeTrailManager && m_smokeTrailManager->IsReady())
        {
            smokeOutputs = passes::setupSmokeTrailPass(
                *m_framegraph,
                m_device,
                trailChainLayout,
                m_smokeTrailManager.get(),
                width,
                height,
                m_blackboard->get_or_add<passes::SmokeTrailPassState>(),
                m_detailManager ? m_detailManager->perlin4dTexture.Get() : nullptr
            );
        }

        sceneColor = smokeOutputs.albedo;
        gbufferForWet = smokeOutputs;

        const auto glowDepth = (rtgiActive && rtgiGuideDepth.is_valid())
            ? rtgiGuideDepth
            : (transparentOutputs.depth.is_valid() ? transparentOutputs.depth : depthBuffer);
        sceneColor = passes::setupGlowBillboardPass(
            *m_framegraph,
            m_device,
            sceneColor,
            glowDepth,
            width,
            height,
            m_blackboard->get_or_add<passes::GlowPassState>());

        if (particleOutputs.distortionRT.is_valid())
            pendingDistortRT = particleOutputs.distortionRT;
        else if (transparentOutputs.distortion.is_valid())
            pendingDistortRT = transparentOutputs.distortion;
        if (rtgiActive && rtgiGuideWorldPos.is_valid())
            pendingDistortWorldPos = rtgiGuideWorldPos;
        else if (transparentOutputs.worldPos.is_valid())
            pendingDistortWorldPos = transparentOutputs.worldPos;
        if (rtgiActive && rtgiGuideBaseColor.is_valid())
            pendingDistortBaseColor = rtgiGuideBaseColor;
        else if (transparentOutputs.baseColor.is_valid())
            pendingDistortBaseColor = transparentOutputs.baseColor;
    }

    passes::RainShadowOutputs rainShadowOut{};
    {
        const float rainDensity = g_pGamePersistent
            ? g_pGamePersistent->Environment().CurrentEnv.rain_density
            : 0.f;
        const bool needRainSM = (rainDensity > 0.001f) || ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF);
        if (needRainSM)
        {
            rainShadowOut = passes::setupRainShadowPass(
                *m_framegraph,
                m_device,
                bindlessConfig,
                m_blackboard->get_or_add<passes::RainShadowPassState>());
        }
    }

    passes::RainHeightmapInfo rainHeightmap{};
    if (m_detailManager && m_detailManager->HasHeightmapGPU())
    {
        rainHeightmap.texture = m_detailManager->GetHeightmapTexture();
        rainHeightmap.worldMinX = m_detailManager->GetHeightmapWorldMinX();
        rainHeightmap.worldMinZ = m_detailManager->GetHeightmapWorldMinZ();
        rainHeightmap.texelSize = m_detailManager->GetHeightmapTexelSize();
        rainHeightmap.valid = true;
    }

    if (g_pGamePersistent && g_pGamePersistent->Environment().eff_Rain)
    {
        auto* effRain = g_pGamePersistent->Environment().eff_Rain;
        effRain->Render();
        if (auto* fgRain = dynamic_cast<FGRainRender*>(effRain->GetRenderer()))
        {
            if (fgRain->HasWork())
            {
                auto rainDepth = (rtgiActive && rtgiGuideDepth.is_valid())
                    ? rtgiGuideDepth
                    : (transparentOutputs.depth.is_valid() ? transparentOutputs.depth : depthBuffer);
                auto rainWorldPos = (rtgiActive && rtgiGuideWorldPos.is_valid())
                    ? rtgiGuideWorldPos
                    : (transparentOutputs.worldPos.is_valid()
                        ? transparentOutputs.worldPos
                        : gbufferForWet.worldPos);
                sceneColor = passes::setupRainPass(
                    *m_framegraph,
                    sceneColor,
                    rainDepth,
                    rainWorldPos,
                    rainShadowOut.valid ? rainShadowOut.rainSM : framegraph::VirtualResourceHandle{},
                    fgRain,
                    rainShadowOut.sampleVP,
                    rainShadowOut.valid,
                    rainHeightmap);
            }
        }
    }

    if (g_pGamePersistent && g_pGamePersistent->Environment().eff_Thunderbolt)
    {
        auto* effTB = g_pGamePersistent->Environment().eff_Thunderbolt;
        effTB->Render();
        if (auto* fgTB = dynamic_cast<FGThunderboltRender*>(effTB->GetRenderer()))
        {
            if (fgTB->HasWork())
            {
                auto tbDepth = (rtgiActive && rtgiGuideDepth.is_valid())
                    ? rtgiGuideDepth
                    : (transparentOutputs.depth.is_valid() ? transparentOutputs.depth : depthBuffer);
                auto tbWorldPos = (rtgiActive && rtgiGuideWorldPos.is_valid())
                    ? rtgiGuideWorldPos
                    : (transparentOutputs.worldPos.is_valid()
                        ? transparentOutputs.worldPos
                        : gbufferForWet.worldPos);
                sceneColor = passes::setupThunderboltPass(
                    *m_framegraph,
                    sceneColor,
                    tbDepth,
                    tbWorldPos,
                    fgTB);
            }
        }
    }

    framegraph::VirtualResourceHandle ssrReflectionSource = sceneColor;
    {
        const float rainForRefl = g_pGamePersistent
            ? g_pGamePersistent->Environment().CurrentEnv.rain_density
            : 0.f;
        const bool needSceneReflection =
            (ps_r_ssr != 0) ||
            (rainForRefl > 0.001f && ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF));
        if (needSceneReflection)
        {
            ssrReflectionSource = passes::setupSceneReflectionCapture(
                *m_framegraph,
                m_device,
                sceneColor,
                width,
                height,
                m_blackboard->get_or_add<passes::SceneReflectionPassState>());
            m_framegraph->GetRTRegistry().RegisterRT("rt_SceneReflection", ssrReflectionSource);
        }
    }
    framegraph::VirtualResourceHandle wetNormal = gbufferForWet.normal;
    {
        passes::WetSurfacesExtras wetExtras{};
        if (rainShadowOut.rainSMTex)
        {
            wetExtras.rainSM = rainShadowOut.rainSM;
            wetExtras.rainSMTex = rainShadowOut.rainSMTex;
            wetExtras.rainSampleVP = rainShadowOut.sampleVP;
            wetExtras.rainSMValid = rainShadowOut.valid;
        }
        if (ssrReflectionSource.is_valid())
        {
            wetExtras.sceneReflection = ssrReflectionSource;
            wetExtras.sceneReflectionValid = true;
        }
        auto wetIn = gbufferForWet;
        wetIn.albedo = sceneColor;
        if (rtgiActive && rtgiGuideNormal.is_valid())
            wetIn.normal = rtgiGuideNormal;
        else if (transparentOutputs.normal.is_valid())
            wetIn.normal = transparentOutputs.normal;
        else if (normalBuffer.is_valid())
            wetIn.normal = normalBuffer;
        if (rtgiActive && rtgiGuideWorldPos.is_valid())
            wetIn.worldPos = rtgiGuideWorldPos;
        else if (transparentOutputs.worldPos.is_valid())
            wetIn.worldPos = transparentOutputs.worldPos;
        else if (worldPosBuffer.is_valid())
            wetIn.worldPos = worldPosBuffer;
        if (rtgiActive && rtgiGuideBaseColor.is_valid())
            wetIn.baseColor = rtgiGuideBaseColor;
        else if (transparentOutputs.baseColor.is_valid())
            wetIn.baseColor = transparentOutputs.baseColor;
        else if (baseColorBuffer.is_valid())
            wetIn.baseColor = baseColorBuffer;
        auto wetOut = passes::setupWetSurfacesPass(
            *m_framegraph,
            m_device,
            wetIn,
            width,
            height,
            m_blackboard->get_or_add<passes::WetSurfacesPassState>(),
            wetExtras);
        sceneColor = wetOut.albedo;
        if (wetOut.normal.is_valid())
            wetNormal = wetOut.normal;
    }

    const bool froxelVolumetrics =
        ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE) && m_volumetricRenderer && m_volumetricRenderer->IsReady()
        && g_pGamePersistent && !g_pGamePersistent->MainMenuActiveOrLevelNotExist()
        && !Device.dwPrecacheFrame && !g_pGamePersistent->IsLoadingScreenShown();

    const auto occludeDepth = (rtgiActive && rtgiGuideDepth.is_valid())
        ? rtgiGuideDepth
        : (transparentOutputs.depth.is_valid() ? transparentOutputs.depth : depthBuffer);

    if (!rtgiActive)
    {
        auto shaftDepth = occludeDepth;
        auto shaftWorldPos = gbufferForWet.worldPos.is_valid() ? gbufferForWet.worldPos : worldPosBuffer;
        sceneColor = passes::setupSunShaftsPass(
            *m_framegraph,
            m_device,
            sceneColor,
            shaftDepth,
            shaftWorldPos,
            bindlessConfig.shadowMapArray,
            bindlessConfig.shadowMapHandle,
            width,
            height,
            m_blackboard->get_or_add<passes::SunShaftsPassState>(),
            bindlessConfig.shadowCascades);
    }

    sceneColor = passes::setupSunPass(
        *m_framegraph,
        sceneColor,
        occludeDepth,
        fgEnv,
        width,
        height
    );

    if (g_pGamePersistent && g_pGamePersistent->Environment().eff_LensFlare)
    {
        auto* effLF = g_pGamePersistent->Environment().eff_LensFlare;
        effLF->Render(false, true, true);
        if (auto* fgLF = dynamic_cast<FGLensFlareRender*>(effLF->GetRenderer()))
        {
            if (fgLF->HasWork())
            {
                sceneColor = passes::setupLensFlarePass(
                    *m_framegraph,
                    sceneColor,
                    occludeDepth,
                    fgLF);
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  FROXEL VOLUMETRIC FOG (optional; off by default for classic look)
    // ═══════════════════════════════════════════════════════
    if (!rtLightingWanted && froxelVolumetrics)
    {
        passes::VolumetricLightingInputs volLighting{};
        volLighting.shadowMapArray = bindlessConfig.shadowMapArray;
        for (int i = 0; i < 3; ++i)
        {
            volLighting.shadowCascades[i] = bindlessConfig.shadowCascades[i];
            volLighting.shadowHZB[i] = bindlessConfig.shadowHZB[i];
        }
        volLighting.lightManager = &fg::ClusteredLightManager::Instance();

        sceneColor = passes::setupVolumetricPass(
            *m_framegraph,
            m_device,
            m_volumetricRenderer.get(),
            sceneColor,
            occludeDepth,
            width,
            height,
            m_blackboard->get_or_add<passes::VolumetricPassState>(),
            &m_worldParticleBatches,
            &volLighting);
    }

    if (!rtgiActive && !m_hudBatches.empty())
    {
        sceneColor = passes::setupHudOverlayPass(
            *m_framegraph,
            m_device,
            sceneColor,
            transparentOutputs.depth,
            transparentOutputs.normal,
            transparentOutputs.baseColor,
            transparentOutputs.worldPos,
            &m_hudBatches,
            m_materialCache.get(),
            width,
            height,
            m_gpuCullingManager.get(),
            &m_blackboard->get_or_add<passes::SkinningPassState>(),
            bindlessConfig.shadowMapArray,
            bindlessConfig.contactDepth,
            bindlessConfig.contactHistory,
            bindlessConfig.envSky0,
            bindlessConfig.envSky1,
            bindlessConfig.hudShadowMap,
            bindlessConfig.shadowCascades,
            bindlessConfig.localShadowAtlas,
            bindlessConfig.shadowHZB,
            bindlessConfig.shadowMask,
            bindlessConfig.localShadowESM
        );
    }

    auto postDepth = occludeDepth;
    auto postNormal = wetNormal.is_valid() ? wetNormal
        : (transparentOutputs.normal.is_valid() ? transparentOutputs.normal : gbufferForWet.normal);
    auto postBase = transparentOutputs.baseColor.is_valid() ? transparentOutputs.baseColor : baseColorBuffer;

    if (!rtLightingWanted && ps_r_contact_shadows)
    {
        passes::setupContactShadowsPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            motionOutput.motionVectors,
            width,
            height,
            m_hasPrevFrameData,
            m_blackboard->get_or_add<passes::ContactShadowsPassState>());
    }

    if (!rtLightingWanted && ps_r_ssgi && postNormal.is_valid() && postBase.is_valid())
    {
        sceneColor = passes::setupSSGIPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            postNormal,
            postBase,
            motionOutput.motionVectors,
            width,
            height,
            m_hasPrevFrameData,
            m_blackboard->get_or_add<passes::SSGIPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_SSGI", sceneColor);
    }

    if (ps_r_ssr && postNormal.is_valid() && postBase.is_valid())
    {
        const auto ssrWorldPos = (rtgiActive && rtgiGuideWorldPos.is_valid())
            ? rtgiGuideWorldPos
            : (transparentOutputs.worldPos.is_valid() ? transparentOutputs.worldPos
                : (gbufferForWet.worldPos.is_valid() ? gbufferForWet.worldPos : worldPosBuffer));
        sceneColor = passes::setupSSRPass(
            *m_framegraph,
            m_device,
            sceneColor,
            ssrReflectionSource,
            postDepth,
            postNormal,
            postBase,
            ssrWorldPos,
            motionOutput.motionVectors,
            width,
            height,
            m_hasPrevFrameData,
            m_blackboard->get_or_add<passes::SSRPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_SSR", sceneColor);
    }

    if (!rtLightingWanted && ps_r_sssss && postNormal.is_valid() && postBase.is_valid())
    {
        sceneColor = passes::setupSSSSSPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            postNormal,
            postBase,
            width,
            height,
            m_blackboard->get_or_add<passes::SSSSSPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_SSSSS", sceneColor);
    }

    if (!rtLightingWanted)
    {
        auto& iblState = m_blackboard->get_or_add<passes::IBLPrefilterPassState>();
        nvrhi::ITexture* rawSky0 = nullptr;
        nvrhi::ITexture* rawSky1 = nullptr;
        passes::ResolveEnvSkyCubes(m_device, rawSky0, rawSky1);
        passes::setupIBLProbeCapturePass(
            *m_framegraph,
            m_device,
            rawSky0,
            rawSky1,
            sceneColor,
            postDepth,
            iblState);
    }

    // TAA after SSR (skipped when temporal upscaler owns history)
    if (ps_r_taa && ps_r_upscale == 0)
    {
        sceneColor = passes::setupTAAPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            motionOutput.motionVectors,
            width,
            height,
            m_hasPrevFrameData,
            m_blackboard->get_or_add<passes::TAAPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_TAA", sceneColor);
    }

    // Classic CoP MiddleGray exposure (r2_tonemap*); NOT photographic histogram EV
    passes::ExposureConfig exposureConfig = passes::GetDefaultExposureConfig();
    auto exposureOutput = passes::setupExposurePass(
        *m_framegraph,
        m_device,
        sceneColor,
        exposureConfig,
        Device.fTimeDelta,
        width,
        height,
        m_blackboard->get_or_add<passes::ExposurePassState>()
    );
    m_exposureTexture = exposureOutput.exposureTexture;

    // FSR/DLSS or 1:1 resolve to display resolution before bloom/tonemap/UI
    {
        passes::UpscaleRRGuides rrGuides{};
        passes::UpscaleRRGuides* rrGuidesPtr = nullptr;
        if (ps_r_upscale == 2 && ps_r_dlss_rr)
        {
            auto& rtgiState = m_blackboard->get_or_add<passes::ReSTIRGIPassState>();
            rrGuides.normals = rtgiGuideNormal;
            rrGuides.baseColor = rtgiGuideBaseColor;
            rrGuides.worldPos = rtgiGuideWorldPos;
            rrGuides.depth = rtgiGuideDepth;
            rrGuides.noisyDiffuse = rtgiState.noisyDiffuse.Get();
            rrGuides.noisySpecular = rtgiState.noisySpecular.Get();
            rrGuides.hitDistance = rtgiState.hitDistance.Get();
            rrGuidesPtr = &rrGuides;
        }
        const auto upscaleDepth = (ps_r_upscale == 2 && ps_r_dlss_rr && rtgiGuideDepth.is_valid())
            ? rtgiGuideDepth
            : postDepth;
        sceneColor = passes::setupUpscaleOrResolvePass(
            *m_framegraph,
            m_device,
            sceneColor,
            upscaleDepth,
            motionOutput.motionVectors,
            exposureOutput.exposureTexture,
            g_upscaleState,
            g_upscaleBackend.get(),
            width,
            height,
            m_blackboard->get_or_add<passes::UpscalePassState>(),
            rrGuidesPtr);
    }
    const u32 postW = upscaleResolved ? displayWidth : width;
    const u32 postH = upscaleResolved ? displayHeight : height;
    const bool upscaleOwnsTemporal = ps_r_upscale != 0;

    if (pendingDistortRT.is_valid() && o.distortion)
    {
        sceneColor = passes::setupDistortionApplyPass(
            *m_framegraph,
            m_device,
            sceneColor,
            pendingDistortRT,
            pendingDistortWorldPos,
            pendingDistortBaseColor,
            postW,
            postH,
            m_blackboard->get_or_add<passes::DistortionApplyPassState>());
    }

    if (rtgiActive && m_rtAccelMgr)
    {
        const auto volWorldPos = rtgiGuideWorldPos.is_valid()
            ? rtgiGuideWorldPos
            : (transparentOutputs.worldPos.is_valid() ? transparentOutputs.worldPos : worldPosBuffer);
        sceneColor = passes::setupRTVolumetricPass(
            *m_framegraph,
            m_device,
            m_rtAccelMgr.get(),
            sceneColor,
            occludeDepth,
            volWorldPos,
            passes::g_taa_unjittered_inv_full_transform,
            Device.vCameraPosition,
            postW,
            postH,
            m_blackboard->get_or_add<passes::ReSTIRGIPassState>());
    }

    if (ps_r_bloom)
    {
        sceneColor = passes::setupBloomPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            postW,
            postH,
            m_blackboard->get_or_add<passes::BloomPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_Bloom", sceneColor);
    }

    if (!upscaleOwnsTemporal && ps_r2_ls_flags.test(R2FLAG_DOF))
    {
        sceneColor = passes::setupDofPass(
            *m_framegraph,
            m_device,
            sceneColor,
            postDepth,
            postW,
            postH,
            m_blackboard->get_or_add<passes::DofPassState>());
        m_framegraph->GetRTRegistry().RegisterRT("rt_DOF", sceneColor);
    }

    const bool needPP = m_pTarget && (m_pTarget->u_need_PP()
        || (g_pGamePersistent && g_pGamePersistent->m_pGShaderConstants
            && g_pGamePersistent->m_pGShaderConstants->m_blender_mode.x > 0.5f));
    const bool needCamera = ps_r_camera != 0 && (
        (ps_r_camera_distort_enable && ps_r_camera_distort > 1e-4f) ||
        (ps_r_camera_ca_enable && ps_r_camera_ca > 1e-4f) ||
        (ps_r_camera_vignette_enable && ps_r_camera_vignette > 1e-4f) ||
        (!upscaleOwnsTemporal && ps_r_camera_grain_enable && ps_r_camera_grain > 1e-4f) ||
        (!upscaleOwnsTemporal && ps_r_camera_mblur_enable && ps_r2_mblur > 1e-4f));
    framegraph::VirtualResourceHandle tonemapTarget = backbufferHandle;
    if (ps_r_cas || needPP || needCamera)
        tonemapTarget = framegraph::VirtualResourceHandle{};

    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        m_device,
        sceneColor,
        exposureOutput.exposureTexture,
        tonemapTarget,
        postW,
        postH,
        m_blackboard->get_or_add<passes::TonemapPassState>(),
        &m_blackboard->get_or_add<passes::ExposurePassState>()
    );

    if (needCamera)
    {
        framegraph::VirtualResourceHandle camTarget =
            (needPP || ps_r_cas) ? framegraph::VirtualResourceHandle{} : backbufferHandle;
        ldrOutput = passes::setupCameraModelPass(
            *m_framegraph,
            m_device,
            ldrOutput,
            motionOutput.motionVectors,
            camTarget,
            postW,
            postH,
            m_blackboard->get_or_add<passes::CameraModelPassState>());
    }

    if (needPP)
    {
        framegraph::VirtualResourceHandle ppTarget = ps_r_cas
            ? framegraph::VirtualResourceHandle{}
            : backbufferHandle;
        ldrOutput = passes::setupPostProcessPass(
            *m_framegraph,
            m_device,
            ldrOutput,
            ppTarget,
            postW,
            postH,
            m_pTarget,
            m_blackboard->get_or_add<passes::PostProcessPassState>());
    }

    if (ps_r_cas && backbufferHandle.is_valid())
    {
        ldrOutput = passes::setupCASPass(
            *m_framegraph,
            m_device,
            ldrOutput,
            backbufferHandle,
            postW,
            postH,
            m_blackboard->get_or_add<passes::CASPassState>());
    }

    // UI / fonts / cursor on LDR after post (menu drawn last inside UIPass)
    auto sceneWithUI = passes::setupUIPass(
        *m_framegraph,
        ldrOutput,
        postW,
        postH
    );

    sceneWithUI = passes::setupFontPass(*m_framegraph, sceneWithUI);

    sceneWithUI = passes::setupCursorPass(
        *m_framegraph,
        sceneWithUI,
        postW,
        postH
    );

    sceneWithUI = passes::setupDebugDrawPass(*m_framegraph, sceneWithUI, postW, postH);

    const bool useDlssFg = ps_r_upscale == 2 && ps_r_dlss_fg && fg::Streamline_IsFGAvailable();
    if (useDlssFg)
    {
        fg::ImGuiRendererNVRHI* imguiRendererFg = GEnv.Render->GetImGuiRendererNVRHI();
        sceneWithUI = passes::setupImGuiPass(
            *m_framegraph,
            sceneWithUI,
            imguiRendererFg,
            postW,
            postH);

        passes::setupDlssFgPass(
            *m_framegraph,
            sceneWithUI,
            framegraph::VirtualResourceHandle{},
            postDepth.is_valid() ? postDepth : depthBuffer,
            motionOutput.motionVectors,
            Device.mProject,
            m_prevViewProj,
            passes::g_taa_unjittered_full_transform,
            width,
            height,
            postW,
            postH,
            !m_hasPrevFrameData);
    }

    ldrOutput = sceneWithUI;

    // ═══════════════════════════════════════════════════════
    //  DEBUG PREVIEW PASS (Render Inspector RT visualization)
    // ═══════════════════════════════════════════════════════
    m_framegraph->GetRTRegistry().RegisterRT("rt_SceneColor", skyColorHandle);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Depth", depthBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Normal", transparentOutputs.normal);
    m_framegraph->GetRTRegistry().RegisterRT("rt_BaseColor", baseColorBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_WorldPos", worldPosBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Exposure", exposureOutput.exposureTexture);
    // Legacy r2/r3 sampler aliases (vanilla materials / particles) → FrameGraph RTs
    m_framegraph->GetRTRegistry().RegisterAliases(worldPosBuffer, {
        "$user$position", "s_position", "s_pos", "rt_Position"
    });
    m_framegraph->GetRTRegistry().RegisterAliases(transparentOutputs.normal, {
        "$user$normal", "s_normal", "rt_Normal"
    });
    m_framegraph->GetRTRegistry().RegisterAliases(baseColorBuffer, {
        "$user$albedo", "s_albedo", "rt_Albedo"
    });
    m_framegraph->GetRTRegistry().RegisterAliases(depthBuffer, {
        "$user$base_depth"
    });
    m_framegraph->GetRTRegistry().RegisterAliases(skyColorHandle, {
        "$user$generic0", "rt_Generic_0"
    });
    if (motionOutput.motionVectors.is_valid())
        m_framegraph->GetRTRegistry().RegisterRT("rt_MotionVectors", motionOutput.motionVectors);
    if (ps_r_rt_gi)
        m_framegraph->GetRTRegistry().RegisterRT("rt_RTGI_SceneColor", sceneColor);

    if (m_statsOverlay && psDeviceFlags.test(rsStatistic) && m_inspectorPreview)
    {
        auto rtNames = m_framegraph->GetRTRegistry().GetAllNames();
        m_statsOverlay->SetInspectorRTList(rtNames);

        auto selectedName = m_statsOverlay->GetSelectedRTName();
        if (selectedName.size() > 0)
        {
            auto selectedHandle = m_framegraph->GetRTRegistry().TryGetRT(selectedName.c_str());
            if (selectedHandle.is_valid())
            {
                framegraph::ResourceDesc previewDesc;
                previewDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                previewDesc.width = 512;
                previewDesc.height = 512;
                previewDesc.format = nvrhi::Format::RGBA16_FLOAT;
                previewDesc.isUAV = true;
                previewDesc.isImported = true;
                previewDesc.debugName = "debug_preview";
                auto previewHandle = m_framegraph->ImportTexture(
                    "debug_preview", m_inspectorPreview.Get(), previewDesc);

                struct DebugPreviewData {
                    framegraph::VirtualResourceHandle source;
                    framegraph::VirtualResourceHandle dest;
                    fg::RenderDevice* device;
                    u32 sourceW, sourceH;
                    int channelMode;
                    int mipLevel;
                };

                auto& previewData = m_framegraph->addCallbackPass<DebugPreviewData>(
                    "Debug Preview",
                    [&, selectedHandle, previewHandle](framegraph::FrameGraph& builder, framegraph::PassHandle pass, DebugPreviewData& data) {
                        data.device = m_device;
                        data.channelMode = m_statsOverlay ? m_statsOverlay->GetChannelMode() : 0;
                        data.mipLevel = m_statsOverlay ? m_statsOverlay->GetSelectedMipLevel() : 0;
                        data.source = selectedHandle;
                        data.dest = previewHandle;
                        auto& srcDesc = builder.GetResourceDesc(selectedHandle);
                        data.sourceW = std::max(1u, srcDesc.width >> data.mipLevel);
                        data.sourceH = std::max(1u, srcDesc.height >> data.mipLevel);
                        if (m_statsOverlay) {
                            m_statsOverlay->SetSelectedRTMipCount(srcDesc.mipLevels);
                            m_statsOverlay->SetSelectedRTSize(srcDesc.width, srcDesc.height);
                        }
                        builder.PassRead(pass, selectedHandle, framegraph::ResourceState::ShaderResource);
                        builder.PassWrite(pass, previewHandle, framegraph::ResourceState::UnorderedAccess);
                    },
                    [](const DebugPreviewData& data, const framegraph::FrameGraph& fg, fg::RenderContext* ctx) {
                        static nvrhi::ComputePipelineHandle s_pipeline;
                        static nvrhi::BindingLayoutHandle s_layout;
                        static nvrhi::BufferHandle s_cb;
                        static bool s_init = false;

                        nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                        if (!s_init) {
                            auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("debug_preview");
                            if (!csResult.handle) return;

                            s_layout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("DebugPreview", *csResult.reflection, nvDevice);

                            nvrhi::ComputePipelineDesc pipeDesc;
                            pipeDesc.CS = csResult.handle;
                            pipeDesc.bindingLayouts = { s_layout };
                            s_pipeline = nvDevice->createComputePipeline(pipeDesc);

                            nvrhi::BufferDesc cbDesc;
                            cbDesc.debugName = "DebugPreviewCB";
                            cbDesc.byteSize = 32;
                            cbDesc.isConstantBuffer = true;
                            cbDesc.isVolatile = true;
                            cbDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
                            cbDesc.keepInitialState = true;
                            cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                            s_cb = nvDevice->createBuffer(cbDesc);

                            s_init = true;
                        }

                        if (!s_pipeline) return;

                        nvrhi::ITexture* srcTex = fg.GetPhysicalTexture(data.source);
                        nvrhi::ITexture* dstTex = fg.GetPhysicalTexture(data.dest);
                        if (!srcTex || !dstTex) return;

                        struct {
                            u32 outputW, outputH;
                            u32 sourceW, sourceH;
                            u32 mode;
                            u32 mipLevel;
                            u32 pad[2];
                        } cb;
                        cb.outputW = 512; cb.outputH = 512;
                        cb.sourceW = data.sourceW; cb.sourceH = data.sourceH;
                        cb.mode = (u32)data.channelMode;
                        cb.mipLevel = (u32)data.mipLevel;
                        cb.pad[0] = cb.pad[1] = 0;

                        nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                        cmdList->writeBuffer(s_cb, &cb, sizeof(cb));

                        auto* debugRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("debug_preview", ".cs");
                        framegraph::BindingSetBuilder bsb(*debugRefl, nvDevice, "FGRenderer.Debug");
                        bsb.ConstantBuffer("DebugPreviewParams", s_cb)
                           .Texture("t_source", srcTex)
                           .TextureUAV("u_output", dstTex);
                        auto bindings = nvDevice->createBindingSet(bsb.Build(), s_layout);
                        if (!bindings) return;

                        ctx->SetComputePipeline(s_pipeline.Get());
                        ctx->SetComputeBindingSet(0, bindings.Get());
                        ctx->Dispatch((512 + 7) / 8, (512 + 7) / 8, 1);
                    }
                );
            }
        }
    }

    framegraph::VirtualResourceHandle finalOutput = ldrOutput;
    if (!useDlssFg)
    {
        fg::ImGuiRendererNVRHI* imguiRenderer = GEnv.Render->GetImGuiRendererNVRHI();
        finalOutput = passes::setupImGuiPass(
            *m_framegraph,
            ldrOutput,
            imguiRenderer,
            width,
            height
        );
    }

    m_finalOutput = finalOutput;

    // ═══════════════════════════════════════════════════════
    //  DEPTH COPY PASS (Temporal Hi-Z: save depth for next frame)
    // ═══════════════════════════════════════════════════════
    {
        nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

        if (!m_prevFrameDepth || m_prevFrameWidth != width || m_prevFrameHeight != height) {
            nvrhi::TextureDesc prevDepthDesc;
            prevDepthDesc.width = width;
            prevDepthDesc.height = height;
            prevDepthDesc.format = nvrhi::Format::D32;
            prevDepthDesc.isShaderResource = true;
            prevDepthDesc.debugName = "PrevFrameDepth";
            prevDepthDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            prevDepthDesc.keepInitialState = true;

            m_prevFrameDepth = nvDevice->createTexture(prevDepthDesc);
            m_prevFrameWidth = width;
            m_prevFrameHeight = height;
            if (m_prevFrameDepth)
                Msg("* [TemporalHiZ] Created persistent depth buffer: %dx%d", width, height);
        }

        if (m_prevFrameDepth) {
            framegraph::ResourceDesc prevDepthImportDesc;
            prevDepthImportDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            prevDepthImportDesc.debugName = "rt_PrevDepthCopyDest";
            prevDepthImportDesc.width = width;
            prevDepthImportDesc.height = height;
            prevDepthImportDesc.format = nvrhi::Format::D32;
            prevDepthImportDesc.isDepthStencil = true;
            prevDepthImportDesc.isImported = true;
            prevDepthImportDesc.isTransient = false;

            auto prevDepthCopyDest = m_framegraph->ImportTexture("rt_PrevDepthCopyDest", m_prevFrameDepth, prevDepthImportDesc);

            auto finalDepth = transparentOutputs.depth;
            framegraph::PassHandle depthCopyPass = m_framegraph->AddPass("DepthCopy");
            m_framegraph->PassRead(depthCopyPass, finalDepth, framegraph::ResourceState::CopySource);
            m_framegraph->PassWrite(depthCopyPass, prevDepthCopyDest, framegraph::ResourceState::CopyDest);
            m_framegraph->SetPassCallback(depthCopyPass,
                [finalDepth, prevDepthCopyDest](fg::RenderContext& ctx, const framegraph::FrameGraph& fg) {
                    nvrhi::ITexture* src = fg.GetPhysicalTexture(finalDepth);
                    nvrhi::ITexture* dst = fg.GetPhysicalTexture(prevDepthCopyDest);
                    if (src && dst)
                        ctx.GetCommandList()->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
                }
            );
        }
    }

    m_prevFrameWidth = width;
    m_prevFrameHeight = height;
}

void FrameGraphRenderer::PrintStats() const {
    Msg("═══════════════════════════════════════");
    Msg("  FrameGraph Renderer Statistics");
    Msg("═══════════════════════════════════════");
    Msg("  Total frame: %.2f ms (%.1f FPS)",
        m_stats.totalFrameMs,
        1000.0f / m_stats.totalFrameMs);
    Msg("  G-Buffer: %.2f ms", m_stats.gbufferMs);
    Msg("  Lighting: %.2f ms", m_stats.lightingMs);
    Msg("  Tonemap: %.2f ms", m_stats.tonemapMs);
    Msg("  Draw calls: %u", m_stats.numDrawCalls);
    Msg("  Triangles: %u", m_stats.numTriangles);
    Msg("═══════════════════════════════════════");
}

bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable, bool isStatic) {
    if (!visual)
        return false;

    // Classic HOM reject (static filtered at sector submit; dynamics here)
    if (!isStatic && !occ_visible(visual->vis))
        return false;
    
    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:           // Static mesh
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:      // Progressive mesh (LOD)
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_TREE_ST:          // SpeedTree static
        case MT_TREE_PM:          // SpeedTree progressive mesh
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:  // Skinned mesh (static)
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:  // Skinned mesh (progressive)
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        case MT_PARTICLE_EFFECT: // particles & particle groups
        case MT_PARTICLE_GROUP:
            return ProcessParticleGeometry(visual, worldTransform, renderable, false);
        default:
            return false;
    }

    if (!meshVisual)
        return false;

    // Check if geometry is valid
    if (!meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return false;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->vb || !geom->ib)
        return false;

    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

    GeometryBatch batch;
    batch.vertexBuffer = nvrhiVB.Get();
    batch.indexBuffer = nvrhiIB.Get();

    u32 visualType = visual->getType();
    bool isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    if (isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_PM) {
            const FSlideWindowItem& swi = static_cast<CSkeletonX_PM*>(visual)->GetSWI();
            if (swi.sw && swi.count > 0) {
                const FSlideWindow& sw = swi.sw[0];  // LOD 0 = max detail
                batch.indexCount = sw.num_tris * 3;
                batch.startIndex = sw.offset;  // NOT iBase + offset (skinned meshes have dedicated IB)
            } else {
                batch.indexCount = meshVisual->iCount;
                batch.startIndex = 0;
            }
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = 0;
        }
        batch.baseVertex = 0;  // Skinned meshes always have vBase = 0
    } else if (visualType == MT_PROGRESSIVE) {
        const FSlideWindowItem& swi = static_cast<FProgressive*>(visual)->GetSWI();
        if (swi.sw && swi.count > 0) {
            const FSlideWindow& sw = swi.sw[0];
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else if (visualType == MT_TREE_PM) {
        const FSlideWindowItem* pSWI = static_cast<FTreeVisual_PM*>(visual)->GetSWI();
        if (pSWI && pSWI->sw && pSWI->count > 0) {
            u32 lodIdx = 0;
            if (!isStatic) {
                Fvector center = visual->vis.sphere.P;
                const float dist = Device.vCameraPosition.distance_to(center) + EPS;
                const float r = std::max(visual->vis.sphere.R, 0.1f);
                lodIdx = u32(clampr(iFloor(dist / (r * 4.f)), 0, int(pSWI->count - 1)));
            }
            const FSlideWindow& sw = pSWI->sw[lodIdx];
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else {
        batch.indexCount = meshVisual->iCount;
        batch.startIndex = meshVisual->iBase;
        batch.baseVertex = meshVisual->vBase;
    }
    batch.vertexStride = meshVisual->vStride;
    batch.worldMatrix = worldTransform;
    batch.visual = visual;
    batch.renderable = renderable;
    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
    batch.isStatic = isStatic;
    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
        if (m_gpuCullingManager && meshVisual->p_rm_Vertices && meshVisual->p_rm_Indices) {
            u32 fmt = fg::SkinnedFormatFromRenderMode(batch.skinningRenderMode, meshVisual->vStride);
            if (m_gpuCullingManager->GetSkinnedPools().Register(
                    meshVisual->p_rm_Vertices, meshVisual->p_rm_Indices,
                    meshVisual->vCount, meshVisual->vStride, meshVisual->iCount, fmt)) {
                batch.skinnedPoolFormat = fmt;
                batch.skinnedPoolBaseVertex = (s32)meshVisual->p_rm_Vertices->skinned_pool_base_vertex;
                batch.skinnedPoolFirstIndex = meshVisual->p_rm_Vertices->skinned_pool_first_index + batch.startIndex;
            }
        }
    }
    if (visualType == MT_TREE_ST || visualType == MT_TREE_PM) {
        batch.worldBoundsCenter = visual->vis.sphere.P;
        batch.worldBoundsRadius = visual->vis.sphere.R;
    } else {
        worldTransform.transform_tiny(batch.worldBoundsCenter, visual->vis.sphere.P);
        batch.worldBoundsRadius = visual->vis.sphere.R;
    }

    float distSQ = Device.vCameraPosition.distance_to_sqr(batch.worldBoundsCenter) + EPS;
    batch.ssa = batch.worldBoundsRadius / distSQ;

    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    if (visual->shaderName.size())
        batch.debugName = visual->shaderName;
    else
        batch.debugName = "<unknown_shader>";
    batch.CacheSortFlags();

    if (!nvrhiVB || !nvrhiIB) {
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p",
            nvrhiVB.Get(), nvrhiIB.Get());
        return false;
    }

    if (m_materialCache) {
        // Check if this is terrain (uses B_BmmD blender with 4-layer detail blending)
        if (m_materialCache->IsTerrainMaterial(visual)) {
            batch.isTerrain = true;
            batch.terrainMaterialID = m_materialCache->PreRegisterTerrainMaterial(visual);
        } else {
            batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
        }
    }

    if (m_gpuCullingManager && m_gpuCullingManager->AreMegaBuffersReady()) {
        batch.megaBufferAlloc = m_gpuCullingManager->GetMeshAllocation(
            meshVisual->vbPoolID, batch.baseVertex, meshVisual->vCount,
            meshVisual->ibPoolID, batch.startIndex, batch.indexCount,
            meshVisual->useAlternativeGeom
        );

        // Dynamic/particle meshes never join mega-buffers (pool IDs stay UINT32_MAX).
        // Only warn when pools were assigned but allocation still failed.
        static int s_allocDebug = 0;
        const bool poolsAssigned =
            meshVisual->vbPoolID != UINT32_MAX && meshVisual->ibPoolID != UINT32_MAX;
        if (s_allocDebug < 10 && !batch.megaBufferAlloc.valid && poolsAssigned) {
            Msg("! [MegaBuffer] Invalid alloc: vbPool=%u, vBase=%u, vCount=%u, ibPool=%u, iBase=%u, iCount=%u, alt=%d",
                meshVisual->vbPoolID, batch.baseVertex, meshVisual->vCount,
                meshVisual->ibPoolID, batch.startIndex, batch.indexCount,
                meshVisual->useAlternativeGeom ? 1 : 0);
            s_allocDebug++;
        }
    }

    // Submit to collector
    m_geometryCollector->Submit(batch);
    return true;
}

bool FrameGraphRenderer::ProcessHudGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable) {
    if (!visual)
        return false;

    IRender_Mesh* meshVisual = nullptr;

    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        case MT_PARTICLE_EFFECT:
        case MT_PARTICLE_GROUP:
            return ProcessParticleGeometry(visual, worldTransform, renderable, true);
        default:
            return false;
    }

    if (!meshVisual)
        return false;

    if (!meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return false;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->vb || !geom->ib)
        return false;

    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

    GeometryBatch batch;
    batch.vertexBuffer = nvrhiVB.Get();
    batch.indexBuffer = nvrhiIB.Get();
    batch.indexCount = meshVisual->iCount;
    batch.startIndex = meshVisual->iBase;
    batch.baseVertex = meshVisual->vBase;
    batch.vertexStride = meshVisual->vStride;
    batch.worldMatrix = worldTransform;
    batch.visual = visual;
    batch.renderable = renderable;
    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    u32 visualType = visual->getType();
    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    if (m_materialCache) {
        batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
    }

    fg::ShaderKey shaderKey;
    if (fg::ExtractShaderKey(visual, shaderKey)) {
        static thread_local std::string s_hudDebugNameBuffer;
        s_hudDebugNameBuffer = "HUD_" + shaderKey.ToString();
        batch.debugName = s_hudDebugNameBuffer.c_str();
    } else {
        batch.debugName = "<hud_unknown_shader>";
    }

    m_hudBatches.push_back(batch);
    return true;
}

static u8 QueryParticleBlendMode(LPCSTR shaderName)
{
    u32 id = 0;
    if (!shader_info::GetParticleBlendIndex(shaderName, id))
        return passes::PARTICLE_BLEND_BLEND;
    return (id < passes::PARTICLE_BLEND_COUNT) ? (u8)id : passes::PARTICLE_BLEND_BLEND;
}

// Particle defs store classic multi-slot texture lists as "s_base[,s_distort]".
// Loading the whole string fails; particle_distort samples the 2nd slot.
static shared_str ResolveParticleTextureName(const shared_str& texList, bool preferDistortSlot)
{
    if (!texList.size() || !texList[0])
        return texList;

    const char* p = texList.c_str();
    const char* comma = strchr(p, ',');
    if (!comma)
        return texList;

    auto trimCopy = [](const char* begin, const char* end) -> shared_str {
        while (begin < end && (*begin == ' ' || *begin == '\t'))
            ++begin;
        while (end > begin && (end[-1] == ' ' || end[-1] == '\t'))
            --end;
        if (begin >= end)
            return shared_str();
        return shared_str(xr_string(begin, end - begin).c_str());
    };

    shared_str first = trimCopy(p, comma);
    const char* secondStart = comma + 1;
    while (*secondStart == ' ' || *secondStart == '\t')
        ++secondStart;
    shared_str second = (*secondStart) ? shared_str(secondStart) : shared_str();

    if (preferDistortSlot && second.size())
        return second;
    return first.size() ? first : texList;
}

void FrameGraphRenderer::ProcessSingleParticleEffect(
    fg::PS::CParticleEffect* pEffect,
    const Fmatrix& worldTransform,
    IRenderable* renderable,
    bool isHUD)
{
    if (!pEffect)
        return;

    bool isHUDParticle = isHUD || pEffect->GetHudMode();

    PAPI::Particle* particles = nullptr;
    u32 particleCount = 0;
    PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);
    if (particleCount == 0)
        return;

    auto* pDef = pEffect->GetDefinition();
    if (!pDef)
        return;

    passes::ParticleBatch batch;
    batch.visual = pEffect;
    batch.worldMatrix = worldTransform;
    batch.renderable = renderable;
    batch.isHUDMode = isHUDParticle;
    batch.particleCount = particleCount;
    batch.blendMode = QueryParticleBlendMode(pDef->m_ShaderName.c_str());
    batch.lightingMode = pDef->m_LightingMode;

    const bool isDistort = pDef->m_ShaderName.c_str() &&
        strstr(pDef->m_ShaderName.c_str(), "distort") != nullptr;
    if (isDistort)
        batch.shaderVariant = passes::ParticleShaderVariant::Distort;
    else if (batch.lightingMode == fg::PS::ParticleLightingMode::Emissive)
        batch.shaderVariant = passes::ParticleShaderVariant::Emissive;
    else if (batch.lightingMode == fg::PS::ParticleLightingMode::Lit)
        batch.shaderVariant = passes::ParticleShaderVariant::Standard;
    else if (batch.lightingMode == fg::PS::ParticleLightingMode::SixWay)
        batch.shaderVariant = passes::ParticleShaderVariant::Standard;

    if (m_materialCache && pDef->m_TextureName.size())
    {
        shared_str tex = ResolveParticleTextureName(pDef->m_TextureName, isDistort);
        batch.bindlessMaterialID = m_materialCache->PreRegisterParticleMaterial(
            tex, batch.blendMode, pDef->m_ShaderName.c_str(),
            pDef->m_SixWayPosXYZ.c_str(), pDef->m_SixWayNegXYZ.c_str());
    }

    if (isHUDParticle)
        m_hudParticleBatches.push_back(batch);
    else
        m_worldParticleBatches.push_back(batch);
}

bool FrameGraphRenderer::ProcessParticleGeometry(
    dxRender_Visual* visual,
    const Fmatrix& worldTransform,
    IRenderable* renderable,
    bool isHUD)
{
    if (!visual)
        return false;

    u32 vType = visual->getType();

    if (vType == MT_PARTICLE_EFFECT) {
        auto* pEffect = static_cast<fg::PS::CParticleEffect*>(visual);
        ProcessSingleParticleEffect(pEffect, worldTransform, renderable, isHUD);
        return true;
    }

    if (vType == MT_PARTICLE_GROUP) {
        auto* pGroup = static_cast<fg::PS::CParticleGroup*>(visual);
        for (auto& item : pGroup->items) {
            if (item._effect) {
                auto* childEffect = static_cast<fg::PS::CParticleEffect*>(item._effect);
                ProcessSingleParticleEffect(childEffect, worldTransform, renderable, isHUD);
            }
            for (auto* child : item._children_related) {
                if (child && child->getType() == MT_PARTICLE_EFFECT)
                    ProcessSingleParticleEffect(
                        static_cast<fg::PS::CParticleEffect*>(child),
                        worldTransform, renderable, isHUD);
            }
            for (auto* child : item._children_free) {
                if (child && child->getType() == MT_PARTICLE_EFFECT)
                    ProcessSingleParticleEffect(
                        static_cast<fg::PS::CParticleEffect*>(child),
                        worldTransform, renderable, isHUD);
            }
        }
        return true;
    }

    return false;
}

template <typename F>
static void ForEachLeafVisual(dxRender_Visual* pVisual, F&& fn) {
    if (!pVisual)
        return;

    switch (pVisual->Type) {
        case MT_HIERRARHY: {
            FHierrarhyVisual* pV = static_cast<FHierrarhyVisual*>(pVisual);
            for (auto& child : pV->children) {
                ForEachLeafVisual(child, fn);
            }
            break;
        }
        case MT_LOD: {
            FLOD* pV = static_cast<FLOD*>(pVisual);
            for (auto& child : pV->children) {
                ForEachLeafVisual(child, fn);
            }
            break;
        }
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            CKinematics* pV = static_cast<CKinematics*>(pVisual);
            pV->CalculateBones_InvalidateFG();
            pV->CalculateBonesFG(TRUE);

            for (auto& child : pV->children) {
                ForEachLeafVisual(child, fn);
            }

            // TODO: Also check for LOD model / progressive skinning
            //if (pV->m_lod) {
                //fn(pV->m_lod);
            //}
            break;
        }
        case MT_SKELETON_GEOMDEF_PM:
        case MT_SKELETON_GEOMDEF_ST: {
            fn(pVisual);
            break;
        }
        case MT_PROGRESSIVE: {
            fn(pVisual);
            break;
        }
        case MT_PARTICLE_GROUP: {
            PS::CParticleGroup* pG = static_cast<PS::CParticleGroup*>(pVisual);
            for (auto& item : pG->items) {
                if (item._effect)
                    ForEachLeafVisual(item._effect, fn);
                for (auto* v : item._children_related)
                    ForEachLeafVisual(v, fn);
                for (auto* v : item._children_free)
                    ForEachLeafVisual(v, fn);
            }
            break;
        }
        case MT_PARTICLE_EFFECT:
            fn(pVisual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
        case MT_NORMAL:
        default: {
            fn(pVisual);
            break;
        }
    }
}

void FrameGraphRenderer::ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs) {
    ForEachLeafVisual(pVisual, [&outLeafs](dxRender_Visual* leaf) { outLeafs.push_back(leaf); });
}

bool FrameGraphRenderer::EnsureSectorStaticCache(size_t sectorIndex, const xr_vector<fg::CSector*>& sectors)
{
    if (sectorIndex >= sectors.size())
        return false;
    if (sectorIndex >= m_sectorCacheReady.size())
        return false;
    if (m_sectorCacheReady[sectorIndex])
        return false;

    CSector* sector = sectors[sectorIndex];
    if (!sector || !sector->root())
    {
        m_sectorCacheReady[sectorIndex] = 1;
        return false;
    }

    ZoneScopedN("EnsureSectorStaticCache");

    xr_vector<dxRender_Visual*> staticVisuals;
    ExtractStaticLeafVisuals(sector->root(), staticVisuals);

    auto& sectorIds = m_sectorStaticBatchIds[sectorIndex];
    sectorIds.clear();

    for (dxRender_Visual* visual : staticVisuals)
    {
        if (!visual)
            continue;

        auto it = m_visualCacheBatchIds.find(visual);
        if (it == m_visualCacheBatchIds.end())
        {
            Fmatrix xform = Fidentity;
            switch (visual->getType())
            {
                case MT_TREE_ST:
                case MT_TREE_PM: {
                    auto* treeVisual = static_cast<FTreeVisual*>(visual);
                    xform = treeVisual->xform;
                    break;
                }
                default:
                    break;
            }

            const u32 before = static_cast<u32>(m_geometryCollector->GetBatches().size());
            if (!ProcessVisualGeometry(visual, xform, nullptr, true))
                continue;
            const u32 after = static_cast<u32>(m_geometryCollector->GetBatches().size());

            xr_vector<u32> ids;
            ids.reserve(after - before);
            const auto& allBatches = m_geometryCollector->GetBatches();
            for (u32 bi = before; bi < after; ++bi)
            {
                const u32 cacheId = static_cast<u32>(m_cachedStaticBatches.size());
                m_cachedStaticBatches.push_back(allBatches[bi]);
                ids.push_back(cacheId);
            }
            it = m_visualCacheBatchIds.emplace(visual, std::move(ids)).first;
        }

        for (u32 bi : it->second)
            sectorIds.push_back(bi);
    }

    m_sectorCacheReady[sectorIndex] = 1;
    return true;
}

void FrameGraphRenderer::CollectVisibleGeometry() {
    if (!g_pGamePersistent)
        return;

    if (!ps_r_portal_cull)
        m_HOM.Disable();

    const auto& sectors = scene_info::GetSceneSectors();
    u32 submittedStatic = 0;
    bool justBuiltStaticCache = false;

    if (!m_staticCacheInitialized && !sectors.empty())
    {
        ZoneScopedN("CollectVisibleGeometry::BuildStaticCache");
        m_sectorStaticBatchIds.assign(sectors.size(), {});
        m_sectorCacheReady.assign(sectors.size(), 0);
        m_cachedStaticBatches.clear();
        m_visualCacheBatchIds.clear();

        for (size_t si = 0; si < sectors.size(); ++si)
            EnsureSectorStaticCache(si, sectors);

        m_staticCacheInitialized = true;
        justBuiltStaticCache = true;
        if (m_gpuCullingManager)
            m_gpuCullingManager->InvalidateStaticCullingData();
        Msg("* [GeomCache] Static cache built: %zu unique batches across %zu sectors",
            m_cachedStaticBatches.size(), sectors.size());
    }

    m_portalTraverseActive = false;
    if (ps_r_portal_cull)
    {
        ZoneScopedN("CollectVisibleGeometry::PortalTraverse");
        if (m_pProcessHOMTask)
        {
            ZoneScopedN("CollectVisibleGeometry::WaitHOM");
            TaskScheduler->Wait(*m_pProcessHOMTask);
            m_pProcessHOMTask = nullptr;
        }
        CFrustum viewFrustum;
        viewFrustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

        IRender_Sector::sector_id_t sid = Scene.detect_sector(Device.vCameraPosition);
        if (sid == IRender_Sector::INVALID_SECTOR_ID)
            sid = m_last_sector_id;
        else
            m_last_sector_id = sid;

        if (sid != IRender_Sector::INVALID_SECTOR_ID && sid < sectors.size() && sectors[sid])
        {
            // HOM is an opt-in refinement (r_hom). The CPU raster can false-cull
            // portal polys → whole rooms vanish. Frustum-only traversal is the
            // stable PVS: it only skips sectors unreachable through visible portals.
            const bool useHom = ps_r_hom != 0;
            const u32 traverseOptions = useHom ? CPortalTraverser::VQ_HOM : 0u;
            if (useHom)
                m_HOM.Enable();
            else
                m_HOM.Disable();
            PortalTraverser.traverse(
                sectors[sid],
                viewFrustum,
                Device.vCameraPosition,
                Device.mFullTransform,
                traverseOptions,
                useHom ? &m_HOM : nullptr);

            // Outdoor (largest) sector owns trees/terrain/bushes. Starting from an
            // indoor/near-door sector, portal PVS drops it as soon as outdoor
            // portals leave the frustum — trees vanish on camera yaw. Always keep
            // outdoor in the visible set; frustum/HOM still cull individual batches.
            const auto outdoorId = Scene.largest_sector_id;
            if (outdoorId != IRender_Sector::INVALID_SECTOR_ID &&
                outdoorId < sectors.size() && sectors[outdoorId])
            {
                CSector* outdoor = sectors[outdoorId];
                if (outdoor->r_marker != PortalTraverser.i_marker)
                {
                    outdoor->r_marker = PortalTraverser.i_marker;
                    outdoor->r_frustums.clear();
                    outdoor->r_frustums.push_back(viewFrustum);
                    outdoor->r_scissors.clear();
                    PortalTraverser.r_sectors.push_back(outdoor);
                }
            }

            m_portalTraverseActive = !PortalTraverser.r_sectors.empty();
        }
    }

    if (m_gpuCullingManager)
        m_gpuCullingManager->SetStaticBatchSource(
            m_staticCacheInitialized ? &m_cachedStaticBatches : nullptr);

    if (!justBuiltStaticCache && m_staticCacheInitialized)
    {
        ZoneScopedN("CollectVisibleGeometry::StaticSubmit");
        const u32 batchCount = static_cast<u32>(m_cachedStaticBatches.size());
        if (m_staticSubmitMark.size() < batchCount)
            m_staticSubmitMark.resize(batchCount, 0);
        if (++m_staticSubmitEpoch == 0)
        {
            std::fill(m_staticSubmitMark.begin(), m_staticSubmitMark.end(), 0);
            m_staticSubmitEpoch = 1;
        }
        const u8 epoch = m_staticSubmitEpoch;

        const bool gpuOpaqueStaticResident =
            m_gpuCullingManager && m_gpuCullingManager->IsEnabled();
#if defined(XR_PLATFORM_APPLE)
        const bool tessFlagOn = false;
#else
        const bool tessFlagOn = ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);
#endif

        auto submitBatchId = [&](u32 bi) {
            if (bi >= batchCount || m_staticSubmitMark[bi] == epoch)
                return;
            m_staticSubmitMark[bi] = epoch;
            auto& batch = m_cachedStaticBatches[bi];

            if (gpuOpaqueStaticResident)
            {
                if (batch.isTerrain)
                    return;
                if (!batch.sortFlagsValid)
                    batch.CacheSortFlags();
                if (!batch.IsStrictB2F())
                {
                    bool routeTess = false;
                    if (tessFlagOn)
                    {
                        constexpr u32 kMaxTessIndexCount = 8192;
                        const auto* mat = bindless::MaterialBuffer::Instance().GetMaterial(batch.bindlessMaterialID);
                        routeTess = mat && mat->tessMethod != 0 && batch.indexCount > 0 &&
                            batch.indexCount <= kMaxTessIndexCount;
                    }
                    if (!routeTess)
                        return;
                }
            }

            if (batch.visual && !occ_visible(batch.visual->vis))
                return;
            if (!batch.sortFlagsValid)
                batch.CacheSortFlags();
            m_geometryCollector->SubmitStatic(batch);
            submittedStatic++;
        };

        if (m_portalTraverseActive)
        {
            for (CSector* s : PortalTraverser.r_sectors)
            {
                if (!s || s->unique_id >= m_sectorStaticBatchIds.size())
                    continue;
                for (u32 bi : m_sectorStaticBatchIds[s->unique_id])
                    submitBatchId(bi);
            }
        }
        else
        {
            for (u32 bi = 0; bi < batchCount; ++bi)
                submitBatchId(bi);
        }
    }

    u32 submittedDynamic = 0;
    u32 notRenderable = 0;

    xr_vector<const light*> collectedLights;
    collectedLights.reserve(256);

    for (ISpatial* spatial : m_lstRenderables)
    {
        const auto& data = spatial->GetSpatialData();

        if (data.type & STYPE_LIGHTSOURCE) {
            const light* L = (const light*)spatial->dcast_Light();
            if (L)
                collectedLights.push_back(L);
            continue;
        }

        if (m_portalTraverseActive && data.sector_id != IRender_Sector::INVALID_SECTOR_ID &&
            data.sector_id < sectors.size())
        {
            CSector* sec = sectors[data.sector_id];
            if (sec && sec->r_marker != PortalTraverser.i_marker)
                continue;
        }

        IRenderable* renderable = spatial->dcast_Renderable();
        if (!renderable) {
            notRenderable++;
            continue;
        }

        renderable->renderable_Render(0, renderable);
        submittedDynamic++;
    }

    if (!collectedLights.empty())
        fg::ClusteredLightManager::Instance().CollectLightsParallel(collectedLights);

    if (g_pGameLevel && g_pGameLevel->pHUD) {
        g_pGameLevel->pHUD->Render_Last(0);
    }
}

void FrameGraphRenderer::add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& xform) {
    if (!V) {
        return;  // No visual to add
    }

    dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(V);
    if (!visual) {
        return;  // Not a valid visual type
    }
    
    bool isHUD = (root && root->renderable_HUD());

    if (!isHUD && visual->Type == MT_LOD)
    {
        auto* lod = static_cast<FLOD*>(visual);
        Fvector center;
        xform.transform_tiny(center, lod->vis.sphere.P);
        const float distSQ = Device.vCameraPosition.distance_to_sqr(center) + EPS;
        const float ssa = lod->vis.sphere.R / distSQ;
        const float gScreen = float(Device.dwWidth) * float(Device.dwHeight);
        const float ssaStart = _sqr(ps_r__GLOD_ssa_start) / std::max(gScreen, 1.f);
        if (ssa * lod->lod_factor < ssaStart)
        {
            m_lodImpostors.push_back({lod, xform});
            return;
        }
    }

    ForEachLeafVisual(visual, [&](dxRender_Visual* leafVisual) {
        if (isHUD) {
            // HUD geometry - separate processing with different projection/culling
            ProcessHudGeometry(leafVisual, xform, root);
        } else {
            // World geometry - standard processing
            ProcessVisualGeometry(leafVisual, xform, root);
        }
    });
}

xr_set<framegraph::RenderPhase> FrameGraphRenderer::ScanRequiredPhases() const {
    xr_set<framegraph::RenderPhase> phases;

    auto& batches = const_cast<GeometryCollector*>(m_geometryCollector.get())->GetBatchesMutable();
    xr_map<framegraph::RenderPhase, u32> phaseCount;

    for (auto& batch : batches) {
        if (!batch.visual) {
            continue;
        }

        framegraph::RenderPhase phase = m_shaderPhaseCache->GetPhase(batch.visual);
        batch.renderPhase = phase;
        phases.insert(phase);
        phaseCount[phase]++;
    }

    return phases;
}

void FrameGraphRenderer::CreatePhasePass(framegraph::RenderPhase phase) {
    PassEntry entry;
    entry.phase = phase;

    switch (phase) {
        case framegraph::RenderPhase::Geometry: {
            return;
        }

        case framegraph::RenderPhase::Lighting:
        case framegraph::RenderPhase::PostProcess:
        case framegraph::RenderPhase::Combine:
        case framegraph::RenderPhase::Shadow:
        case framegraph::RenderPhase::Custom:
        default:
            return;
    }
}

void FrameGraphRenderer::CreateAllRequiredPasses() {
    xr_set<framegraph::RenderPhase> requiredPhases = ScanRequiredPhases();
    for (framegraph::RenderPhase phase : requiredPhases) {
        CreatePhasePass(phase);
    }
}

void FrameGraphRenderer::RouteBatchesToPasses() {
    auto& batches = m_geometryCollector->GetBatchesMutable();
    xr_map<framegraph::RenderPhase, xr_vector<GeometryBatch*>> batchesByPhase;

    for (auto& batch : batches) {
        batchesByPhase[batch.renderPhase].push_back(&batch);
    }

    for (const auto& [phase, phaseBatches] : batchesByPhase) {
        const char* phaseName = framegraph::IPass::GetPhaseName(phase);

        switch (phase) {
            case framegraph::RenderPhase::Geometry:
                break;

            case framegraph::RenderPhase::Lighting:
            case framegraph::RenderPhase::PostProcess:
            case framegraph::RenderPhase::Combine:
            case framegraph::RenderPhase::Shadow:
            case framegraph::RenderPhase::Custom:
            default:
                break;
        }
    }
}

void FrameGraphRenderer::RenderImGui(ImDrawData* drawData, fg::ImGuiRendererNVRHI* imguiRenderer) {
    if (!drawData || drawData->TotalVtxCount == 0)
        return;

    if (!imguiRenderer) {
        static bool warned = false;
        if (!warned) {
            Msg("! [FrameGraphRenderer] ImGui renderer not provided");
            warned = true;
        }
        return;
    }

    nvrhi::ITexture* finalTexture = m_framegraph->GetPhysicalTexture(m_finalOutput);
    if (!finalTexture) {
        Msg("! [FrameGraphRenderer] Failed to get final output texture for ImGui");
        return;
    }
    
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(nvrhi::TextureHandle(finalTexture));

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [FrameGraphRenderer] Failed to create framebuffer for ImGui");
        return;
    }

    nvrhi::ICommandList* cmdList = m_device->GetImmediateCommandList();
    if (!cmdList) {
        Msg("! [FrameGraphRenderer] No command list available for ImGui");
        return;
    }

    imguiRenderer->Render(drawData, framebuffer.Get(), cmdList);
}

void FrameGraphRenderer::UpdateSmokeTrail(
    const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode)
{
    if (!m_smokeTrailManager || !m_smokeTrailManager->IsReady())
        return;

    Fvector correctedPos = muzzlePos;
    Fvector correctedDir = muzzleDir;

    if (isHUDMode)
    {
        const Fmatrix hudMat = BuildHUDFOVMatrix();
        hudMat.transform_tiny(correctedPos);
        hudMat.transform_dir(correctedDir);
        correctedDir.normalize_safe();
    }

    m_smokeTrailManager->Update(dt, correctedPos, correctedDir);
}

void FrameGraphRenderer::NotifySmokeShot()
{
    // TODO: forward to m_smokeTrailManager->OnShot() when heat system is added
}

namespace
{
class CGlow : public IRender_Glow
{
public:
    light* m_light = nullptr;
    Fvector m_pos{};
    Fvector m_dir{ 0.f, -1.f, 0.f };
    float m_radius = 1.f;
    Fcolor m_color{ 1.f, 1.f, 1.f, 1.f };
    shared_str m_texture;
    bool bActive{ false };

    explicit CGlow(light* L) : m_light(L)
    {
        fg::passes::GlowRegistry_Register(this);
        if (!m_light)
            return;
        m_light->flags.type = IRender_Light::POINT;
        m_light->flags.bShadow = false;
        m_light->flags.bVolumetric = false;
        m_light->set_active(false);
    }

    ~CGlow() override
    {
        fg::passes::GlowRegistry_Unregister(this);
        if (m_light)
        {
            m_light->set_active(false);
            xr_delete(m_light);
        }
    }

    static bool CollectBillboard(void* glow, fg::passes::GlowBillboard& out)
    {
        auto* g = static_cast<CGlow*>(glow);
        if (!g || !g->bActive)
            return false;
        out.pos = g->m_pos;
        out.radius = g->m_radius;
        out.color = g->m_color;
        out.texture = g->m_texture;
        return true;
    }

    void SyncLight()
    {
        if (!m_light)
            return;
        m_light->set_position(m_pos);
        m_light->set_rotation(m_dir, m_dir);
        m_light->set_range(_max(m_radius * 2.5f, 0.5f));
        m_light->set_color(m_color);
        m_light->set_active(bActive);
    }

    void set_active(bool b) override
    {
        bActive = b;
        SyncLight();
    }
    bool get_active() override { return bActive; }
    void set_position(const Fvector& P) override
    {
        m_pos = P;
        SyncLight();
    }
    void set_direction(const Fvector& D) override
    {
        m_dir = D;
        if (m_dir.magnitude() < 1e-4f)
            m_dir.set(0.f, -1.f, 0.f);
        else
            m_dir.normalize();
        SyncLight();
    }
    void set_radius(float R) override
    {
        m_radius = _max(R, 0.05f);
        SyncLight();
    }
    void set_texture(LPCSTR name) override { m_texture = name; }
    void set_color(const Fcolor& C) override
    {
        m_color = C;
        SyncLight();
    }
    void set_color(float r, float g, float b) override
    {
        m_color.set(r, g, b, 1.f);
        SyncLight();
    }
};

float EstimateSplatUVRadius(const fg::decals::MeshPickResult& pickResult, float worldRadius)
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
}

IRender_ObjectSpecific* FrameGraphRenderer::ros_create(IRenderable*) { return xr_new<CROS_impl>(); }
void FrameGraphRenderer::ros_destroy(IRender_ObjectSpecific*& p) { xr_delete(p); }

IRender_Light* FrameGraphRenderer::light_create() { return Lights.Create(); }
IRender_Glow* FrameGraphRenderer::glow_create()
{
    static bool s_glowCollectHooked = false;
    if (!s_glowCollectHooked)
    {
        fg::passes::GlowRegistry_SetCollect(&CGlow::CollectBillboard);
        s_glowCollectHooked = true;
    }
    return xr_new<CGlow>(static_cast<light*>(Lights.Create()));
}

IRenderVisual* FrameGraphRenderer::model_Create(pcstr name, IReader* data)        { return g_pModelPool->Create(name, data); }
IRenderVisual* FrameGraphRenderer::model_CreateChild(pcstr name, IReader* data)   { return g_pModelPool->CreateChild(name, data); }
IRenderVisual* FrameGraphRenderer::model_Duplicate(IRenderVisual* V)              { return g_pModelPool->Instance_Duplicate((fg::dxRender_Visual*)V); }

void FrameGraphRenderer::model_Delete(IRenderVisual*& V, bool bDiscard)
{
    auto* pVisual = (fg::dxRender_Visual*)V;
    g_pModelPool->Delete(pVisual, bDiscard);
    V = nullptr;
}

IRenderVisual* FrameGraphRenderer::model_CreateParticles(pcstr name)
{
    fg::PS::CPEDef* SE = m_PSLibrary.FindPED(name);
    if (SE)
        return g_pModelPool->CreatePE(SE);

    fg::PS::CPGDef* SG = m_PSLibrary.FindPGD(name);
    R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
    return g_pModelPool->CreatePG(SG);
}

void FrameGraphRenderer::model_Logging(bool bEnable) { g_pModelPool->Logging(bEnable); }
void FrameGraphRenderer::models_Prefetch()           { g_pModelPool->Prefetch(); }
void FrameGraphRenderer::models_Clear(bool b)        { g_pModelPool->ClearPool(b); }

bool FrameGraphRenderer::occ_visible(vis_data& V) { return m_HOM.visible(V); }
bool FrameGraphRenderer::occ_visible(Fbox& B)     { return m_HOM.visible(B); }
bool FrameGraphRenderer::occ_visible(sPoly& P)    { return m_HOM.visible(P); }

IRenderVisual* FrameGraphRenderer::getVisual(int id) { return fg::BufferPool.getVisual(id); }

void FrameGraphRenderer::add_Visual(u32, IRenderable* root, IRenderVisual* V, Fmatrix& m)
{
    if (IsEnabled())
        add_Visual(root, V, m);
}

void FrameGraphRenderer::add_StaticWallmark(const wm_shader&, const Fvector&, float, CDB::TRI*, Fvector*)
{
}

void FrameGraphRenderer::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
    if (!T || T->suppress_wm || !V || s <= EPS_L)
        return;
    auto* fgArray = static_cast<fg::decals::fgWallMarkArray*>(pArray);
    shared_str wallmarkTexture;
    u32 matID = fgArray->GenerateBindlessMaterialID(&wallmarkTexture);
    if (matID == UINT32_MAX)
        return;
    Fvector N;
    N.mknormal(V[T->verts[0]], V[T->verts[1]], V[T->verts[2]]);
    float decalSize = s * 2.0f;
    GetDecalManager()->AddStaticDecal(P, N, decalSize, matID, wallmarkTexture.c_str());
}

void FrameGraphRenderer::clear_static_wallmarks()
{
    if (m_decalManager)
        m_decalManager->Clear();
}

void FrameGraphRenderer::add_SkeletonWallmark(
    const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size)
{
    if (!xf || !obj || size <= EPS_L)
        return;
    float distSq = xf->c.distance_to_sqr(Device.vCameraPosition);
    if (distSq > _sqr(50.f))
        return;

    const u32 splatMode = ps_r4_skeleton_wallmark_mode > 0
        ? fg::decals::SPLAT_MODE_PROCEDURAL_BLOOD
        : fg::decals::SPLAT_MODE_DECAL;

    shared_str wallmarkTexture;
    u32 matID = UINT32_MAX;
    if (splatMode == fg::decals::SPLAT_MODE_DECAL)
    {
        if (!pArray)
            return;
        auto* fgArray = static_cast<fg::decals::fgWallMarkArray*>(pArray);
        matID = fgArray->GenerateBindlessMaterialID(&wallmarkTexture);
        if (matID == UINT32_MAX)
            return;
    }
    float decalSize = size * 2.0f;
    fg::decals::MeshPickResult entryPick;
    if (fg::decals::PickMeshDirect((CKinematics*)obj, *xf, start, dir, 100.f, entryPick))
    {
        auto* overlayMgr = GetOverlayManager();
        float worldRadius = decalSize * 0.5f;
        Fvector bloodColor = { 0.4f, 0.02f, 0.02f };
        const float lifetime = splatMode == fg::decals::SPLAT_MODE_PROCEDURAL_BLOOD ? 18.f : 12.f;

        auto queueSplat = [&](const fg::decals::MeshPickResult& pick)
        {
            const float uvRadius = EstimateSplatUVRadius(pick, worldRadius);
            overlayMgr->AddSplat((CKinematics*)obj, pick.triVerts,
                                  pick.baryU, pick.baryV,
                                  worldRadius, bloodColor, 0.8f,
                                  pick.uv, uvRadius, matID,
                                  splatMode, lifetime,
                                  pick.hitTextureName.c_str(),
                                  wallmarkTexture.c_str());
        };

        queueSplat(entryPick);

        Fvector shotDir = dir;
        shotDir.normalize_safe();
        const float entryAdvance = entryPick.dist + 0.01f;
        const float remaining = 100.f - entryAdvance;
        if (remaining > 0.01f)
        {
            Fvector exitStart;
            exitStart.mad(start, shotDir, entryAdvance);

            fg::decals::MeshPickResult exitPick;
            if (fg::decals::PickMeshDirect((CKinematics*)obj, *xf, exitStart, shotDir, remaining, exitPick))
            {
                const float minSeparation = _max(0.02f, worldRadius * 0.25f);
                if (entryPick.worldPos.distance_to(exitPick.worldPos) > minSeparation)
                    queueSplat(exitPick);
            }
        }
    }
}

CompiledLevelShader* FrameGraphRenderer::getCompiledShader(int id)
{
    if (id < 0 || id >= int(m_CompiledLevelShaders.size()))
        return nullptr;
    return &m_CompiledLevelShaders[id];
}

bool FrameGraphRenderer::getShaderHandles(int id, nvrhi::ShaderHandle& outVS, nvrhi::ShaderHandle& outPS)
{
    auto* compiled = getCompiledShader(id);
    if (!compiled || !compiled->vsHandle || !compiled->psHandle)
        return false;
    outVS = compiled->vsHandle;
    outPS = compiled->psHandle;
    return true;
}

void FrameGraphRenderer::Calculate() {}

void FrameGraphRenderer::OnFrame()
{
    ZoneScoped;
    g_pModelPool->DeleteQueue();
    if (g_pGamePersistent->MainMenuActiveOrLevelNotExist())
        return;
}

void FrameGraphRenderer::OnCameraUpdated()
{
    ZoneScoped;
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
    if (g_pGamePersistent->MainMenuActiveOrLevelNotExist())
        return;
    // Don't run HOM during save/level load — blocks workers and races with loading
    if (g_pGamePersistent->IsLoadingScreenShown())
        return;
    // HOM only when explicitly opted in (r_hom) alongside portal traversal.
    // Without it, portal PVS is frustum-only (stable) and occ_visible() no-ops
    // (bEnabled=false → visible), so nothing is HOM-rejected.
    if (ps_r_portal_cull && ps_r_hom)
        m_pProcessHOMTask = &m_HOM.DispatchMTRender();
    else
        m_HOM.Disable();
}

namespace
{
IReader* fg_open_shader(pcstr shader)
{
    string_path sname;
    strconcat(sname, "r5\\", shader);
    return FS.r_open("$game_shaders$", sname);
}

bool ssao_hdao_cs_shaders_exist()
{
    IReader* hdao_ps = fg_open_shader("ao\\hdao.ps");
    if (!hdao_ps)
        hdao_ps = fg_open_shader("ao/hdao.ps");
    const bool exist = hdao_ps != nullptr;
    FS.r_close(hdao_ps);
    return exist;
}
}

void FrameGraphRenderer::create()
{
    ZoneScoped;

    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345679);

    m_skinning = -1;
    m_MSAASample = -1;

    const auto& caps = GEnv.Backend->GetCapabilities();
    o.mrt = (caps.raster.dwMRT_count >= 3);
    o.mrtmixdepth = (caps.raster.b_MRT_mixdepth);
    o.nullrt = false;

    o.HW_smap_FETCH4 = FALSE;
    o.HW_smap = true;
    o.HW_smap_PCF = o.HW_smap;
    if (o.HW_smap)
    {
        if (caps.id_vendor == 0x1002)
            o.HW_smap_FORMAT = nvrhi::Format::D32;
        else
            o.HW_smap_FORMAT = nvrhi::Format::D24S8;
    }

    o.fp16_filter = true;
    o.fp16_blend = true;

    if (strstr(Core.Params, "-r4xx"))
    {
        o.mrtmixdepth = FALSE;
        o.HW_smap = FALSE;
        o.HW_smap_PCF = FALSE;
        o.fp16_filter = FALSE;
        o.fp16_blend = FALSE;
    }

    if (o.mrtmixdepth)        o.albedo_wo = FALSE;
    else if (o.fp16_blend)    o.albedo_wo = FALSE;
    else                      o.albedo_wo = TRUE;

    o.nvstencil = FALSE;
    o.nvdbt = false;
    o.ffp = false;

    if      (strstr(Core.Params, "-smap1024")) o.smapsize = 1024;
    else if (strstr(Core.Params, "-smap1536")) o.smapsize = 1536;
    else if (strstr(Core.Params, "-smap2048")) o.smapsize = 2048;
    else if (strstr(Core.Params, "-smap2560")) o.smapsize = 2560;
    else if (strstr(Core.Params, "-smap3072")) o.smapsize = 3072;
    else if (strstr(Core.Params, "-smap4096")) o.smapsize = 4096;
    else if (strstr(Core.Params, "-smap8192")) o.smapsize = 8192;
    else                                       o.smapsize = ps_r2_smapsize;

    cpcstr g = strstr(Core.Params, "-gloss ");
    o.forcegloss = g ? TRUE : FALSE;
    if (g) o.forcegloss_v = float(atoi(g + xr_strlen("-gloss "))) / 255.f;

    o.bug = (strstr(Core.Params, "-bug")) ? TRUE : FALSE;
    o.sunfilter = (strstr(Core.Params, "-sunfilter")) ? TRUE : FALSE;
    o.sunstatic = ps_r2_sun_static;
    o.advancedpp = ps_r2_advanced_pp;
    o.volumetricfog = ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE);
    o.sjitter = (strstr(Core.Params, "-sjitter")) ? TRUE : FALSE;
    o.depth16 = (strstr(Core.Params, "-depth16")) ? TRUE : FALSE;
    o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
    o.Tshadows = (strstr(Core.Params, "-tsh")) ? TRUE : FALSE;
    o.oldshadowcascades = ps_r2_ls_flags_ext.test(R2FLAGEXT_SUN_OLD);
    o.mblur = (strstr(Core.Params, "-mblur")) ? TRUE : FALSE;
    o.distortion_enabled = (strstr(Core.Params, "-nodistort")) ? FALSE : TRUE;
    o.distortion = o.distortion_enabled;
    o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
    o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

    o.ssao_blur_on = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) && (ps_r_ssao != 0);
    o.ssao_opt_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_OPT_DATA) && (ps_r_ssao != 0);
    o.ssao_half_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA) && o.ssao_opt_data && (ps_r_ssao != 0);
    o.ssao_hdao = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HDAO) && (ps_r_ssao != 0);
    o.ssao_ultra = ssao_hdao_cs_shaders_exist();
    o.ssao_hbao = !o.ssao_hdao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HBAO) && (ps_r_ssao != 0);
    o.hbao_vectorized = (o.ssao_hbao && caps.id_vendor == 0x1002);

    o.dx11_sm4_1 = ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);

    o.msaa = !!ps_r3_msaa;
    o.msaa_samples = (1 << ps_r3_msaa);
    o.msaa_opt = ps_r2_ls_flags.test(R3FLAG_MSAA_OPT);
    o.msaa_opt = (o.msaa_opt && o.msaa) || o.msaa;
    o.msaa_hybrid = ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);
    o.msaa_hybrid &= !o.msaa_opt && o.msaa;

    o.msaa_alphatest = 0;
    if (o.msaa)
    {
        if (o.msaa_opt || o.msaa_hybrid)
        {
            if (ps_r3_msaa_atest == 1) o.msaa_alphatest = FrameGraphRenderer::MSAA_ATEST_DX10_1_ATOC;
            else if (ps_r3_msaa_atest == 2) o.msaa_alphatest = FrameGraphRenderer::MSAA_ATEST_DX10_1_NATIVE;
        }
        else if (ps_r3_msaa_atest)
            o.msaa_alphatest = FrameGraphRenderer::MSAA_ATEST_DX10_0_ATOC;
    }

    o.gbuffer_opt = ps_r2_ls_flags.test(R3FLAG_GBUFFER_OPT);
    o.minmax_sm = ps_r3_minmax_sm;
    o.minmax_sm_screenarea_threshold = 1600 * 1200;

    o.tessellation = ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);
#if defined(XR_PLATFORM_APPLE)
    // Metal tessellation allocates temporary patch buffers in IOGPU. Over-subscription
    // (many NPC/level patches × factor) has caused hard kernel panics:
    //   IOGPUGroupMemory::remove_memory_object() memory object not found
    // Parallax (r2_steep_parallax) is the supported relief path on macOS.
    if (o.tessellation || ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION))
    {
        Msg("! [Tessellation] Forcibly disabled on Apple/MoltenVK — Metal tess temp "
            "buffers can panic the kernel (IOGPUGroupMemory). Use r2_steep_parallax for relief.");
        ps_r2_ls_flags_ext.set(R2FLAGEXT_ENABLE_TESSELLATION, FALSE);
        o.tessellation = false;
    }
#endif
    o.support_rt_arrays = true;

    if (o.minmax_sm == FrameGraphRenderer::MMSM_AUTODETECT)
    {
        o.minmax_sm = FrameGraphRenderer::MMSM_OFF;
        if (caps.id_vendor == 0x1002)
        {
            if (ps_r_sun_quality >= 3) o.minmax_sm = FrameGraphRenderer::MMSM_AUTO;
            else if (ps_r_sun_shafts >= 2)
            {
                o.minmax_sm = FrameGraphRenderer::MMSM_AUTODETECT;
                o.minmax_sm_screenarea_threshold = 1600 * 1200;
            }
        }
        if (caps.id_vendor == 0x10DE)
        {
            if (ps_r_sun_shafts >= 2)
            {
                o.minmax_sm = FrameGraphRenderer::MMSM_AUTODETECT;
                o.minmax_sm_screenarea_threshold = 1280 * 1024;
            }
        }
    }

    if (!GEnv.Backend || !GEnv.Backend->IsInitialized())
        return;

    auto* renderDevice = xr_new<fg::RenderDevice>();
    if (!renderDevice->InitializeFromBackend(GEnv.Backend))
    {
        Msg("! RenderDevice initialization failed");
        xr_delete(renderDevice);
        return;
    }
    m_device = renderDevice;

    if (!Initialize(m_device))
    {
        Msg("! FrameGraphRenderer initialization failed");
        return;
    }
    GEnv.Render = this;

    Vertex.Create();
    Index.Create();
    CreateQuadIB();

    InitializeImGuiRenderer(m_device);
    GEnv.UIRender = GetUIRender();

    MaterialSystem::Instance().Initialize(m_device->GetFGResourceManager(), GetShaderLoader());

    SetEnabled(true);

    m_pTarget = xr_new<fg::CRenderTarget>();

    if (!g_pModelPool)
        g_pModelPool = xr_new<CModelPool>();

    m_PSLibrary.OnCreate();
    m_HWOCC.occq_create(occq_size);
}

void FrameGraphRenderer::destroy()
{
    QuadIB.Release();
    Index.Destroy();
    Vertex.Destroy();

    m_HWOCC.occq_destroy();
    m_PSLibrary.OnDestroy();

    xr_delete(m_pTarget);

    if (g_pModelPool)
    {
        xr_delete(g_pModelPool);
        g_pModelPool = nullptr;
    }

    MaterialSystem::Instance().Shutdown();

    ShutdownImGuiRenderer();

    Shutdown();

    if (m_device)
    {
        m_device->Shutdown();
        xr_delete(m_device);
        m_device = nullptr;
    }

    Device.seqFrame.Remove(this);
}

void FrameGraphRenderer::reset_begin()
{
    ZoneScoped;
    if (Resources)
        Resources->reset_begin();

    m_Lights_LastFrame.clear();

    xr_delete(m_pTarget);
    m_HWOCC.occq_destroy();
}

void FrameGraphRenderer::reset_end()
{
    ZoneScoped;
    m_HWOCC.occq_create(occq_size);
    m_pTarget = xr_new<fg::CRenderTarget>();

    m_bFirstFrameAfterReset = true;
}

void FrameGraphRenderer::OnBackBufferResizing(u32, u32)
{
    ZoneScoped;
    framegraph::GetPassResourceCache().ClearFramebufferDependent();
    if (m_materialCache)
        m_materialCache->Clear();
    if (m_uiMaterialCache)
        m_uiMaterialCache->Clear();
}

void FrameGraphRenderer::OnBackBufferResized(u32, u32)
{
    ZoneScoped;
    m_bFirstFrameAfterReset = true;
}

void FrameGraphRenderer::SetPostProcessParams(const SPPInfo& ppi)
{
    if (!m_pTarget)
        return;
    m_pTarget->set_blur(ppi.blur);
    m_pTarget->set_gray(ppi.gray);
    m_pTarget->set_duality_h(ppi.duality.h);
    m_pTarget->set_duality_v(ppi.duality.v);
    m_pTarget->set_noise(ppi.noise.intensity);
    m_pTarget->set_noise_scale(ppi.noise.grain);
    m_pTarget->set_noise_fps(ppi.noise.fps);
    m_pTarget->set_color_base(ppi.color_base);
    m_pTarget->set_color_gray(ppi.color_gray);
    m_pTarget->set_color_add(ppi.color_add);
    m_pTarget->set_cm_imfluence(ppi.cm_influence);
    m_pTarget->set_cm_interpolate(ppi.cm_interpolate);
    m_pTarget->set_cm_textures(ppi.cm_tex1, ppi.cm_tex2);
}

void FrameGraphRenderer::Screenshot(IRender::ScreenshotMode mode, pcstr name)
{
    UNUSED(mode);
    UNUSED(name);
    Msg("! Screenshot not yet implemented for FrameGraph renderer");
}

void FrameGraphRenderer::RequestGrassInteraction(const Fvector& world_pos, float radius, float strength, uint8_t type)
{
}

bool FrameGraphRenderer::SampleTerrainHeight(float x, float z, float& outY)
{
    if (!m_detailManager)
        return false;
    return m_detailManager->SampleHeight(x, z, outY);
}

void FrameGraphRenderer::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    FGRenderBase::DumpStatistics(font, alert);
    m_Stats.FrameEnd();
    font.OutNext("Lights:");
    font.OutNext("- total:      %u", m_Stats.l_total);
    font.OutNext("- visible:    %u", m_Stats.l_visible);
    font.OutNext("- shadowed:   %u", m_Stats.l_shadowed);
    font.OutNext("- unshadowed: %u", m_Stats.l_unshadowed);
    font.OutNext("Shadow maps:");
    font.OutNext("- used:       %d", m_Stats.s_used);
    font.OutNext("- merged:     %d", m_Stats.s_merged - m_Stats.s_used);
    font.OutNext("- finalclip:  %d", m_Stats.s_finalclip);
    u32 ict = m_Stats.ic_total + m_Stats.ic_culled;
    font.OutNext("ICULL:        %03.1f", 100.f * f32(m_Stats.ic_culled) / f32(ict ? ict : 1));
    font.OutNext("- visible:    %u", m_Stats.ic_total);
    font.OutNext("- culled:     %u", m_Stats.ic_culled);
    m_Stats.FrameStart();
    m_HOM.DumpStatistics(font, alert);
    fg::Scene.Sectors_xrc.DumpStatistics(font, alert);
}

} // namespace xray::render
