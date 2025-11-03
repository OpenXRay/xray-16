// xrRender/r_FrameGraphRenderer.cpp
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

    // Create FrameGraph with ModernResourceManager integration
    m_framegraph = xr_make_unique<framegraph::FrameGraph>(
        device->GetNVRHIDevice(),
        device->GetModernResourceManager()
    );

    // Create shader phase cache (Week 16 - for precompilation phase detection)
    m_shaderPhaseCache = xr_make_unique<framegraph::ShaderPhaseCache>();

    // Create passes with correct render target resolution (not screen resolution!)
    passes::GBufferPassConfig gbufferConfig;
    gbufferConfig.width = Device.dwWidth;   // Game render resolution (e.g., 1280x720)
    gbufferConfig.height = Device.dwHeight; // NOT screen resolution (1920x1080)!

    m_gbufferPass = xr_make_unique<passes::GBufferPass>(device, gbufferConfig);
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
    m_shaderPhaseCache.reset();
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

    // Reset for next frame
    m_framegraph->Reset();
}

void FrameGraphRenderer::SetupFrame() {
    // Clear buffer handle cache (X-Ray may recreate buffers each frame)
    m_bufferHandleCache.clear();

    // Begin geometry collection
    m_geometryCollector->BeginFrame();

    // Collect visible geometry (CPU culling for now, GPU later)
    CollectVisibleGeometry();

    // End geometry collection (sorts batches)
    m_geometryCollector->EndFrame();
}

void FrameGraphRenderer::BuildFrameGraph() {
    // ═══════════════════════════════════════════════════════
    //  STEP 1: SETUP RENDER TARGETS FIRST
    // ═══════════════════════════════════════════════════════
    // Render targets must exist in FrameGraph before MaterialPSO creation!

    // Create backbuffer as transient resource
    framegraph::ResourceDesc backbufferDesc;
    backbufferDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    backbufferDesc.width = 1920;  // TODO: Get from Device
    backbufferDesc.height = 1080;
    backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.isTransient = true;
    backbufferDesc.debugName = "Backbuffer";

    framegraph::VirtualResourceHandle backbuffer =
        m_framegraph->CreateTexture("Backbuffer", backbufferDesc);

    // G-Buffer pass - Setup creates render targets in FrameGraph
    m_gbufferPass->Setup(*m_framegraph);
    auto gbufferOutputs = m_gbufferPass->GetOutputs();

    // ═══════════════════════════════════════════════════════
    //  STEP 2: DYNAMIC PASS ROUTING (Week 16)
    // ═══════════════════════════════════════════════════════
    // Now that render targets exist, we can scan materials and create PSOs

    // Scan materials and create required passes
    CreateAllRequiredPasses();

    // Route batches to appropriate passes
    RouteBatchesToPasses();

    // Lighting pass
    //auto lightingOutput = m_lightingPass->Setup(*m_framegraph, gbufferOutputs);

     //Tonemap pass
    //m_tonemapPass->Setup(*m_framegraph, lightingOutput.hdrColor, backbuffer);

    // ═══════════════════════════════════════════════════════
    //  REGISTER RENDER TARGETS IN REGISTRY (Week 14)
    // ═══════════════════════════════════════════════════════

    Msg("! [FrameGraphRenderer] Registering render targets in RT registry...");
    auto& registry = m_framegraph->GetRTRegistry();

    // ─── G-Buffer Outputs ───

    // Albedo RT (RT0: Albedo.rgb + Metallic.a)
    registry.RegisterRT("rt_Albedo", gbufferOutputs.albedo);
    registry.RegisterRT("rt_Color", gbufferOutputs.albedo);  // Alias
    registry.RegisterAliases(gbufferOutputs.albedo, {
        "s_albedo",
        "s_diffuse",
        "s_image",
        "s_base"
    });

    // Normal RT (RT1: Normal.xyz + Roughness.a)
    registry.RegisterRT("rt_Normal", gbufferOutputs.normal);
    registry.RegisterAliases(gbufferOutputs.normal, {
        "s_normal",
        "s_nmap"
    });

    // Material RT (RT2: Material ID)
    registry.RegisterRT("rt_Material", gbufferOutputs.material);
    registry.RegisterAliases(gbufferOutputs.material, {
        "s_material",
        "s_mat"
    });

    // Depth/Stencil
    registry.RegisterRT("rt_Depth", gbufferOutputs.depth);
    registry.RegisterAliases(gbufferOutputs.depth, {
        "s_depth",
        "s_position"  // Some shaders use depth to reconstruct position
    });

    // ─── Lighting Outputs ───

    // HDR Color (Lit scene)
    //registry.RegisterRT("rt_HDRColor", lightingOutput.hdrColor);
    //registry.RegisterRT("rt_Accumulator", lightingOutput.hdrColor);  // Legacy name
    //registry.RegisterAliases(lightingOutput.hdrColor, {
    //    "s_hdr",
    //    "s_accumulator",
    //    "s_acc"
    //});

    // ─── Backbuffer ───

    registry.RegisterRT("rt_Backbuffer", backbuffer);
    registry.RegisterAliases(backbuffer, {
        "s_backbuffer",
        "s_screen"
    });

    // Print registry for debugging
    registry.PrintRegistry();

    // Store final output for presenting to backbuffer
    m_finalOutput = gbufferOutputs.albedo;
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
//  VISIBILITY & CULLING
// ═══════════════════════════════════════════════════════

// Helper to process a visual and submit geometry batch (returns true if submitted)
bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform) {
    if (!visual)
        return false;

    // Get mesh interface based on visual type
    // IRender_Mesh is not polymorphic, so we must cast to concrete types
    IRender_Mesh* meshVisual = nullptr;

    switch (visual->getType()) {
        case MT_NORMAL:           // Static mesh
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_TREE_ST:          // SpeedTree static
        case MT_TREE_PM:          // SpeedTree progressive mesh
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        // MT_HIERRARHY, MT_SKELETON_*, MT_PARTICLE_GROUP, MT_LOD don't have direct mesh data
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

    Msg("! [ProcessVisualGeometry] Visual '%s': D3D VB=%p, IB=%p, vBase=%d, iBase=%d, vCount=%d, iCount=%d",
        visual->dbg_name.c_str(), d3dVB, d3dIB,
        meshVisual->vBase, meshVisual->iBase, meshVisual->vCount, meshVisual->iCount);

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

    // PSO and binding set will be created by MaterialCache in GBufferPass
    batch.pipeline = nullptr;
    batch.bindingSet = nullptr;

    batch.debugName = "VisibleMesh";

    // Submit to collector
    m_geometryCollector->Submit(batch);
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
            // Simple approach: just get geometry from current sector
            // TODO: Full portal traversal for multi-sector visibility
            CSector* sector = static_cast<CSector*>(dsgraph.Sectors[sectorID]);
            if (sector && sector->root()) {
                // Recursively extract all leaf visuals from sector hierarchy
                ExtractStaticLeafVisuals(sector->root(), staticVisuals);
            }
        }
    }

    Msg("  [FrameGraph] Found %u static visuals from sectors", (u32)staticVisuals.size());

    // ═══════════════════════════════════════════════════════
    //  COLLECT DYNAMIC GEOMETRY FROM SPATIAL DATABASE
    // ═══════════════════════════════════════════════════════

    // Query spatial database for renderable objects (same as legacy dsgraph)
    xr_vector<ISpatial*> spatialObjects;
    g_pGamePersistent->SpatialSpace.q_frustum(
        spatialObjects,
        0,  // spatial_traverse_flags (0 = no special flags)
        STYPE_RENDERABLE,  // Only renderables (not COLLIDEABLE!)
        frustum
    );

    Msg("  [FrameGraph] Found %u dynamic objects from spatial DB", (u32)spatialObjects.size());

    // ═══════════════════════════════════════════════════════
    //  PROCESS STATIC GEOMETRY
    // ═══════════════════════════════════════════════════════

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
                continue; // TODO: break so that we can render more than just trees
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

    Msg("  [FrameGraph] Static: submitted %u/%u (filtered: %u not mesh, %u no geom, %u no buffers)",
        submittedStatic, (u32)staticVisuals.size(), notFvisual_static, noGeometry_static, noBuffers_static);

    // ═══════════════════════════════════════════════════════
    //  PROCESS DYNAMIC GEOMETRY
    // ═══════════════════════════════════════════════════════

    u32 submittedDynamic = 0;
    u32 notRenderable = 0;
    u32 noVisual = 0;

    // Extract geometry from each visible dynamic object
    for (ISpatial* spatial : spatialObjects)
    {
        // Get the renderable object
        IRenderable* renderable = spatial->dcast_Renderable();
        if (!renderable) {
            notRenderable++;
            continue;
        }

        // Get the visual (geometry) - stored in RenderData, not ROS
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

        // Process dynamic visual with its transform
        if (ProcessVisualGeometry(visual, renderable->GetRenderData().xform)) {
            submittedDynamic++;
        }
    }

    Msg("  [FrameGraph] Dynamic: submitted %u/%u (filtered: %u not renderable, %u no visual)",
        submittedDynamic, (u32)spatialObjects.size(), notRenderable, noVisual);
    Msg("  [FrameGraph] TOTAL: %u geometry batches submitted", submittedStatic + submittedDynamic);
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

} // namespace xray::render
