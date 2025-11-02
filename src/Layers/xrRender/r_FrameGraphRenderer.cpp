#include "stdafx.h"
#include "r_FrameGraphRenderer.h"
#include "FBasicVisual.h"
#include "FVisual.h"
#include "FTreeVisual.h"
#include "FHierrarhyVisual.h"
#include "FLOD.h"
#include "Shader.h"
#include "r__dsgraph_structure.h"

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

    m_framegraph = xr_make_unique<framegraph::FrameGraph>(device->GetNVRHIDevice());

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

    PresentToBackbuffer();

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

    CollectVisibleGeometry();

    m_geometryCollector->EndFrame();
}

void FrameGraphRenderer::BuildFrameGraph() {

    xr_set<framegraph::RenderPhase> requiredPhases = ScanRequiredPhases();

    for (framegraph::RenderPhase phase : requiredPhases) {
        CreatePhasePass(phase);
    }

    RouteBatchesToPasses();

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

    Msg("! [FrameGraphRenderer] Registering render targets in RT registry...");
    auto& registry = m_framegraph->GetRTRegistry();

    registry.RegisterRT("rt_Albedo", gbufferOutputs.albedo);
    registry.RegisterRT("rt_Color", gbufferOutputs.albedo);
    registry.RegisterAliases(gbufferOutputs.albedo, {
        "s_albedo",
        "s_diffuse",
        "s_image",
        "s_base"
    });

    registry.RegisterRT("rt_Normal", gbufferOutputs.normal);
    registry.RegisterAliases(gbufferOutputs.normal, {
        "s_normal",
        "s_nmap"
    });

    registry.RegisterRT("rt_Material", gbufferOutputs.material);
    registry.RegisterAliases(gbufferOutputs.material, {
        "s_material",
        "s_mat"
    });

    registry.RegisterRT("rt_Depth", gbufferOutputs.depth);
    registry.RegisterAliases(gbufferOutputs.depth, {
        "s_depth",
        "s_position"
    });

    registry.RegisterRT("rt_HDRColor", lightingOutput.hdrColor);
    registry.RegisterRT("rt_Accumulator", lightingOutput.hdrColor);
    registry.RegisterAliases(lightingOutput.hdrColor, {
        "s_hdr",
        "s_accumulator",
        "s_acc"
    });

    registry.RegisterRT("rt_Backbuffer", backbuffer);
    registry.RegisterAliases(backbuffer, {
        "s_backbuffer",
        "s_screen"
    });

    registry.PrintRegistry();

    m_finalOutput = backbuffer;
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

    nvrhi::BufferDesc vbDesc;
    vbDesc.debugName = "VisibleMesh_VB";
    vbDesc.byteSize = meshVisual->vCount * geom->vb_stride;
    vbDesc.isVertexBuffer = true;
    vbDesc.keepInitialState = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

    nvrhi::BufferHandle nvrhiVB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
        nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(geom->vb), vbDesc);

    if (!nvrhiVB)
        return false;

    nvrhi::BufferDesc ibDesc;
    ibDesc.debugName = "VisibleMesh_IB";
    ibDesc.byteSize = meshVisual->iCount * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.keepInitialState = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

    nvrhi::BufferHandle nvrhiIB = m_device->GetNVRHIDevice()->createHandleForNativeBuffer(
        nvrhi::ObjectTypes::D3D11_Buffer, nvrhi::Object(geom->ib), ibDesc);

    if (!nvrhiIB)
        return false;

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

    batch.debugName = "VisibleMesh";

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

    for (dxRender_Visual* visual : staticVisuals) {
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
                continue;
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

    const auto& batches = m_geometryCollector->GetBatches();

    Msg("! [FrameGraphRenderer] Scanning %u batches for required phases...", batches.size());

    for (const auto& batch : batches) {
        if (!batch.pipeline) {
            continue;
        }

        phases.insert(framegraph::RenderPhase::Geometry);
    }

    Msg("! [FrameGraphRenderer] Found %u required phases", phases.size());
    for (framegraph::RenderPhase phase : phases) {
        const char* phaseName = "Unknown";
        switch (phase) {
            case framegraph::RenderPhase::Geometry: phaseName = "Geometry"; break;
            case framegraph::RenderPhase::Lighting: phaseName = "Lighting"; break;
            case framegraph::RenderPhase::Combine: phaseName = "Combine"; break;
            case framegraph::RenderPhase::PostProcess: phaseName = "PostProcess"; break;
            case framegraph::RenderPhase::Shadow: phaseName = "Shadow"; break;
            case framegraph::RenderPhase::Custom: phaseName = "Custom"; break;
        }
        Msg("!   - %s", phaseName);
    }

    return phases;
}

void FrameGraphRenderer::CreatePhasePass(framegraph::RenderPhase phase) {
    Msg("! [FrameGraphRenderer] Creating pass for phase: %d", (int)phase);

    switch (phase) {
        case framegraph::RenderPhase::Geometry:
            Msg("!   Geometry pass already exists");
            break;

        case framegraph::RenderPhase::Lighting:
            Msg("!   Lighting pass already exists");
            break;

        case framegraph::RenderPhase::PostProcess:
            Msg("!   PostProcess pass already exists");
            break;

        default:
            Msg("!   ⚠️ Unsupported phase: %d", (int)phase);
            break;
    }
}

void FrameGraphRenderer::RouteBatchesToPasses() {
    Msg("! [FrameGraphRenderer] Routing batches to passes...");

    const auto& batches = m_geometryCollector->GetBatches();

    u32 geometryBatches = 0;
    u32 lightingBatches = 0;
    u32 otherBatches = 0;

    for (const auto& batch : batches) {
        geometryBatches++;
    }

    Msg("!   Geometry: %u batches", geometryBatches);
    Msg("!   Lighting: %u batches", lightingBatches);
    Msg("!   Other: %u batches", otherBatches);
}

}
