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
#include "Shader.h"
#include "r__dsgraph_structure.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/ShaderKey.h"
#include "xrEngine/CustomHUD.h"
#include "ImGuiRendererNVRHI.h"
#include "xrEngine/device.h"
#include <imgui.h>

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

    // Create passes with correct render target resolution (not screen resolution!)
    passes::GBufferPassConfig gbufferConfig;
    gbufferConfig.width = Device.dwWidth;   // Game render resolution (e.g., 1280x720)
    gbufferConfig.height = Device.dwHeight; // NOT screen resolution (1920x1080)!

    m_gbufferPass = xr_make_unique<passes::GBufferPass>(device, gbufferConfig);

    // Create HUD pass (renders HUD in world space with depth range)
    passes::HUDPassConfig hudConfig;
    hudConfig.width = Device.dwWidth;
    hudConfig.height = Device.dwHeight;
    m_hudPass = xr_make_unique<passes::HUDPass>(device, hudConfig);

    // Create particle pass (renders billboards/sprites for world+HUD particles)
    // Shares MaterialCache with GBufferPass for shader caching and render state extraction
    passes::ParticlePassConfig particleConfig;
    particleConfig.width = Device.dwWidth;
    particleConfig.height = Device.dwHeight;
    m_particlePass = xr_make_unique<passes::ParticlePass>(device, m_gbufferPass->GetMaterialCache(), particleConfig);

    m_lightingPass = xr_make_unique<passes::LightingPass>(device);
    m_tonemapPass = xr_make_unique<passes::TonemapPass>(device);

    // Create UI rendering passes (4-step pipeline - works for menu AND in-game)
    passes::UIPassConfig uiConfig;
    uiConfig.width = Device.dwWidth;
    uiConfig.height = Device.dwHeight;
    m_uiPass = xr_make_unique<passes::UIPass>(device, uiConfig);

    passes::TextPassConfig textConfig;
    textConfig.width = Device.dwWidth;
    textConfig.height = Device.dwHeight;
    m_textPass = xr_make_unique<passes::TextPass>(device, textConfig);

    passes::MenuDistortPassConfig menuDistortConfig;
    menuDistortConfig.width = Device.dwWidth;
    menuDistortConfig.height = Device.dwHeight;
    m_menuDistortPass = xr_make_unique<passes::MenuDistortPass>(device, menuDistortConfig);

    passes::MenuCompositePassConfig menuCompositeConfig;
    menuCompositeConfig.width = Device.dwWidth;
    menuCompositeConfig.height = Device.dwHeight;
    m_menuCompositePass = xr_make_unique<passes::MenuCompositePass>(device, menuCompositeConfig);

    // Create geometry collector
    m_geometryCollector = xr_make_unique<GeometryCollector>();

    // Set global geometry collector pointer
    g_geometryCollector = m_geometryCollector.get();

    // Create RenderContext for execution
    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD FRAMEGRAPH STRUCTURE (ONCE)
    // ═══════════════════════════════════════════════════════
    // Create all vanilla RTs and register them once at startup
    // Passes will be set up per-frame (dynamic routing in Week 16)

    BuildFrameGraphStructure();

    m_framegraph->Compile();

    // Register with Device render sequencer so OnRender() gets called
    // Priority 0 = middle of render sequence (after setup, before UI)
    Device.seqRender.Add(this, 0);

    Msg("  ✓ FrameGraphRenderer initialized and registered with seqRender");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    // Unregister from Device render sequencer
    Device.seqRender.Remove(this);

    // Clear global geometry collector pointer
    g_geometryCollector = nullptr;

    m_renderContext.reset();
    m_geometryCollector.reset();
    m_tonemapPass.reset();
    m_lightingPass.reset();
    m_particlePass.reset();
    m_hudPass.reset();
    m_gbufferPass.reset();
    m_menuCompositePass.reset();
    m_menuDistortPass.reset();
    m_textPass.reset();
    m_uiPass.reset();
    m_shaderPhaseCache.reset();
    m_framegraph.reset();

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    auto frameStart = std::chrono::high_resolution_clock::now();

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
    //  SETUP PASSES (PER-FRAME: Route geometry to passes)
    // ═══════════════════════════════════════════════════════

    SetupFrameGraphPasses();

    // ═══════════════════════════════════════════════════════
    //  EXECUTE
    // ═══════════════════════════════════════════════════════

    // Set RenderContext for execution
    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->Execute();

    // ═══════════════════════════════════════════════════════
    //  RT VISUALIZATION: View what GBufferPass is rendering
    // ═══════════════════════════════════════════════════════
    // For now, m_finalOutput is pointing to gbufferOutputs.albedo (prototype RT)
    // This should already show the GBuffer rendering if it's working
    // No additional copy needed - just log what we're presenting

    Msg("* [FrameGraphRenderer] Presenting GBuffer albedo to backbuffer");

    // ═══════════════════════════════════════════════════════
    //  RENDER HUD (after world geometry, before present)
    // ═══════════════════════════════════════════════════════

    RenderHUD();

    // ═══════════════════════════════════════════════════════
    //  RENDER IMGUI (inline after framegraph, before present)
    // ═══════════════════════════════════════════════════════
    // Render ImGui onto m_finalOutput before presenting to backbuffer
    // This ensures ImGui is composited onto the framegraph output

    // Generate ImGui draw data (normally done later in device.cpp, but we need it now)
    ImGui::Render();

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    if (imguiRenderer)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData)
        {
            RenderImGui(drawData, imguiRenderer);
        }
    }

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

    m_stats.gbufferMs = m_gbufferPass->GetGBufferStats().cpuTimeMs;
    m_stats.lightingMs = m_lightingPass->GetStats().cpuTimeMs;
    m_stats.tonemapMs = m_tonemapPass->GetStats().cpuTimeMs;
    m_stats.numDrawCalls = m_gbufferPass->GetGBufferStats().numDrawCalls;
    m_stats.numTriangles = m_gbufferPass->GetGBufferStats().numTriangles;

    // Reset per-frame state for next frame (keeps structure: RTs, passes, registry)
    m_framegraph->ResetForNextFrame();
}

// ═══════════════════════════════════════════════════════
//  RENDER MENU (Simplified for Main Menu)
// ═══════════════════════════════════════════════════════
// Main menu rendering: No 3D geometry, lighting, or post-process
// Just clear background + ImGui UI overlay

void FrameGraphRenderer::RenderMenu() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    Msg("* [FrameGraphRenderer::RenderMenu] Rendering main menu frame");

    // ═══════════════════════════════════════════════════════
    //  UPDATE RESOURCE MANAGER (Video textures, streaming)
    // ═══════════════════════════════════════════════════════

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    // ═══════════════════════════════════════════════════════
    //  EXECUTE 4-STEP UI PIPELINE
    // ═══════════════════════════════════════════════════════
    // Step 1: UIPass - Render UI sprites/widgets to rt_UIMain
    // Step 2: TextPass - Render text/fonts on top
    // Step 3: UIDistortPass - Render distortion mask to rt_UIDistort
    // Step 4: UICompositePass - Composite all layers to final output

    // Set RenderContext for execution
    m_framegraph->SetRenderContext(m_renderContext.get());

    // Execute all four passes in sequence
    m_uiPass->Execute(*m_renderContext, *m_framegraph);
    m_textPass->Execute(*m_renderContext, *m_framegraph);
    m_menuDistortPass->Execute(*m_renderContext, *m_framegraph);
    m_menuCompositePass->Execute(*m_renderContext, *m_framegraph);

    Msg("  [RenderMenu] 4-step UI pipeline complete (UI → Text → Distort → Composite)");

    // ═══════════════════════════════════════════════════════
    //  RENDER IMGUI (Menu UI overlay)
    // ═══════════════════════════════════════════════════════
    ImGui::Render();

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    if (imguiRenderer)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData)
        {
            RenderImGui(drawData, imguiRenderer);
        }
    }

    // ═══════════════════════════════════════════════════════
    //  PRESENT TO BACKBUFFER
    // ═══════════════════════════════════════════════════════
    PresentToBackbuffer();

    Msg("* [FrameGraphRenderer::RenderMenu] Menu frame complete");
}

void FrameGraphRenderer::SetupFrame() {
    // Clear buffer handle cache (X-Ray may recreate buffers each frame)
    m_bufferHandleCache.clear();

    // Clear cached spatial queries from previous frame
    m_lstRenderables.clear();

    // Query spatial database ONCE per frame (mimicking render_main::calculate())
    // This populates m_lstRenderables which we reuse throughout the frame
    if (g_pGamePersistent && g_pGameLevel)
    {
        // Setup frustum (same as render_main)
        // Safety check: Ensure spatial database is initialized
        // SpatialSpace isn't ready during early level loading
        if (g_pGameLevel && g_pGamePersistent && !g_pGamePersistent->IsLoadingScreenShown())
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
    CollectVisibleGeometry();

    // End geometry collection (sorts batches)
    m_geometryCollector->EndFrame();

    // Log HUD batch count
    if (!m_hudBatches.empty()) {
        Msg("! [FrameGraphRenderer] Collected %u HUD batches (will render in HUDPass)", (u32)m_hudBatches.size());
    }
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
//  BUILD FRAMEGRAPH STRUCTURE (CALLED ONCE IN INITIALIZE)
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::BuildFrameGraphStructure() {
    // ═══════════════════════════════════════════════════════
    //  CREATE ALL VANILLA X-RAY RENDER TARGETS (ONCE)
    // ═══════════════════════════════════════════════════════
    // Phase 1: Create as native NVRHI resources via NativeRTFactory
    // Then import into FrameGraph as external resources

    Msg("! [FrameGraphRenderer] Creating vanilla X-Ray render targets (native NVRHI)...");

    u32 w = Device.dwWidth;
    u32 h = Device.dwHeight;

    // Get NativeRTFactory from FGResourceManager
    auto* resMgr = m_device->GetFGResourceManager();
    VERIFY(resMgr);
    auto* rtFactory = resMgr->GetRTFactory();
    VERIFY(rtFactory);
    auto* textureMgr = resMgr->GetTextureManager();
    VERIFY(textureMgr);

    // ─── G-Buffer Targets (Deferred Geometry Phase) ───
    // Create native NVRHI textures
    m_native_Position = rtFactory->CreatePositionBuffer(w, h, "rt_Position");      // r2_RT_P
    m_native_Normal = rtFactory->CreateNormalBuffer(w, h, "rt_Normal");            // r2_RT_N
    m_native_Albedo = rtFactory->CreateAlbedoBuffer(w, h, false, "rt_Albedo");    // r2_RT_albedo (non-sRGB)
    m_native_Depth = rtFactory->CreateDepthStencil(w, h, false, "rt_Depth");      // r2_RT_base_depth (D24S8)

    // Get physical NVRHI textures from TextureManager
    nvrhi::ITexture* physicalPosition = textureMgr->GetNVRHITexture(m_native_Position);
    nvrhi::ITexture* physicalNormal = textureMgr->GetNVRHITexture(m_native_Normal);
    nvrhi::ITexture* physicalAlbedo = textureMgr->GetNVRHITexture(m_native_Albedo);
    nvrhi::ITexture* physicalDepth = textureMgr->GetNVRHITexture(m_native_Depth);

    VERIFY(physicalPosition && physicalNormal && physicalAlbedo && physicalDepth);

    // Import into FrameGraph as external resources
    framegraph::ResourceDesc positionDesc;
    positionDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    positionDesc.width = w;
    positionDesc.height = h;
    positionDesc.format = nvrhi::Format::RGBA16_FLOAT;
    positionDesc.isRenderTarget = true;
    positionDesc.debugName = "rt_Position";
    m_rt_Position = m_framegraph->ImportTexture("rt_Position", physicalPosition, positionDesc);

    framegraph::ResourceDesc normalDesc;
    normalDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    normalDesc.width = w;
    normalDesc.height = h;
    normalDesc.format = nvrhi::Format::RGBA16_FLOAT;
    normalDesc.isRenderTarget = true;
    normalDesc.debugName = "rt_Normal";
    m_rt_Normal = m_framegraph->ImportTexture("rt_Normal", physicalNormal, normalDesc);

    framegraph::ResourceDesc albedoDesc;
    albedoDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    albedoDesc.width = w;
    albedoDesc.height = h;
    albedoDesc.format = nvrhi::Format::RGBA8_UNORM;
    albedoDesc.isRenderTarget = true;
    albedoDesc.debugName = "rt_Albedo";
    m_rt_Albedo = m_framegraph->ImportTexture("rt_Albedo", physicalAlbedo, albedoDesc);

    framegraph::ResourceDesc depthDesc;
    depthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    depthDesc.width = w;
    depthDesc.height = h;
    depthDesc.format = nvrhi::Format::D24S8;
    depthDesc.isDepthStencil = true;
    depthDesc.debugName = "rt_Depth";
    m_rt_Depth = m_framegraph->ImportTexture("rt_Depth", physicalDepth, depthDesc);

    Msg("  ✓ Created 4 native G-Buffer RTs (Position, Normal, Albedo, Depth)");

    // ─── Menu Targets (native NVRHI for menu rendering pipeline) ───
    m_native_MenuMain = rtFactory->CreateAlbedoBuffer(w, h, false, "rt_MenuMain");      // Main UI RT (RGBA8)
    m_native_MenuDistort = rtFactory->CreateAlbedoBuffer(w, h, false, "rt_MenuDistort"); // Distortion mask RT (RGBA8)

    nvrhi::ITexture* physicalMenuMain = textureMgr->GetNVRHITexture(m_native_MenuMain);
    nvrhi::ITexture* physicalMenuDistort = textureMgr->GetNVRHITexture(m_native_MenuDistort);

    VERIFY(physicalMenuMain && physicalMenuDistort);

    // Import into FrameGraph as external resources
    framegraph::ResourceDesc menuMainDesc;
    menuMainDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    menuMainDesc.width = w;
    menuMainDesc.height = h;
    menuMainDesc.format = nvrhi::Format::RGBA8_UNORM;
    menuMainDesc.isRenderTarget = true;
    menuMainDesc.debugName = "rt_MenuMain";
    m_rt_MenuMain = m_framegraph->ImportTexture("rt_MenuMain", physicalMenuMain, menuMainDesc);

    framegraph::ResourceDesc menuDistortDesc;
    menuDistortDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    menuDistortDesc.width = w;
    menuDistortDesc.height = h;
    menuDistortDesc.format = nvrhi::Format::RGBA8_UNORM;
    menuDistortDesc.isRenderTarget = true;
    menuDistortDesc.debugName = "rt_MenuDistort";
    m_rt_MenuDistort = m_framegraph->ImportTexture("rt_MenuDistort", physicalMenuDistort, menuDistortDesc);

    Msg("  ✓ Created 2 native Menu RTs (MenuMain, MenuDistort)");

    // ─── Lighting Targets (still using legacy CreateRT for now) ───
    m_rt_Accumulator = CreateRT("rt_Accumulator", w, h, nvrhi::Format::RGBA16_FLOAT);  // r2_RT_accum

    // ─── Post-Processing Targets (still using legacy CreateRT for now) ───
    m_rt_Generic_0 = CreateRT("rt_Generic_0", w, h, nvrhi::Format::RGBA8_UNORM);       // r2_RT_generic0
    m_rt_Generic_1 = CreateRT("rt_Generic_1", w, h, nvrhi::Format::RGBA8_UNORM);       // r2_RT_generic1
    m_rt_Generic_2 = CreateRT("rt_Generic_2", w, h, nvrhi::Format::RGBA16_FLOAT);      // r2_RT_generic2 (HDR)

    m_backbuffer = CreateRT("Backbuffer", 1920, 1080, nvrhi::Format::RGBA8_UNORM);     // TODO: Get from Device

    Msg("  ✓ Created remaining legacy RTs (Accumulator, Generic0/1/2, Backbuffer)");
    Msg("  ✓ Total: 10 render targets (6 native: G-Buffer + Menu, 4 legacy)");

    // ═══════════════════════════════════════════════════════
    //  SETUP PASSES (ONCE) - PROTOTYPE FOR NOW
    // ═══════════════════════════════════════════════════════
    // NOTE: This creates prototype GBuffer pass with its own RTs
    // Week 15-16 will make this dynamic and use vanilla RTs

    m_gbufferPass->Setup(*m_framegraph);
    auto gbufferOutputs = m_gbufferPass->GetOutputs();

    // Setup HUD pass (always registered, even if no HUD batches yet)
    // HUD shares GBuffer outputs and MaterialCache (set per-frame in SetupFrameGraphPasses)
    m_hudPass->SetOutputs(gbufferOutputs);
    m_hudPass->SetMaterialCache(m_gbufferPass->GetMaterialCache());
    m_hudPass->Setup(*m_framegraph);

    // Setup Particle pass (always registered, renders after HUD)
    // Particles share GBuffer outputs, render both world and HUD particles
    m_particlePass->SetOutputs(gbufferOutputs);
    m_particlePass->Setup(*m_framegraph);

    // Setup UI rendering passes (4-step pipeline - works for menu AND in-game)
    // Step 1: Render UI sprites/widgets to rt_MenuMain (TODO: rename to rt_UIMain)
    m_uiPass->SetOutputs(m_rt_MenuMain, m_rt_Depth);
    m_uiPass->Setup(*m_framegraph);

    // Step 2: Render text/fonts on top of UI (same RT)
    m_textPass->SetOutputs(m_rt_MenuMain, m_rt_Depth);
    m_textPass->Setup(*m_framegraph);

    // Step 3: Render distortion mask to rt_MenuDistort
    m_menuDistortPass->SetOutputs(m_rt_MenuDistort, m_rt_Depth);
    m_menuDistortPass->Setup(*m_framegraph);

    // Step 4: Composite rt_MenuMain + rt_MenuDistort to final output
    m_menuCompositePass->SetInputs(m_rt_MenuMain, m_rt_MenuDistort);
    m_menuCompositePass->SetOutput(gbufferOutputs.albedo);  // Use GBuffer albedo as temp final output
    m_menuCompositePass->Setup(*m_framegraph);

    // ═══════════════════════════════════════════════════════
    //  REGISTER RENDER TARGETS IN REGISTRY (Week 14)
    // ═══════════════════════════════════════════════════════

    Msg("! [FrameGraphRenderer] Registering vanilla render targets in RT registry...");
    auto& registry = m_framegraph->GetRTRegistry();

    // ─── VANILLA G-BUFFER TARGETS ───

    // rt_Position (r2_RT_P) - World space position
    registry.RegisterRT("rt_Position", m_rt_Position);
    registry.RegisterAliases(m_rt_Position, {
        "$user$position",  // Shader output target name
        "s_position"       // Shader input sampler name
    });

    // rt_Normal (r2_RT_N) - View space normal
    registry.RegisterRT("rt_Normal", m_rt_Normal);
    registry.RegisterAliases(m_rt_Normal, {
        "s_normal",
        "s_nmap"
    });

    // rt_Albedo (r2_RT_albedo) - Diffuse color
    registry.RegisterRT("rt_Albedo", m_rt_Albedo);
    registry.RegisterRT("rt_Color", m_rt_Albedo);  // Legacy alias
    registry.RegisterAliases(m_rt_Albedo, {
        "$user$albedo",    // Shader output target name
        "s_albedo",        // Shader input sampler name
        "s_diffuse",
        "s_image",
        "s_base"
    });

    // rt_Depth (r2_RT_base_depth) - Depth/stencil
    registry.RegisterRT("rt_Depth", m_rt_Depth);
    registry.RegisterAliases(m_rt_Depth, {
        "$user$base_depth",  // Shader output target name
        "s_depth"            // Shader input sampler name
    });

    // ─── VANILLA LIGHTING TARGETS ───

    // rt_Accumulator (r2_RT_accum) - HDR lighting accumulation
    registry.RegisterRT("rt_Accumulator", m_rt_Accumulator);
    registry.RegisterAliases(m_rt_Accumulator, {
        "s_accumulator",
        "s_acc"
    });

    // ─── VANILLA POST-PROCESSING TARGETS ───

    // rt_Generic_0/1/2 (r2_RT_generic0/1/2) - Generic targets for combine/post-processing
    registry.RegisterRT("rt_Generic_0", m_rt_Generic_0);
    registry.RegisterAliases(m_rt_Generic_0, {
        "$user$generic0",  // Shader output target name
        "s_generic0"       // Shader input sampler name
    });

    registry.RegisterRT("rt_Generic_1", m_rt_Generic_1);
    registry.RegisterAliases(m_rt_Generic_1, {
        "$user$generic1",  // Shader output target name
        "s_generic1"       // Shader input sampler name
    });

    registry.RegisterRT("rt_Generic_2", m_rt_Generic_2);
    registry.RegisterAliases(m_rt_Generic_2, {
        "s_generic2"
    });

    // ─── MENU TARGETS ───

    // rt_MenuMain - Main menu UI rendering target
    registry.RegisterRT("rt_MenuMain", m_rt_MenuMain);
    registry.RegisterAliases(m_rt_MenuMain, {
        "$user$menu_main",  // Shader output target name
        "s_menu_main"       // Shader input sampler name
    });

    // rt_MenuDistort - Menu distortion/post-process mask
    registry.RegisterRT("rt_MenuDistort", m_rt_MenuDistort);
    registry.RegisterAliases(m_rt_MenuDistort, {
        "$user$menu_distort",  // Shader output target name
        "s_menu_distort"       // Shader input sampler name
    });

    // ─── BACKBUFFER ───

    registry.RegisterRT("rt_Backbuffer", m_backbuffer);
    registry.RegisterAliases(m_backbuffer, {
        "s_backbuffer",
        "s_screen"
    });

    // ─── PROTOTYPE GBUFFER (TEMPORARY - Week 15-16 will remove) ───

    // Keep prototype GBufferPass outputs registered for now (used by current test rendering)
    registry.RegisterRT("GBuffer.Albedo", gbufferOutputs.albedo);
    registry.RegisterRT("GBuffer.Normal", gbufferOutputs.normal);
    registry.RegisterRT("GBuffer.Material", gbufferOutputs.material);
    registry.RegisterRT("GBuffer.Depth", gbufferOutputs.depth);

    // Print registry for debugging
    registry.PrintRegistry();

    // Store final output for presenting to backbuffer (use prototype for now)
    m_finalOutput = gbufferOutputs.albedo;
}

// ═══════════════════════════════════════════════════════
//  SETUP FRAMEGRAPH PASSES (CALLED PER-FRAME)
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::SetupFrameGraphPasses() {
    // ═══════════════════════════════════════════════════════
    //  DYNAMIC PASS ROUTING (Week 16)
    // ═══════════════════════════════════════════════════════
    // Scan materials and route batches to appropriate passes

    // Scan materials and create required passes
    CreateAllRequiredPasses();

    // Route batches to appropriate passes
    RouteBatchesToPasses();

    // ═══════════════════════════════════════════════════════
    //  HUD PASS SETUP (PER-FRAME)
    // ═══════════════════════════════════════════════════════
    // HUD pass is always registered (in BuildFrameGraphStructure)
    // Just set batches here - pass will skip execution if empty

    m_hudPass->SetHUDBatches(&m_hudBatches);

    // ═══════════════════════════════════════════════════════
    //  PARTICLE PASS SETUP (PER-FRAME)
    // ═══════════════════════════════════════════════════════
    // Particle pass is always registered (in BuildFrameGraphStructure)
    // Set both world and HUD particle batches - pass will skip execution if both empty

    m_particlePass->SetWorldParticleBatches(&m_worldParticleBatches);
    m_particlePass->SetHUDParticleBatches(&m_hudParticleBatches);

    // TODO: Week 15-16 will add dynamic pass creation here
    // For now, we just route to existing GBufferPass
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
    VERIFY(m_device != nullptr);
    VERIFY(m_framegraph != nullptr);

    // Get the physical texture from FrameGraph
    nvrhi::ITexture* finalTexture = m_framegraph->GetPhysicalTexture(m_finalOutput);
    if (!finalTexture) {
        Msg("! [FrameGraphRenderer] Failed to get final output texture for present");
        return;
    }

    // Get game backbuffer from RenderTarget system
    ID3D11RenderTargetView* backbufferRTV = RImplementation.Target->get_base_rt();
    if (!backbufferRTV) {
        Msg("! [FrameGraphRenderer] Failed to get game backbuffer RTV");
        return;
    }

    // Extract D3D11 texture from RTV
    ID3D11Resource* d3dBackbufferResource = nullptr;
    backbufferRTV->GetResource(&d3dBackbufferResource);
    if (!d3dBackbufferResource) {
        Msg("! [FrameGraphRenderer] Failed to get backbuffer resource");
        return;
    }

    ID3D11Texture2D* d3dBackbufferTex = nullptr;
    HRESULT hr = d3dBackbufferResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&d3dBackbufferTex);
    d3dBackbufferResource->Release();

    if (FAILED(hr) || !d3dBackbufferTex) {
        Msg("! [FrameGraphRenderer] Backbuffer is not a 2D texture");
        return;
    }

    // Wrap D3D11 texture as NVRHI texture
    nvrhi::TextureDesc backbufferDesc;
    D3D11_TEXTURE2D_DESC d3dDesc;
    d3dBackbufferTex->GetDesc(&d3dDesc);

    backbufferDesc.width = d3dDesc.Width;
    backbufferDesc.height = d3dDesc.Height;
    backbufferDesc.format = nvrhi::Format::RGBA8_UNORM; // Assume RGBA8 for backbuffer
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.debugName = "GameBackbuffer";

    nvrhi::TextureHandle backbufferHandle = m_device->GetNativeDevice()->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D11_Resource,
        nvrhi::Object(d3dBackbufferTex),
        backbufferDesc
    );
    d3dBackbufferTex->Release();

    if (!backbufferHandle) {
        Msg("! [FrameGraphRenderer] Failed to wrap backbuffer as NVRHI texture");
        return;
    }

    // Copy final output to backbuffer using NVRHI command list
    ng::RenderContext* ctx = m_renderContext.get();
    VERIFY(ctx != nullptr);

    // Copy texture (full mip 0 to mip 0)
    ctx->CopyTexture(
        backbufferHandle.Get(),  // dest
        finalTexture            // src
    );

    Msg("  [FrameGraphRenderer] Presented final output to game backbuffer (%ux%u)",
        d3dDesc.Width, d3dDesc.Height);
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
    // HUD rendering is now handled by HUDPass in the FrameGraph pipeline
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
    //  WRAP D3D11 BUFFERS AS NVRHI HANDLES
    // ═══════════════════════════════════════════════════════

    // Wrap buffers with caching - geom->vb and geom->ib are SHARED buffers used by many meshes!
    // Check cache first to avoid creating duplicate NVRHI handles
    ID3D11Buffer* d3dVB = geom->vb;
    ID3D11Buffer* d3dIB = geom->ib;

    //Msg("! [ProcessVisualGeometry] Visual '%s': D3D VB=%p, IB=%p, vBase=%d, iBase=%d, vCount=%d, iCount=%d",
    //    visual->dbg_name.c_str(), d3dVB, d3dIB,
    //    meshVisual->vBase, meshVisual->iBase, meshVisual->vCount, meshVisual->iCount);

    // Get or create vertex buffer handle
    nvrhi::BufferHandle nvrhiVB;
    auto vbIt = m_bufferHandleCache.find(d3dVB);
    if (vbIt != m_bufferHandleCache.end()) {
        nvrhiVB = vbIt->second;
    } else {
        // First time seeing this buffer - wrap it
        D3D11_BUFFER_DESC d3dVBDesc;
        d3dVB->GetDesc(&d3dVBDesc);

        nvrhi::BufferDesc vbDesc;
        vbDesc.debugName = "Shared_VB";
        vbDesc.byteSize = d3dVBDesc.ByteWidth;
        vbDesc.isVertexBuffer = true;
        vbDesc.keepInitialState = true;
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

        nvrhiVB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
            nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(d3dVB), vbDesc);

        if (!nvrhiVB)
            return false;

        m_bufferHandleCache[d3dVB] = nvrhiVB;
    }

    // Get or create index buffer handle
    nvrhi::BufferHandle nvrhiIB;
    auto ibIt = m_bufferHandleCache.find(d3dIB);
    if (ibIt != m_bufferHandleCache.end()) {
        nvrhiIB = ibIt->second;
    } else {
        // First time seeing this buffer - wrap it
        D3D11_BUFFER_DESC d3dIBDesc;
        d3dIB->GetDesc(&d3dIBDesc);

        nvrhi::BufferDesc ibDesc;
        ibDesc.debugName = "Shared_IB";
        ibDesc.byteSize = d3dIBDesc.ByteWidth;
        ibDesc.isIndexBuffer = true;
        ibDesc.keepInitialState = true;
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

        nvrhiIB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
            nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(d3dIB), ibDesc);

        if (!nvrhiIB)
            return false;

        m_bufferHandleCache[d3dIB] = nvrhiIB;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE GEOMETRY BATCH
    // ═══════════════════════════════════════════════════════

    GeometryBatch batch;

    // Store NVRHI buffer handles directly
    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;

    batch.indexCount = meshVisual->iCount;
    batch.startIndex = meshVisual->iBase;
    batch.baseVertex = meshVisual->vBase;

    // Set world matrix
    batch.worldMatrix = worldTransform;

    // Store visual for material system
    batch.visual = visual;

    // Store renderable (for skeletons - provides bone data)
    batch.renderable = renderable;

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
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p, D3DVB=%p, D3DIB=%p",
            nvrhiVB.Get(), nvrhiIB.Get(), d3dVB, d3dIB);
        return false;
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

    // Wrap D3D11 buffers (same caching logic as world geometry)
    ID3D11Buffer* d3dVB = geom->vb;
    ID3D11Buffer* d3dIB = geom->ib;

    // Get or create vertex buffer handle
    nvrhi::BufferHandle nvrhiVB;
    auto vbIt = m_bufferHandleCache.find(d3dVB);
    if (vbIt != m_bufferHandleCache.end()) {
        nvrhiVB = vbIt->second;
    } else {
        D3D11_BUFFER_DESC d3dVBDesc;
        d3dVB->GetDesc(&d3dVBDesc);

        nvrhi::BufferDesc vbDesc;
        vbDesc.debugName = "HUD_VB";
        vbDesc.byteSize = d3dVBDesc.ByteWidth;
        vbDesc.isVertexBuffer = true;
        vbDesc.keepInitialState = true;
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

        nvrhiVB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
            nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(d3dVB), vbDesc);

        if (!nvrhiVB)
            return false;

        m_bufferHandleCache[d3dVB] = nvrhiVB;
    }

    // Get or create index buffer handle
    nvrhi::BufferHandle nvrhiIB;
    auto ibIt = m_bufferHandleCache.find(d3dIB);
    if (ibIt != m_bufferHandleCache.end()) {
        nvrhiIB = ibIt->second;
    } else {
        D3D11_BUFFER_DESC d3dIBDesc;
        d3dIB->GetDesc(&d3dIBDesc);

        nvrhi::BufferDesc ibDesc;
        ibDesc.debugName = "HUD_IB";
        ibDesc.byteSize = d3dIBDesc.ByteWidth;
        ibDesc.isIndexBuffer = true;
        ibDesc.keepInitialState = true;
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

        nvrhiIB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
            nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(d3dIB), ibDesc);

        if (!nvrhiIB)
            return false;

        m_bufferHandleCache[d3dIB] = nvrhiIB;
    }

    // Create HUD batch
    GeometryBatch batch;
    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;
    batch.indexCount = meshVisual->iCount;
    batch.startIndex = meshVisual->iBase;
    batch.baseVertex = meshVisual->vBase;
    batch.worldMatrix = worldTransform;
    batch.visual = visual;
    batch.renderable = renderable;
    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

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

    // Check if this particle has HUD mode flag set (for CParticleEffect)
    bool isHUDParticle = isHUD;
    if (vType == MT_PARTICLE_EFFECT) {
        RENDER_NAMESPACE::PS::CParticleEffect* pEffect =
            static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(visual);
        // Override with actual HUD mode from particle
        isHUDParticle = pEffect->GetHudMode();
    }

    // Create particle batch
    passes::ParticleBatch batch;
    batch.visual = visual;
    batch.worldMatrix = worldTransform;
    batch.renderable = renderable;
    batch.isHUDMode = isHUDParticle;

    // Add to appropriate list (world or HUD)
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

    for (dxRender_Visual* visual : staticVisuals) {
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
    Msg("! [FrameGraphRenderer] Scanning %u batches for required phases...", batches.size());

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

    // Log phase distribution
    Msg("! [FrameGraphRenderer] Found %u required phases:", phases.size());
    for (const auto& [phase, count] : phaseCount) {
        const char* phaseName = framegraph::IPass::GetPhaseName(phase);
        Msg("!   %s: %u batches", phaseName, count);
    }

    // Log cache statistics
    const auto& cacheStats = m_shaderPhaseCache->GetStats();
    Msg("! [ShaderPhaseCache] Stats: %u cached, %u hits, %u misses",
        cacheStats.numCached, cacheStats.numHits, cacheStats.numMisses);

    return phases;
}

void FrameGraphRenderer::CreatePhasePass(framegraph::RenderPhase phase) {
    const char* phaseName = framegraph::IPass::GetPhaseName(phase);
    Msg("! [FrameGraphRenderer] Creating pass for phase: %s", phaseName);

    PassEntry entry;
    entry.phase = phase;

    switch (phase) {
        case framegraph::RenderPhase::Geometry: {
            // For Geometry phase, we already have m_gbufferPass created in Initialize()
            // Just store a reference to it (not owned by m_activePasses)
            Msg("!   Using existing GBufferPass instance");
            // We'll handle this specially in BuildFrameGraph() since we can't move m_gbufferPass
            return;
        }

        case framegraph::RenderPhase::Lighting:
        case framegraph::RenderPhase::PostProcess:
        case framegraph::RenderPhase::Combine:
        case framegraph::RenderPhase::Shadow:
        case framegraph::RenderPhase::Custom:
        default:
            Msg("!   ⚠️ Unsupported phase: %s (not yet implemented)", phaseName);
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

    Msg("! [FrameGraphRenderer] Created %u passes", m_activePasses.size());
}

void FrameGraphRenderer::RouteBatchesToPasses() {
    Msg("! [FrameGraphRenderer] Routing batches to passes...");

    // Get all batches
    auto& batches = m_geometryCollector->GetBatchesMutable();

    // DEBUG: Check if batches have valid buffers before routing
    u32 nullVBCount = 0, nullIBCount = 0;
    for (const auto& batch : batches) {
        if (!batch.vertexBuffer) nullVBCount++;
        if (!batch.indexBuffer) nullIBCount++;
    }
    if (nullVBCount > 0 || nullIBCount > 0) {
        Msg("! [RouteBatches] BEFORE ROUTING: %u batches with null VB, %u with null IB (total %u)",
            nullVBCount, nullIBCount, batches.size());
    } else {
        Msg("  [RouteBatches] All %u batches have valid buffers before routing", batches.size());
    }

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
                // Assign to GBufferPass
                m_gbufferPass->SetBatches(phaseBatches);
                Msg("!   %s: %u batches assigned to GBufferPass",
                    phaseName, phaseBatches.size());
                break;

            case framegraph::RenderPhase::Lighting:
            case framegraph::RenderPhase::PostProcess:
            case framegraph::RenderPhase::Combine:
            case framegraph::RenderPhase::Shadow:
            case framegraph::RenderPhase::Custom:
            default:
                Msg("!   %s: %u batches (no pass implemented yet)",
                    phaseName, phaseBatches.size());
                break;
        }
    }

    Msg("! [FrameGraphRenderer] Batch routing complete");
}

// ═══════════════════════════════════════════════════════
//  SEQRENDER INTERFACE (Device calls this)
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::OnRender() {
    if (!m_enabled) return;

    // This is called by Device.seqRender.Process()
    // Skip if main menu is active or no level loaded (main menu uses RenderMenu() instead)
    if (g_pGamePersistent->MainMenuActiveOrLevelNotExist())
        return;

    // Forward to our internal Render() implementation
    Render();
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
