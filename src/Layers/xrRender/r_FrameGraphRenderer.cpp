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
#include "FrameGraphPasses/PassStates.h"             // Aggregate pass state (owns all per-pass state)
#include "FrameGraphPasses/ImGuiPassSetup.h"
#include "FrameGraphPasses/PathTracerPassSetup.h"
#include "RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrParticles/psystem.h"
#include "blenders/Blender_Particle.h"

namespace xray::render {

using namespace RENDER_NAMESPACE;

// Forward declaration and extern for accessing RImplementation
namespace RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
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

    // Create FrameGraph
    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device);

    // Create shader phase cache (Week 16 - for precompilation phase detection)
    m_shaderPhaseCache = xr_make_unique<framegraph::ShaderPhaseCache>();

    // Create VCB pool for geometry rendering
    m_geometryVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();

    // Create material cache for geometry rendering
    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_geometryVCBPool.get()
    );

    // Create UI rendering infrastructure
    m_uiVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_uiMaterialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_uiVCBPool.get()
    );
    m_uiCollector = xr_make_unique<ui::UIRenderCollector>();
    m_uiRenderer = xr_make_unique<ui::NVRHIUIRenderer>();
    m_uiRenderer->Initialize(device, m_uiMaterialCache.get());

    Msg("* [FrameGraphRenderer] UI infrastructure initialized");

    // Create Text rendering infrastructure
    m_textVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_textMaterialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_textVCBPool.get()
    );

    Msg("* [FrameGraphRenderer] Text infrastructure initialized");

    // Create geometry collector
    m_geometryCollector = xr_make_unique<GeometryCollector>();

    // Set global geometry collector pointer
    g_geometryCollector = m_geometryCollector.get();

    // ═══════════════════════════════════════════════════════
    //  INITIALIZE BINDLESS BUFFERS (EARLY - before level load!)
    // ═══════════════════════════════════════════════════════
    // CRITICAL: Must initialize BEFORE level geometry is loaded!
    // Level load calls ProcessVisualGeometry -> PreRegisterTerrainMaterial
    // which needs TerrainMaterialBuffer to be initialized, otherwise
    // terrain batches get UINT32_MAX material IDs and render black.
    bindless::MaterialBuffer::Instance().Initialize(m_device);
    bindless::TerrainMaterialBuffer::Instance().Initialize(m_device);
    bindless::VariantTextureBuffer::Instance().Initialize(m_device);
    bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);  // Max 64K draws
    Msg("* [FrameGraphRenderer] Bindless material buffers initialized (early)");

    // Create GPU culling manager (Phase 3.5)
    // NOTE: Initialization is deferred to first frame (SetupFrameGraphPasses)
    // because ShaderLoader isn't ready during FrameGraphRenderer::Initialize
    m_gpuCullingManager = xr_make_unique<RENDER_NAMESPACE::GPUCullingManager>();

    // Create detail manager (Framegraph)
    // Note: Will be loaded during level loading (see r2_loader.cpp), not here
    m_detailManager = xr_make_unique<RENDER_NAMESPACE::FGDetailManager>();

    m_decalManager = xr_make_unique<RENDER_NAMESPACE::decals::DecalManager>();
    m_decalManager->Initialize(device);

    m_overlayManager = xr_make_unique<RENDER_NAMESPACE::decals::OverlayManager>();
    m_overlayManager->Initialize(device);

    m_rtAccelMgr = xr_make_unique<RENDER_NAMESPACE::RTAccelStructManager>();
    m_rtAccelMgr->Initialize(device);

    // Create RenderContext for execution
    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    // ═══════════════════════════════════════════════════════
    //  TRANSIENT RESOURCES (NEW ARCHITECTURE)
    // ═══════════════════════════════════════════════════════
    // Resources now created per-frame in SetupFrameGraphPasses()
    // No need for BuildFrameGraphStructure() anymore

    // BuildFrameGraphStructure();  // REMOVED - Using transient resources
    // m_framegraph->Compile();     // REMOVED - Compile per-frame now

    // ═══════════════════════════════════════════════════════
    //  INITIALIZE PASS RESOURCES
    // ═══════════════════════════════════════════════════════
    m_passStates = xr_make_unique<passes::PassStates>();
    passes::InitializeSkyGeometry(device, m_passStates->sky);
    passes::InitializeSunPass(device, m_passStates->sun);
    passes::InitializeTonemapPass(device->GetNVRHIDevice(), m_passStates->tonemap);

    // ═══════════════════════════════════════════════════════
    //  PROFILER (GPU timing + ImGui overlay)
    // ═══════════════════════════════════════════════════════
    m_gpuProfiler = xr_make_unique<xray::profiler::GPUProfiler>();
    m_gpuProfiler->Initialize(device->GetNVRHIDevice());

    m_statsOverlay = xr_make_unique<xray::profiler::StatsOverlay>();
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

    // Clear global geometry collector pointer
    g_geometryCollector = nullptr;

    // Cleanup profiler
    m_statsOverlay = nullptr;
    if (m_gpuProfiler) {
        m_gpuProfiler->Shutdown();
        m_gpuProfiler = nullptr;
    }

    m_inspectorPreview = nullptr;
    m_renderContext = nullptr;
    m_geometryCollector = nullptr;
    m_materialCache = nullptr;

    // Cleanup UI infrastructure
    m_uiRenderer = nullptr;
    m_uiCollector = nullptr;
    m_uiMaterialCache = nullptr;
    m_uiVCBPool = nullptr;

    // Cleanup Text infrastructure
    m_textMaterialCache = nullptr;
    m_textVCBPool = nullptr;

    // Clear static geometry cache
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
    passes::ShutdownPathTracer();

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    if (m_passStates) {
        passes::ShutdownSkyGeometry(m_passStates->sky);
        passes::ShutdownSunPass(m_passStates->sun);
        passes::ShutdownTonemapPass(m_passStates->tonemap);
        m_passStates.reset();
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

    // ═══════════════════════════════════════════════════════
    //  GPU PROFILER FRAME START
    // ═══════════════════════════════════════════════════════
    if (m_gpuProfiler)
    {
        // Sync GPU profiler with CPU profiler's throttled state
        // (CPU profiler only runs every N frames, GPU should match)
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }

    auto frameStart = std::chrono::high_resolution_clock::now();

    // ═══════════════════════════════════════════════════════
    //  UPDATE LIGHTS (Sun direction, color from environment)
    // ═══════════════════════════════════════════════════════
    // Must be called before rendering to update sun from environment system
    RImplementation.Lights.Update();

    // ═══════════════════════════════════════════════════════
    //  UPDATE RESOURCE MANAGER (Video textures, streaming)
    // ═══════════════════════════════════════════════════════

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    // NOTE: Bindless buffers are initialized in FrameGraphRenderer::Initialize()
    // (before level load) so terrain materials get valid IDs during geometry processing.
    // The Initialize() calls are idempotent so this is just a safety fallback.

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
    // With lambda-based passes, we rebuild the graph every frame
    // (Frostbite-style: graph rebuilt, GPU resources reused from pool)
    m_framegraph->ResetForNextFrame();

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

    // Set RenderContext for execution
    m_framegraph->SetRenderContext(m_renderContext.get());

    // Set GPUProfiler for per-pass timing
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

    // Execute the compiled graph (FrameGraph orchestrates all passes)
    {
        ZoneScopedN("FG::Execute");
        m_framegraph->Execute();
    }

    // Schedule GPU culling stats readback (for profiling overlay)
    if (m_gpuCullingManager && psDeviceFlags.test(rsStatistic))
    {
        m_gpuCullingManager->ScheduleStatsReadback(m_renderContext->GetCommandList());
    }

    // Mark that we have valid previous frame data for next frame's Hi-Z
    m_hasPrevFrameData = true;
    m_prevViewProj = Device.mFullTransform;
    m_prevCameraPos = Device.vCameraPosition;

    // ═══════════════════════════════════════════════════════
    //  GPU PROFILER FRAME END
    // ═══════════════════════════════════════════════════════
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

    // NOTE: We call Reset() at the start of each frame (line 199)
    // No need to reset here at the end
}

// ═══════════════════════════════════════════════════════
//  RENDER MENU (Simplified for Main Menu)
// ═══════════════════════════════════════════════════════
// Main menu rendering: No 3D geometry, lighting, or post-process
// Just clear background + ImGui UI overlay

void FrameGraphRenderer::RenderMenu() {
    ZoneScopedN("FrameGraphRenderer::RenderMenu");

    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    // GPU profiler frame start - sync with CPU profiler's throttled state
    if (m_gpuProfiler)
    {
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }

    // Msg("* [FrameGraphRenderer::RenderMenu] Rendering main menu frame");

    // ═══════════════════════════════════════════════════════
    //  UPDATE RESOURCE MANAGER (Video textures, streaming)
    // ═══════════════════════════════════════════════════════

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    // ═══════════════════════════════════════════════════════
    //  RESET FRAMEGRAPH FOR NEW FRAME
    // ═══════════════════════════════════════════════════════
    m_framegraph->ResetForNextFrame();

    // ═══════════════════════════════════════════════════════
    //  MENU PASS SETUP (LAMBDA-BASED)
    // ═══════════════════════════════════════════════════════
    // For menu, we skip GBuffer/HUD and just render UI

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    // ═══════════════════════════════════════════════════════
    //  IMPORT BACKBUFFER (Frostbite Pattern)
    // ═══════════════════════════════════════════════════════
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

    // 1. Create black background target
    framegraph::ResourceDesc bgDesc;
    bgDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    bgDesc.width = width;
    bgDesc.height = height;
    bgDesc.format = nvrhi::Format::RGBA8_UNORM;
    bgDesc.isRenderTarget = true;
    bgDesc.debugName = "rt_MenuBackground";

    auto backgroundTarget = m_framegraph->CreateTexture("rt_MenuBackground", bgDesc);

    // Clear background in a simple pass (using manual PassWrite since we need the PassHandle)
    framegraph::PassHandle clearPass = m_framegraph->AddPass("ClearBackground");
    m_framegraph->PassWrite(clearPass, backgroundTarget, framegraph::ResourceState::RenderTarget);
    m_framegraph->SetPassCallback(clearPass,
        [backgroundTarget](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
            auto* bgRT = fg.GetPhysicalTexture(backgroundTarget);
            if (bgRT) {
                nvrhi::ICommandList* cmdList = ctx.GetCommandList();
                cmdList->clearTextureFloat(bgRT, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
        }
    );

    // 2. UI Pass - Renders menu UI directly to background with alpha blending
    auto sceneWithUI = passes::setupUIPass(*m_framegraph, backgroundTarget, width, height);

    // 3. Text Pass - Renders menu text
    sceneWithUI = passes::setupTextPass(*m_framegraph, sceneWithUI, width, height, m_passStates->uiText);

    // 4. Cursor Pass - Renders cursor
    sceneWithUI = passes::setupCursorPass(*m_framegraph, sceneWithUI, width, height);

    // 5. Tonemap Pass - Convert HDR to LDR using ACES filmic tonemap
    // Note: No exposure pass for menu rendering, pass invalid handle for fixed exposure
    // Renders directly to backbuffer (Frostbite pattern)
    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        sceneWithUI,  // HDR input (RGBA16_FLOAT)
        framegraph::VirtualResourceHandle(),  // No exposure for menu
        backbufferHandle,  // Output directly to imported backbuffer
        width,
        height,
        m_passStates->tonemap
    );

    // 6. ImGui Pass - Debug overlay on LDR output
    // Note: Stats overlay is rendered from device.cpp between ImGui::NewFrame/EndFrame
    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,  // LDR input (RGBA8_UNORM)
        imguiRenderer,
        width,
        height
    );

    // Store final output for presentation
    m_finalOutput = finalOutput;

    // ═══════════════════════════════════════════════════════
    //  COMPILE & EXECUTE
    // ═══════════════════════════════════════════════════════

    m_framegraph->SetRenderContext(m_renderContext.get());
    m_framegraph->SetGPUProfiler(m_gpuProfiler.get());
    m_framegraph->SetAsyncCompute(nullptr, nullptr);
    m_framegraph->Compile();
    m_framegraph->Execute();

    // GPU profiler frame end
    if (m_gpuProfiler)
        m_gpuProfiler->FrameEnd();

    // NOTE: We call Reset() at the start of each frame (line 285)
    // No need to reset here at the end

    // Msg("* [FrameGraphRenderer::RenderMenu] Menu frame complete");
}

void FrameGraphRenderer::RenderStatsOverlay()
{
    // Called from device.cpp between ImGui::NewFrame and EndFrame
    // This ensures proper ImGui input processing
    if (m_statsOverlay && psDeviceFlags.test(rsStatistic))
    {
        // Collect render stats before displaying
        xray::profiler::RenderStats stats;
        stats.Reset();

        // Collect geometry stats from collector
        if (m_geometryCollector)
        {
            const auto& batches = m_geometryCollector->GetBatches();
            stats.totalBatches = static_cast<u32>(batches.size());

            // Track unique skeletons for bone counting
            xr_set<IRenderVisual*> uniqueSkeletons;

            for (const auto& batch : batches)
            {
                u32 triangles = batch.indexCount / 3;
                stats.totalTriangles += triangles;

                if (batch.isSkinned)
                {
                    stats.skinnedBatches++;
                    stats.skinnedTriangles += triangles;

                    // Count unique skeletons and their bones
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

    // Process GPU culling stats readback from previous frame
    if (m_gpuCullingManager) {
        m_gpuCullingManager->ProcessStatsReadback();
        m_gpuCullingManager->ProcessSkinnedVisibilityReadback();
    }

    // Process detail culling stats readback from previous frame
    if (m_detailManager && m_device) {
        m_detailManager->ProcessStatsReadback(m_device->GetNVRHIDevice());
    }

    // Clear buffer handle cache (X-Ray may recreate buffers each frame)
    m_bufferHandleCache.clear();

    // Clear cached spatial queries from previous frame
    m_lstRenderables.clear();

    // Query spatial database ONCE per frame (mimicking render_main::calculate())
    // This populates m_lstRenderables which we reuse throughout the frame
    if (levelLoaded)
    {
        // Setup frustum (same as render_main)
        // Safety check: Ensure spatial database is initialized
        // SpatialSpace isn't ready during early level loading
        if (levelLoaded && !g_pGamePersistent->IsLoadingScreenShown())
        {
            CFrustum view_frustum;
            view_frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

            // Query spatial DB with EXACT same parameters as render_main::calculate()
            // See: src/Layers/xrRender_R2/r2_R_calculate.cpp lines 54-58
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

    // Begin geometry collection
    m_geometryCollector->BeginFrame();

    // Clear HUD batches from previous frame
    m_hudBatches.clear();

    // Clear particle batches from previous frame
    m_worldParticleBatches.clear();
    m_hudParticleBatches.clear();

    // Collect visible geometry (CPU culling for now, GPU later)
    if (levelLoaded)
        CollectVisibleGeometry();

    // End geometry collection (sorts batches)
    m_geometryCollector->EndFrame();

}

// ═══════════════════════════════════════════════════════
//  HELPER: CREATE RENDER TARGET (DRY)
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  SETUP FRAMEGRAPH PASSES (CALLED PER-FRAME)
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::SetupFrameGraphPasses() {
    // ═══════════════════════════════════════════════════════
    //  LAMBDA-BASED PASS SETUP (NEW ARCHITECTURE)
    // ═══════════════════════════════════════════════════════
    // ALL passes now use lambda pattern - executed by FrameGraph

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    // ═══════════════════════════════════════════════════════
    //  IMPORT BACKBUFFER (Frostbite Pattern)
    // ═══════════════════════════════════════════════════════
    // Import swapchain backbuffer as external resource.
    // Final pass (TonemapPass/ImGuiPass) renders directly to it.
    // This eliminates the need for a separate copy-to-backbuffer pass.

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
    // Instead of rendering geometry twice (depth prepass + forward), we:
    // 1. Build Hi-Z from PREVIOUS frame's depth (1 frame latency, usually fine)
    // 2. Render forward pass with fresh depth buffer
    // 3. Copy depth at end of frame for next frame's Hi-Z
    //
    // PERFORMANCE: Saves ~1.5-2.0ms by eliminating depth prepass
    // TRADEOFF: 1 frame latency for occlusion culling (conservative errors OK)

    // Create fresh depth buffer for this frame (forward pass will write to it)
    framegraph::ResourceDesc depthDesc;
    depthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    depthDesc.debugName = "rt_Depth";
    depthDesc.width = width;
    depthDesc.height = height;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;
    // Note: Depth clear happens in ForwardColorPass execute lambda

    framegraph::VirtualResourceHandle depthBuffer = m_framegraph->CreateTexture("rt_Depth", depthDesc);

    framegraph::ResourceDesc normalDesc;
    normalDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    normalDesc.debugName = "rt_Normal";
    normalDesc.width = width;
    normalDesc.height = height;
    normalDesc.format = nvrhi::Format::RGBA16_FLOAT;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    framegraph::VirtualResourceHandle normalBuffer = m_framegraph->CreateTexture("rt_Normal", normalDesc);

    framegraph::ResourceDesc baseColorDesc;
    baseColorDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    baseColorDesc.debugName = "rt_BaseColor";
    baseColorDesc.width = width;
    baseColorDesc.height = height;
    baseColorDesc.format = nvrhi::Format::RGBA8_UNORM;
    baseColorDesc.isRenderTarget = true;
    baseColorDesc.isTransient = true;
    framegraph::VirtualResourceHandle baseColorBuffer = m_framegraph->CreateTexture("rt_BaseColor", baseColorDesc);

    framegraph::ResourceDesc worldPosDesc;
    worldPosDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    worldPosDesc.debugName = "rt_WorldPos";
    worldPosDesc.width = width;
    worldPosDesc.height = height;
    worldPosDesc.format = nvrhi::Format::RGBA32_FLOAT;
    worldPosDesc.isRenderTarget = true;
    worldPosDesc.isTransient = true;
    framegraph::VirtualResourceHandle worldPosBuffer = m_framegraph->CreateTexture("rt_WorldPos", worldPosDesc);

    // ═══════════════════════════════════════════════════════
    //  TEMPORAL HI-Z PYRAMID BUILD (From Previous Frame)
    // ═══════════════════════════════════════════════════════
    // Build Hi-Z pyramid from previous frame's depth buffer.
    // First frame: Hi-Z culling disabled (frustum-only culling)
    //
    // PERFORMANCE: ~0.2-0.3ms (async compute capable)

    passes::HiZPyramidOutput hizOutput;
    hizOutput.pyramid = framegraph::VirtualResourceHandle();  // Invalid by default
    hizOutput.mipLevels = 0;
    hizOutput.width = width / 2;
    hizOutput.height = height / 2;

    // Check if we have valid previous frame depth
    bool hasPrevDepth = m_hasPrevFrameData && m_prevFrameDepth &&
                        m_prevFrameWidth == width && m_prevFrameHeight == height;

    if (hasPrevDepth) {
        // Import previous frame's depth into framegraph
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

        // Build Hi-Z from previous frame's depth
        hizOutput = passes::setupHiZBuildPass(
            *m_framegraph,
            m_device,
            prevDepthHandle,
            width,
            height,
            m_passStates->hiZBuild
        );
    }

    // Store Hi-Z pyramid handle for future GPU culling pass
    m_hizPyramid = hizOutput.pyramid;
    if (m_hizPyramid.is_valid())
        m_framegraph->GetRTRegistry().RegisterRT("rt_HiZ", m_hizPyramid);

    // Import previous frame normals for ReSTIR temporal validation
    framegraph::VirtualResourceHandle prevNormalsHandle;
    if (m_hasPrevFrameData && m_prevFrameNormals) {
        framegraph::ResourceDesc prevNormalsDesc;
        prevNormalsDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevNormalsDesc.debugName = "rt_PrevNormals";
        prevNormalsDesc.width = width;
        prevNormalsDesc.height = height;
        prevNormalsDesc.format = nvrhi::Format::RGBA16_FLOAT;
        prevNormalsDesc.isRenderTarget = true;
        prevNormalsDesc.isImported = true;
        prevNormalsDesc.isTransient = false;
        prevNormalsHandle = m_framegraph->ImportTexture("rt_PrevNormals", m_prevFrameNormals, prevNormalsDesc);
        m_framegraph->GetRTRegistry().RegisterRT("rt_PrevNormals", prevNormalsHandle);
    }

    framegraph::VirtualResourceHandle prevWorldPosHandle;
    if (m_hasPrevFrameData && m_prevFrameWorldPos) {
        framegraph::ResourceDesc prevWorldPosDesc;
        prevWorldPosDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        prevWorldPosDesc.debugName = "rt_PrevWorldPos";
        prevWorldPosDesc.width = width;
        prevWorldPosDesc.height = height;
        prevWorldPosDesc.format = nvrhi::Format::RGBA32_FLOAT;
        prevWorldPosDesc.isRenderTarget = true;
        prevWorldPosDesc.isImported = true;
        prevWorldPosDesc.isTransient = false;
        prevWorldPosHandle = m_framegraph->ImportTexture("rt_PrevWorldPos", m_prevFrameWorldPos, prevWorldPosDesc);
        m_framegraph->GetRTRegistry().RegisterRT("rt_PrevWorldPos", prevWorldPosHandle);
    }

    // ═══════════════════════════════════════════════════════
    //  PHASE 3.5: GPU CULLING PASS (Frustum + Occlusion)
    // ═══════════════════════════════════════════════════════
    // Uses Hi-Z pyramid to perform GPU-side occlusion culling.
    // Outputs draw args buffer for indirect draw in forward pass.
    //
    // PERFORMANCE:
    // - ~0.3-0.5ms for 100K objects (async compute capable)
    // - 10-100x faster than CPU culling for large scenes
    //
    // ARCHITECTURE:
    // - GPU culling pass writes to draw args buffer (UAV)
    // - Forward pass reads draw args buffer (IndirectArgument)
    // - FrameGraph ensures proper state transitions and execution order

    framegraph::VirtualResourceHandle drawArgsBuffer;  // Will be passed to forward pass

    if (m_gpuCullingManager && hizOutput.pyramid.is_valid()) {
        // Lazy initialization - ShaderLoader isn't ready during FrameGraphRenderer::Initialize
        m_gpuCullingManager->Initialize(m_device);

        if (m_detailManager) {
            static bool detailShadersLoaded = false;
            if (!detailShadersLoaded) {
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                m_detailManager->LoadCullComputeShader(shaderLoader);
                m_detailManager->LoadInstanceGenShader(shaderLoader);
                m_detailManager->LoadPrefixSumShaders(shaderLoader);
                m_detailManager->LoadGraphicsShaders(shaderLoader);

                // Create compute pipelines now that shaders are loaded
                if (!m_detailManager->computePipeline) {
                    m_detailManager->CreateComputePipeline(m_device);
                }
                if (!m_detailManager->instanceGenPipeline) {
                    m_detailManager->CreateInstanceGenPipeline(m_device);
                }
                if (!m_detailManager->prefixSumScanPipeline) {
                    m_detailManager->CreatePrefixSumPipeline(m_device);
                }

                // Initialize wind system (FBM wind texture + compute shader)
                if (!m_detailManager->windTexture) {
                    m_detailManager->CreateWindTexture(m_device->GetNVRHIDevice());
                }
                if (!m_detailManager->windComputeShader) {
                    m_detailManager->LoadWindComputeShader(shaderLoader);
                }
                if (!m_detailManager->windPipeline && m_detailManager->windComputeShader) {
                    m_detailManager->CreateWindPipeline(m_device->GetNVRHIDevice());
                }

                detailShadersLoaded = true;
            }
        }

        // NOTE: Bindless buffers initialized earlier in RenderFrame() before SetupFrame()
        // This ensures materials are ready when CollectVisibleGeometry() calls PreRegisterBindlessMaterial

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

        // Setup skinned mesh culling (uses same Hi-Z pyramid)
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
    // Renders sky dome geometry with cubemap textures
    // Creates the HDR color RT and fills it with sky
    // Must render BEFORE forward geometry (sky = background)

    // Create HDR color buffer for sky (will be reused by forward pass)
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
        m_passStates->sky
    );

    // ═══════════════════════════════════════════════════════
    //  SUN PASS (Sun disc with additive blending)
    // ═══════════════════════════════════════════════════════
    // Renders sun disc as camera-facing billboard
    // Uses additive blending on top of sky

    auto sunOutput = passes::setupSunPass(
        *m_framegraph,
        m_device,
        skyOutput,
        g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr,
        width,
        height,
        m_passStates->sun
    );

    // ═══════════════════════════════════════════════════════
    //  PHASE 1: FORWARD COLOR PASS (Single-RT, Reuses Depth)
    // ═══════════════════════════════════════════════════════
    // Simplified from wasteful 3-RT G-buffer to single HDR color output
    // BANDWIDTH SAVINGS: 60% reduction (3 RTs → 1 RT)
    // EARLY-Z OPTIMIZATION: Reuses depth from prepass (20-30% faster)
    // Reuses sky color RT (no clear - sky is background)

    // Configure bindless rendering if GPU culling manager has compaction data
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
        // If mega-buffers are ready, provide them for unified VB/IB rendering
        if (m_gpuCullingManager->AreMegaBuffersReady()) {
            bindlessConfig.megaVertexBuffer = m_gpuCullingManager->GetMegaVertexBuffer();
            bindlessConfig.megaIndexBuffer = m_gpuCullingManager->GetMegaIndexBuffer();
            bindlessConfig.megaBuffersReady = true;
        }

        // ═══════════════════════════════════════════════════════
        //  TERRAIN CONFIGURATION (4-layer detail blending)
        // ═══════════════════════════════════════════════════════
        // Terrain uses separate TerrainMaterialBuffer and terrain shader
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
        &m_passStates->forwardColor
    );

    // ═══════════════════════════════════════════════════════
    //  GPU CULLING DEBUG VISUALIZATION (Optional overlay)
    // ═══════════════════════════════════════════════════════
    // Renders colored bounding spheres showing culling state:
    // - Green: Visible (passed all tests)
    // - Blue: Occluder (close to camera)
    // - Red: Culled (distance/frustum)
    // - Yellow: Culled by Hi-Z occlusion
    // Enable with: r4_debug_gpu_culling 1

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

    // Static callback for skinned culling stats (can't use lambda with captures)
    static auto skinnedStatsCallback = +[](u32 rendered, u32 culled, void* userData) {
        static_cast<GPUCullingManager*>(userData)->UpdateSkinnedCullingStats(rendered, culled);
    };

    // Static callback for visual-based visibility lookup (handles batch reordering)
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
        &m_passStates->skinning,
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
        &m_passStates->detail
    );

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
    // Renders AFTER detail so grass depth is already written.
    // Depth test on, depth write off, SrcAlpha/InvSrcAlpha blend.
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
        m_passStates->transparent
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
                m_passStates->decal
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
            m_passStates->motionVector
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
        &m_passStates->particle
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
        &m_passStates->ribbon
    );

    auto sceneColor = ribbonOutputs.layout.albedo;

    if (particleOutputs.distortionRT.is_valid()) {
        framegraph::ResourceDesc snapDesc;
        snapDesc.type = framegraph::ResourceDesc::Type::Texture2D;
        snapDesc.width = width;
        snapDesc.height = height;
        snapDesc.format = nvrhi::Format::RGBA16_FLOAT;
        snapDesc.isRenderTarget = true;
        snapDesc.isTransient = true;
        snapDesc.isUAV = true;
        snapDesc.debugName = "rt_SceneSnapshot";
        auto snapshotHandle = m_framegraph->CreateTexture("rt_SceneSnapshot", snapDesc);

        framegraph::PassHandle copyPass = m_framegraph->AddPass("SceneSnapshotCopy");
        m_framegraph->PassRead(copyPass, sceneColor, framegraph::ResourceState::CopySource);
        m_framegraph->PassWrite(copyPass, snapshotHandle, framegraph::ResourceState::CopyDest);
        m_framegraph->SetPassCallback(copyPass,
            [sceneColor, snapshotHandle](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
                auto* src = fg.GetPhysicalTexture(sceneColor);
                auto* dst = fg.GetPhysicalTexture(snapshotHandle);
                if (src && dst)
                    ctx.GetCommandList()->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
            });

        sceneColor = passes::setupDistortionApplyPass(
            *m_framegraph, snapshotHandle, particleOutputs.distortionRT,
            particleOutputs.layout.worldPos, width, height);
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
            m_passStates->restirGI, m_hasPrevFrameData
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
        m_passStates->exposure
    );

    // Store exposure texture for future sky pass integration
    // (Sky pass will read exposure via s_tonemap.Load(int3(0,0,0)).x)
    m_exposureTexture = exposureOutput.exposureTexture;

    // 4. UI Pass - Renders 2D UI directly to scene HDR target with alpha blending
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
        m_passStates->uiText
    );

    // 5. Cursor Pass - Renders cursor on top of UI+Text
    sceneWithUI = passes::setupCursorPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height
    );

    // 6. Tonemap Pass - Convert HDR to LDR using ACES filmic tonemap
    // Now uses exposure from ExposurePass for auto-exposure
    // Renders directly to backbuffer if available (Frostbite pattern)
    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        sceneWithUI,
        exposureOutput.exposureTexture,
        backbufferHandle,
        width,
        height,
        m_passStates->tonemap,
        &m_passStates->exposure
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
                        static ref_cs s_debugPreviewCS;
                        static nvrhi::ComputePipelineHandle s_pipeline;
                        static nvrhi::BindingLayoutHandle s_layout;
                        static nvrhi::BufferHandle s_cb;
                        static bool s_init = false;

                        nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                        if (!s_init) {
                            s_debugPreviewCS.create("debug_preview");
                            if (!s_debugPreviewCS || !s_debugPreviewCS->nvrhiShader) return;

                            nvrhi::BindingLayoutDesc layoutDesc;
                            layoutDesc.visibility = nvrhi::ShaderType::Compute;
                            layoutDesc.bindings = {
                                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                                nvrhi::BindingLayoutItem::Texture_SRV(0),
                                nvrhi::BindingLayoutItem::Texture_UAV(0),
                            };
                            s_layout = nvDevice->createBindingLayout(layoutDesc);

                            nvrhi::ComputePipelineDesc pipeDesc;
                            pipeDesc.CS = s_debugPreviewCS->nvrhiShader;
                            pipeDesc.bindingLayouts = { s_layout };
                            s_pipeline = nvDevice->createComputePipeline(pipeDesc);

                            nvrhi::BufferDesc cbDesc;
                            cbDesc.debugName = "DebugPreviewCB";
                            cbDesc.byteSize = 32;
                            cbDesc.isConstantBuffer = true;
                            cbDesc.isVolatile = true;
                            cbDesc.maxVersions = 16;
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

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, s_cb),
                            nvrhi::BindingSetItem::Texture_SRV(0, srcTex),
                            nvrhi::BindingSetItem::Texture_UAV(0, dstTex),
                        };
                        auto bindings = nvDevice->createBindingSet(bindDesc, s_layout);
                        if (!bindings) return;

                        ctx->SetComputePipeline(s_pipeline.Get());
                        ctx->SetComputeBindingSet(0, bindings.Get());
                        ctx->Dispatch((512 + 7) / 8, (512 + 7) / 8, 1);
                    }
                );
            }
        }
    }

    // 7. ImGui Pass - Renders debug UI on top of LDR output (backbuffer)
    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,  // LDR input (backbuffer if available, else rt_Final)
        imguiRenderer,
        width,
        height
    );

    // Store final output for presentation (now points to backbuffer)
    m_finalOutput = finalOutput;

    // ═══════════════════════════════════════════════════════
    //  DEPTH COPY PASS (Temporal Hi-Z: save depth for next frame)
    // ═══════════════════════════════════════════════════════
    // Copy this frame's depth to persistent storage for next frame's Hi-Z build.
    // The framegraph handles all barriers and lifetime tracking automatically.
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

    // ═══════════════════════════════════════════════════════
    //  NORMAL COPY PASS (Save normals for next frame's ReSTIR temporal resampling)
    // ═══════════════════════════════════════════════════════
    {
        nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

        if (!m_prevFrameNormals || m_prevFrameWidth != width || m_prevFrameHeight != height) {
            nvrhi::TextureDesc desc;
            desc.width = width;
            desc.height = height;
            desc.format = nvrhi::Format::RGBA16_FLOAT;
            desc.isShaderResource = true;
            desc.debugName = "PrevFrameNormals";
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            m_prevFrameNormals = nvDevice->createTexture(desc);
        }

        if (m_prevFrameNormals) {
            framegraph::ResourceDesc importDesc;
            importDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            importDesc.debugName = "rt_PrevNormalsCopyDest";
            importDesc.width = width;
            importDesc.height = height;
            importDesc.format = nvrhi::Format::RGBA16_FLOAT;
            importDesc.isRenderTarget = true;
            importDesc.isImported = true;
            importDesc.isTransient = false;

            auto prevNormalsCopyDest = m_framegraph->ImportTexture("rt_PrevNormalsCopyDest", m_prevFrameNormals, importDesc);

            auto finalNormals = transparentOutputs.normal;
            framegraph::PassHandle normalsCopyPass = m_framegraph->AddPass("NormalsCopy");
            m_framegraph->PassRead(normalsCopyPass, finalNormals, framegraph::ResourceState::CopySource);
            m_framegraph->PassWrite(normalsCopyPass, prevNormalsCopyDest, framegraph::ResourceState::CopyDest);
            m_framegraph->SetPassCallback(normalsCopyPass,
                [finalNormals, prevNormalsCopyDest](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
                    nvrhi::ITexture* src = fg.GetPhysicalTexture(finalNormals);
                    nvrhi::ITexture* dst = fg.GetPhysicalTexture(prevNormalsCopyDest);
                    if (src && dst)
                        ctx.GetCommandList()->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
                }
            );
        }
    }

    // ═══════════════════════════════════════════════════════
    //  WORLD POS COPY PASS (Save world positions for next frame's ReSTIR temporal resampling)
    // ═══════════════════════════════════════════════════════
    {
        nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

        if (!m_prevFrameWorldPos || m_prevFrameWidth != width || m_prevFrameHeight != height) {
            nvrhi::TextureDesc desc;
            desc.width = width;
            desc.height = height;
            desc.format = nvrhi::Format::RGBA32_FLOAT;
            desc.isShaderResource = true;
            desc.debugName = "PrevFrameWorldPos";
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            m_prevFrameWorldPos = nvDevice->createTexture(desc);
        }

        if (m_prevFrameWorldPos) {
            framegraph::ResourceDesc importDesc;
            importDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            importDesc.debugName = "rt_PrevWorldPosCopyDest";
            importDesc.width = width;
            importDesc.height = height;
            importDesc.format = nvrhi::Format::RGBA32_FLOAT;
            importDesc.isRenderTarget = true;
            importDesc.isImported = true;
            importDesc.isTransient = false;

            auto prevWorldPosCopyDest = m_framegraph->ImportTexture("rt_PrevWorldPosCopyDest", m_prevFrameWorldPos, importDesc);

            auto finalWorldPos = transparentOutputs.worldPos;
            framegraph::PassHandle worldPosCopyPass = m_framegraph->AddPass("WorldPosCopy");
            m_framegraph->PassRead(worldPosCopyPass, finalWorldPos, framegraph::ResourceState::CopySource);
            m_framegraph->PassWrite(worldPosCopyPass, prevWorldPosCopyDest, framegraph::ResourceState::CopyDest);
            m_framegraph->SetPassCallback(worldPosCopyPass,
                [finalWorldPos, prevWorldPosCopyDest](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
                    nvrhi::ITexture* src = fg.GetPhysicalTexture(finalWorldPos);
                    nvrhi::ITexture* dst = fg.GetPhysicalTexture(prevWorldPosCopyDest);
                    if (src && dst)
                        ctx.GetCommandList()->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
                }
            );
        }
    }
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

// ═══════════════════════════════════════════════════════
//  HUD RENDERING
// ═══════════════════════════════════════════════════════
//
// HUD items use special rendering state:
// - psHUD_FOV projection matrix (wider FOV)
// - HUD_VIEWPORT_NEAR near plane (closer to camera)
// - Optional custom culling for left-handed mode
//
// Original engine: hud_transform_helper + mapHUD/mapHUDSorted/mapHUDEmissive

void FrameGraphRenderer::RenderHUD() {
    // HUD rendering is now handled by SkinningPass in the FrameGraph pipeline
    // This function is kept as a stub for future HUD-specific post-processing
    // (e.g., HUD-only effects, UI overlays, etc.)
}

// ═══════════════════════════════════════════════════════
//  VISIBILITY & CULLING
// ═══════════════════════════════════════════════════════

// Helper to process a visual and submit geometry batch (returns true if submitted)
bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable, bool isStatic) {
    if (!visual)
        return false;

    // Get mesh interface based on visual type
    // IRender_Mesh is not polymorphic, so we must cast to concrete types
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

        // ═══════════════════════════════════════════════════════
        //  PARTICLES - Collected separately for ParticlePass
        // ═══════════════════════════════════════════════════════
        case MT_PARTICLE_EFFECT:
        case MT_PARTICLE_GROUP:
            // Particles use a different rendering path (billboards/sprites)
            // They don't have traditional mesh geometry (vb/ib)
            // Collect them in separate particle batches for ParticlePass
            return ProcessParticleGeometry(visual, worldTransform, renderable, false);

        // MT_HIERRARHY, MT_SKELETON_ANIM, MT_SKELETON_RIGID, MT_LOD are containers, not leaf meshes
        // They should have been unpacked by ExtractStaticLeafVisuals
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

    // ═══════════════════════════════════════════════════════
    //  GET NVRHI BUFFER HANDLES
    // ═══════════════════════════════════════════════════════

    // geom->vb and geom->ib are already nvrhi::BufferHandle (created via NVRHI)
    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

    // Diagnostic logging for skeleton meshes - investigating "wrong mesh" issue
    u32 visualType = visual->getType();
    if (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM) {
        // Get RenderMode from skeleton
        u16 renderMode = 0;
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            renderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            renderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE GEOMETRY BATCH
    // ═══════════════════════════════════════════════════════

    GeometryBatch batch;

    // Store NVRHI buffer handles directly
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
    bool isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    if (isSkinned) {
        // Skinned meshes: indices are mesh-relative, ignore iBase
        if (visualType == MT_SKELETON_GEOMDEF_PM) {
            // Progressive skinned: use SWI for proper index range
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
            // Static skinned: startIndex = 0
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = 0;
        }
        batch.baseVertex = 0;  // Skinned meshes always have vBase = 0
    } else if (visualType == MT_PROGRESSIVE) {
        // Progressive meshes (terrain, water, etc.): use SWI for LOD 0 (max detail)
        // Vanilla: Render(vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris)
        const FSlideWindowItem& swi = static_cast<FProgressive*>(visual)->GetSWI();
        if (swi.sw && swi.count > 0) {
            const FSlideWindow& sw = swi.sw[0];  // LOD 0 = max detail
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;  // iBase + SW.offset
        } else {
            // Fallback if SWI not available
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else if (visualType == MT_TREE_PM) {
        // Progressive tree mesh: SWI is a pointer (loaded from global SWIs)
        // Vanilla: Render(vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris)
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
        // Non-progressive meshes (MT_NORMAL, MT_TREE_ST): use iBase/iCount directly
        batch.indexCount = meshVisual->iCount;
        batch.startIndex = meshVisual->iBase;
        batch.baseVertex = meshVisual->vBase;
    }
    batch.vertexStride = meshVisual->vStride;

    // Set world matrix (used for shader constants)
    batch.worldMatrix = worldTransform;

    // Store visual for material system
    batch.visual = visual;

    // Store renderable (for skeletons - provides bone data)
    batch.renderable = renderable;

    // Mark skinned meshes for separate rendering pipeline
    // Skinned meshes use bindless_skinned.vs/ps with per-draw bone matrices
    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
    batch.isStatic = isStatic;

    // Store skinning render mode for correct shader selection (1B, 2B, 3B, 4B)
    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    // Compute world-space bounding sphere for GPU culling
    // Different visual types have different sphere conventions:
    // - Static geometry: sphere already in world space, worldTransform = identity
    // - Trees: sphere already in world space (level compiler pre-transforms), but worldTransform != identity
    // - Dynamic objects: sphere in local space, needs worldTransform applied
    // Note: visualType already computed above for skeleton logging
    if (visualType == MT_TREE_ST || visualType == MT_TREE_PM) {
        // Trees: sphere is ALREADY in world space (pre-transformed by level compiler)
        // Do NOT apply worldTransform to sphere (it's only for shader constants)
        batch.worldBoundsCenter = visual->vis.sphere.P;
        batch.worldBoundsRadius = visual->vis.sphere.R;
    } else {
        // Static/dynamic geometry: transform sphere by worldMatrix
        worldTransform.transform_tiny(batch.worldBoundsCenter, visual->vis.sphere.P);
        batch.worldBoundsRadius = visual->vis.sphere.R;
    }

    // Calculate SSA (Screen Space Area) for sorting - matches vanilla CalcSSA()
    // SSA = R / distSQ - larger SSA = closer/bigger = render first (front-to-back)
    float distSQ = Device.vCameraPosition.distance_to_sqr(batch.worldBoundsCenter) + EPS;
    batch.ssa = batch.worldBoundsRadius / distSQ;

    // PSO and binding set will be created by MaterialCache in GBufferPass
    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    // Extract shader key for debug name (production-safe)
    RENDER_NAMESPACE::ShaderKey shaderKey;
    if (RENDER_NAMESPACE::ExtractShaderKey(visual, shaderKey)) {
        // Store as static string to avoid dangling pointer
        static thread_local std::string s_debugNameBuffer;
        s_debugNameBuffer = shaderKey.ToString();
        batch.debugName = s_debugNameBuffer.c_str();
    } else {
        batch.debugName = "<unknown_shader>";
    }

    // DEBUG: Verify buffers are valid before submit
    if (!nvrhiVB || !nvrhiIB) {
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p",
            nvrhiVB.Get(), nvrhiIB.Get());
        return false;
    }

    // Pre-register bindless material for GPU-driven rendering
    // This assigns material ID before GPU culling uploads the batch
    if (m_materialCache) {
        // Check if this is terrain (uses B_BmmD blender with 4-layer detail blending)
        if (m_materialCache->IsTerrainMaterial(visual)) {
            batch.isTerrain = true;
            batch.terrainMaterialID = m_materialCache->PreRegisterTerrainMaterial(visual);
        } else {
            batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  GPU-DRIVEN: Compute mega-buffer allocation
    // ═══════════════════════════════════════════════════════
    // Use mesh's pool IDs to get offsets into unified mega-buffers
    // CRITICAL: Use batch.startIndex/indexCount (adjusted for SWI) not meshVisual->iBase/iCount
    // For progressive meshes, batch.startIndex = iBase + sw.offset, batch.indexCount = sw.num_tris * 3
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

// ═══════════════════════════════════════════════════════
//  PROCESS HUD GEOMETRY (separate from world geometry)
// ═══════════════════════════════════════════════════════
// HUD geometry is rendered with:
// - Different projection matrix (psHUD_FOV instead of regular FOV)
// - Different near plane (HUD_VIEWPORT_NEAR instead of VIEWPORT_NEAR)
// - Separate depth buffer or depth range
// - Custom culling for left-handed mode
//
// Original engine uses: mapHUD, mapHUDSorted, mapHUDEmissive

bool FrameGraphRenderer::ProcessHudGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable) {
    if (!visual)
        return false;

    // Get mesh interface based on visual type (same logic as world geometry)
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
            // HUD particles need special FOV handling - collect them separately
            return ProcessParticleGeometry(visual, worldTransform, renderable, true);
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

    // geom->vb and geom->ib are already nvrhi::BufferHandle (created via NVRHI)
    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

    // Create HUD batch
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

    // Mark skinned meshes (HUD weapons/hands are typically skinned)
    u32 visualType = visual->getType();
    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    // Store skinning render mode for correct shader selection (1B, 2B, 3B, 4B)
    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    // Pre-register bindless material for HUD rendering
    if (m_materialCache) {
        batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
    }

    // Extract shader key for debug name
    RENDER_NAMESPACE::ShaderKey shaderKey;
    if (RENDER_NAMESPACE::ExtractShaderKey(visual, shaderKey)) {
        static thread_local std::string s_hudDebugNameBuffer;
        s_hudDebugNameBuffer = "HUD_" + shaderKey.ToString();
        batch.debugName = s_hudDebugNameBuffer.c_str();
    } else {
        batch.debugName = "<hud_unknown_shader>";
    }

    // Add to HUD batch list (NOT to world geometry collector)
    m_hudBatches.push_back(batch);
    return true;
}

// ═══════════════════════════════════════════════════════
//  PROCESS PARTICLE GEOMETRY (billboards/sprites)
// ═══════════════════════════════════════════════════════
// Particles use dynamic vertex buffers and billboard rendering.
// They don't have traditional static mesh geometry (vb/ib).
// Collected separately for ParticlePass to render.
//
// World particles: Normal rendering, depth test against world geometry
// HUD particles: Apply hud_transform_helper FOV adjustment
//
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

// Helper to recursively extract leaf visuals from static hierarchy
void FrameGraphRenderer::ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs) {
    if (!pVisual)
        return;

    switch (pVisual->Type) {
        case MT_HIERRARHY: {
            // Recursively process children
            FHierrarhyVisual* pV = static_cast<FHierrarhyVisual*>(pVisual);
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }
            break;
        }
        case MT_LOD: {
            // For now, just take all children (TODO: proper LOD selection)
            FLOD* pV = static_cast<FLOD*>(pVisual);
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }
            break;
        }
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            // Skeletons have children meshes (FHierrarhyVisual base)
            // CRITICAL: Must calculate bones BEFORE extracting children!
            // Bone matrices are needed by skinned mesh children for rendering
            CKinematics* pV = static_cast<CKinematics*>(pVisual);
            pV->CalculateBones_InvalidateFG();  // Compute bone transformation matrices
            pV->CalculateBonesFG(TRUE);  // Compute bone transformation matrices

            // Extract children - these are the actual renderable skinned meshes
            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }

            // Also check for LOD model
            //if (pV->m_lod) {
                //outLeafs.push_back(pV->m_lod);
            //}
            break;
        }
        case MT_SKELETON_GEOMDEF_PM:
        case MT_SKELETON_GEOMDEF_ST: {
            // These are skinned meshes - they ARE leaf visuals themselves
            // (CSkeletonX_PM/ST inherit from FProgressive/Fvisual + have mesh data)
            outLeafs.push_back(pVisual);
            break;
        }
        case MT_PROGRESSIVE: {
            // Progressive meshes are leaf visuals (e.g., water surfaces with LOD)
            outLeafs.push_back(pVisual);
            break;
        }
        case MT_PARTICLE_GROUP: {
            // Particle groups contain effects - recursively extract visuals
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
            // Particle effects are leaf visuals
            outLeafs.push_back(pVisual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
        case MT_NORMAL:
        default: {
            // Leaf visual - add to list
            outLeafs.push_back(pVisual);
            break;
        }
    }
}

void FrameGraphRenderer::CollectVisibleGeometry() {
    // ═══════════════════════════════════════════════════════
    //  STATIC GEOMETRY CACHING
    // ═══════════════════════════════════════════════════════
    // Static geometry never changes - collect once at first frame, cache forever
    // GPU culling handles visibility (frustum + Hi-Z), so we submit ALL static
    // This saves ~5-10ms CPU time per frame by avoiding per-frame collection

    if (!g_pGamePersistent)
        return;

    auto& dsgraph = RImplementation.get_imm_context();
    u32 submittedStatic = 0;

    // ═══════════════════════════════════════════════════════
    //  FIRST FRAME: Collect ALL static geometry from ALL sectors
    // ═══════════════════════════════════════════════════════
    if (!m_staticBatchesCached && !dsgraph.Sectors.empty()) {
        Msg("* [GeomCache] Building static geometry cache from %zu sectors...", dsgraph.Sectors.size());

        xr_vector<dxRender_Visual*> staticVisuals;
        xr_set<dxRender_Visual*> uniqueVisuals;

        // Collect from ALL sectors (not just portal-visible)
        // GPU culling will filter visibility - we just need all geometry uploaded
        for (CSector* sector : dsgraph.Sectors) {
            if (sector && sector->root()) {
                ExtractStaticLeafVisuals(sector->root(), staticVisuals);
            }
        }

        // Remove duplicates
        for (dxRender_Visual* v : staticVisuals) {
            uniqueVisuals.insert(v);
        }

        // Process each unique visual and cache the batches
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

        // Cache the static batches
        const auto& allBatches = m_geometryCollector->GetBatches();
        m_cachedStaticBatches.assign(allBatches.begin() + batchCountBefore, allBatches.end());
        m_staticBatchesCached = true;

        Msg("* [GeomCache] Cached %zu static batches from %zu unique visuals (total sectors: %zu)",
            m_cachedStaticBatches.size(), uniqueVisuals.size(), dsgraph.Sectors.size());
    }
    // ═══════════════════════════════════════════════════════
    //  SUBSEQUENT FRAMES: Just submit cached batches
    // ═══════════════════════════════════════════════════════
    else if (m_staticBatchesCached) {
        // Fast path: just submit cached static batches
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

        // Skip lights for now (we'll handle them later)
        if (data.type & STYPE_LIGHTSOURCE) {
            notRenderable++;
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

// ═══════════════════════════════════════════════════════
//  GAME OBJECT RENDERING CALLBACK INTEGRATION
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& xform) {
    // This method is called by game objects during their renderable_Render() callbacks
    // via CRender::add_Visual() -> FrameGraphRenderer::add_Visual()
    //
    // This allows game objects to add:
    // - Their main visual
    // - Attachments (weapons, items, equipment)
    // - HUD items (if renderable_HUD() is true)
    // - Any custom sub-objects
    //
    // This is CRITICAL for rendering NPCs, weapons, attachments, HUD items, etc.

    if (!V) {
        return;  // No visual to add
    }

    dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(V);
    if (!visual) {
        return;  // Not a valid visual type
    }

    // ═══════════════════════════════════════════════════════
    //  HUD FILTERING (original engine: r__dsgraph_build.cpp:83-96)
    // ═══════════════════════════════════════════════════════
    // HUD items are rendered in a separate pass with:
    // - Special projection matrix (psHUD_FOV)
    // - Different near plane (HUD_VIEWPORT_NEAR)
    // - Custom culling mode (for left-handed mode)
    //
    // Original code: if (root && root->renderable_HUD()) { add to mapHUD; return; }
    bool isHUD = (root && root->renderable_HUD());

    // Extract leaf visuals from the visual hierarchy
    // This handles:
    // - Hierarchical visuals (FHierrarhyVisual)
    // - Skinned meshes (CKinematics)
    // - LOD meshes (FLOD)
    // - Progressive meshes (FProgressive)
    xr_vector<dxRender_Visual*> leafVisuals;
    ExtractStaticLeafVisuals(visual, leafVisuals);

    // Route to appropriate processor based on HUD flag
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

// ═══════════════════════════════════════════════════════
//  DYNAMIC PASS ROUTING (Week 16)
// ═══════════════════════════════════════════════════════

xr_set<framegraph::RenderPhase> FrameGraphRenderer::ScanRequiredPhases() const {
    xr_set<framegraph::RenderPhase> phases;

    // Get batches and populate phase info
    auto& batches = const_cast<GeometryCollector*>(m_geometryCollector.get())->GetBatchesMutable();
    // Msg("! [FrameGraphRenderer] Scanning %u batches for required phases...", batches.size());

    // ═══════════════════════════════════════════════════════
    //  TRUE PHASE DETECTION (Week 16 - with ShaderPhaseCache)
    // ═══════════════════════════════════════════════════════
    //
    // Use ShaderPhaseCache to extract phase info from shader reflection
    // WITHOUT creating full PSOs. This works because:
    //
    // 1. ShaderPhaseCache only runs shader reflection (no RT dependencies)
    // 2. Phase is determined purely from shader outputs
    // 3. No physical textures needed - just bytecode analysis
    // 4. MaterialPSOs still created lazily during Execute() (after Compile)
    // ═══════════════════════════════════════════════════════

    // Track phase distribution
    xr_map<framegraph::RenderPhase, u32> phaseCount;

    for (auto& batch : batches) {
        // Skip batches without visual
        if (!batch.visual) {
            continue;
        }

        // Query shader phase cache (runs reflection if not cached)
        framegraph::RenderPhase phase = m_shaderPhaseCache->GetPhase(batch.visual);

        // Store phase in batch for routing
        batch.renderPhase = phase;

        // Track for statistics
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
            // For Geometry phase, we already have m_gbufferPass created in Initialize()
            // Just store a reference to it (not owned by m_activePasses)
            // We'll handle this specially in BuildFrameGraph() since we can't move m_gbufferPass
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

    // This code is unreachable for now, but will be used when we add more pass types
    // m_activePasses.push_back(std::move(entry));
}

void FrameGraphRenderer::CreateAllRequiredPasses() {
    // Scan materials to determine which phases are needed
    xr_set<framegraph::RenderPhase> requiredPhases = ScanRequiredPhases();

    // Create passes for each required phase
    for (framegraph::RenderPhase phase : requiredPhases) {
        CreatePhasePass(phase);
    }

    // Msg("! [FrameGraphRenderer] Created %u passes", m_activePasses.size());
}

void FrameGraphRenderer::RouteBatchesToPasses() {
    // Get all batches
    auto& batches = m_geometryCollector->GetBatchesMutable();

    // DEBUG: Check if batches have valid buffers before routing
    // u32 nullVBCount = 0, nullIBCount = 0;
    // for (const auto& batch : batches) {
    //     if (!batch.vertexBuffer) nullVBCount++;
    //     if (!batch.indexBuffer) nullIBCount++;
    // }
    // if (nullVBCount > 0 || nullIBCount > 0) {
    //     Msg("! [RouteBatches] BEFORE ROUTING: %u batches with null VB, %u with null IB (total %u)",
    //         nullVBCount, nullIBCount, batches.size());
    // } else {
    //     Msg("  [RouteBatches] All %u batches have valid buffers before routing", batches.size());
    // }

    // ═══════════════════════════════════════════════════════
    //  TRUE PHASE-BASED ROUTING (Week 16)
    // ═══════════════════════════════════════════════════════
    //
    // Use cached phase info from ScanRequiredPhases().
    // Each batch has a renderPhase field populated from ShaderPhaseCache.
    //
    // MaterialPSOs are still created lazily during Execute()
    // (after FrameGraph compilation).
    // ═══════════════════════════════════════════════════════

    // Group batches by phase
    xr_map<framegraph::RenderPhase, xr_vector<GeometryBatch*>> batchesByPhase;

    for (auto& batch : batches) {
        // Use cached phase from batch (populated in ScanRequiredPhases)
        batchesByPhase[batch.renderPhase].push_back(&batch);
    }

    // Assign batches to pass instances
    for (const auto& [phase, phaseBatches] : batchesByPhase) {
        const char* phaseName = framegraph::IPass::GetPhaseName(phase);

        switch (phase) {
            case framegraph::RenderPhase::Geometry:
                // DEPRECATED: Old class-based GBufferPass removed (using lambda-based pass now)
                // Batches are passed directly to setupGBufferPass() via GeometryCollector
                //m_gbufferPass->SetBatches(phaseBatches);
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

// ═══════════════════════════════════════════════════════
//  IMGUI RENDERING (inline, on final output)
// ═══════════════════════════════════════════════════════

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

    // Get the physical texture for m_finalOutput
    nvrhi::ITexture* finalTexture = m_framegraph->GetPhysicalTexture(m_finalOutput);
    if (!finalTexture) {
        Msg("! [FrameGraphRenderer] Failed to get final output texture for ImGui");
        return;
    }

    // Create framebuffer from final output
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(nvrhi::TextureHandle(finalTexture));

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [FrameGraphRenderer] Failed to create framebuffer for ImGui");
        return;
    }

    // Get main command list (already open from BeginFrame, will execute at EndFrame)
    nvrhi::ICommandList* cmdList = m_device->GetImmediateCommandList();
    if (!cmdList) {
        Msg("! [FrameGraphRenderer] No command list available for ImGui");
        return;
    }

    // Render ImGui onto final output
    // Commands are batched into main cmdlist, executed at EndFrame
    imguiRenderer->Render(drawData, framebuffer.Get(), cmdList);
}

} // namespace xray::render
