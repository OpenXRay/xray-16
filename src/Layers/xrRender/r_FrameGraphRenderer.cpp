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
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/ShaderKey.h"
#include "xrEngine/CustomHUD.h"
#include "ImGuiRendererNVRHI.h"
#include "xrEngine/device.h"
#include <imgui.h>

#include "FrameGraphPasses/DepthPrepassSetup.h"
#include "FrameGraphPasses/ForwardColorPassSetup.h"
#include "FrameGraphPasses/HUDPassSetup.h"
#include "FrameGraphPasses/UIPassSetup.h"
#include "FrameGraphPasses/ImGuiPassSetup.h"

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
    m_uiRenderer->Initialize(device, m_uiMaterialCache.get());

    Msg("* [FrameGraphRenderer] UI infrastructure initialized");

    m_textVCBPool = xr_make_unique<framegraph::VolatileConstantBufferPool>();
    m_textMaterialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_textVCBPool.get()
    );

    Msg("* [FrameGraphRenderer] Text infrastructure initialized");

    m_geometryCollector = xr_make_unique<GeometryCollector>();

    g_geometryCollector = m_geometryCollector.get();

    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    Msg("* [FrameGraphRenderer] initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    g_geometryCollector = nullptr;

    m_renderContext = nullptr;
    m_geometryCollector = nullptr;
    m_materialCache = nullptr;

    m_uiRenderer = nullptr;
    m_uiCollector = nullptr;
    m_uiMaterialCache = nullptr;
    m_uiVCBPool = nullptr;

    m_textMaterialCache = nullptr;
    m_textVCBPool = nullptr;

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    auto frameStart = std::chrono::high_resolution_clock::now();

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    SetupFrame();

    m_framegraph->ResetForNextFrame();

    SetupFrameGraphPasses();

    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->Compile();

    m_framegraph->Execute();

    PresentToBackbuffer();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_stats.totalFrameMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart
    ).count();

    m_stats.gbufferMs = 0.0f;
    m_stats.lightingMs = 0.0f;
    m_stats.tonemapMs = 0.0f;
    m_stats.numDrawCalls = 0;
    m_stats.numTriangles = 0;

}

void FrameGraphRenderer::RenderMenu() {
    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    Msg("* [FrameGraphRenderer::RenderMenu] Rendering main menu frame");

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    m_framegraph->ResetForNextFrame();

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

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
                cmdList->clearTextureFloat(bgRT, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
        }
    );

    auto uiTarget = passes::setupUIPass(*m_framegraph, width, height);

    uiTarget = passes::setupTextPass(*m_framegraph, uiTarget, width, height);

    uiTarget = passes::setupCursorPass(*m_framegraph, uiTarget, width, height);

    auto compositeOutput = passes::setupCompositePass(
        *m_framegraph,
        backgroundTarget,
        uiTarget,
        width,
        height
    );

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        compositeOutput,
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

    m_framegraph->SetRenderContext(m_renderContext.get());
    m_framegraph->Compile();
    m_framegraph->Execute();

    PresentToBackbuffer();

    Msg("* [FrameGraphRenderer::RenderMenu] Menu frame complete");
}

void FrameGraphRenderer::SetupFrame() {
    const bool levelLoaded = g_pGamePersistent && g_pGameLevel;
    m_bufferHandleCache.clear();

    m_lstRenderables.clear();

    if (levelLoaded)
    {
        if (levelLoaded && !g_pGamePersistent->IsLoadingScreenShown())
        {
            CFrustum view_frustum;
            view_frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

            u32 spatial_traverse_flags = ISpatial_DB::O_ORDERED;
            u32 spatial_types = STYPE_RENDERABLE | STYPE_LIGHTSOURCE;

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

    if (levelLoaded)
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

void FrameGraphRenderer::SetupFrameGraphPasses() {

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    framegraph::VirtualResourceHandle depthBuffer = passes::setupDepthPrepass(
        *m_framegraph,
        m_device,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height
    );

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height
    );

    auto hudOutputs = passes::setupHUDPass(
        *m_framegraph,
        m_device,
        forwardOutputs,
        &m_hudBatches,
        m_materialCache.get(),
        width,
        height
    );

    auto uiTarget = passes::setupUIPass(
        *m_framegraph,
        width,
        height
    );

    uiTarget = passes::setupTextPass(
        *m_framegraph,
        uiTarget,
        width,
        height
    );

    uiTarget = passes::setupCursorPass(
        *m_framegraph,
        uiTarget,
        width,
        height
    );

    auto compositeOutput = passes::setupCompositePass(
        *m_framegraph,
        hudOutputs.albedo,
        uiTarget,
        width,
        height
    );

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        compositeOutput,
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

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

void FrameGraphRenderer::RenderHUD() {
}

bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable) {
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
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;

        case MT_PARTICLE_EFFECT:
        case MT_PARTICLE_GROUP:
            return ProcessParticleGeometry(visual, worldTransform, renderable, false);

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

    batch.renderable = renderable;

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
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p, D3DVB=%p, D3DIB=%p",
            nvrhiVB.Get(), nvrhiIB.Get(), d3dVB, d3dIB);
        return false;
    }

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

bool FrameGraphRenderer::ProcessParticleGeometry(
    dxRender_Visual* visual,
    const Fmatrix& worldTransform,
    IRenderable* renderable,
    bool isHUD)
{
    if (!visual)
        return false;

    u32 vType = visual->getType();
    if (vType != MT_PARTICLE_EFFECT && vType != MT_PARTICLE_GROUP)
        return false;

    bool isHUDParticle = isHUD;
    if (vType == MT_PARTICLE_EFFECT) {
        RENDER_NAMESPACE::PS::CParticleEffect* pEffect =
            static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(visual);
        isHUDParticle = pEffect->GetHudMode();
    }

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
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            CKinematics* pV = static_cast<CKinematics*>(pVisual);
            pV->CalculateBones(TRUE);

            for (auto& child : pV->children) {
                ExtractStaticLeafVisuals(child, outLeafs);
            }

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

    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    xr_vector<dxRender_Visual*> staticVisuals;

    auto& dsgraph = RImplementation.get_imm_context();

    if (!dsgraph.Sectors.empty()) {
        Fvector camPos = Device.vCameraPosition;
        auto sectorID = dsgraph.detect_sector(camPos);

        if (sectorID != IRender_Sector::INVALID_SECTOR_ID) {
            dsgraph.PortalTraverser.traverse(
                dsgraph.Sectors[sectorID],
                frustum,
                camPos,
                Device.mFullTransform,
                CPortalTraverser::VQ_SSA
            );

            for (CSector* sector : dsgraph.PortalTraverser.r_sectors) {
                if (sector && sector->root()) {
                    ExtractStaticLeafVisuals(sector->root(), staticVisuals);
                }
            }

        }
    }

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

        Fmatrix xform = Fidentity;

        switch (visual->getType())
        {
            case MT_TREE_ST:
            case MT_TREE_PM:
            {
                FTreeVisual* treeVisual = static_cast<FTreeVisual*>(visual);
                xform = treeVisual->xform;
                break;
            }
            case MT_NORMAL:
            case MT_PROGRESSIVE:
            case MT_SKELETON_GEOMDEF_ST:
            case MT_SKELETON_GEOMDEF_PM:
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


    u32 submittedDynamic = 0;
    u32 notRenderable = 0;

    u32 portalTraversalMarker = dsgraph.PortalTraverser.i_marker;

    for (ISpatial* spatial : m_lstRenderables)
    {
        const auto& data = spatial->GetSpatialData();
        const auto sector_id = data.sector_id;
        IRenderable* renderable = spatial->dcast_Renderable();

        if (data.type & STYPE_LIGHTSOURCE) {
            notRenderable++;
            continue;
        }
        if (!renderable) {
            notRenderable++;
            continue;
        }

        renderable->renderable_Render(0, renderable);
        submittedDynamic++;
    }

    if (g_pGameLevel && g_pGameLevel->pHUD) {
        g_pGameLevel->pHUD->Render_Last(0);
    }
}

void FrameGraphRenderer::add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& xform) {

    if (!V) {
        return;
    }

    dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(V);
    if (!visual) {
        return;
    }

    bool isHUD = (root && root->renderable_HUD());

    xr_vector<dxRender_Visual*> leafVisuals;
    ExtractStaticLeafVisuals(visual, leafVisuals);

    u32 processed = 0;
    for (dxRender_Visual* leafVisual : leafVisuals) {
        bool success = false;
        if (isHUD) {
            success = ProcessHudGeometry(leafVisual, xform, root);
        } else {
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

    Msg("! [FrameGraphRenderer] Created %u passes", m_activePasses.size());
}

void FrameGraphRenderer::RouteBatchesToPasses() {
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

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);
    cmdList->open();
}

}
