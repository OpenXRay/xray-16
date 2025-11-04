#include "stdafx.h"
#include "r_FrameGraphRenderer.h"
#include "FBasicVisual.h"
#include "FVisual.h"
#include "FTreeVisual.h"
#include "FHierrarhyVisual.h"
#include "FLOD.h"
#include "Shader.h"
#include "r__dsgraph_structure.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render {

using namespace RENDER_NAMESPACE;

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

    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device);

    m_shaderPhaseCache = xr_make_unique<framegraph::ShaderPhaseCache>();

    passes::GBufferPassConfig gbufferConfig;
    gbufferConfig.width = Device.dwWidth;
    gbufferConfig.height = Device.dwHeight;

    m_gbufferPass = xr_make_unique<passes::GBufferPass>(device, gbufferConfig);
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

    BuildFrameGraphStructure();

    m_framegraph->Compile();

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
    m_shaderPhaseCache.reset();
    m_framegraph.reset();

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    auto frameStart = std::chrono::high_resolution_clock::now();

    SetupFrame();

    SetupFrameGraphPasses();

    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->Execute();

    PresentToBackbuffer();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_stats.totalFrameMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart
    ).count();

    m_stats.gbufferMs = m_gbufferPass->GetGBufferStats().cpuTimeMs;
    m_stats.lightingMs = m_lightingPass->GetStats().cpuTimeMs;
    m_stats.tonemapMs = m_tonemapPass->GetStats().cpuTimeMs;
    m_stats.numDrawCalls = m_gbufferPass->GetGBufferStats().numDrawCalls;
    m_stats.numTriangles = m_gbufferPass->GetGBufferStats().numTriangles;

    m_framegraph->ResetForNextFrame();
}

void FrameGraphRenderer::SetupFrame() {
    m_bufferHandleCache.clear();

    m_geometryCollector->BeginFrame();

    CollectVisibleGeometry();

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

void FrameGraphRenderer::BuildFrameGraphStructure() {

    Msg("! [FrameGraphRenderer] Creating vanilla X-Ray render targets...");

    u32 w = Device.dwWidth;
    u32 h = Device.dwHeight;

    m_rt_Position = CreateRT("rt_Position", w, h, nvrhi::Format::RGBA16_FLOAT);
    m_rt_Normal = CreateRT("rt_Normal", w, h, nvrhi::Format::RGBA16_FLOAT);
    m_rt_Albedo = CreateRT("rt_Albedo", w, h, nvrhi::Format::RGBA8_UNORM);
    m_rt_Depth = CreateRT("rt_Depth", w, h, nvrhi::Format::D24S8, true);

    m_rt_Accumulator = CreateRT("rt_Accumulator", w, h, nvrhi::Format::RGBA16_FLOAT);

    m_rt_Generic_0 = CreateRT("rt_Generic_0", w, h, nvrhi::Format::RGBA8_UNORM);
    m_rt_Generic_1 = CreateRT("rt_Generic_1", w, h, nvrhi::Format::RGBA8_UNORM);
    m_rt_Generic_2 = CreateRT("rt_Generic_2", w, h, nvrhi::Format::RGBA16_FLOAT);

    m_backbuffer = CreateRT("Backbuffer", 1920, 1080, nvrhi::Format::RGBA8_UNORM);

    Msg("  ✓ Created %d vanilla render targets", 8);

    m_gbufferPass->Setup(*m_framegraph);
    auto gbufferOutputs = m_gbufferPass->GetOutputs();

    Msg("! [FrameGraphRenderer] Registering vanilla render targets in RT registry...");
    auto& registry = m_framegraph->GetRTRegistry();

    registry.RegisterRT("rt_Position", m_rt_Position);
    registry.RegisterAliases(m_rt_Position, {
        "$user$position",
        "s_position"
    });

    registry.RegisterRT("rt_Normal", m_rt_Normal);
    registry.RegisterAliases(m_rt_Normal, {
        "s_normal",
        "s_nmap"
    });

    registry.RegisterRT("rt_Albedo", m_rt_Albedo);
    registry.RegisterRT("rt_Color", m_rt_Albedo);
    registry.RegisterAliases(m_rt_Albedo, {
        "$user$albedo",
        "s_albedo",
        "s_diffuse",
        "s_image",
        "s_base"
    });

    registry.RegisterRT("rt_Depth", m_rt_Depth);
    registry.RegisterAliases(m_rt_Depth, {
        "$user$base_depth",
        "s_depth"
    });

    registry.RegisterRT("rt_Accumulator", m_rt_Accumulator);
    registry.RegisterAliases(m_rt_Accumulator, {
        "s_accumulator",
        "s_acc"
    });

    registry.RegisterRT("rt_Generic_0", m_rt_Generic_0);
    registry.RegisterAliases(m_rt_Generic_0, {
        "$user$generic0",
        "s_generic0"
    });

    registry.RegisterRT("rt_Generic_1", m_rt_Generic_1);
    registry.RegisterAliases(m_rt_Generic_1, {
        "$user$generic1",
        "s_generic1"
    });

    registry.RegisterRT("rt_Generic_2", m_rt_Generic_2);
    registry.RegisterAliases(m_rt_Generic_2, {
        "s_generic2"
    });

    registry.RegisterRT("rt_Backbuffer", m_backbuffer);
    registry.RegisterAliases(m_backbuffer, {
        "s_backbuffer",
        "s_screen"
    });

    registry.RegisterRT("GBuffer.Albedo", gbufferOutputs.albedo);
    registry.RegisterRT("GBuffer.Normal", gbufferOutputs.normal);
    registry.RegisterRT("GBuffer.Material", gbufferOutputs.material);
    registry.RegisterRT("GBuffer.Depth", gbufferOutputs.depth);

    registry.PrintRegistry();

    m_finalOutput = gbufferOutputs.albedo;
}

void FrameGraphRenderer::SetupFrameGraphPasses() {

    CreateAllRequiredPasses();

    RouteBatchesToPasses();

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

    nvrhi::ITexture* finalTexture = m_framegraph->GetPhysicalTexture(m_finalOutput);
    if (!finalTexture) {
        Msg("! [FrameGraphRenderer] Failed to get final output texture for present");
        return;
    }

    ID3D11RenderTargetView* backbufferRTV = RImplementation.Target->get_base_rt();
    if (!backbufferRTV) {
        Msg("! [FrameGraphRenderer] Failed to get game backbuffer RTV");
        return;
    }

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

    nvrhi::TextureDesc backbufferDesc;
    D3D11_TEXTURE2D_DESC d3dDesc;
    d3dBackbufferTex->GetDesc(&d3dDesc);

    backbufferDesc.width = d3dDesc.Width;
    backbufferDesc.height = d3dDesc.Height;
    backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
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

    ng::RenderContext* ctx = m_renderContext.get();
    VERIFY(ctx != nullptr);

    ctx->CopyTexture(
        backbufferHandle.Get(),
        finalTexture
    );

    Msg("  [FrameGraphRenderer] Presented final output to game backbuffer (%ux%u)",
        d3dDesc.Width, d3dDesc.Height);
}

bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform) {
    if (!visual)
        return false;

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
            return false;
    }

    if (!meshVisual)
        return false;

    if (!meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return false;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->vb || !geom->ib)
        return false;

    ID3D11Buffer* d3dVB = geom->vb;
    ID3D11Buffer* d3dIB = geom->ib;

    nvrhi::BufferHandle nvrhiVB;
    auto vbIt = m_bufferHandleCache.find(d3dVB);
    if (vbIt != m_bufferHandleCache.end()) {
        nvrhiVB = vbIt->second;
    } else {
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

    nvrhi::BufferHandle nvrhiIB;
    auto ibIt = m_bufferHandleCache.find(d3dIB);
    if (ibIt != m_bufferHandleCache.end()) {
        nvrhiIB = ibIt->second;
    } else {
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

    GeometryBatch batch;

    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;

    batch.indexCount = meshVisual->iCount;
    batch.startIndex = meshVisual->iBase;
    batch.baseVertex = meshVisual->vBase;

    batch.worldMatrix = worldTransform;

    batch.visual = visual;

    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    batch.debugName = visual->dbg_name.c_str();

    if (!nvrhiVB || !nvrhiIB) {
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p, D3DVB=%p, D3DIB=%p",
            nvrhiVB.Get(), nvrhiIB.Get(), d3dVB, d3dIB);
        return false;
    }

    m_geometryCollector->Submit(batch);
    return true;
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

    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    xr_vector<dxRender_Visual*> staticVisuals;

    auto& dsgraph = RImplementation.get_imm_context();

    if (!dsgraph.Sectors.empty()) {
        Fvector camPos = Device.vCameraPosition;
        auto sectorID = dsgraph.detect_sector(camPos);

        if (sectorID != IRender_Sector::INVALID_SECTOR_ID) {
            CSector* sector = static_cast<CSector*>(dsgraph.Sectors[sectorID]);
            if (sector && sector->root()) {
                ExtractStaticLeafVisuals(sector->root(), staticVisuals);
            }
        }
    }

    Msg("  [FrameGraph] Found %u static visuals from sectors", (u32)staticVisuals.size());

    xr_vector<ISpatial*> spatialObjects;
    g_pGamePersistent->SpatialSpace.q_frustum(
        spatialObjects,
        0,
        STYPE_RENDERABLE,
        frustum
    );

    Msg("  [FrameGraph] Found %u dynamic objects from spatial DB", (u32)spatialObjects.size());

    u32 submittedStatic = 0;
    u32 notFvisual_static = 0;
    u32 noGeometry_static = 0;
    u32 noBuffers_static = 0;

    u32 culledStatic = 0;

    for (dxRender_Visual* visual : staticVisuals) {
        if (!frustum.testSphere_dirty(visual->vis.sphere.P, visual->vis.sphere.R)) {
            culledStatic++;
            continue;
        }

        Fmatrix xform;
        switch (visual->getType())
        {
            case MT_TREE_ST:
            case MT_TREE_PM:
            {
                FTreeVisual* treeVisual = dynamic_cast<FTreeVisual*>(visual);
                if (treeVisual)
                    xform = treeVisual->xform;
                break;
            }
            case MT_NORMAL:
            default:
                xform = Fidentity;
                break;
        }

        if (ProcessVisualGeometry(visual, xform)) {
            submittedStatic++;
        } else {
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

    Msg("  [FrameGraph] Static: submitted %u/%u (filtered: %u not mesh, %u no geom, %u no buffers)",
        submittedStatic, (u32)staticVisuals.size(), notFvisual_static, noGeometry_static, noBuffers_static);

    u32 submittedDynamic = 0;
    u32 notRenderable = 0;
    u32 noVisual = 0;

    for (ISpatial* spatial : spatialObjects)
    {
        IRenderable* renderable = spatial->dcast_Renderable();
        if (!renderable) {
            notRenderable++;
            continue;
        }

        IRenderVisual* iVisual = renderable->GetRenderData().visual;
        if (!iVisual) {
            noVisual++;
            continue;
        }

        dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(iVisual);
        if (!visual) {
            noVisual++;
            continue;
        }

        if (ProcessVisualGeometry(visual, renderable->GetRenderData().xform)) {
            submittedDynamic++;
        }
    }

    Msg("  [FrameGraph] Dynamic: submitted %u/%u (filtered: %u not renderable, %u no visual)",
        submittedDynamic, (u32)spatialObjects.size(), notRenderable, noVisual);
    Msg("  [FrameGraph] TOTAL: %u geometry batches submitted", submittedStatic + submittedDynamic);
}

xr_set<framegraph::RenderPhase> FrameGraphRenderer::ScanRequiredPhases() const {
    xr_set<framegraph::RenderPhase> phases;

    auto& batches = const_cast<GeometryCollector*>(m_geometryCollector.get())->GetBatchesMutable();
    Msg("! [FrameGraphRenderer] Scanning %u batches for required phases...", batches.size());

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

    Msg("! [FrameGraphRenderer] Found %u required phases:", phases.size());
    for (const auto& [phase, count] : phaseCount) {
        const char* phaseName = framegraph::IPass::GetPhaseName(phase);
        Msg("!   %s: %u batches", phaseName, count);
    }

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
            Msg("!   Using existing GBufferPass instance");
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

}

void FrameGraphRenderer::CreateAllRequiredPasses() {
    xr_set<framegraph::RenderPhase> requiredPhases = ScanRequiredPhases();

    for (framegraph::RenderPhase phase : requiredPhases) {
        CreatePhasePass(phase);
    }

    Msg("! [FrameGraphRenderer] Created %u passes", m_activePasses.size());
}

void FrameGraphRenderer::RouteBatchesToPasses() {
    Msg("! [FrameGraphRenderer] Routing batches to passes...");

    auto& batches = m_geometryCollector->GetBatchesMutable();

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

    xr_map<framegraph::RenderPhase, xr_vector<GeometryBatch*>> batchesByPhase;

    for (auto& batch : batches) {
        batchesByPhase[batch.renderPhase].push_back(&batch);
    }

    for (const auto& [phase, phaseBatches] : batchesByPhase) {
        const char* phaseName = framegraph::IPass::GetPhaseName(phase);

        switch (phase) {
            case framegraph::RenderPhase::Geometry:
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

}
