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

    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device->GetNVRHIDevice());

    m_gbufferPass = xr_make_unique<passes::GBufferPass>(device);
    m_lightingPass = xr_make_unique<passes::LightingPass>(device);
    m_tonemapPass = xr_make_unique<passes::TonemapPass>(device);

    m_geometryCollector = xr_make_unique<GeometryCollector>();

    g_geometryCollector = m_geometryCollector.get();

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

    SetupFrame();

    BuildFrameGraph();

    m_framegraph->Compile();

    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->Execute();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_stats.totalFrameMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart
    ).count();

    m_stats.gbufferMs = m_gbufferPass->GetStats().cpuTimeMs;
    m_stats.lightingMs = m_lightingPass->GetStats().cpuTimeMs;
    m_stats.tonemapMs = m_tonemapPass->GetStats().cpuTimeMs;
    m_stats.numDrawCalls = m_gbufferPass->GetStats().numDrawCalls;
    m_stats.numTriangles = m_gbufferPass->GetStats().numTriangles;

    m_framegraph->Reset();
}

void FrameGraphRenderer::SetupFrame() {
    m_geometryCollector->BeginFrame();

    SubmitTestGeometry();

    m_geometryCollector->EndFrame();
}

void FrameGraphRenderer::BuildFrameGraph() {
    framegraph::ResourceDesc backbufferDesc;
    backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    backbufferDesc.width = 1920;
    backbufferDesc.height = 1080;
    backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.isTransient = true;
    backbufferDesc.debugName = "Backbuffer";

    framegraph::VirtualResourceHandle backbuffer =
        m_framegraph->CreateTexture("Backbuffer", backbufferDesc);

    auto gbufferOutputs = m_gbufferPass->Setup(*m_framegraph);

    auto lightingOutput = m_lightingPass->Setup(*m_framegraph, gbufferOutputs);

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

void FrameGraphRenderer::CreateTestGeometry() {
    if (m_testGeometryCreated)
        return;

    Msg("* [FrameGraphRenderer] Creating test triangle geometry");

    struct TestVertex {
        Fvector position;
        Fvector normal;
        Fvector2 texcoord;
        Fvector tangent;
        Fvector binormal;
    };

    TestVertex vertices[3] = {
        { { -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.0f,  0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
    };

    u32 indices[3] = { 0, 1, 2 };

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

    m_renderContext->GetNativeCommandList()->open();

    m_renderContext->GetNativeCommandList()->writeBuffer(m_testVertexBuffer, vertices, sizeof(vertices));
    m_renderContext->GetNativeCommandList()->writeBuffer(m_testIndexBuffer, indices, sizeof(indices));

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

    GeometryBatch batch;
    batch.vertexBuffer = m_testVertexBuffer;
    batch.indexBuffer = m_testIndexBuffer;
    batch.indexCount = 3;
    batch.startIndex = 0;
    batch.baseVertex = 0;
    batch.materialID = 0;

    batch.worldMatrix.identity();

    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    batch.debugName = "TestTriangle";

    m_geometryCollector->Submit(batch);
}

}
