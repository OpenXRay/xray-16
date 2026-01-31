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
#include "FrameGraphPasses/DepthPrepassSetup.h"      // Phase 2: Depth prepass for early-Z
#include "FrameGraphPasses/HiZBuildPassSetup.h"      // Phase 3.5: Hi-Z pyramid for GPU culling
#include "FrameGraphPasses/ForwardColorPassSetup.h"  // Phase 1: Single-RT forward rendering + pipeline init
#include "GPUCullingManager.h"                       // Phase 3.5: GPU frustum/occlusion culling
#include "FGDetailManager.h"                         // Detail system (grass/vegetation)
#include "FrameGraphPasses/DetailPassSetup.h"        // Detail rendering pass
// SM6 bindless: Textures registered directly with D3D12Backend via RegisterBindlessTexture()
#include "Bindless/MaterialBuffer.h"                 // Bindless material buffer
#include "Bindless/TerrainMaterialBuffer.h"          // Terrain material buffer
#include "FrameGraphPasses/SkyPassSetup.h"           // Sky dome rendering
#include "FrameGraphPasses/SunPassSetup.h"           // Sun disc rendering
#include "FrameGraphPasses/SkinningPassSetup.h"
#include "FrameGraphPasses/ParticlePassSetup.h"      // Particle rendering (billboards/sprites)
#include "FrameGraphPasses/ExposurePassSetup.h"      // Auto-exposure from histogram
#include "FrameGraphPasses/UIPassSetup.h"
#include "FrameGraphPasses/TonemapPassSetup.h"       // Tonemap pass: HDR→LDR conversion
#include "FrameGraphPasses/ImGuiPassSetup.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrParticles/psystem.h"

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
    bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);  // Max 64K draws
    Msg("* [FrameGraphRenderer] Bindless material buffers initialized (early)");

    // Create GPU culling manager (Phase 3.5)
    // NOTE: Initialization is deferred to first frame (SetupFrameGraphPasses)
    // because ShaderLoader isn't ready during FrameGraphRenderer::Initialize
    m_gpuCullingManager = xr_make_unique<RENDER_NAMESPACE::GPUCullingManager>();

    // Create detail manager (Framegraph)
    // Note: Will be loaded during level loading (see r2_loader.cpp), not here
    m_detailManager = xr_make_unique<RENDER_NAMESPACE::FGDetailManager>();

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
    passes::InitializeSkyGeometry(device);
    passes::InitializeSunPass(device);
    passes::InitializeTonemapPass(device->GetNVRHIDevice());

    // ═══════════════════════════════════════════════════════
    //  PROFILER (GPU timing + ImGui overlay)
    // ═══════════════════════════════════════════════════════
    m_gpuProfiler = xr_make_unique<xray::profiler::GPUProfiler>();
    m_gpuProfiler->Initialize(device->GetNVRHIDevice());

    m_statsOverlay = xr_make_unique<xray::profiler::StatsOverlay>();
    m_statsOverlay->SetGPUProfiler(m_gpuProfiler.get());
    Msg("* [FrameGraphRenderer] Profiler initialized");

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

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    // Cleanup pass resources
    passes::ShutdownSkyGeometry();
    passes::ShutdownSunPass();
    passes::ShutdownTonemapPass();

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
        m_gpuProfiler->FrameStart();
    }

    // ═══════════════════════════════════════════════════════
    //  EAGER PIPELINE INITIALIZATION (First frame only)
    // ═══════════════════════════════════════════════════════
    // Initialize forward pass pipelines on first render when ShaderLoader is ready
    static bool s_pipelinesInitialized = false;
    if (!s_pipelinesInitialized && m_device) {
        passes::InitializeForwardPipelines(m_device);
        passes::InitializeSkinningPipelines(m_device);
        passes::InitializeParticlePipelines(m_device);
        // Note: Detail pipelines initialized later (after shader loading, see line ~622)
        s_pipelinesInitialized = true;
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

    // ═══════════════════════════════════════════════════════
    //  RT VISUALIZATION: View what GBufferPass is rendering
    // ═══════════════════════════════════════════════════════
    // For now, m_finalOutput is pointing to gbufferOutputs.albedo (prototype RT)
    // This should already show the GBuffer rendering if it's working
    // No additional copy needed

    // ═══════════════════════════════════════════════════════
    //  ALL RENDERING NOW HANDLED BY FRAMEGRAPH
    // ═══════════════════════════════════════════════════════
    // The FrameGraph Execute() above handles all passes:
    // - GBuffer, HUD, UI, Text, Cursor, Composite, ImGui
    // No more immediate mode rendering!

    // ═══════════════════════════════════════════════════════
    //  PRESENT TO BACKBUFFER
    // ═══════════════════════════════════════════════════════

    {
        ZoneScopedN("FG::Present");
        PresentToBackbuffer();
    }

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

    // GPU profiler frame start
    if (m_gpuProfiler)
        m_gpuProfiler->FrameStart();

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
    sceneWithUI = passes::setupTextPass(*m_framegraph, sceneWithUI, width, height);

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
        height
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
    m_framegraph->Compile();
    m_framegraph->Execute();

    // ═══════════════════════════════════════════════════════
    //  PRESENT TO BACKBUFFER
    // ═══════════════════════════════════════════════════════
    PresentToBackbuffer();

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
        m_statsOverlay->SetVisible(true);
        m_statsOverlay->Render();
    }
}

void FrameGraphRenderer::SetupFrame() {
    const bool levelLoaded = g_pGamePersistent && g_pGameLevel;
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

    // Store for later copy to persistent storage
    m_currentDepthHandle = depthBuffer;

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
            height
        );
    }

    // Store Hi-Z pyramid handle for future GPU culling pass
    m_hizPyramid = hizOutput.pyramid;

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

        // Initialize detail manager shaders (lazy - needs ShaderLoader)
        if (m_detailManager && m_detailManager->total_instance_count > 0) {
            static bool detailShadersLoaded = false;
            if (!detailShadersLoaded) {
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                m_detailManager->LoadCullComputeShader(shaderLoader);
                m_detailManager->LoadGraphicsShaders(shaderLoader);

                // Create compute pipeline now that shaders are loaded
                if (!m_detailManager->computePipeline) {
                    m_detailManager->CreateComputePipeline(m_device);
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

        if (m_gpuCullingManager->IsEnabled()) {
            // NOTE: UploadSceneObjects is now called inside the culling pass execute lambda
            // This ensures it uses the correct command list during framegraph execution

            // Setup GPU culling pass (upload happens inside execute lambda)
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
        height
    );

    // ═══════════════════════════════════════════════════════
    //  SUN PASS (Sun disc with additive blending)
    // ═══════════════════════════════════════════════════════
    // Renders sun disc as camera-facing billboard
    // Uses additive blending on top of sky

    auto sunOutput = passes::setupSunPass(
        *m_framegraph,
        m_device,
        skyOutput,  // Color buffer from sky pass
        g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr,
        width,
        height
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
    }

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,  // Reuse depth from prepass for early-Z
        sunOutput,    // Color buffer from sun pass (has sky + sun)
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height,
        drawArgsBuffer,  // Draw args from GPU culling (enables indirect draw if valid)
        bindlessConfig   // Bindless rendering configuration
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
    auto hudOutputs = passes::setupSkinningPass(
        *m_framegraph,
        m_device,
        forwardOutputs,
        m_geometryCollector.get(),  // World skinned batches from GeometryCollector
        &m_hudBatches,              // HUD skinned batches
        m_materialCache.get(),
        width,
        height
    );

    auto particleOutputs = passes::setupParticlePass(
        *m_framegraph,
        m_device,
        hudOutputs,
        &m_worldParticleBatches,
        &m_hudParticleBatches,
        m_materialCache.get(),
        width,
        height,
        hizOutput.pyramid,       // Hi-Z pyramid for GPU particle culling
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels
    );

    // ═══════════════════════════════════════════════════════
    //  DETAIL PASS (Grass/Vegetation)
    // ═══════════════════════════════════════════════════════
    // Renders detail objects (grass, small vegetation) using:
    // - GPU compute culling (frustum + Hi-Z occlusion)
    // - Single unified draw call via DrawIndexedIndirect
    // Renders after particles but before post-processing.

    auto detailOutputs = passes::setupDetailPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        particleOutputs,          // Render on top of particles
        width,
        height,
        hizOutput.pyramid,        // Hi-Z pyramid for GPU culling
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels,
        m_hasPrevFrameData ? &m_prevViewProj : nullptr  // Previous frame's viewProj for temporal Hi-Z
    );

    // ═══════════════════════════════════════════════════════
    //  EXPOSURE PASS (Auto-Exposure / Eye Adaptation)
    // ═══════════════════════════════════════════════════════
    // Computes scene exposure using histogram-based analysis.
    // Outputs 1×1 R32_FLOAT texture for sky and tonemap passes.
    // Uses temporal adaptation for smooth transitions.

    passes::ExposureConfig exposureConfig = passes::GetDefaultExposureConfig();
    auto exposureOutput = passes::setupExposurePass(
        *m_framegraph,
        m_device,
        detailOutputs.albedo,     // HDR scene color for histogram (after detail pass)
        exposureConfig,
        Device.fTimeDelta,       // Frame delta for temporal adaptation
        width,
        height
    );

    // Store exposure texture for future sky pass integration
    // (Sky pass will read exposure via s_tonemap.Load(int3(0,0,0)).x)
    m_exposureTexture = exposureOutput.exposureTexture;

    // 4. UI Pass - Renders 2D UI directly to scene HDR target with alpha blending
    auto sceneWithUI = passes::setupUIPass(
        *m_framegraph,
        detailOutputs.albedo,  // Scene + HUD + Particles + Details (HDR)
        width,
        height
    );

    // 4. Text Pass - Renders text on top of UI
    sceneWithUI = passes::setupTextPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height
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
        sceneWithUI,  // HDR input (RGBA16_FLOAT)
        exposureOutput.exposureTexture,  // Auto-exposure from histogram
        backbufferHandle,  // Output directly to imported backbuffer
        width,
        height
    );

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

    // OLD SYSTEM - DISABLED
    // All passes now use lambda pattern, no need for old routing
    // CreateAllRequiredPasses();  // REMOVED
    // RouteBatchesToPasses();     // REMOVED
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

void FrameGraphRenderer::PresentToBackbuffer() {
    // ═══════════════════════════════════════════════════════
    //  TEMPORAL HI-Z: Copy depth for next frame
    // ═══════════════════════════════════════════════════════
    // Copy this frame's depth to persistent storage for next frame's Hi-Z build.
    // This enables single-pass rendering without depth prepass.

    if (!m_device || !m_currentDepthHandle.is_valid())
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    // Get the physical depth texture from this frame
    nvrhi::ITexture* currentDepth = m_framegraph->GetPhysicalTexture(m_currentDepthHandle);
    if (!currentDepth)
        return;

    const nvrhi::TextureDesc& depthDesc = currentDepth->getDesc();
    u32 width = depthDesc.width;
    u32 height = depthDesc.height;

    // Create/recreate persistent depth texture if needed
    if (!m_prevFrameDepth || m_prevFrameWidth != width || m_prevFrameHeight != height) {
        nvrhi::TextureDesc prevDepthDesc;
        prevDepthDesc.width = width;
        prevDepthDesc.height = height;
        prevDepthDesc.format = nvrhi::Format::D32;
        prevDepthDesc.isRenderTarget = false;
        prevDepthDesc.isShaderResource = true;
        prevDepthDesc.debugName = "PrevFrameDepth";
        // Use keepInitialState so NVRHI assumes ShaderResource at start of each command list
        // This is needed because texture is used across frames/command lists
        prevDepthDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        prevDepthDesc.keepInitialState = true;

        m_prevFrameDepth = nvDevice->createTexture(prevDepthDesc);
        m_prevFrameWidth = width;
        m_prevFrameHeight = height;

        if (m_prevFrameDepth) {
            Msg("* [TemporalHiZ] Created persistent depth buffer: %dx%d", width, height);
        }
    }

    if (!m_prevFrameDepth)
        return;

    // Copy current depth to persistent storage
    nvrhi::CommandListHandle cmdList = nvDevice->createCommandList();
    cmdList->open();

    // Tell NVRHI the initial state of currentDepth (comes from framegraph after forward pass)
    cmdList->beginTrackingTextureState(currentDepth, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthWrite);
    // m_prevFrameDepth uses keepInitialState=true, so NVRHI assumes ShaderResource at start

    // Transition states for copy
    cmdList->setTextureState(currentDepth, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
    cmdList->setTextureState(m_prevFrameDepth, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
    cmdList->commitBarriers();

    // Copy entire texture
    cmdList->copyTexture(m_prevFrameDepth, nvrhi::TextureSlice(), currentDepth, nvrhi::TextureSlice());

    // Transition prev depth to shader resource for next frame's Hi-Z build
    cmdList->setTextureState(m_prevFrameDepth, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    cmdList->commitBarriers();

    cmdList->close();
    nvDevice->executeCommandList(cmdList);

    // Mark that we have valid previous frame data
    m_hasPrevFrameData = true;
    m_prevViewProj = Device.mFullTransform;
    m_prevCameraPos = Device.vCameraPosition;
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
bool FrameGraphRenderer::ProcessParticleGeometry(
    dxRender_Visual* visual,
    const Fmatrix& worldTransform,
    IRenderable* renderable,
    bool isHUD)
{
    if (!visual)
        return false;

    // Verify this is actually a particle system
    u32 vType = visual->getType();
    if (vType != MT_PARTICLE_EFFECT && vType != MT_PARTICLE_GROUP)
        return false;

    bool isHUDParticle = isHUD;
    u32 particleCount = 0;
    shared_str textureName;

    if (vType == MT_PARTICLE_EFFECT) {
        RENDER_NAMESPACE::PS::CParticleEffect* pEffect =
            static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(visual);
        isHUDParticle = pEffect->GetHudMode();

        PAPI::Particle* particles = nullptr;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

        // Get texture name from particle definition for bindless material
        auto* pDef = pEffect->GetDefinition();
        if (pDef) {
            textureName = pDef->m_TextureName;
        }
    }

    if (particleCount == 0)
        return false;

    passes::ParticleBatch batch;
    batch.visual = visual;
    batch.worldMatrix = worldTransform;
    batch.renderable = renderable;
    batch.isHUDMode = isHUDParticle;
    batch.particleCount = particleCount;

    // Register bindless material for particle texture
    if (m_materialCache && textureName.size()) {
        batch.bindlessMaterialID = m_materialCache->PreRegisterParticleMaterial(textureName);
    }

    if (isHUDParticle) {
        m_hudParticleBatches.push_back(batch);
    } else {
        m_worldParticleBatches.push_back(batch);
    }

    return true;
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
            pV->CalculateBones(TRUE);  // Compute bone transformation matrices

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
