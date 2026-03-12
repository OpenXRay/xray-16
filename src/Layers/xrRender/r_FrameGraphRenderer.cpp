// xrRender/r_FrameGraphRenderer.cpp
#include "stdafx.h"
#include "r_FrameGraphRenderer.h"
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
#include "r__dsgraph_structure.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/ShaderKey.h"
#include "xrEngine/CustomHUD.h"
#include "ImGuiRendererNVRHI.h"
#include "xrEngine/device.h"
#include <imgui.h>

// Lambda-based pass setup functions
#include "FrameGraphPasses/HiZBuildPassSetup.h"      // Phase 3.5: Hi-Z pyramid for GPU culling
#include "FrameGraphPasses/ForwardColorPassSetup.h"  // Phase 1: Single-RT forward rendering + pipeline init
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
#include "FrameGraphPasses/DistortionApplyPassSetup.h" // Distortion post-process
#include "FrameGraphPasses/DecalPassSetup.h"          // Screen-space box decals
#include "Decals/DecalManager.h"                      // Decal manager
#include "Decals/OverlayManager.h"                    // Per-NPC overlay textures
#include "FrameGraphPasses/ExposurePassSetup.h"      // Auto-exposure from histogram
#include "FrameGraphPasses/UIPassSetup.h"
#include "FrameGraphPasses/TonemapPassSetup.h"       // Tonemap pass: HDR→LDR conversion
#include "FrameGraphPasses/SmokeTrailPassSetup.h"
#include "FrameGraphPasses/ClusterLightPassSetup.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_DB.h"
#include "FrameGraphPasses/MotionVectorPassSetup.h"
#include "FrameGraphPasses/ReSTIRGIPassSetup.h"
#include "FrameGraphPasses/RibbonPassSetup.h"
#include "FrameGraphPasses/TrailPassSetup.h"
#include "Layers/xrRender/FrameGraph/Blackboard.h"
#include "FrameGraphPasses/ImGuiPassSetup.h"
#include "FrameGraphPasses/PathTracerPassSetup.h"
#include "RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "FrameGraphPasses/ShaderConstants.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrParticles/psystem.h"
#include "blenders/Blender_Particle.h"

extern ENGINE_API float psHUD_FOV;

namespace xray::render {

using namespace RENDER_NAMESPACE;

// Forward declaration and extern for accessing RImplementation
namespace RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
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
    Msg("* [FrameGraphRenderer] Created");
}

FrameGraphRenderer::~FrameGraphRenderer() {
    Shutdown();
}

bool FrameGraphRenderer::Initialize(ng::RenderDevice* device) {
    VERIFY(device != nullptr);
    m_device = device;

    Msg("* [FrameGraphRenderer] Initializing...");

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
    m_uiCollector = xr_make_unique<ui::UIRenderCollector>();
    m_uiRenderer = xr_make_unique<ui::NVRHIUIRenderer>();
    m_textVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_textMaterialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_textVCBPool.get()
    );
    m_geometryCollector = xr_make_unique<GeometryCollector>();
    g_geometryCollector = m_geometryCollector.get();


    m_gpuCullingManager = xr_make_unique<RENDER_NAMESPACE::GPUCullingManager>();
    m_detailManager = xr_make_unique<RENDER_NAMESPACE::FGDetailManager>();
    m_decalManager = xr_make_unique<RENDER_NAMESPACE::decals::DecalManager>();
    m_overlayManager = xr_make_unique<RENDER_NAMESPACE::decals::OverlayManager>();
    m_rtAccelMgr = xr_make_unique<RENDER_NAMESPACE::RTAccelStructManager>();
    m_smokeTrailManager = xr_make_unique<RENDER_NAMESPACE::passes::SmokeTrailManager>();


    bindless::MaterialBuffer::Instance().Initialize(m_device);
    bindless::TerrainMaterialBuffer::Instance().Initialize(m_device);
    bindless::VariantTextureBuffer::Instance().Initialize(m_device);
    bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);
    RENDER_NAMESPACE::ClusteredLightManager::Instance().Initialize(m_device);
    Msg("* [FrameGraphRenderer] Bindless material buffers initialized (early)");

    m_uiRenderer->Initialize(device, m_uiMaterialCache.get());
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

    Msg("* [FrameGraphRenderer] initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    g_geometryCollector = nullptr;
    m_statsOverlay = nullptr;
    if (m_gpuProfiler) {
        m_gpuProfiler->Shutdown();
        m_gpuProfiler = nullptr;
    }

    m_inspectorPreview = nullptr;
    m_renderContext = nullptr;
    m_geometryCollector = nullptr;
    m_materialCache = nullptr;
    m_uiRenderer = nullptr;
    m_uiCollector = nullptr;
    m_uiMaterialCache = nullptr;
    m_uiVCBPool = nullptr;
    m_textMaterialCache = nullptr;
    m_textVCBPool = nullptr;
    m_cachedStaticBatches.clear();
    m_staticBatchesCached = false;
    if (m_gpuCullingManager) {
        m_gpuCullingManager->InvalidateStaticCullingData();
    }

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

    RENDER_NAMESPACE::ClusteredLightManager::Instance().Shutdown();

    passes::ShutdownPathTracer();

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    if (m_blackboard) {
        if (auto* sky = m_blackboard->try_get<passes::SkyPassState>())
            passes::ShutdownSkyGeometry(*sky);
        if (auto* sun = m_blackboard->try_get<passes::SunPassState>())
            passes::ShutdownSunPass(*sun);
        if (auto* tonemap = m_blackboard->try_get<passes::TonemapPassState>())
            passes::ShutdownTonemapPass(*tonemap);
        m_blackboard.reset();
    }

    bindless::VariantTextureBuffer::Instance().Shutdown();
    bindless::MaterialBuffer::Instance().Shutdown();
    bindless::TerrainMaterialBuffer::Instance().Shutdown();

    m_device = nullptr;
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
    
    RImplementation.Lights.Update();

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
            if (RImplementation.m_shaderLoader && RImplementation.m_shaderLoader->CheckForChangedFiles())
            {
                Msg("* Shader hot-reload detected, validating changed shaders...");
                if (!RImplementation.m_shaderLoader->ValidateChangedFiles())
                {
                    Msg("! Shader hot-reload skipped: validation failed, keeping current shaders");
                }
                else
                {
                    Msg("* Hot-reloading shaders...");

                    // Ensure no in-flight work references old pipelines/shaders.
                    m_device->GetNVRHIDevice()->waitForIdle();

                    RImplementation.m_shaderLoader->ClearAllCaches();
                    framegraph::GetPassResourceCache().Clear();
                    if (m_blackboard)
                    {
                        m_blackboard->clear();
                    }

                    if (m_detailManager)
                        m_detailManager->InvalidateShadersAndPipelines();

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
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(passes::StaticGlobals), m_device);
    auto staticGlobalsData = passes::BuildStaticGlobals();

    auto& clm = RENDER_NAMESPACE::ClusteredLightManager::Instance();
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
    m_prevViewProj = Device.mFullTransform;
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
        backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
        backbufferDesc.isRenderTarget = true;
        backbufferDesc.isImported = true;
        backbufferDesc.isTransient = false;
        backbufferDesc.debugName = "Backbuffer";

        backbufferHandle = m_framegraph->ImportTexture("Backbuffer", backbufferTexture, backbufferDesc);
    }

    framegraph::ResourceDesc bgDesc;
    bgDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    bgDesc.width = width;
    bgDesc.height = height;
    bgDesc.format = nvrhi::Format::RGBA8_UNORM;
    bgDesc.isRenderTarget = true;
    bgDesc.debugName = "rt_MenuBackground";

    auto backgroundTarget = m_framegraph->CreateTexture("rt_MenuBackground", bgDesc);
    
    framegraph::PassHandle clearPass = m_framegraph->AddPass("ClearBackground");
    m_framegraph->PassWrite(clearPass, backgroundTarget, framegraph::ResourceState::RenderTarget);
    m_framegraph->SetPassCallback(clearPass,
        [backgroundTarget](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
            auto* bgRT = fg.GetPhysicalTexture(backgroundTarget);
            if (bgRT) {
                nvrhi::ICommandList* cmdList = ctx.GetCommandList();
                cmdList->clearTextureFloat(bgRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            }
        }
    );

    auto sceneWithUI = passes::setupUIPass(*m_framegraph, backgroundTarget, width, height);
    sceneWithUI = passes::setupTextPass(*m_framegraph, sceneWithUI, width, height, m_blackboard->get_or_add<passes::UITextPassState>());
    sceneWithUI = passes::setupCursorPass(*m_framegraph, sceneWithUI, width, height);
    
    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        m_device,
        sceneWithUI,  // HDR input (RGBA16_FLOAT)
        framegraph::VirtualResourceHandle(),  // No exposure for menu
        backbufferHandle,  // Output directly to imported backbuffer
        width,
        height,
        m_blackboard->get_or_add<passes::TonemapPassState>()
    );

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,  // LDR input (RGBA8_UNORM)
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

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
            auto& clmStats = RENDER_NAMESPACE::ClusteredLightManager::Instance();
            const auto& pkg = RImplementation.Lights.package;
            stats.lightsPoint = static_cast<u32>(pkg.v_point.size());
            stats.lightsSpot = static_cast<u32>(pkg.v_spot.size());
            stats.lightsShadowed = static_cast<u32>(pkg.v_shadowed.size());
            stats.lightsTotal = stats.lightsPoint + stats.lightsSpot + stats.lightsShadowed;
            stats.lightsClustered = clmStats.GetLightCount();
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
        m_gpuCullingManager->ProcessStatsReadback();
        m_gpuCullingManager->ProcessSkinnedVisibilityReadback();
    }

    if (m_detailManager && m_device) {
        m_detailManager->ProcessStatsReadback(m_device->GetNVRHIDevice());
    }

    m_bufferHandleCache.clear();
    m_lstRenderables.clear();

    if (levelLoaded)
    {
        if (levelLoaded && !g_pGamePersistent->IsLoadingScreenShown())
        {
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

    m_geometryCollector->BeginFrame();
    m_hudBatches.clear();
    m_worldParticleBatches.clear();
    m_hudParticleBatches.clear();

    if (levelLoaded)
        CollectVisibleGeometry();

    if (levelLoaded)
        RENDER_NAMESPACE::ClusteredLightManager::Instance().BuildLightBuffer(RImplementation.Lights.package);

    m_geometryCollector->EndFrame();

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
    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    nvrhi::ITexture* backbufferTexture = GEnv.Backend->GetBackBuffer();
    framegraph::VirtualResourceHandle backbufferHandle;

    if (backbufferTexture) {
        framegraph::ResourceDesc backbufferDesc;
        backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        backbufferDesc.width = width;
        backbufferDesc.height = height;
        backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;  // Swapchain format
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
    //  TEMPORAL HI-Z PYRAMID BUILD (From Previous Frame)
    // ═══════════════════════════════════════════════════════
    passes::HiZPyramidOutput hizOutput;
    hizOutput.pyramid = framegraph::VirtualResourceHandle();  // Invalid by default
    hizOutput.mipLevels = 0;
    hizOutput.width = width / 2;
    hizOutput.height = height / 2;

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
    //  PHASE 3.5: GPU CULLING PASS (Frustum + Occlusion)
    // ═══════════════════════════════════════════════════════

    framegraph::VirtualResourceHandle drawArgsBuffer;  // Will be passed to forward pass

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

        if (m_gpuCullingManager->IsParticleCullingEnabled() && !m_worldParticleBatches.empty()) {
            m_gpuCullingManager->SetupParticleCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                &m_worldParticleBatches
            );
        }

        if (m_gpuCullingManager->IsSkinnedCullingEnabled()) {
            m_gpuCullingManager->SetupSkinnedCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                m_geometryCollector.get(),
                m_prevViewProj
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

    auto skyOutput = passes::setupSkyPass(
        *m_framegraph,
        m_device,
        skyColorHandle,
        depthBuffer,
        g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr,
        width,
        height,
        m_blackboard->get_or_add<passes::SkyPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  SUN PASS (Sun disc with additive blending)
    // ═══════════════════════════════════════════════════════

    auto sunOutput = passes::setupSunPass(
        *m_framegraph,
        m_device,
        skyOutput,
        g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr,
        width,
        height,
        m_blackboard->get_or_add<passes::SunPassState>()
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

    auto& clmSetup = RENDER_NAMESPACE::ClusteredLightManager::Instance();
    if (clmSetup.IsReady() && clmSetup.GetLightCount() > 0) {
        passes::setupClusterLightPass(
            *m_framegraph,
            m_device,
            &clmSetup,
            width,
            height,
            &m_blackboard->get_or_add<passes::ClusterLightPassState>()
        );
    }

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        sunOutput,
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

    // 2. Skinning Pass - Renders all skinned meshes (world + HUD)
    // World skinned: NPCs, monsters with normal depth [0.0, 1.0]
    // HUD skinned: First-person weapons/hands with depth [0.0, 0.1]
    static auto skinnedStatsCallback = +[](u32 rendered, u32 culled, void* userData) {
        static_cast<GPUCullingManager*>(userData)->UpdateSkinnedCullingStats(rendered, culled);
    };

    static auto visibilityByVisualCallback = +[](const dxRender_Visual* visual, void* userData) -> u32 {
        return static_cast<GPUCullingManager*>(userData)->GetSkinnedVisibilityByVisual(visual);
    };

    passes::SkinnedVisibilityData skinnedVisibility;
    if (m_gpuCullingManager && m_gpuCullingManager->IsSkinnedCullingEnabled()) {
        skinnedVisibility.enabled = m_gpuCullingManager->HasSkinnedVisibilityData();
        skinnedVisibility.visibilityByVisualCallback = visibilityByVisualCallback;
        skinnedVisibility.visibilityByVisualUserData = m_gpuCullingManager.get();
        skinnedVisibility.statsCallback = skinnedStatsCallback;
        skinnedVisibility.statsUserData = m_gpuCullingManager.get();
    }

    auto hudOutputs = passes::setupSkinningPass(
        *m_framegraph,
        m_device,
        forwardOutputs,
        m_geometryCollector.get(),
        &m_hudBatches,
        m_materialCache.get(),
        width,
        height,
        skinnedVisibility,
        &m_blackboard->get_or_add<passes::SkinningPassState>(),
        m_overlayManager.get()
    );

    // ═══════════════════════════════════════════════════════
    //  DETAIL CULL PASS (Async Compute)
    // ═══════════════════════════════════════════════════════
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
        &m_blackboard->get_or_add<passes::DetailPassState>()
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
            [](const Perlin4DGenData& data, const framegraph::FrameGraph&, ng::RenderContext* ctx)
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
        hudOutputs,
        width,
        height,
        m_gpuProfiler.get()
    );

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

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
            transparentConfig.variantPartition = m_gpuCullingManager->GetTransparentPartition().ToConfig();
    }

    auto transparentOutputs = passes::setupTransparentPass(
        *m_framegraph,
        m_device,
        detailOutputs,
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

    // ═══════════════════════════════════════════════════════
    //  MOTION VECTOR PASS (Depth-based reprojection)
    // ═══════════════════════════════════════════════════════
    passes::MotionVectorOutput motionOutput;
    if (m_hasPrevFrameData) {
        motionOutput = passes::setupMotionVectorPass(
            *m_framegraph, m_device,
            transparentOutputs.depth,
            Device.mInvFullTransform, m_prevViewProj,
            width, height,
            m_blackboard->get_or_add<passes::MotionVectorPassState>()
        );
    }

    // ═══════════════════════════════════════════════════════
    //  PARTICLE PASS (after all opaque + transparent geometry)
    // ═══════════════════════════════════════════════════════
    auto particleOutputs = passes::setupParticlePass(
        *m_framegraph,
        m_device,
        transparentOutputs,
        &m_worldParticleBatches,
        &m_hudParticleBatches,
        m_materialCache.get(),
        width,
        height,
        hizOutput.pyramid,
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels,
        &m_blackboard->get_or_add<passes::ParticlePassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  RIBBON PASS (test quad, after particles)
    // ═══════════════════════════════════════════════════════
    auto ribbonOutputs = passes::setupRibbonPass(
        *m_framegraph,
        m_device,
        particleOutputs.layout,
        width,
        height,
        &m_blackboard->get_or_add<passes::RibbonPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  TRAIL PASS (after ribbon, stored-direction width)
    // ═══════════════════════════════════════════════════════
    auto trailOutputs = passes::setupTrailPass(
        *m_framegraph,
        m_device,
        ribbonOutputs.layout,
        width,
        height,
        &m_blackboard->get_or_add<passes::TrailPassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  SMOKE TRAIL PASS (GPU-simulated weapon muzzle smoke)
    // ═══════════════════════════════════════════════════════
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

    auto sceneColor = smokeOutputs.albedo;

    if (particleOutputs.distortionRT.is_valid()) {
        sceneColor = passes::setupDistortionApplyPass(
            *m_framegraph, m_device, sceneColor, particleOutputs.distortionRT,
            particleOutputs.layout.worldPos, width, height,
            m_blackboard->get_or_add<passes::DistortionApplyPassState>());
    }

    // ═══════════════════════════════════════════════════════
    //  ReSTIR GI (RT Shadows + Indirect Lighting)
    // ═══════════════════════════════════════════════════════

    extern ENGINE_API int ps_r_rt_gi;
    extern ENGINE_API float ps_r_rt_gi_intensity;

    if (ps_r_rt_gi && m_rtAccelMgr && m_rtAccelMgr->IsSupported() && m_rtAccelMgr->IsReady()) {
        auto rtgiOutput = passes::setupReSTIRGIPass(
            *m_framegraph, m_device, m_rtAccelMgr.get(),
            transparentOutputs.depth, transparentOutputs.normal,
            transparentOutputs.baseColor, transparentOutputs.worldPos,
            prevNormalsHandle, prevWorldPosHandle,
            motionOutput.motionVectors,
            sceneColor,
            Device.mInvFullTransform, m_prevViewProj,
            Device.vCameraPosition, ps_r_rt_gi_intensity,
            width, height,
            m_blackboard->get_or_add<passes::ReSTIRGIPassState>(), m_hasPrevFrameData
        );
        sceneColor = rtgiOutput.sceneColor;
    }

    // ═══════════════════════════════════════════════════════
    //  DYNAMIC BLAS BUILD (shared by Path Tracer + ReSTIR GI)
    // ═══════════════════════════════════════════════════════
    extern ENGINE_API int ps_r_path_tracer;
    extern ENGINE_API int ps_r_path_tracer_bounces;

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
                [](const DynamicBLASData& data, const framegraph::FrameGraph&, ng::RenderContext* ctx) {
                    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                    xr_vector<GeometryBatch> worldSkinned;
                    for (const auto& b : data.geometry->GetBatches()) {
                        if (b.isSkinned && b.visual && b.indexCount > 0)
                            worldSkinned.push_back(b);
                    }

                    extern ENGINE_API float psHUD_FOV;
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

                    data.gpuCulling->BeginSkinnedFrame();
                    data.accelMgr->BuildSkinnedBLAS(cmdList, data.gpuCulling, worldSkinned, hudSkinned);
                    data.accelMgr->BuildGrassBLAS(cmdList, data.detailMgr);
                    data.accelMgr->RebuildDynamic(cmdList, data.gpuCulling);
                }
            );
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

    auto sceneWithUI = passes::setupUIPass(
        *m_framegraph,
        sceneColor,
        width,
        height
    );

    sceneWithUI = passes::setupTextPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height,
        m_blackboard->get_or_add<passes::UITextPassState>()
    );

    // 5. Cursor Pass - Renders cursor on top of UI+Text
    sceneWithUI = passes::setupCursorPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height
    );

    // 6. Tonemap Pass - Convert HDR to LDR using ACES filmic tonemap
    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        m_device,
        sceneWithUI,
        exposureOutput.exposureTexture,
        backbufferHandle,
        width,
        height,
        m_blackboard->get_or_add<passes::TonemapPassState>(),
        &m_blackboard->get_or_add<passes::ExposurePassState>()
    );

    // ═══════════════════════════════════════════════════════
    //  DEBUG PREVIEW PASS (Render Inspector RT visualization)
    // ═══════════════════════════════════════════════════════
    m_framegraph->GetRTRegistry().RegisterRT("rt_SceneColor", skyColorHandle);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Depth", depthBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Normal", transparentOutputs.normal);
    m_framegraph->GetRTRegistry().RegisterRT("rt_BaseColor", baseColorBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_WorldPos", worldPosBuffer);
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
                    ng::RenderDevice* device;
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
                    [](const DebugPreviewData& data, const framegraph::FrameGraph& fg, ng::RenderContext* ctx) {
                        static nvrhi::ComputePipelineHandle s_pipeline;
                        static nvrhi::BindingLayoutHandle s_layout;
                        static nvrhi::BufferHandle s_cb;
                        static bool s_init = false;

                        nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                        if (!s_init) {
                            auto csResult = RImplementation.m_shaderLoader->LoadComputeShader("debug_preview");
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
                            cbDesc.maxVersions = ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
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

                        auto* debugRefl = RImplementation.m_shaderLoader->GetCachedReflection("debug_preview", ".cs");
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

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,
        imguiRenderer,
        width,
        height
    );

    // Store final output for presentation (now points to backbuffer)
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
                [finalDepth, prevDepthCopyDest](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
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

    RENDER_NAMESPACE::ShaderKey shaderKey;
    if (RENDER_NAMESPACE::ExtractShaderKey(visual, shaderKey)) {
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

    RENDER_NAMESPACE::ShaderKey shaderKey;
    if (RENDER_NAMESPACE::ExtractShaderKey(visual, shaderKey)) {
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
    if (!shaderName || !shaderName[0])
        return passes::PARTICLE_BLEND_BLEND;

    IBlender* B = RENDER_NAMESPACE::RImplementation.Resources->_FindBlender(shaderName);
    if (!B)
        return passes::PARTICLE_BLEND_BLEND;

    if (B->getDescription().CLS == B_PARTICLE) {
        auto* bp = static_cast<RENDER_NAMESPACE::CBlender_Particle*>(B);
        u32 id = bp->oBlend.IDselected;
        return (id < passes::PARTICLE_BLEND_COUNT) ? (u8)id : passes::PARTICLE_BLEND_BLEND;
    }

    return passes::PARTICLE_BLEND_BLEND;
}

void FrameGraphRenderer::ProcessSingleParticleEffect(
    RENDER_NAMESPACE::PS::CParticleEffect* pEffect,
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
        auto* pEffect = static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(visual);
        ProcessSingleParticleEffect(pEffect, worldTransform, renderable, isHUD);
        return true;
    }

    if (vType == MT_PARTICLE_GROUP) {
        auto* pGroup = static_cast<RENDER_NAMESPACE::PS::CParticleGroup*>(visual);
        for (auto& item : pGroup->items) {
            if (item._effect) {
                auto* childEffect = static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(item._effect);
                ProcessSingleParticleEffect(childEffect, worldTransform, renderable, isHUD);
            }
            for (auto* child : item._children_related) {
                if (child && child->getType() == MT_PARTICLE_EFFECT)
                    ProcessSingleParticleEffect(
                        static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(child),
                        worldTransform, renderable, isHUD);
            }
            for (auto* child : item._children_free) {
                if (child && child->getType() == MT_PARTICLE_EFFECT)
                    ProcessSingleParticleEffect(
                        static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(child),
                        worldTransform, renderable, isHUD);
            }
        }
        return true;
    }

    return false;
}

void FrameGraphRenderer::ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs) {
    if (!pVisual)
        return;

    switch (pVisual->Type) {
        case MT_HIERRARHY: {
            FHierrarhyVisual* pV = static_cast<FHierrarhyVisual*>(pVisual);
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }
            break;
        }
        case MT_LOD: {
            FLOD* pV = static_cast<FLOD*>(pVisual);
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }
            break;
        }
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            CKinematics* pV = static_cast<CKinematics*>(pVisual);
            pV->CalculateBones_InvalidateFG();
            pV->CalculateBonesFG(TRUE);
            
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }

            // TODO: Also check for LOD model / progressive skinning
            //if (pV->m_lod) {
                //outLeafs.push_back(pV->m_lod);
            //}
            break;
        }
        case MT_SKELETON_GEOMDEF_PM:
        case MT_SKELETON_GEOMDEF_ST: {
            outLeafs.push_back(pVisual);
            break;
        }
        case MT_PROGRESSIVE: {
            outLeafs.push_back(pVisual);
            break;
        }
        case MT_PARTICLE_GROUP: {
            PS::CParticleGroup* pG = static_cast<PS::CParticleGroup*>(pVisual);
            for (auto& item : pG->items) {
                xr_vector<dxRender_Visual*> visuals;
                item.GetVisuals(visuals);
                for (auto* v : visuals) {
                    ExtractStaticLeafVisuals(v, outLeafs);
                }
            }
            break;
        }
        case MT_PARTICLE_EFFECT:
            outLeafs.push_back(pVisual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
        case MT_NORMAL:
        default: {
            outLeafs.push_back(pVisual);
            break;
        }
    }
}

void FrameGraphRenderer::CollectVisibleGeometry() {
    if (!g_pGamePersistent)
        return;

    auto& dsgraph = RImplementation.get_imm_context();
    u32 submittedStatic = 0;

    if (!m_staticBatchesCached && !dsgraph.Sectors.empty()) {
        Msg("* [GeomCache] Building static geometry cache from %zu sectors...", dsgraph.Sectors.size());

        xr_vector<dxRender_Visual*> staticVisuals;
        xr_set<dxRender_Visual*> uniqueVisuals;

        for (CSector* sector : dsgraph.Sectors) {
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
            m_cachedStaticBatches.size(), uniqueVisuals.size(), dsgraph.Sectors.size());
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

    // Process each visible dynamic object (from cached list)
    u32 portalTraversalMarker = dsgraph.PortalTraverser.i_marker;

    for (ISpatial* spatial : m_lstRenderables)
    {
        const auto& data = spatial->GetSpatialData();
        const auto sector_id = data.sector_id;
        IRenderable* renderable = spatial->dcast_Renderable();

        if (data.type & STYPE_LIGHTSOURCE) {
            light* L = (light*)spatial->dcast_Light();
            if (L) {
                float lod = L->get_LOD();
                if (lod > EPS_L) {
                    vis_data& vis = L->get_homdata();
                    if (RImplementation.HOM.visible(vis))
                        RImplementation.Lights.add_light(L);
                }
            }
            continue;
        }
        // Get the renderable object
        if (!renderable) {
            notRenderable++;
            continue;
        }

        renderable->renderable_Render(0, renderable);
        submittedDynamic++;  // Count objects that got a chance to render
    }

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
    
    xr_vector<dxRender_Visual*> leafVisuals;
    ExtractStaticLeafVisuals(visual, leafVisuals);

    u32 processed = 0;
    for (dxRender_Visual* leafVisual : leafVisuals) {
        bool success = false;
        if (isHUD) {
            // HUD geometry - separate processing with different projection/culling
            success = ProcessHudGeometry(leafVisual, xform, root);
        } else {
            // World geometry - standard processing
            success = ProcessVisualGeometry(leafVisual, xform, root);
        }

        if (success) {
            processed++;
        }
    }
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

void FrameGraphRenderer::RenderImGui(ImDrawData* drawData, ng::ImGuiRendererNVRHI* imguiRenderer) {
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

} // namespace xray::render
