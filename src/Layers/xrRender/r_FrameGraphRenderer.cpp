// xrRender/r_FrameGraphRenderer.cpp
#include "stdafx.h"
#include "r_FrameGraphRenderer.h"

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

    // Submit test triangle for pipeline verification
    SubmitTestGeometry();

    // TODO: Collect visible geometry from scene
    // For now, this will be handled by legacy renderer or manual submission

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
//  TEST GEOMETRY (TEMPORARY)
// ═══════════════════════════════════════════════════════

void FrameGraphRenderer::CreateTestGeometry() {
    if (m_testGeometryCreated)
        return;

    Msg("* [FrameGraphRenderer] Creating test triangle geometry");

    // Define vertex structure matching GBufferPass layout
    struct TestVertex {
        Fvector position;   // float3
        Fvector normal;     // float3
        Fvector2 texcoord;  // float2
        Fvector tangent;    // float3
        Fvector binormal;   // float3
    };

    // Create triangle vertices (counter-clockwise winding)
    TestVertex vertices[3] = {
        // Bottom-left (red debug color via normal)
        { { -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        // Top (green debug color via normal)
        { {  0.0f,  0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        // Bottom-right (blue debug color via normal)
        { {  0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
    };

    // Create indices
    u32 indices[3] = { 0, 1, 2 };

    // Create vertex buffer with initial data
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = sizeof(vertices);
    vbDesc.debugName = "TestTriangleVB";
    vbDesc.isVertexBuffer = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;

    m_testVertexBuffer = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!m_testVertexBuffer) {
        Msg("! [FrameGraphRenderer] Failed to create test vertex buffer");
        return;
    }

    // Create index buffer with initial data
    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = sizeof(indices);
    ibDesc.debugName = "TestTriangleIB";
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;

    m_testIndexBuffer = m_device->GetNVRHIDevice()->createBuffer(ibDesc);
    if (!m_testIndexBuffer) {
        Msg("! [FrameGraphRenderer] Failed to create test index buffer");
        return;
    }

    // Open command list to upload data
    m_renderContext->GetNativeCommandList()->open();

    // Upload data
    m_renderContext->GetNativeCommandList()->writeBuffer(m_testVertexBuffer, vertices, sizeof(vertices));
    m_renderContext->GetNativeCommandList()->writeBuffer(m_testIndexBuffer, indices, sizeof(indices));

    // Close and execute
    m_renderContext->GetNativeCommandList()->close();
    m_device->GetNVRHIDevice()->executeCommandList(m_renderContext->GetNativeCommandList());

    m_testGeometryCreated = true;
    Msg("  ✓ Test triangle geometry created");
}

void FrameGraphRenderer::SubmitTestGeometry() {
    if (!m_testGeometryCreated)
        CreateTestGeometry();

    if (!m_testGeometryCreated)
        return;

    // Create geometry batch for test triangle
    GeometryBatch batch;
    batch.vertexBuffer = m_testVertexBuffer;
    batch.indexBuffer = m_testIndexBuffer;
    batch.indexCount = 3;
    batch.startIndex = 0;
    batch.baseVertex = 0;
    batch.materialID = 0;

    // Set identity world matrix
    batch.worldMatrix.identity();

    // Pipeline and binding set will be set by GBufferPass
    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    batch.debugName = "TestTriangle";

    // Submit to collector
    m_geometryCollector->Submit(batch);
}

} // namespace xray::render
