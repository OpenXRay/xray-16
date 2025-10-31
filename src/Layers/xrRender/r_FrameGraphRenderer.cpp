// xrRender/r_FrameGraphRenderer.cpp
#include "stdafx.h"
#include "r_FrameGraphRenderer.h"
#include "FVisual.h"
#include "Shader.h"

namespace xray::render {

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

    // Create FrameGraph (needs NVRHI device)
    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device->GetNVRHIDevice());

    // Create passes (pass device for shader loading)
    m_gbufferPass = xr_make_unique<passes::GBufferPass>(device);
    m_lightingPass = xr_make_unique<passes::LightingPass>(device);
    m_tonemapPass = xr_make_unique<passes::TonemapPass>(device);

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

    Msg("  ✓ FrameGraphRenderer initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    // Clear global geometry collector pointer
    g_geometryCollector = nullptr;

    m_renderContext.reset();
    m_geometryCollector.reset();
    m_tonemapPass.reset();
    m_lightingPass.reset();
    m_gbufferPass.reset();
    m_framegraph.reset();

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    auto frameStart = std::chrono::high_resolution_clock::now();

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAME
    // ═══════════════════════════════════════════════════════

    SetupFrame();

    // ═══════════════════════════════════════════════════════
    //  BUILD FRAMEGRAPH
    // ═══════════════════════════════════════════════════════

    BuildFrameGraph();

    // ═══════════════════════════════════════════════════════
    //  COMPILE & EXECUTE
    // ═══════════════════════════════════════════════════════

    m_framegraph->Compile();

    // Set RenderContext for execution
    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->Execute();

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_stats.totalFrameMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart
    ).count();

    m_stats.gbufferMs = m_gbufferPass->GetStats().cpuTimeMs;
    m_stats.lightingMs = m_lightingPass->GetStats().cpuTimeMs;
    m_stats.tonemapMs = m_tonemapPass->GetStats().cpuTimeMs;
    m_stats.numDrawCalls = m_gbufferPass->GetStats().numDrawCalls;
    m_stats.numTriangles = m_gbufferPass->GetStats().numTriangles;

    // Reset for next frame
    m_framegraph->Reset();
}

void FrameGraphRenderer::SetupFrame() {
    // Begin geometry collection
    m_geometryCollector->BeginFrame();

    // Collect visible geometry (CPU culling for now, GPU later)
    CollectVisibleGeometry();

    // End geometry collection (sorts batches)
    m_geometryCollector->EndFrame();
}

void FrameGraphRenderer::BuildFrameGraph() {
    // Create backbuffer as transient resource for now
    // TODO: Import actual backbuffer from HW later
    framegraph::ResourceDesc backbufferDesc;
    backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    backbufferDesc.width = 1920;  // TODO: Get from Device
    backbufferDesc.height = 1080;
    backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.isTransient = true;  // Let FrameGraph allocate it
    backbufferDesc.debugName = "Backbuffer";

    framegraph::VirtualResourceHandle backbuffer =
        m_framegraph->CreateTexture("Backbuffer", backbufferDesc);

    // ═══════════════════════════════════════════════════════
    //  SETUP RENDERING PASSES
    // ═══════════════════════════════════════════════════════

    // G-Buffer pass
    auto gbufferOutputs = m_gbufferPass->Setup(*m_framegraph);

    // Lighting pass
    auto lightingOutput = m_lightingPass->Setup(*m_framegraph, gbufferOutputs);

    // Tonemap pass
    m_tonemapPass->Setup(*m_framegraph, lightingOutput.hdrColor, backbuffer);
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
//  VISIBILITY & CULLING
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::CollectVisibleGeometry() {
    // Query spatial database for visible renderables
    // For now, do simple frustum culling on CPU
    // TODO: Move to GPU compute culling in future phases

    if (!g_pGamePersistent)
        return;

    // Get camera frustum
    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    // Query spatial database for renderable objects
    xr_vector<ISpatial*> spatialObjects;
    g_pGamePersistent->SpatialSpace.q_frustum(
        spatialObjects,
        0,  // Only query objects in immediate portals
        STYPE_RENDERABLE,  // Only renderables
        frustum
    );

    Msg("  [FrameGraph] Found %u potentially visible objects", (u32)spatialObjects.size());

    u32 submittedCount = 0;

    // Extract geometry from each visible object
    for (ISpatial* spatial : spatialObjects)
    {
        // Get the renderable object
        IRenderable* renderable = spatial->dcast_Renderable();
        if (!renderable)
            continue;

        // Get the visual (geometry)
        dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(renderable->renderable_ROS());
        if (!visual)
            continue;

        // Only handle simple mesh visuals for now (type MT_NORMAL)
        if (visual->getType() != MT_NORMAL)
            continue;

        // Cast to Fvisual to access geometry
        Fvisual* meshVisual = dynamic_cast<Fvisual*>(visual);
        if (!meshVisual)
            continue;

        // Check if geometry is valid
        if (!meshVisual->rm_geom || !meshVisual->rm_geom._get())
            continue;

        SGeometry* geom = meshVisual->rm_geom._get();
        if (!geom->vb || !geom->ib)
            continue;

        // TODO: Wrap D3D11 buffers as NVRHI handles
        // TODO: Create PSO for material
        // TODO: Create binding set for textures
        // TODO: Build GeometryBatch and submit

        submittedCount++;
    }

    Msg("  [FrameGraph] Submitted %u/%u objects to collector",
        submittedCount, (u32)spatialObjects.size());
}

} // namespace xray::render
