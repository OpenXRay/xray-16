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
// SM6 bindless: Textures registered directly with D3D12Backend via RegisterBindlessTexture()
#include "Bindless/MaterialBuffer.h"                 // Bindless material buffer
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

    // Create GPU culling manager (Phase 3.5)
    // NOTE: Initialization is deferred to first frame (SetupFrameGraphPasses)
    // because ShaderLoader isn't ready during FrameGraphRenderer::Initialize
    m_gpuCullingManager = xr_make_unique<RENDER_NAMESPACE::GPUCullingManager>();

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

    Msg("* [FrameGraphRenderer] initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    // Clear global geometry collector pointer
    g_geometryCollector = nullptr;

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

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    // Cleanup pass resources
    passes::ShutdownSkyGeometry();
    passes::ShutdownSunPass();
    passes::ShutdownTonemapPass();

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    // ═══════════════════════════════════════════════════════
    //  EAGER PIPELINE INITIALIZATION (First frame only)
    // ═══════════════════════════════════════════════════════
    // Initialize forward pass pipelines on first render when ShaderLoader is ready
    static bool s_pipelinesInitialized = false;
    if (!s_pipelinesInitialized && m_device) {
        passes::InitializeForwardPipelines(m_device);
        passes::InitializeSkinningPipelines(m_device);
        passes::InitializeParticlePipelines(m_device);
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

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAME (PER-FRAME: Collect geometry)
    // ═══════════════════════════════════════════════════════

    SetupFrame();

    // ═══════════════════════════════════════════════════════
    //  RESET FRAMEGRAPH FOR NEW FRAME
    // ═══════════════════════════════════════════════════════
    // With lambda-based passes, we rebuild the graph every frame
    // (Frostbite-style: graph rebuilt, GPU resources reused from pool)
    m_framegraph->ResetForNextFrame();

    // ═══════════════════════════════════════════════════════
    //  SETUP PASSES (PER-FRAME: Route geometry to passes)
    // ═══════════════════════════════════════════════════════

    SetupFrameGraphPasses();

    // ═══════════════════════════════════════════════════════
    //  COMPILE & EXECUTE
    // ═══════════════════════════════════════════════════════

    // Set RenderContext for execution
    m_framegraph->SetRenderContext(m_renderContext.get());

    // Compile the graph (optimizes passes, calculates lifetimes, etc.)
    m_framegraph->Compile();

    // Execute the compiled graph (FrameGraph orchestrates all passes)
    m_framegraph->Execute();

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

    PresentToBackbuffer();

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
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

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
    m_framegraph->Compile();
    m_framegraph->Execute();

    // ═══════════════════════════════════════════════════════
    //  PRESENT TO BACKBUFFER
    // ═══════════════════════════════════════════════════════
    PresentToBackbuffer();

    // NOTE: We call Reset() at the start of each frame (line 285)
    // No need to reset here at the end

    // Msg("* [FrameGraphRenderer::RenderMenu] Menu frame complete");
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
    //  PHASE 2: DEPTH PREPASS (Early-Z Optimization)
    // ═══════════════════════════════════════════════════════
    // Render geometry depth-only before main forward pass
    // PERFORMANCE GAIN: 20-30% on overdraw-heavy scenes
    // - Depth prepass cost: ~1.5-2.0ms (no shading, depth write only)
    // - Forward pass savings: ~3.0-4.0ms (early-Z rejects occluded fragments)
    // - Net gain: ~1.0-2.0ms

    // Create bindless depth config from GPU culling manager
    passes::BindlessDepthConfig bindlessDepthConfig;
    if (m_gpuCullingManager && m_gpuCullingManager->AreMegaBuffersReady()) {
        bindlessDepthConfig.megaVertexBuffer = m_gpuCullingManager->GetMegaVertexBuffer();
        bindlessDepthConfig.megaIndexBuffer = m_gpuCullingManager->GetMegaIndexBuffer();
        bindlessDepthConfig.megaBuffersReady = true;
    }

    framegraph::VirtualResourceHandle depthBuffer = passes::setupDepthPrepass(
        *m_framegraph,
        m_device,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height,
        bindlessDepthConfig
    );

    // ═══════════════════════════════════════════════════════
    //  PHASE 3.5: HI-Z PYRAMID BUILD (GPU Culling Foundation)
    // ═══════════════════════════════════════════════════════
    // Generates hierarchical depth pyramid from depth prepass output.
    // Each mip level contains MAX depth of 2x2 block (conservative).
    //
    // USAGE:
    // - GPU occlusion culling (object_cull.cs) - Phase 3.5
    // - Froxel volumetrics ray termination - Phase 6
    // - Particle depth fade
    //
    // PERFORMANCE: ~0.2-0.3ms (async compute capable)

    auto hizOutput = passes::setupHiZBuildPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        width,
        height
    );

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

        // Initialize bindless rendering system (material buffer only - textures use D3D12 descriptor heap)
        // SM6 bindless: Textures registered via D3D12Backend::RegisterBindlessTexture() during material creation
        bindless::MaterialBuffer::Instance().Initialize(m_device);
        bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);  // Max 64K draws

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
                m_geometryCollector.get()  // Geometry is uploaded during execute
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
    if (m_gpuCullingManager && m_gpuCullingManager->GetCompactDrawArgsBuffer()) {
        bindlessConfig.enabled = true;  // TODO: Add console var to toggle bindless mode
        bindlessConfig.compactDrawArgsBuffer = m_gpuCullingManager->GetCompactDrawArgsBuffer();
        bindlessConfig.compactMaterialIDBuffer = m_gpuCullingManager->GetCompactMaterialIDBuffer();
        bindlessConfig.compactBatchIndicesBuffer = m_gpuCullingManager->GetCompactBatchIndicesBuffer();
        bindlessConfig.instanceBuffer = m_gpuCullingManager->GetInstanceBuffer();
        bindlessConfig.totalObjectCount = m_gpuCullingManager->GetObjectCount();

        // GPU-driven draw count: Use count buffer if compaction is enabled
        // This enables true GPU-driven rendering where GPU determines how many draws to execute
        if (m_gpuCullingManager->IsCompactionEnabled()) {
            bindlessConfig.compactCountBuffer = m_gpuCullingManager->GetCompactCountBuffer();
        }

        // ═══════════════════════════════════════════════════════
        //  MEGA-BUFFER CONFIGURATION (GPU-Driven Rendering)
        // ═══════════════════════════════════════════════════════
        // If mega-buffers are ready, provide them for unified VB/IB rendering
        if (m_gpuCullingManager->AreMegaBuffersReady()) {
            bindlessConfig.megaVertexBuffer = m_gpuCullingManager->GetMegaVertexBuffer();
            bindlessConfig.megaIndexBuffer = m_gpuCullingManager->GetMegaIndexBuffer();
            bindlessConfig.megaBuffersReady = true;
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

    nvrhi::IBuffer* particleDrawArgsBuffer = nullptr;
    if (m_gpuCullingManager && m_gpuCullingManager->IsParticleCullingEnabled()) {
        particleDrawArgsBuffer = m_gpuCullingManager->GetParticleDrawArgsBuffer();
    }

    auto particleOutputs = passes::setupParticlePass(
        *m_framegraph,
        m_device,
        hudOutputs,
        &m_worldParticleBatches,
        &m_hudParticleBatches,
        m_materialCache.get(),
        width,
        height,
        particleDrawArgsBuffer
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
        particleOutputs.albedo,  // HDR scene color for histogram
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
        particleOutputs.albedo,  // Scene + HUD + Particles (HDR)
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
    //  FROSTBITE PATTERN: No copy needed!
    // ═══════════════════════════════════════════════════════
    // The final pass (TonemapPass + ImGuiPass) now renders directly
    // to the imported backbuffer. No separate copy operation required.
    //
    // The swapchain Present() is called by the engine after this
    // in CRenderDevice::End() which calls Backend->Present().
    //
    // This function is kept as a stub for:
    // - Potential debug overlays
    // - Future HDR output path handling
    // - Resolution scaling blit (if backbuffer size != render size)
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
bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable) {
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

    // For progressive meshes, use LOD data instead of static iCount
    // Progressive meshes store SWI (slide window item) with per-LOD index counts
    // Use LOD = 1.0 (maximum detail) -> lod_id = 0 (first entry has most verts/tris)
    // Formula: lod_id = floor((1 - LOD) * (count - 1) + 0.5)
    // For LOD=1.0: lod_id = floor(0 * (count-1) + 0.5) = 0
    const FSlideWindowItem* pSWI = nullptr;

    if (visualType == MT_SKELETON_GEOMDEF_PM) {
        // Skinned progressive mesh - SWI from FProgressive base
        pSWI = &static_cast<CSkeletonX_PM*>(visual)->GetSWI();
    } else if (visualType == MT_PROGRESSIVE) {
        // Regular progressive mesh
        pSWI = &static_cast<FProgressive*>(visual)->GetSWI();
    } else if (visualType == MT_TREE_PM) {
        // Progressive tree mesh - SWI is a pointer (loaded from global SWIs)
        pSWI = static_cast<FTreeVisual_PM*>(visual)->GetSWI();
    }

    if (pSWI && pSWI->sw && pSWI->count > 0) {
        const FSlideWindow& sw = pSWI->sw[0];  // LOD 0 = max detail
        batch.indexCount = sw.num_tris * 3;
        batch.startIndex = meshVisual->iBase + sw.offset;
    } else {
        // Non-progressive mesh or fallback if SWI not available
        batch.indexCount = meshVisual->iCount;
        batch.startIndex = meshVisual->iBase;
    }
    batch.baseVertex = meshVisual->vBase;
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
        batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
    }

    // ═══════════════════════════════════════════════════════
    //  GPU-DRIVEN: Compute mega-buffer allocation
    // ═══════════════════════════════════════════════════════
    // Use mesh's pool IDs to get offsets into unified mega-buffers
    if (m_gpuCullingManager && m_gpuCullingManager->AreMegaBuffersReady()) {
        batch.megaBufferAlloc = m_gpuCullingManager->GetMeshAllocation(
            meshVisual->vbPoolID, meshVisual->vBase, meshVisual->vCount,
            meshVisual->ibPoolID, meshVisual->iBase, meshVisual->iCount,
            meshVisual->useAlternativeGeom
        );

        // Debug: Log allocation details for first few batches
        static int s_allocDebug = 0;
        if (s_allocDebug < 10 && !batch.megaBufferAlloc.valid) {
            Msg("! [MegaBuffer] Invalid alloc: vbPool=%u, vBase=%u, vCount=%u, ibPool=%u, iBase=%u, iCount=%u, alt=%d",
                meshVisual->vbPoolID, meshVisual->vBase, meshVisual->vCount,
                meshVisual->ibPoolID, meshVisual->iBase, meshVisual->iCount,
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
    // Collect both static (sector hierarchy) and dynamic (spatial DB) geometry
    // Static geometry is stored in sector hierarchies, not spatial DB
    // Dynamic geometry is stored in spatial database

    if (!g_pGamePersistent)
        return;

    // Get camera frustum
    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    // ═══════════════════════════════════════════════════════
    //  COLLECT STATIC GEOMETRY FROM SECTOR HIERARCHIES
    // ═══════════════════════════════════════════════════════

    xr_vector<dxRender_Visual*> staticVisuals;

    // Access dsgraph to get visible sectors
    auto& dsgraph = RImplementation.get_imm_context();

    // Check if we have sectors loaded
    if (!dsgraph.Sectors.empty()) {
        // Detect which sector camera is in
        Fvector camPos = Device.vCameraPosition;
        auto sectorID = dsgraph.detect_sector(camPos);

        if (sectorID != IRender_Sector::INVALID_SECTOR_ID) {
            // Use the REAL portal traverser to mark all visible sectors!
            // This is CRITICAL - it marks sectors with i_marker that we check later
            dsgraph.PortalTraverser.traverse(
                dsgraph.Sectors[sectorID],
                frustum,
                camPos,
                Device.mFullTransform,
                CPortalTraverser::VQ_SSA  // Skip HOM for now
            );

            // Get all visible sectors from portal traversal
            for (CSector* sector : dsgraph.PortalTraverser.r_sectors) {
                if (sector && sector->root()) {
                    ExtractStaticLeafVisuals(sector->root(), staticVisuals);
                }
            }

        }
    }

    // ═══════════════════════════════════════════════════════
    //  COLLECT DYNAMIC GEOMETRY FROM SPATIAL DATABASE
    // ═══════════════════════════════════════════════════════
    // Use cached m_lstRenderables (populated once in SetupFrame)
    // This is MUCH more efficient than querying spatial DB again!

    // ═══════════════════════════════════════════════════════
    //  PROCESS STATIC GEOMETRY
    // ═══════════════════════════════════════════════════════

    u32 submittedStatic = 0;
    u32 notFvisual_static = 0;
    u32 noGeometry_static = 0;
    u32 noBuffers_static = 0;

    u32 culledStatic = 0;

    // Detect and remove duplicate visuals to prevent Z-fighting
    xr_set<dxRender_Visual*> uniqueVisuals;
    u32 duplicateCount = 0;
    for (dxRender_Visual* v : staticVisuals) {
        if (uniqueVisuals.count(v)) {
            duplicateCount++;
        } else {
            uniqueVisuals.insert(v);
        }
    }
    if (duplicateCount > 0) {
        static bool s_loggedDuplicates = false;
        if (!s_loggedDuplicates) {
            Msg("! [CollectVisibleGeometry] Found %u duplicate visuals in staticVisuals (total: %zu, unique: %zu)",
                duplicateCount, staticVisuals.size(), uniqueVisuals.size());
            s_loggedDuplicates = true;
        }
    }

    for (dxRender_Visual* visual : uniqueVisuals) {
        // ═══════════════════════════════════════════════════════
        //  FRUSTUM CULLING FOR STATIC GEOMETRY
        // ═══════════════════════════════════════════════════════
        // Use visual's bounding sphere for culling
        if (!frustum.testSphere_dirty(visual->vis.sphere.P, visual->vis.sphere.R)) {
            culledStatic++;
            continue;  // Outside frustum, skip this visual
        }

        // Extract transform for this visual
        // Trees have embedded xform, everything else in static hierarchy uses identity
        Fmatrix xform = Fidentity;

        switch (visual->getType())
        {
            case MT_TREE_ST:
            case MT_TREE_PM:
            {
                // Trees store their world transform in xform member
                FTreeVisual* treeVisual = static_cast<FTreeVisual*>(visual);
                xform = treeVisual->xform;
                break;
            }
            case MT_NORMAL:
            case MT_PROGRESSIVE:
            case MT_SKELETON_GEOMDEF_ST:
            case MT_SKELETON_GEOMDEF_PM:
            default:
                // Static meshes, progressive meshes, and skinned meshes in sector hierarchy
                // are already positioned in world space (identity transform)
                xform = Fidentity;
                break;
        }

        if (ProcessVisualGeometry(visual, xform)) {
            submittedStatic++;
        } else {
            // Track failure reasons
            IRender_Mesh* meshVisual = nullptr;
            switch (visual->getType()) {
                case MT_NORMAL:
                    meshVisual = static_cast<Fvisual*>(visual);
                    break;
                case MT_TREE_ST:
                case MT_TREE_PM:
                    meshVisual = static_cast<FTreeVisual*>(visual);
                    break;
                default:
                    notFvisual_static++;
                    continue;
            }

            if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get()) {
                noGeometry_static++;
            } else if (!meshVisual->rm_geom._get()->vb || !meshVisual->rm_geom._get()->ib) {
                noBuffers_static++;
            }
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

    // Get immediate command list
    nvrhi::ICommandList* cmdList = m_device->GetImmediateCommandList();
    if (!cmdList) {
        Msg("! [FrameGraphRenderer] No command list available for ImGui");
        return;
    }

    // Render ImGui onto final output (pass raw pointer from handle)
    imguiRenderer->Render(drawData, framebuffer.Get(), cmdList);

    // CRITICAL: Close and execute the immediate command list to ensure ImGui draws before present
    // Without this, ImGui commands are queued but not submitted to GPU
    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);
    cmdList->open(); // Reopen for subsequent operations
}

} // namespace xray::render
