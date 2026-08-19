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
#include <imgui.h>

// Lambda-based pass setup functions
#include "FrameGraphPasses/DebugDrawPassSetup.h"
#include "FrameGraphPasses/HiZBuildPassSetup.h"      // Phase 3.5: Hi-Z pyramid for GPU culling
#include "FrameGraphPasses/ForwardColorPassSetup.h"  // Phase 1: Single-RT forward rendering + pipeline init
#include "GPUCullingManager.h"                       // Phase 3.5: GPU frustum/occlusion culling
#include "FGDetailManager.h"                         // Detail system (grass/vegetation)
#include "FrameGraphPasses/DetailCullPassSetup.h"    // Detail culling (async compute)
#include "FrameGraphPasses/DetailPassSetup.h"        // Detail rendering pass
#include "FrameGraphPasses/TransparentPassSetup.h"   // Transparent alpha-blended geometry (after detail)
#include "FrameGraphPasses/PassCommon.h"
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
#include "FrameGraphPasses/TonemapPassSetup.h"
#include "FrameGraphPasses/PostProcessPassSetup.h"
#include "FrameGraphPasses/SmokeTrailPassSetup.h"
#include "FrameGraphPasses/ClusterLightPassSetup.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "FrameGraphPasses/MotionVectorPassSetup.h"
#include "FrameGraphPasses/TAAPassSetup.h"
#include "FrameGraphPasses/ReSTIRGIPassSetup.h"
#include "FrameGraphPasses/WetSurfacesPassSetup.h"
#include "FrameGraphPasses/RainShadowPassSetup.h"
#include "FrameGraphPasses/ShadowPassSetup.h"
#include "FrameGraphPasses/VolumetricFogPassSetup.h"
#include "Upscaling/UpscaleState.h"
#include "Upscaling/IUpscaleBackend.h"
#include "Upscaling/UpscalePassSetup.h"
#include "Upscaling/StreamlineDLSS.h"
#include "Upscaling/DlssFgPassSetup.h"
#include "Denoising/IDenoiseBackend.h"
#include "RayTracing/ReSTIRMemoryManager.h"
#include "FrameGraphPasses/RibbonPassSetup.h"
#include "FrameGraphPasses/TrailPassSetup.h"
#include "Layers/xrRender/FrameGraph/Blackboard.h"
#include "FrameGraphPasses/ImGuiPassSetup.h"
#include "FrameGraphPasses/RainPassSetup.h"
#include "FrameGraphPasses/ThunderboltPassSetup.h"
#include "FrameGraphPasses/LensFlarePassSetup.h"
#include "FrameGraphPasses/PathTracerPassSetup.h"
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
extern ENGINE_API int ps_r_upscale;
extern ENGINE_API int ps_r_denoise;
extern ENGINE_API int ps_r_nrd_method;
extern ENGINE_API int ps_r_dlss;
extern ENGINE_API int ps_r_dlss_rr;
extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_hdr_debug;

namespace xray::render {

using namespace fg;

static framegraph::VirtualResourceHandle CreateHdrCompose(framegraph::FrameGraph& fg, u32 w, u32 h)
{
    framegraph::ResourceDesc d;
    d.type = framegraph::ResourceDesc::Type::Texture2D;
    d.width = w;
    d.height = h;
    d.format = nvrhi::Format::RGBA16_FLOAT;
    d.isRenderTarget = true;
    d.debugName = "rt_HdrCompose";
    return fg.CreateTexture("rt_HdrCompose", d);
}

static framegraph::VirtualResourceHandle CreateDisplayColor(framegraph::FrameGraph& fg, u32 w, u32 h)
{
    nvrhi::Format fmt = nvrhi::Format::RGBA8_UNORM;
    if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
        fmt = GEnv.Backend->GetBackBuffer()->getDesc().format;
    framegraph::ResourceDesc d;
    d.type = framegraph::ResourceDesc::Type::Texture2D;
    d.width = w;
    d.height = h;
    d.format = fmt;
    d.isRenderTarget = true;
    d.debugName = "rt_FgDisplay";
    return fg.CreateTexture("rt_FgDisplay", d);
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

    // Create RenderContext for execution
    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    m_blackboard = xr_make_unique<framegraph::Blackboard>();
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
    m_staticBatchesCached = false;
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

    fg::ClusteredLightManager::Instance().Shutdown();

    if (m_blackboard) {
        if (auto* restir = m_blackboard->try_get<passes::ReSTIRGIPassState>())
            passes::ShutdownReSTIRGI(*restir);
        if (auto* wet = m_blackboard->try_get<passes::WetSurfacesPassState>())
            wet->initialized = false;
        if (auto* rainSM = m_blackboard->try_get<passes::RainShadowPassState>())
            passes::ShutdownRainShadowPass(m_device, *rainSM);
        if (auto* grassSM = m_blackboard->try_get<passes::GrassShadowPassState>())
            passes::ShutdownGrassShadowPass(m_device, *grassSM);
        if (auto* volFog = m_blackboard->try_get<passes::VolumetricFogPassState>())
            passes::ShutdownVolumetricFog(*volFog);
    }

    passes::ShutdownPathTracer();

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    if (g_upscaleBackend) {
        g_upscaleBackend->Shutdown();
        g_upscaleBackend.reset();
    }
    if (g_denoiseBackend) {
        g_denoiseBackend->Shutdown();
        g_denoiseBackend.reset();
    }

    if (m_blackboard) {
        if (auto* tonemap = m_blackboard->try_get<passes::TonemapPassState>())
            passes::ShutdownTonemapPass(*tonemap);
        if (auto* taa = m_blackboard->try_get<passes::TAAPassState>())
            passes::ShutdownTAAPass(*taa);
        m_blackboard.reset();
    }

    m_prevFrameDepth = nullptr;
    m_normals[0] = nullptr;
    m_normals[1] = nullptr;
    m_inspectorPreview = nullptr;
    old_QuadIB = nullptr;
}

void FrameGraphRenderer::Render() {
    ZoneScopedN("FrameGraphRenderer::Render");

    if (!m_enabled) return;

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
        SetupFrameGraphPasses();
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

    // Compile the graph (optimizes passes, calculates lifetimes, etc.)
    {
        ZoneScopedN("FG::Compile");
        m_framegraph->Compile();
    }

    auto& cache = framegraph::GetPassResourceCache();
    auto* cmdList = m_renderContext->GetCommandList();
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(passes::StaticGlobals), m_device, 512);
    auto staticGlobalsData = passes::BuildStaticGlobals();

    auto& clm = fg::ClusteredLightManager::Instance();
    if (clm.IsReady() && clm.GetLightCount() > 0) {
        float zNear = VIEWPORT_NEAR;
        float zFar = g_pGamePersistent->Environment().CurrentEnv.far_plane;
        auto ccb = clm.BuildClusterCB(
            passes::GetRenderWidth(), passes::GetRenderHeight(), zNear, zFar);
        staticGlobalsData.cluster_params.set(ccb.gridDims.x, ccb.gridDims.y, ccb.gridDims.z, ccb.gridDims.w);
        staticGlobalsData.cluster_scales.set(ccb.depthParams.x, ccb.depthParams.y, ccb.depthParams.z, ccb.depthParams.w);
    }

    cmdList->writeBuffer(staticGlobalsCB, &staticGlobalsData, sizeof(staticGlobalsData));

    auto dynamicTransformsCB = cache.GetOrCreateVolatileCB("Frame", "DynamicTransforms",
        sizeof(passes::DynamicTransforms), m_device, 256);
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
    m_prevInvFullTransform = Device.mInvFullTransform;
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

    const bool hdr10 = GEnv.Backend && GEnv.Backend->IsHdr10();

    framegraph::ResourceDesc bgDesc;
    bgDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    bgDesc.width = width;
    bgDesc.height = height;
    bgDesc.format = hdr10 ? nvrhi::Format::RGBA16_FLOAT : nvrhi::Format::RGBA8_UNORM;
    bgDesc.isRenderTarget = true;
    const char* bgName = hdr10 ? "rt_HdrCompose" : "rt_MenuBackground";
    bgDesc.debugName = bgName;

    auto backgroundTarget = m_framegraph->CreateTexture(bgName, bgDesc);
    
    framegraph::PassHandle clearPass = m_framegraph->AddPass("ClearBackground");
    m_framegraph->PassWrite(clearPass, backgroundTarget, framegraph::ResourceState::RenderTarget);
    m_framegraph->SetPassCallback(clearPass,
        [backgroundTarget](fg::RenderContext& ctx, const framegraph::FrameGraph& fg) {
            auto* bgRT = fg.GetPhysicalTexture(backgroundTarget);
            if (bgRT) {
                nvrhi::ICommandList* cmdList = ctx.GetCommandList();
                cmdList->clearTextureFloat(bgRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            }
        }
    );

    auto sceneWithUI = passes::setupUIPass(*m_framegraph, backgroundTarget, width, height);
    sceneWithUI = passes::setupFontPass(*m_framegraph, sceneWithUI);
    sceneWithUI = passes::setupCursorPass(*m_framegraph, sceneWithUI, width, height);
    sceneWithUI = passes::setupDebugDrawPass(*m_framegraph, sceneWithUI, width, height);

    framegraph::VirtualResourceHandle ldrOutput = sceneWithUI;
    if (!hdr10)
    {
        ldrOutput = passes::setupTonemapPass(
            *m_framegraph,
            m_device,
            sceneWithUI,
            framegraph::VirtualResourceHandle(),
            backbufferHandle,
            width,
            height,
            m_blackboard->get_or_add<passes::TonemapPassState>()
        );
    }

    fg::ImGuiRendererNVRHI* imguiRenderer = GEnv.Render->GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,
        imguiRenderer,
        width,
        height
    );
    if (hdr10)
        finalOutput = passes::setupHdr10EncodePass(
            *m_framegraph, m_device, finalOutput, backbufferHandle, width, height,
            m_blackboard->get_or_add<passes::TonemapPassState>());

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
    if (ps_r_hdr_debug)
    {
        const fg::passes::ExposurePassState* exp = nullptr;
        if (m_blackboard)
        {
            if (auto* e = m_blackboard->try_get<fg::passes::ExposurePassState>())
            {
                if (m_device && m_device->GetNVRHIDevice())
                    fg::passes::PollExposureHistogram(*e, m_device->GetNVRHIDevice());
                exp = e;
            }
        }
        fg::passes::RenderHdrDebugUI(exp);
    }

    if (m_statsOverlay && psDeviceFlags.test(rsStatistic))
    {
        xray::profiler::RenderStats stats;
        stats.Reset();

        if (m_geometryCollector)
        {
            const auto& batches = m_geometryCollector->GetBatches();
            stats.totalBatches = static_cast<u32>(batches.size());

            for (const auto& batch : batches)
            {
                u32 triangles = batch.indexCount / 3;
                stats.totalTriangles += triangles;

                if (batch.isSkinned)
                {
                    stats.skinnedBatches++;
                    stats.skinnedTriangles += triangles;
                    stats.skinnedMeshes++;
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

        // Collect detail/grass stats
        {
            auto& clmStats = fg::ClusteredLightManager::Instance();
            stats.lightsClustered = clmStats.GetLightCount();
            stats.lightsPoint = clmStats.GetPointCount();
            stats.lightsSpot = clmStats.GetSpotCount();
            stats.lightsOmni = clmStats.GetOmniCount();
            u32 visCount = clmStats.GetVisibleLightCount();
            stats.lightsHiZVisible = (visCount > 0) ? visCount : stats.lightsClustered;
        }

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
        m_worldParticleBatches.clear();
        m_hudParticleBatches.clear();

        fg::ClusteredLightManager::Instance().BeginFrame();
    }

    if (levelLoaded) {
        ZoneScopedN("SetupFrame::CollectVisibleGeometry");
        CollectVisibleGeometry();
    }

    {
        ZoneScopedN("SetupFrame::CollectorEnd");
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
    passes::ApplyTAAJitter();

    {
        if (ps_r_dlss != 0 && ps_r_upscale == 0)
            ps_r_upscale = 2;
        const int reqUpscale = ps_r_upscale;
        fg::UpscaleBackendType wantType = fg::UpscaleBackendType::None;
        if (reqUpscale == 3)
            wantType = fg::UpscaleBackendType::DLSS;
        else if (reqUpscale == 2)
            wantType = fg::UpscaleBackendType::DLSS;
        else if (reqUpscale == 1)
            wantType = fg::UpscaleBackendType::None;
        const fg::UpscaleBackendType haveType =
            g_upscaleBackend ? g_upscaleBackend->GetType() : fg::UpscaleBackendType::None;
        const bool haveOk = g_upscaleBackend && g_upscaleBackend->IsAvailable();
        const bool needSwitch =
            (wantType == fg::UpscaleBackendType::None && haveType != fg::UpscaleBackendType::None) ||
            (wantType != fg::UpscaleBackendType::None && (!haveOk || haveType != wantType));
        if (needSwitch)
        {
            if (GEnv.Backend)
                GEnv.Backend->WaitForIdle();
            if (g_upscaleBackend)
            {
                if (haveType == fg::UpscaleBackendType::DLSS &&
                    wantType != fg::UpscaleBackendType::DLSS)
                    fg::Streamline_ReleaseFeatures();
                g_upscaleBackend->Shutdown();
                g_upscaleBackend.reset();
            }
            if (wantType != fg::UpscaleBackendType::None)
                g_upscaleBackend.reset(fg::CreateUpscaleBackendAuto());
            m_hasPrevFrameData = false;
        }
    }
    const bool upscaleBackendOk = g_upscaleBackend && g_upscaleBackend->IsAvailable();

    {
        static int s_denoiseCvar = -1;
        static int s_nrdMethodCvar = -1;
        static int s_dlssRrCvar = -1;
        static int s_upscaleCvar = -1;
        static int s_rrAvail = -1;
        static u32 s_denoiseW = 0;
        static u32 s_denoiseH = 0;
        UpdateUpscaleState(g_upscaleState, Device.dwWidth, Device.dwHeight, upscaleBackendOk);
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
            if (GEnv.Backend)
                GEnv.Backend->WaitForIdle();
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

    UpdateUpscaleState(g_upscaleState, Device.dwWidth, Device.dwHeight, upscaleBackendOk);
    g_upscaleState.jitterX = passes::g_taa_jitter_px;
    g_upscaleState.jitterY = passes::g_taa_jitter_py;
    g_upscaleState.prevJitterX = passes::g_taa_jitter_prev_px;
    g_upscaleState.prevJitterY = passes::g_taa_jitter_prev_py;
    if (m_bFirstFrameAfterReset)
    {
        m_hasPrevFrameData = false;
        m_bFirstFrameAfterReset = false;
        fg::ReSTIRMemoryManager::Instance().RequestHistoryReset();
    }
    g_upscaleState.resetHistory = !m_hasPrevFrameData || fg::Streamline_ConsumeFeatureReset();

    const u32 displayWidth = g_upscaleState.displayWidth;
    const u32 displayHeight = g_upscaleState.displayHeight;
    const u32 width = g_upscaleState.renderWidth;
    const u32 height = g_upscaleState.renderHeight;
    passes::SetRenderResolution(width, height);
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

    // ═══════════════════════════════════════════════════════
    //  TEMPORAL HI-Z PYRAMID BUILD (From Previous Frame)
    // ═══════════════════════════════════════════════════════
    passes::HiZPyramidOutput hizOutput;
    hizOutput.pyramid = framegraph::VirtualResourceHandle();  // Invalid by default
    hizOutput.mipLevels = 0;
    hizOutput.width = width / 2;
    hizOutput.height = height / 2;

    framegraph::VirtualResourceHandle prevDepthHandle;
    bool hasPrevDepth = m_hasPrevFrameData && m_prevFrameDepth &&
                        m_prevFrameWidth == width && m_prevFrameHeight == height;

    if (hasPrevDepth) {
        framegraph::ResourceDesc prevDepthDesc;
        prevDepthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevDepthDesc.debugName = "rt_PrevDepth";
        prevDepthDesc.width = width;
        prevDepthDesc.height = height;
        prevDepthDesc.format = nvrhi::Format::D32;
        prevDepthDesc.isDepthStencil = true;
        prevDepthDesc.isImported = true;
        prevDepthDesc.isTransient = false;

        prevDepthHandle = m_framegraph->ImportTexture("rt_PrevDepth", m_prevFrameDepth, prevDepthDesc);

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

    // ═══════════════════════════════════════════════════════
    //  PHASE 3.5: GPU CULLING PASS (Frustum + Occlusion)
    // ═══════════════════════════════════════════════════════

    framegraph::VirtualResourceHandle drawArgsBuffer;  // Will be passed to forward pass
    framegraph::VirtualResourceHandle skinnedDrawArgsBuffer;  // Will be passed to skinning pass

    if (m_gpuCullingManager && hizOutput.pyramid.is_valid()) {
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

        if (m_gpuCullingManager->IsEnabled()) {
            auto cullOutput = m_gpuCullingManager->SetupCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                m_geometryCollector.get(),  // Geometry is uploaded during execute
                m_prevViewProj              // Previous frame's viewProj for temporal Hi-Z
            );

            drawArgsBuffer = cullOutput.drawArgsBuffer;
        }

        if (m_gpuCullingManager->IsSkinnedCullingEnabled()) {
            skinnedDrawArgsBuffer = m_gpuCullingManager->SetupSkinnedCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
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
    colorDesc.allowUAV = true;

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

    // ═══════════════════════════════════════════════════════
    //  SUN PASS (Sun disc with additive blending)
    // ═══════════════════════════════════════════════════════

    auto sunOutput = passes::setupSunPass(
        *m_framegraph,
        skyOutput,
        fgEnv,
        width,
        height
    );

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

        bindlessConfig.dynamicSet.compactDrawArgsBuffer = m_gpuCullingManager->GetDynamicCompactDrawArgsBuffer();
        bindlessConfig.dynamicSet.compactMaterialIDBuffer = m_gpuCullingManager->GetDynamicCompactMaterialIDBuffer();
        bindlessConfig.dynamicSet.compactBatchIndicesBuffer = m_gpuCullingManager->GetDynamicCompactBatchIndicesBuffer();
        bindlessConfig.dynamicSet.compactCountBuffer = m_gpuCullingManager->GetDynamicCompactCountBuffer();
        bindlessConfig.dynamicSet.instanceBuffer = m_gpuCullingManager->GetDynamicInstanceBuffer();
        bindlessConfig.dynamicSet.totalObjectCount = m_gpuCullingManager->GetDynamicObjectCount();

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
            bindlessConfig.variantPartition = m_gpuCullingManager->GetStaticPartition().ToConfig();
    }

    auto& clmSetup = fg::ClusteredLightManager::Instance();
    if (clmSetup.IsReady() && clmSetup.GetLightCount() > 0) {
        passes::setupClusterLightPass(
            *m_framegraph,
            m_device,
            &clmSetup,
            width,
            height,
            &m_blackboard->get_or_add<passes::ClusterLightPassState>(),
            hizOutput.pyramid,
            hizOutput.width,
            hizOutput.height,
            hizOutput.mipLevels,
            m_prevViewProj,
            m_hasPrevFrameData
        );
    }

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        sunOutput,
        normalBuffer,
        baseColorBuffer,
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
            m_prevViewProj,
            &m_worldParticleBatches
        );
    }

    // 2. Skinning Pass - world skinned only
    auto skinnedOutputs = passes::setupSkinningPass(
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
        m_overlayManager.get()
    );

    // ═══════════════════════════════════════════════════════
    //  DETAIL CULL PASS (Async Compute)
    // ═══════════════════════════════════════════════════════
    auto& detailPassState = m_blackboard->get_or_add<passes::DetailPassState>();
    passes::setupDetailCullPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        hizOutput.pyramid,
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels,
        m_hasPrevFrameData ? &m_prevViewProj : nullptr,
        m_gpuProfiler.get(),
        &detailPassState
    );

    // ═══════════════════════════════════════════════════════
    //  PERLIN4D NOISE GENERATION (Compute — updates shared noise texture)
    // ═══════════════════════════════════════════════════════
    if (m_detailManager && m_detailManager->perlin4dPipeline)
    {
        struct Perlin4DGenData { FGDetailManager* dm = nullptr; };
        m_framegraph->addCallbackPass<Perlin4DGenData>(
            "Perlin4DGen",
            [&](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, Perlin4DGenData& data)
            {
                framegraph::RenderPassBuilder passBuilder(builder, passHandle);
                passBuilder.sideEffects();
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

    // ═══════════════════════════════════════════════════════
    //  DETAIL DRAW PASS (Graphics)
    // ═══════════════════════════════════════════════════════
    auto detailOutputs = passes::setupDetailPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        skinnedOutputs,
        width,
        height,
        m_gpuProfiler.get(),
        detailPassState.cullArgs
    );

    passes::GrassShadowOutputs grassShadowOut{};
    if (m_detailManager && g_pGamePersistent)
    {
        grassShadowOut = passes::setupGrassShadowPass(
            *m_framegraph,
            m_device,
            m_detailManager.get(),
            g_pGamePersistent->Environment().CurrentEnv.sun_dir,
            m_blackboard->get_or_add<passes::GrassShadowPassState>(),
            detailOutputs.albedo,
            detailPassState.cullArgs);
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
        passes::ResolveEnvSkyCubes(m_device, transparentConfig.envSky0, transparentConfig.envSky1);

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
            transparentConfig.variantPartition = m_gpuCullingManager->GetTransparentPartition().ToConfig();
        if ((ps_r_rt_gi != 0) && m_rtAccelMgr && m_rtAccelMgr->IsSupported() && m_rtAccelMgr->IsReady())
            transparentConfig.skipWmark = true;
    }

    auto transparentOutputs = passes::setupTransparentPass(
        *m_framegraph,
        m_device,
        detailOutputs,
        transparentConfig,
        width, height,
        m_blackboard->get_or_add<passes::TransparentPassState>()
    );

    if (!m_hudBatches.empty()) {
        framegraph::DefaultOutputLayout hudIn = transparentOutputs;
        auto hudOut = passes::setupSkinningPass(
            *m_framegraph,
            m_device,
            hudIn,
            nullptr,
            &m_hudBatches,
            m_materialCache.get(),
            width,
            height,
            m_gpuCullingManager.get(),
            {},
            &m_blackboard->get_or_add<passes::SkinningPassState>(),
            m_overlayManager.get());
        transparentOutputs.albedo = hudOut.albedo;
        transparentOutputs.normal = hudOut.normal;
        transparentOutputs.baseColor = hudOut.baseColor;
        if (hudOut.worldPos.is_valid())
            transparentOutputs.worldPos = hudOut.worldPos;
        transparentOutputs.depth = hudOut.depth;
    }

    if (m_decalManager)
        m_decalManager->Update(Device.fTimeDelta, Device.fTimeGlobal);

    const bool restirWallmarks = transparentConfig.skipWmark;
    if (restirWallmarks && transparentConfig.IsValid() && transparentOutputs.baseColor.is_valid()) {
        framegraph::DefaultOutputLayout wmIn = transparentOutputs;
        wmIn.albedo = transparentOutputs.baseColor;
        auto wmOut = passes::setupWallmarkPass(
            *m_framegraph, m_device, wmIn, transparentConfig,
            width, height,
            m_blackboard->get_or_add<passes::TransparentPassState>());
        transparentOutputs.baseColor = wmOut.albedo;
    }
    if (m_decalManager && m_decalManager->GetActiveCount() > 0) {
        framegraph::DefaultOutputLayout decalIn = transparentOutputs;
        if (restirWallmarks && transparentOutputs.baseColor.is_valid())
            decalIn.albedo = transparentOutputs.baseColor;
        auto decalOut = passes::setupDecalPass(
            *m_framegraph, m_device,
            decalIn, m_decalManager.get(),
            width, height,
            m_blackboard->get_or_add<passes::DecalPassState>());
        if (restirWallmarks && transparentOutputs.baseColor.is_valid())
            transparentOutputs.baseColor = decalOut.albedo;
        else
            transparentOutputs.albedo = decalOut.albedo;
    }

    // ═══════════════════════════════════════════════════════
    //  MOTION VECTOR PASS (Depth-based reprojection)
    // ═══════════════════════════════════════════════════════
    passes::MotionVectorOutput motionOutput = passes::setupMotionVectorPass(
        *m_framegraph, m_device,
        transparentOutputs.depth,
        passes::g_taa_unjittered_full_transform,
        m_prevViewProj,
        Device.mInvFullTransform,
        width, height,
        m_blackboard->get_or_add<passes::MotionVectorPassState>()
    );

    auto sceneColor = transparentOutputs.albedo;
    framegraph::VirtualResourceHandle pendingDistortRT = transparentOutputs.distortion;

    // ═══════════════════════════════════════════════════════
    //  RT ACCEL + DYNAMIC BLAS (before ReSTIR / Path Tracer)
    // ═══════════════════════════════════════════════════════
    {
        static int s_lastGi = -1;
        if (s_lastGi != ps_r_rt_gi) {
            s_lastGi = ps_r_rt_gi;
            const bool supported = m_rtAccelMgr && m_rtAccelMgr->IsSupported();
            const bool ready = m_rtAccelMgr && m_rtAccelMgr->IsReady();
            Msg("* [ReSTIR] r_rt_gi=%d supported=%d ready=%d", ps_r_rt_gi, supported ? 1 : 0, ready ? 1 : 0);
        }
    }

    bool needsRT = (ps_r_path_tracer || ps_r_rt_gi) && m_rtAccelMgr && m_rtAccelMgr->IsSupported();
    const bool levelLoading = g_pGamePersistent && g_pGamePersistent->IsLoadingScreenShown();

    if (needsRT && m_gpuCullingManager && !levelLoading) {
        m_gpuCullingManager->Initialize(m_device);
        m_gpuCullingManager->SetRTAccelStructManager(m_rtAccelMgr.get());

        if (!m_rtAccelMgr->IsReady()) {
            static bool s_rtWaitLogged = false;
            if (!s_rtWaitLogged) {
                s_rtWaitLogged = true;
                Msg("* [RT] Waiting for mega geometry upload to build TLAS");
            }
        }

        struct RTBuildData {
            RTAccelStructManager* accelMgr;
            GPUCullingManager* gpuCulling;
            FGDetailManager* detailMgr;
            const GeometryCollector* geometry;
            const xr_vector<GeometryBatch>* hudBatches;
            const xr_vector<passes::ParticleBatch>* worldParticles;
            bool buildDynamic;
        };

        const bool buildDynamic = m_rtAccelMgr->IsReady() &&
            ((ps_r_path_tracer && m_ptSampleIndex == 0) || (ps_r_rt_gi && !ps_r_path_tracer));

        m_framegraph->addCallbackPass<RTBuildData>(
            "RT Accel Build",
            [&](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, RTBuildData& data) {
                framegraph::RenderPassBuilder pb(builder, passHandle);
                pb.sideEffects();
                if (detailPassState.cullArgs.is_valid())
                    pb.read(detailPassState.cullArgs, framegraph::ResourceState::ShaderResource);
                data.accelMgr = m_rtAccelMgr.get();
                data.gpuCulling = m_gpuCullingManager.get();
                data.detailMgr = m_detailManager.get();
                data.geometry = m_geometryCollector.get();
                data.hudBatches = &m_hudBatches;
                data.worldParticles = &m_worldParticleBatches;
                data.buildDynamic = buildDynamic;
            },
            [](const RTBuildData& data, const framegraph::FrameGraph&, fg::RenderContext* ctx) {
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                data.accelMgr->BuildIfNeeded(cmdList, data.gpuCulling);

                if (data.accelMgr->IsReady()) {
                    if (!data.accelMgr->GetMaterialBuffer())
                        data.accelMgr->SetMaterialBuffer(bindless::MaterialBuffer::Instance().GetBuffer());
                    if (!data.accelMgr->GetTerrainMaterialBuffer())
                        data.accelMgr->SetTerrainMaterialBuffer(bindless::TerrainMaterialBuffer::Instance().GetBuffer());
                }

                if (!data.buildDynamic || !data.accelMgr->IsReady())
                    return;

                xr_vector<GeometryBatch> worldSkinned;
                for (const auto& b : data.geometry->GetBatches()) {
                    if (b.isSkinned && b.visual && b.indexCount > 0)
                        worldSkinned.push_back(b);
                }

                xr_vector<GeometryBatch> hudSkinned;
                data.accelMgr->BuildSkinnedBLAS(cmdList, data.gpuCulling, worldSkinned, hudSkinned);
                data.accelMgr->BuildGrassBLAS(cmdList, data.detailMgr);
                if (data.worldParticles)
                    data.accelMgr->BuildParticleBLAS(cmdList, *data.worldParticles);
                else
                    data.accelMgr->InvalidateParticles();
                data.accelMgr->RebuildDynamic(cmdList, data.gpuCulling);
            }
        );
    }

    // ═══════════════════════════════════════════════════════
    //  ReSTIR GI (RT Shadows + Indirect Lighting)
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* restirNoisyDiffuse = nullptr;
    nvrhi::ITexture* restirNoisySpecular = nullptr;
    nvrhi::ITexture* restirHitDistance = nullptr;

    if (ps_r_rt_gi && m_rtAccelMgr && m_rtAccelMgr->IsSupported()) {
        if (!m_rtAccelMgr->IsReady()) {
            g_restirReplaceForward = false;
            static u32 s_lastWaitFrame = 0;
            if (Device.dwFrame - s_lastWaitFrame > 60) {
                s_lastWaitFrame = Device.dwFrame;
                Msg("! [ReSTIR] r_rt_gi=1 but TLAS not ready yet");
            }
        } else {
        auto& restirState = m_blackboard->get_or_add<passes::ReSTIRGIPassState>();
        {
            auto& tpState = m_blackboard->get_or_add<passes::TransparentPassState>();
            restirState.waterUnderWorldPos = tpState.waterSceneWorldPos;
            restirState.waterUnderColor = tpState.waterSsrColor;
        }
        auto rtgiOutput = passes::setupReSTIRGIPass(
            *m_framegraph, m_device, m_rtAccelMgr.get(),
            transparentOutputs.depth, transparentOutputs.normal,
            transparentOutputs.baseColor,
            transparentOutputs.worldPos,
            prevNormalsHandle, prevDepthHandle,
            motionOutput.motionVectors,
            sceneColor,
            Device.mInvFullTransform, m_prevViewProj, m_prevInvFullTransform,
            Device.vCameraPosition, ps_r_rt_gi_intensity,
            width, height,
            restirState, m_hasPrevFrameData, grassShadowOut
        );
        sceneColor = rtgiOutput.sceneColor;
        restirNoisyDiffuse = rtgiOutput.noisyDiffuse;
        restirNoisySpecular = rtgiOutput.noisySpecular;
        restirHitDistance = rtgiOutput.hitDistance;

        if (false && g_denoiseBackend && g_denoiseBackend->IsAvailable() &&
            ps_r_denoise != 0 && !(ps_r_dlss_rr != 0 && ps_r_upscale == 2))
        {
            struct NrdPassData {
                framegraph::VirtualResourceHandle depth;
                framegraph::VirtualResourceHandle normal;
                framegraph::VirtualResourceHandle baseColor;
                framegraph::VirtualResourceHandle worldPos;
                framegraph::VirtualResourceHandle motion;
                framegraph::VirtualResourceHandle sceneColor;
                nvrhi::ITexture* noisyDiffuse = nullptr;
                nvrhi::ITexture* noisySpecular = nullptr;
                nvrhi::ITexture* hitDistance = nullptr;
                nvrhi::ITexture* directLighting = nullptr;
                u32 width = 0;
                u32 height = 0;
            };

            m_framegraph->addCallbackPass<NrdPassData>(
                "ReSTIR NRD",
                [&](framegraph::FrameGraph& builder, framegraph::PassHandle passHandle, NrdPassData& data) {
                    framegraph::RenderPassBuilder pb(builder, passHandle);
                    data.depth = pb.read(transparentOutputs.depth, framegraph::ResourceState::ShaderResource);
                    data.normal = pb.read(transparentOutputs.normal, framegraph::ResourceState::ShaderResource);
                    data.baseColor = pb.read(transparentOutputs.baseColor, framegraph::ResourceState::ShaderResource);
                    if (transparentOutputs.worldPos.is_valid())
                        data.worldPos = pb.read(transparentOutputs.worldPos, framegraph::ResourceState::ShaderResource);
                    if (motionOutput.motionVectors.is_valid())
                        data.motion = pb.read(motionOutput.motionVectors, framegraph::ResourceState::ShaderResource);
                    data.sceneColor = pb.readWrite(sceneColor, framegraph::ResourceState::UnorderedAccess);
                    pb.sideEffects();
                    data.noisyDiffuse = restirNoisyDiffuse;
                    data.noisySpecular = restirNoisySpecular;
                    data.hitDistance = restirHitDistance;
                    data.directLighting = fg::ReSTIRMemoryManager::Instance().GetDirectLighting();
                    data.width = width;
                    data.height = height;
                },
                [](const NrdPassData& data, const framegraph::FrameGraph& fgGraph, fg::RenderContext* ctx) {
                    if (!g_denoiseBackend || !g_denoiseBackend->IsAvailable() || !ctx)
                        return;
                    auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                    auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                    auto* baseTex = fgGraph.GetPhysicalTexture(data.baseColor);
                    auto* sceneTex = fgGraph.GetPhysicalTexture(data.sceneColor);
                    auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                    auto* motionTex = data.motion.is_valid() ? fgGraph.GetPhysicalTexture(data.motion) : nullptr;
                    if (!depthTex || !normalTex || !baseTex || !sceneTex || !data.noisyDiffuse || !worldPosTex)
                        return;

                    fg::DenoiseInputs in;
                    in.noisyDiffuse = data.noisyDiffuse;
                    in.noisySpecular = data.noisySpecular;
                    in.hitDistance = data.hitDistance;
                    in.normals = normalTex;
                    in.roughness = normalTex;
                    in.depth = depthTex;
                    in.baseColor = baseTex;
                    in.worldPos = worldPosTex;
                    in.motionVectors = motionTex;
                    in.directLighting = data.directLighting;
                    in.sceneColorIn = sceneTex;
                    in.outSceneColor = sceneTex;
                    in.outDiffuse = data.noisyDiffuse;
                    in.outSpecular = data.noisySpecular;
                    in.width = data.width;
                    in.height = data.height;
                    in.frameIndex = Device.dwFrame;
                    in.nearZ = 0.001f;
                    in.farZ = 500.f;
                    g_denoiseBackend->Evaluate(ctx->GetCommandList(), in);
                }
            );
        }
        }
    } else {
        g_restirReplaceForward = false;
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

    {
        passes::WetSurfacesExtras wetExtras{};
        if (rainShadowOut.rainSMTex)
        {
            wetExtras.rainSM = rainShadowOut.rainSM;
            wetExtras.rainSMTex = rainShadowOut.rainSMTex;
            wetExtras.rainSampleVP = rainShadowOut.sampleVP;
            wetExtras.rainSMValid = rainShadowOut.valid;
        }
        auto wetIn = transparentOutputs;
        wetIn.albedo = sceneColor;
        if (ps_r_rt_gi && fg::ReSTIRMemoryManager::Instance().GetSkyOpen())
            wetExtras.skyOpenTex = fg::ReSTIRMemoryManager::Instance().GetSkyOpen();
        auto wetOut = passes::setupWetSurfacesPass(
            *m_framegraph,
            m_device,
            wetIn,
            width,
            height,
            m_blackboard->get_or_add<passes::WetSurfacesPassState>(),
            wetExtras);
        sceneColor = wetOut.albedo;
    }

    {
        framegraph::DefaultOutputLayout fxIn = transparentOutputs;
        fxIn.albedo = sceneColor;
        auto particleOutputs = passes::setupParticlePass(
            *m_framegraph,
            m_device,
            fxIn,
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
            prevDepthHandle,
            &m_blackboard->get_or_add<passes::ParticlePassState>(),
            pendingDistortRT
        );
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
        auto smokeOutputs = trailOutputs.layout;
        if (m_smokeTrailManager && m_smokeTrailManager->IsReady())
        {
            smokeOutputs = passes::setupSmokeTrailPass(
                *m_framegraph,
                m_device,
                trailOutputs.layout,
                m_smokeTrailManager.get(),
                width,
                height,
                m_blackboard->get_or_add<passes::SmokeTrailPassState>(),
                m_detailManager ? m_detailManager->perlin4dTexture.Get() : nullptr
            );
        }
        sceneColor = smokeOutputs.albedo;
        sceneColor = passes::setupGlowBillboardPass(
            *m_framegraph,
            m_device,
            sceneColor,
            transparentOutputs.depth.is_valid() ? transparentOutputs.depth : depthBuffer,
            width,
            height,
            m_blackboard->get_or_add<passes::GlowPassState>());
        if (particleOutputs.distortionRT.is_valid())
            pendingDistortRT = particleOutputs.distortionRT;
        if (pendingDistortRT.is_valid()) {
            auto& distortState = m_blackboard->get_or_add<passes::DistortionApplyPassState>();
            {
                auto& tpState = m_blackboard->get_or_add<passes::TransparentPassState>();
                distortState.waterUnderColor = tpState.waterSsrColor;
            }
            sceneColor = passes::setupDistortionApplyPass(
                *m_framegraph, m_device, sceneColor, pendingDistortRT,
                transparentOutputs.worldPos, transparentOutputs.baseColor,
                transparentOutputs.depth,
                width, height,
                distortState);
        }
    }

    sceneColor = passes::setupVolumetricFogPass(
        *m_framegraph,
        m_device,
        sceneColor,
        transparentOutputs.depth,
        transparentOutputs.worldPos,
        Device.mInvFullTransform,
        m_prevViewProj,
        Device.vCameraPosition,
        width,
        height,
        m_blackboard->get_or_add<passes::VolumetricFogPassState>(),
        m_rtAccelMgr.get());

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

    if (g_pGamePersistent && g_pGamePersistent->Environment().eff_Rain)
    {
        auto* effRain = g_pGamePersistent->Environment().eff_Rain;
        effRain->Render();
        if (auto* fgRain = dynamic_cast<FGRainRender*>(effRain->GetRenderer()))
        {
            if (fgRain->HasWork())
            {
                sceneColor = passes::setupRainPass(
                    *m_framegraph,
                    sceneColor,
                    transparentOutputs.depth,
                    fgRain);
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
                sceneColor = passes::setupThunderboltPass(
                    *m_framegraph,
                    sceneColor,
                    transparentOutputs.depth,
                    fgTB);
            }
        }
    }

    if (g_pGamePersistent && g_pGamePersistent->Environment().eff_LensFlare)
    {
        auto* effLF = g_pGamePersistent->Environment().eff_LensFlare;
        effLF->Render(true, true, true);
        if (auto* fgLF = dynamic_cast<FGLensFlareRender*>(effLF->GetRenderer()))
        {
            if (fgLF->HasWork())
            {
                sceneColor = passes::setupLensFlarePass(
                    *m_framegraph,
                    sceneColor,
                    transparentOutputs.depth,
                    fgLF);
            }
        }
    }

    if (ps_r_taa && ps_r_upscale == 0)
    {
        sceneColor = passes::setupTAAPass(
            *m_framegraph,
            m_device,
            sceneColor,
            transparentOutputs.depth,
            motionOutput.motionVectors,
            width,
            height,
            m_hasPrevFrameData,
            m_blackboard->get_or_add<passes::TAAPassState>(),
            transparentOutputs.worldPos);
        m_framegraph->GetRTRegistry().RegisterRT("rt_TAA", sceneColor);
    }

    // ═══════════════════════════════════════════════════════
    //  EXPOSURE PASS (Auto-Exposure / Eye Adaptation)
    // ═══════════════════════════════════════════════════════
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

    passes::UpscaleRRGuides rrGuides{};
    const passes::UpscaleRRGuides* rrGuidesPtr = nullptr;
    if (restirNoisyDiffuse && restirNoisySpecular && transparentOutputs.normal.is_valid() &&
        transparentOutputs.baseColor.is_valid() && transparentOutputs.worldPos.is_valid())
    {
        rrGuides.normals = transparentOutputs.normal;
        rrGuides.baseColor = transparentOutputs.baseColor;
        rrGuides.worldPos = transparentOutputs.worldPos;
        rrGuides.depth = transparentOutputs.depth;
        rrGuides.noisyDiffuse = restirNoisyDiffuse;
        rrGuides.noisySpecular = restirNoisySpecular;
        rrGuides.hitDistance = restirHitDistance;
        rrGuidesPtr = &rrGuides;
    }

    sceneColor = passes::setupUpscaleOrResolvePass(
        *m_framegraph,
        m_device,
        sceneColor,
        transparentOutputs.depth,
        motionOutput.motionVectors,
        exposureOutput.exposureTexture,
        g_upscaleState,
        g_upscaleBackend.get(),
        width,
        height,
        m_blackboard->get_or_add<passes::UpscalePassState>(),
        rrGuidesPtr);

    const u32 postW = upscaleResolved ? displayWidth : width;
    const u32 postH = upscaleResolved ? displayHeight : height;

    const bool hdr10 = GEnv.Backend && GEnv.Backend->IsHdr10();
    const bool useDlssFg =
        g_upscaleBackend &&
        g_upscaleBackend->SupportsFG() &&
        g_upscaleBackend->GetType() == fg::UpscaleBackendType::DLSS &&
        ps_r_dlss_fg != 0;

    framegraph::VirtualResourceHandle tonemapTarget;
    if (hdr10)
        tonemapTarget = CreateHdrCompose(*m_framegraph, postW, postH);
    else if (useDlssFg)
        tonemapTarget = CreateDisplayColor(*m_framegraph, postW, postH);
    else
        tonemapTarget = backbufferHandle;

    bool needPP = false;
    if (m_pTarget && m_pTarget->u_need_PP())
        needPP = true;
    if (g_pGamePersistent && g_pGamePersistent->m_pGShaderConstants
        && g_pGamePersistent->m_pGShaderConstants->m_blender_mode.x > 0.5f)
        needPP = true;

    auto tonemapDest = tonemapTarget;
    if (needPP)
        tonemapDest = hdr10 ? CreateHdrCompose(*m_framegraph, postW, postH)
                            : CreateDisplayColor(*m_framegraph, postW, postH);

    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        m_device,
        sceneColor,
        exposureOutput.exposureTexture,
        tonemapDest,
        postW,
        postH,
        m_blackboard->get_or_add<passes::TonemapPassState>(),
        &m_blackboard->get_or_add<passes::ExposurePassState>(),
        transparentOutputs.depth,
        transparentOutputs.worldPos
    );

    if (needPP)
    {
        ldrOutput = passes::setupPostProcessPass(
            *m_framegraph,
            m_device,
            ldrOutput,
            tonemapTarget,
            postW,
            postH,
            m_pTarget,
            m_blackboard->get_or_add<passes::PostProcessPassState>());
    }

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

    ldrOutput = sceneWithUI;

    // ═══════════════════════════════════════════════════════
    //  DEBUG PREVIEW PASS (Render Inspector RT visualization)
    // ═══════════════════════════════════════════════════════
    m_framegraph->GetRTRegistry().RegisterRT("rt_SceneColor", skyColorHandle);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Depth", depthBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Normal", transparentOutputs.normal);
    m_framegraph->GetRTRegistry().RegisterRT("rt_BaseColor", baseColorBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Exposure", exposureOutput.exposureTexture);
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

    fg::ImGuiRendererNVRHI* imguiRenderer = GEnv.Render->GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,
        imguiRenderer,
        postW,
        postH
    );
    if (hdr10)
    {
        auto encodeDst = useDlssFg
            ? CreateDisplayColor(*m_framegraph, postW, postH)
            : backbufferHandle;
        finalOutput = passes::setupHdr10EncodePass(
            *m_framegraph, m_device, finalOutput, encodeDst, postW, postH,
            m_blackboard->get_or_add<passes::TonemapPassState>());
    }

    if (useDlssFg)
    {
        const bool resetFg = !m_hasPrevFrameData || g_upscaleState.resetHistory;
        passes::setupDlssFgPass(
            *m_framegraph,
            finalOutput,
            framegraph::VirtualResourceHandle{},
            depthBuffer,
            motionOutput.motionVectors,
            Device.mProject,
            m_prevViewProj,
            Device.mFullTransform,
            width,
            height,
            postW,
            postH,
            resetFg);
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

nvrhi::ITexture* FrameGraphRenderer::GetPersistentExposureTexture() const
{
    if (!m_blackboard)
        return nullptr;
    if (auto* exp = m_blackboard->try_get<passes::ExposurePassState>())
    {
        if (exp->exposureTexture)
            return exp->exposureTexture.Get();
    }
    if (auto* tm = m_blackboard->try_get<passes::TonemapPassState>())
    {
        if (tm->fallbackExposureTexture)
            return tm->fallbackExposureTexture.Get();
    }
    return nullptr;
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
    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;

    // ═══════════════════════════════════════════════════════
    //  INDEX/VERTEX OFFSET HANDLING
    // ═══════════════════════════════════════════════════════
    // Different mesh types have different buffer layouts:
    //
    // SKINNED MESHES (CSkeletonX_ST, CSkeletonX_PM):
    //   - Have dedicated VB/IB (not shared pools)
    //   - vBase = 0, startIndex = 0 (ST) or SW.offset (PM)
    //   - Vanilla: _Render(rm_geom, vCount, 0, dwPrimitives) for ST
    //   - Vanilla: _Render(rm_geom, SW.num_verts, SW.offset, SW.num_tris) for PM
    //
    // STATIC/PROGRESSIVE MESHES (Fvisual, FProgressive, FTreeVisual):
    //   - Use shared VB/IB pools from level geometry
    //   - For now, always use iBase/iCount (max detail, no LOD)
    //   - TODO: Implement proper LOD selection for progressive meshes
    //

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
            const FSlideWindow& sw = swi.sw[0];  // LOD 0 = max detail
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;  // iBase + SW.offset
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else if (visualType == MT_TREE_PM) {
        const FSlideWindowItem* pSWI = static_cast<FTreeVisual_PM*>(visual)->GetSWI();
        if (pSWI && pSWI->sw && pSWI->count > 0) {
            const FSlideWindow& sw = pSWI->sw[0];  // LOD 0 = max detail
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

    fg::ShaderKey shaderKey;
    if (fg::ExtractShaderKey(visual, shaderKey)) {
        static thread_local std::string s_debugNameBuffer;
        s_debugNameBuffer = shaderKey.ToString();
        batch.debugName = s_debugNameBuffer.c_str();
    } else {
        batch.debugName = "<unknown_shader>";
    }

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

        // Debug: Log allocation details for first few batches
        static int s_allocDebug = 0;
        if (s_allocDebug < 10 && !batch.megaBufferAlloc.valid) {
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
    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;
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

static bool ParticleNameLooksAdditive(LPCSTR shaderName, LPCSTR texName)
{
    auto has = [](LPCSTR s, const char* k) { return s && strstr(s, k); };
    if (has(shaderName, "smoke") || has(texName, "smoke")
        || has(texName, "dust") || has(texName, "steam"))
        return false;
    return has(shaderName, "add") || has(shaderName, "glow") || has(shaderName, "flare")
        || has(shaderName, "anomaly") || has(shaderName, "heat")
        || has(texName, "anomaly") || has(texName, "heat") || has(texName, "zhar")
        || has(texName, "fire") || has(texName, "flame") || has(texName, "glow")
        || has(texName, "flash") || has(texName, "spark") || has(texName, "flare")
        || has(texName, "explosion") || has(texName, "grenade") || has(texName, "blast")
        || has(texName, "tracer");
}

static u8 QueryParticleBlendMode(LPCSTR shaderName, LPCSTR texName)
{
    u32 id = 0;
    if (shader_info::GetParticleBlendIndex(shaderName, id)
        && id > 0 && id < passes::PARTICLE_BLEND_COUNT)
        return (u8)id;
    if (shaderName && (strstr(shaderName, "s-aadd") || strstr(shaderName, "alpha-add")
        || strstr(shaderName, "alphaadd")))
        return passes::PARTICLE_BLEND_ALPHA_ADD;
    if (ParticleNameLooksAdditive(shaderName, texName))
        return passes::PARTICLE_BLEND_ADD;
    if (shaderName && (strstr(shaderName, "mul2x") || strstr(shaderName, "mul_2x")))
        return passes::PARTICLE_BLEND_MUL_2X;
    if (shaderName && strstr(shaderName, "mul"))
        return passes::PARTICLE_BLEND_MUL;
    return passes::PARTICLE_BLEND_BLEND;
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
    batch.blendMode = QueryParticleBlendMode(
        pDef->m_ShaderName.c_str(),
        pDef->m_TextureName.size() ? pDef->m_TextureName.c_str() : "");

    if (strstr(pDef->m_ShaderName.c_str(), "distort"))
        batch.shaderVariant = passes::ParticleShaderVariant::Distort;

    if (m_materialCache && pDef->m_TextureName.size())
        batch.bindlessMaterialID = m_materialCache->PreRegisterParticleMaterial(pDef->m_TextureName);

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
            fn(pVisual);
            for (auto& child : pV->children) {
                ForEachLeafVisual(child, fn);
            }
            break;
        }
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            CKinematics* pV = static_cast<CKinematics*>(pVisual);
            pV->CalculateBones(TRUE);

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

void FrameGraphRenderer::CollectVisibleGeometry() {
    if (!g_pGamePersistent)
        return;

    const auto& sectors = scene_info::GetSceneSectors();
    u32 submittedStatic = 0;

    if (!m_staticBatchesCached && !sectors.empty()) {
        m_lodImpostors.clear();
        Msg("* [GeomCache] Building static geometry cache from %zu sectors...", sectors.size());

        xr_vector<dxRender_Visual*> staticVisuals;
        xr_set<dxRender_Visual*> uniqueVisuals;

        for (CSector* sector : sectors) {
            if (sector && sector->root()) {
                ExtractStaticLeafVisuals(sector->root(), staticVisuals);
            }
        }

        for (dxRender_Visual* v : staticVisuals) {
            uniqueVisuals.insert(v);
        }

        u32 batchCountBefore = static_cast<u32>(m_geometryCollector->GetBatches().size());

        for (dxRender_Visual* visual : uniqueVisuals) {
            Fmatrix xform = Fidentity;

            switch (visual->getType()) {
                case MT_TREE_ST:
                case MT_TREE_PM: {
                    FTreeVisual* treeVisual = static_cast<FTreeVisual*>(visual);
                    xform = treeVisual->xform;
                    break;
                }
                default:
                    xform = Fidentity;
                    break;
            }

            if (ProcessVisualGeometry(visual, xform, nullptr, true)) {
                submittedStatic++;
            }
        }

        const auto& allBatches = m_geometryCollector->GetBatches();
        m_cachedStaticBatches.assign(allBatches.begin() + batchCountBefore, allBatches.end());
        m_staticBatchesCached = true;

        Msg("* [GeomCache] Cached %zu static batches from %zu unique visuals (total sectors: %zu)",
            m_cachedStaticBatches.size(), uniqueVisuals.size(), sectors.size());
    }
    else if (m_staticBatchesCached) {
        for (const auto& batch : m_cachedStaticBatches) {
            m_geometryCollector->Submit(batch);
            submittedStatic++;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  PROCESS DYNAMIC GEOMETRY
    // ═══════════════════════════════════════════════════════
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

        IRenderable* renderable = spatial->dcast_Renderable();
        if (!renderable) {
            notRenderable++;
            continue;
        }

        renderable->renderable_Render(0, renderable);
        submittedDynamic++;
    }

    fg::ClusteredLightManager::Instance().CollectLightsParallel(collectedLights);

    // ═══════════════════════════════════════════════════════
    //  HUD RENDERING (after dynamic objects)
    // ═══════════════════════════════════════════════════════

    if (g_pGameLevel && g_pGameLevel->pHUD) {
        g_pGameLevel->pHUD->Render_Last(0);  // context_id = 0 (not using legacy contexts)
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

void FrameGraphRenderer::CreatePhasePass(framegraph::RenderPhase /*phase*/) {
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
        if (m_light)
        {
            m_light->set_active(false);
            xr_delete(m_light);
            m_light = nullptr;
        }
    }

    ~CGlow() override
    {
        fg::passes::GlowRegistry_Unregister(this);
        if (m_light)
        {
            m_light->set_active(false);
            xr_delete(m_light);
            m_light = nullptr;
        }
    }

    static bool TooCloseToCamera(const Fvector& pos, float radius)
    {
        const float minDist = _max(0.45f, radius * 2.f);
        return Device.vCameraPosition.distance_to_sqr(pos) < minDist * minDist;
    }

    static bool CollectBillboard(void* glow, fg::passes::GlowBillboard& out)
    {
        auto* g = static_cast<CGlow*>(glow);
        if (!g || !g->bActive)
            return false;
        if (TooCloseToCamera(g->m_pos, g->m_radius))
            return false;
        out.pos = g->m_pos;
        out.radius = g->m_radius;
        out.color = g->m_color;
        out.texture = g->m_texture;
        return true;
    }

    void set_active(bool b) override { bActive = b; }
    bool get_active() override { return bActive; }
    void set_position(const Fvector& P) override { m_pos = P; }
    void set_direction(const Fvector& D) override
    {
        m_dir = D;
        if (m_dir.magnitude() < 1e-4f)
            m_dir.set(0.f, -1.f, 0.f);
        else
            m_dir.normalize();
    }
    void set_radius(float R) override { m_radius = _max(R, 0.05f); }
    void set_texture(LPCSTR name) override { m_texture = name; }
    void set_color(const Fcolor& C) override { m_color = C; }
    void set_color(float r, float g, float b) override { m_color.set(r, g, b, 1.f); }
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
IRender_Glow*  FrameGraphRenderer::glow_create()
{
    static bool s_glowCollectHooked = false;
    if (!s_glowCollectHooked)
    {
        fg::passes::GlowRegistry_SetCollect(&CGlow::CollectBillboard);
        s_glowCollectHooked = true;
    }
    return xr_new<CGlow>(nullptr);
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
    u32 matID = fgArray->GenerateBindlessMaterialID();
    if (matID == UINT32_MAX)
        return;
    Fvector N;
    N.mknormal(V[T->verts[0]], V[T->verts[1]], V[T->verts[2]]);
    float decalSize = s * 2.0f;
    GetDecalManager()->AddStaticDecal(P, N, decalSize, matID);
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
    m_pProcessHOMTask = &m_HOM.DispatchMTRender();
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
    IReader* hdao_cs      = fg_open_shader("ssao_hdao.cs");
    IReader* hdao_cs_msaa = fg_open_shader("ssao_hdao_msaa.cs");
    const bool exist      = hdao_cs && hdao_cs_msaa;
    FS.r_close(hdao_cs);
    FS.r_close(hdao_cs_msaa);
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
