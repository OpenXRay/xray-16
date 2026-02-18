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

#include "FrameGraphPasses/HiZBuildPassSetup.h"
#include "FrameGraphPasses/ForwardColorPassSetup.h"
#include "GPUCullingManager.h"
#include "FGDetailManager.h"
#include "FrameGraphPasses/DetailCullPassSetup.h"
#include "FrameGraphPasses/DetailPassSetup.h"
#include "FrameGraphPasses/TransparentPassSetup.h"
#include "Bindless/MaterialBuffer.h"
#include "Bindless/TerrainMaterialBuffer.h"
#include "Bindless/VariantTextureBuffer.h"
#include "FrameGraphPasses/SkyPassSetup.h"
#include "FrameGraphPasses/SunPassSetup.h"
#include "FrameGraphPasses/SkinningPassSetup.h"
#include "FrameGraphPasses/ParticlePassSetup.h"
#include "FrameGraphPasses/ExposurePassSetup.h"
#include "FrameGraphPasses/UIPassSetup.h"
#include "FrameGraphPasses/TonemapPassSetup.h"
#include "FrameGraphPasses/PassStates.h"
#include "FrameGraphPasses/ImGuiPassSetup.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrParticles/psystem.h"

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

    bindless::MaterialBuffer::Instance().Initialize(m_device);
    bindless::TerrainMaterialBuffer::Instance().Initialize(m_device);
    bindless::VariantTextureBuffer::Instance().Initialize(m_device);
    bindless::DrawMaterialIDBuffer::Instance().Initialize(m_device, 65536);
    Msg("* [FrameGraphRenderer] Bindless material buffers initialized (early)");

    m_gpuCullingManager = xr_make_unique<RENDER_NAMESPACE::GPUCullingManager>();

    m_detailManager = xr_make_unique<RENDER_NAMESPACE::FGDetailManager>();

    m_renderContext.reset(device->CreateContext());
    if (!m_renderContext)
    {
        Msg("! [FrameGraphRenderer] Failed to create RenderContext");
        return false;
    }

    m_passStates = xr_make_unique<passes::PassStates>();
    passes::InitializeSkyGeometry(device, m_passStates->sky);
    passes::InitializeSunPass(device, m_passStates->sun);
    passes::InitializeTonemapPass(device->GetNVRHIDevice(), m_passStates->tonemap);

    m_gpuProfiler = xr_make_unique<xray::profiler::GPUProfiler>();
    m_gpuProfiler->Initialize(device->GetNVRHIDevice());

    m_statsOverlay = xr_make_unique<xray::profiler::StatsOverlay>();
    m_statsOverlay->SetGPUProfiler(m_gpuProfiler.get());
    Msg("* [FrameGraphRenderer] Profiler initialized");

    {
        nvrhi::TextureDesc previewDesc;
        previewDesc.debugName = "InspectorPreview";
        previewDesc.width = 512;
        previewDesc.height = 512;
        previewDesc.format = nvrhi::Format::RGBA16_FLOAT;
        previewDesc.isUAV = true;
        previewDesc.isRenderTarget = false;
        previewDesc.keepInitialState = true;
        previewDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        m_inspectorPreview = device->GetNVRHIDevice()->createTexture(previewDesc);
    }

    Msg("* [FrameGraphRenderer] initialized");

    return true;
}

void FrameGraphRenderer::Shutdown() {
    if (!m_device) return;

    Msg("* [FrameGraphRenderer] Shutting down");

    g_geometryCollector = nullptr;

    m_statsOverlay = nullptr;
    if (m_gpuProfiler) {
        m_gpuProfiler->Shutdown();
        m_gpuProfiler = nullptr;
    }

    m_inspectorPreview = nullptr;
    m_renderContext = nullptr;
    m_geometryCollector = nullptr;
    m_materialCache = nullptr;

    m_uiRenderer = nullptr;
    m_uiCollector = nullptr;
    m_uiMaterialCache = nullptr;
    m_uiVCBPool = nullptr;

    m_textMaterialCache = nullptr;
    m_textVCBPool = nullptr;

    m_cachedStaticBatches.clear();
    m_staticBatchesCached = false;
    if (m_gpuCullingManager) {
        m_gpuCullingManager->InvalidateStaticCullingData();
    }

    m_shaderPhaseCache = nullptr;
    m_framegraph = nullptr;

    if (m_passStates) {
        passes::ShutdownSkyGeometry(m_passStates->sky);
        passes::ShutdownSunPass(m_passStates->sun);
        passes::ShutdownTonemapPass(m_passStates->tonemap);
        m_passStates.reset();
    }

    bindless::VariantTextureBuffer::Instance().Shutdown();
    bindless::MaterialBuffer::Instance().Shutdown();
    bindless::TerrainMaterialBuffer::Instance().Shutdown();

    m_device = nullptr;
}

void FrameGraphRenderer::Render() {
    ZoneScopedN("FrameGraphRenderer::Render");

    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    if (m_gpuProfiler)
    {
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }

    auto frameStart = std::chrono::high_resolution_clock::now();

    RImplementation.Lights.Update();

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    {
        ZoneScopedN("FG::SetupFrame");
        SetupFrame();
    }

    m_framegraph->ResetForNextFrame();

    {
        ZoneScopedN("FG::SetupPasses");
        SetupFrameGraphPasses();
    }

    m_framegraph->SetRenderContext(m_renderContext.get());

    m_framegraph->SetGPUProfiler(m_gpuProfiler.get());

    if (ps_fg_render_mode == FG_RENDER_VULKAN && GEnv.Backend->HasAsyncCompute())
        m_framegraph->SetAsyncCompute(GEnv.Backend->GetComputeCommandList(), GEnv.Backend);
    else
        m_framegraph->SetAsyncCompute(nullptr, nullptr);

    {
        ZoneScopedN("FG::Compile");
        m_framegraph->Compile();
    }

    {
        ZoneScopedN("FG::Execute");
        m_framegraph->Execute();
    }

    if (m_gpuCullingManager && psDeviceFlags.test(rsStatistic))
    {
        m_gpuCullingManager->ScheduleStatsReadback(m_renderContext->GetCommandList());
    }

    m_hasPrevFrameData = true;
    m_prevViewProj = Device.mFullTransform;
    m_prevCameraPos = Device.vCameraPosition;

    if (m_gpuProfiler)
    {
        m_gpuProfiler->FrameEnd();
    }

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
    ZoneScopedN("FrameGraphRenderer::RenderMenu");

    if (!m_enabled) return;

    VERIFY(m_framegraph != nullptr);

    if (m_gpuProfiler)
    {
        m_gpuProfiler->SetEnabled(xray::profiler::IsEnabled());
        m_gpuProfiler->FrameStart();
    }

    if (m_device && m_device->GetFGResourceManager()) {
        m_device->GetFGResourceManager()->Update(Device.fTimeDelta);
    }

    m_framegraph->ResetForNextFrame();

    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

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

    auto sceneWithUI = passes::setupUIPass(*m_framegraph, backgroundTarget, width, height);

    sceneWithUI = passes::setupTextPass(*m_framegraph, sceneWithUI, width, height, m_passStates->uiText);

    sceneWithUI = passes::setupCursorPass(*m_framegraph, sceneWithUI, width, height);

    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        sceneWithUI,
        framegraph::VirtualResourceHandle(),
        backbufferHandle,
        width,
        height,
        m_passStates->tonemap
    );

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

    m_framegraph->SetRenderContext(m_renderContext.get());
    m_framegraph->SetGPUProfiler(m_gpuProfiler.get());
    m_framegraph->SetAsyncCompute(nullptr, nullptr);
    m_framegraph->Compile();
    m_framegraph->Execute();

    if (m_gpuProfiler)
        m_gpuProfiler->FrameEnd();

}

void FrameGraphRenderer::RenderStatsOverlay()
{
    if (m_statsOverlay && psDeviceFlags.test(rsStatistic))
    {
        xray::profiler::RenderStats stats;
        stats.Reset();

        if (m_geometryCollector)
        {
            const auto& batches = m_geometryCollector->GetBatches();
            stats.totalBatches = static_cast<u32>(batches.size());

            xr_set<IRenderVisual*> uniqueSkeletons;

            for (const auto& batch : batches)
            {
                u32 triangles = batch.indexCount / 3;
                stats.totalTriangles += triangles;

                if (batch.isSkinned)
                {
                    stats.skinnedBatches++;
                    stats.skinnedTriangles += triangles;

                    if (batch.renderable)
                    {
                        IRenderVisual* rootVisual = batch.renderable->GetRenderData().visual;
                        if (rootVisual && uniqueSkeletons.find(rootVisual) == uniqueSkeletons.end())
                        {
                            uniqueSkeletons.insert(rootVisual);
                            stats.skinnedMeshes++;

                            IKinematics* K = rootVisual->dcast_PKinematics();
                            if (K)
                            {
                                u32 boneCount = K->LL_BoneCount();
                                stats.totalBones += boneCount;
                                if (boneCount > stats.maxBonesPerMesh)
                                    stats.maxBonesPerMesh = boneCount;
                            }
                        }
                    }
                }
                else if (batch.isTerrain)
                {
                    stats.terrainBatches++;
                    stats.terrainTriangles += triangles;
                }
                else if (batch.isStatic)
                {
                    stats.staticBatches++;
                    stats.staticTriangles += triangles;
                }
                else
                {
                    stats.dynamicBatches++;
                    stats.dynamicTriangles += triangles;
                }
            }
        }

        stats.particleBatches = static_cast<u32>(m_worldParticleBatches.size() + m_hudParticleBatches.size());

        if (m_gpuCullingManager)
        {
            stats.objectsSubmitted = m_gpuCullingManager->GetStaticObjectCount() +
                                     m_gpuCullingManager->GetDynamicObjectCount() +
                                     m_gpuCullingManager->GetTerrainObjectCount();

            const auto& cullStats = m_gpuCullingManager->GetCullingStats();
            stats.objectsVisible = cullStats.totalVisible();
            stats.objectsCulled = (stats.objectsSubmitted > stats.objectsVisible)
                                  ? (stats.objectsSubmitted - stats.objectsVisible)
                                  : 0;

            stats.megaBufferVertices = m_gpuCullingManager->GetTotalVertexCount();
            stats.megaBufferIndices = m_gpuCullingManager->GetTotalIndexCount();

            const auto& skinnedCullStats = m_gpuCullingManager->GetSkinnedCullingStats();
            stats.skinnedSubmitted = skinnedCullStats.submitted;
            stats.skinnedVisible = skinnedCullStats.visible;
            stats.skinnedCulled = skinnedCullStats.culled;
        }

        if (m_detailManager)
        {
            stats.detailSlots = m_detailManager->slot_count;
            for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++)
                stats.detailTrisPerBlade[lod] = m_detailManager->bladeIndexCount[lod] / 3;

            const auto& cullStats = m_detailManager->GetCullingStats();
            stats.detailVisibleSlots = cullStats.visibleSlotsCount;
            stats.detailVisibleLOD0 = cullStats.visibleLOD0Count;
            stats.detailVisibleLOD1 = cullStats.visibleLOD1Count;
            stats.detailVisibleLOD2 = cullStats.visibleLOD2Count;
            stats.detailVisibleDecals = cullStats.visibleDecalCount;
            stats.detailGeneratedInstances = m_detailManager->totalGeneratedInstances;
            stats.detailVisibleCapacity = m_detailManager->visibleBufferCapacity;
            stats.detailDecalCapacity = std::max(m_detailManager->visibleBufferCapacity / 4, 10000u);
        }

        m_statsOverlay->SetRenderStats(stats);
        m_statsOverlay->SetInspectorPreview(m_inspectorPreview ? m_inspectorPreview.Get() : nullptr);
        m_statsOverlay->SetVisible(true);
        m_statsOverlay->Render();
    }
}

void FrameGraphRenderer::SetupFrame() {
    const bool levelLoaded = g_pGamePersistent && g_pGameLevel;

    if (m_gpuCullingManager) {
        m_gpuCullingManager->ProcessStatsReadback();
        m_gpuCullingManager->ProcessSkinnedVisibilityReadback();
    }

    if (m_detailManager && m_device) {
        m_detailManager->ProcessStatsReadback(m_device->GetNVRHIDevice());
    }

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

    m_worldParticleBatches.clear();
    m_hudParticleBatches.clear();

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

    framegraph::ResourceDesc depthDesc;
    depthDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    depthDesc.debugName = "rt_Depth";
    depthDesc.width = width;
    depthDesc.height = height;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;

    framegraph::VirtualResourceHandle depthBuffer = m_framegraph->CreateTexture("rt_Depth", depthDesc);

    framegraph::ResourceDesc normalDesc;
    normalDesc.type = framegraph::ResourceDesc::Type::Texture2D;
    normalDesc.debugName = "rt_Normal";
    normalDesc.width = width;
    normalDesc.height = height;
    normalDesc.format = nvrhi::Format::RGBA16_FLOAT;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    framegraph::VirtualResourceHandle normalBuffer = m_framegraph->CreateTexture("rt_Normal", normalDesc);

    passes::HiZPyramidOutput hizOutput;
    hizOutput.pyramid = framegraph::VirtualResourceHandle();
    hizOutput.mipLevels = 0;
    hizOutput.width = width / 2;
    hizOutput.height = height / 2;

    bool hasPrevDepth = m_hasPrevFrameData && m_prevFrameDepth &&
                        m_prevFrameWidth == width && m_prevFrameHeight == height;

    if (hasPrevDepth) {
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

        hizOutput = passes::setupHiZBuildPass(
            *m_framegraph,
            m_device,
            prevDepthHandle,
            width,
            height,
            m_passStates->hiZBuild
        );
    }

    m_hizPyramid = hizOutput.pyramid;
    if (m_hizPyramid.is_valid())
        m_framegraph->GetRTRegistry().RegisterRT("rt_HiZ", m_hizPyramid);

    framegraph::VirtualResourceHandle drawArgsBuffer;

    if (m_gpuCullingManager && hizOutput.pyramid.is_valid()) {
        m_gpuCullingManager->Initialize(m_device);

        if (m_detailManager) {
            static bool detailShadersLoaded = false;
            if (!detailShadersLoaded) {
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                m_detailManager->LoadCullComputeShader(shaderLoader);
                m_detailManager->LoadInstanceGenShader(shaderLoader);
                m_detailManager->LoadPrefixSumShaders(shaderLoader);
                m_detailManager->LoadGraphicsShaders(shaderLoader);

                if (!m_detailManager->computePipeline) {
                    m_detailManager->CreateComputePipeline(m_device);
                }
                if (!m_detailManager->instanceGenPipeline) {
                    m_detailManager->CreateInstanceGenPipeline(m_device);
                }
                if (!m_detailManager->prefixSumScanPipeline) {
                    m_detailManager->CreatePrefixSumPipeline(m_device);
                }

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

        if (m_gpuCullingManager->IsEnabled()) {

            auto cullOutput = m_gpuCullingManager->SetupCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                m_geometryCollector.get(),
                m_prevViewProj
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

        if (m_gpuCullingManager->IsSkinnedCullingEnabled()) {
            m_gpuCullingManager->SetupSkinnedCullingPass(
                *m_framegraph,
                m_hizPyramid,
                hizOutput.width,
                hizOutput.height,
                hizOutput.mipLevels,
                m_geometryCollector.get(),
                m_prevViewProj
            );
        }
    }

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
        height,
        m_passStates->sky
    );

    auto sunOutput = passes::setupSunPass(
        *m_framegraph,
        m_device,
        skyOutput,
        g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr,
        width,
        height,
        m_passStates->sun
    );

    passes::BindlessForwardConfig bindlessConfig;
    if (m_gpuCullingManager && m_gpuCullingManager->IsCompactionEnabled()) {
        bindlessConfig.enabled = true;

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

        if (m_gpuCullingManager->AreMegaBuffersReady()) {
            bindlessConfig.megaVertexBuffer = m_gpuCullingManager->GetMegaVertexBuffer();
            bindlessConfig.megaIndexBuffer = m_gpuCullingManager->GetMegaIndexBuffer();
            bindlessConfig.megaBuffersReady = true;
        }

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

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
            bindlessConfig.variantPartition = m_gpuCullingManager->GetStaticPartition().ToConfig();
    }

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        sunOutput,
        normalBuffer,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height,
        drawArgsBuffer,
        bindlessConfig,
        &m_passStates->forwardColor
    );

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

    static auto skinnedStatsCallback = +[](u32 rendered, u32 culled, void* userData) {
        static_cast<GPUCullingManager*>(userData)->UpdateSkinnedCullingStats(rendered, culled);
    };

    static auto visibilityByVisualCallback = +[](const dxRender_Visual* visual, void* userData) -> u32 {
        return static_cast<GPUCullingManager*>(userData)->GetSkinnedVisibilityByVisual(visual);
    };

    passes::SkinnedVisibilityData skinnedVisibility;
    if (m_gpuCullingManager && m_gpuCullingManager->IsSkinnedCullingEnabled()) {
        skinnedVisibility.enabled = m_gpuCullingManager->HasSkinnedVisibilityData();
        skinnedVisibility.visibilityByVisualCallback = visibilityByVisualCallback;
        skinnedVisibility.visibilityByVisualUserData = m_gpuCullingManager.get();
        skinnedVisibility.statsCallback = skinnedStatsCallback;
        skinnedVisibility.statsUserData = m_gpuCullingManager.get();
    }

    auto hudOutputs = passes::setupSkinningPass(
        *m_framegraph,
        m_device,
        forwardOutputs,
        m_geometryCollector.get(),
        &m_hudBatches,
        m_materialCache.get(),
        width,
        height,
        skinnedVisibility,
        &m_passStates->skinning
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
        hizOutput.pyramid,
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels,
        &m_passStates->particle
    );

    passes::setupDetailCullPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        hizOutput.pyramid,
        hizOutput.width,
        hizOutput.height,
        hizOutput.mipLevels,
        m_hasPrevFrameData ? &m_prevViewProj : nullptr,
        m_gpuProfiler.get(),
        &m_passStates->detail
    );

    auto detailOutputs = passes::setupDetailPass(
        *m_framegraph,
        m_device,
        m_detailManager.get(),
        particleOutputs,
        width,
        height,
        m_gpuProfiler.get()
    );

    passes::TransparentPassConfig transparentConfig;
    if (m_gpuCullingManager && m_gpuCullingManager->GetTransparentObjectCount() > 0) {
        transparentConfig.megaVertexBuffer = bindlessConfig.megaVertexBuffer;
        transparentConfig.megaIndexBuffer = bindlessConfig.megaIndexBuffer;
        transparentConfig.instanceBuffer = m_gpuCullingManager->GetTransparentInstanceBuffer();
        transparentConfig.compactDrawArgsBuffer = m_gpuCullingManager->GetTransparentCompactDrawArgsBuffer();
        transparentConfig.compactBatchIndicesBuffer = m_gpuCullingManager->GetTransparentCompactBatchIndicesBuffer();
        transparentConfig.compactMaterialIDBuffer = m_gpuCullingManager->GetTransparentCompactMaterialIDBuffer();
        transparentConfig.compactCountBuffer = m_gpuCullingManager->GetTransparentCompactCountBuffer();
        transparentConfig.objectCount = m_gpuCullingManager->GetTransparentObjectCount();

        if (m_gpuCullingManager->IsVariantPartitionEnabled())
            transparentConfig.variantPartition = m_gpuCullingManager->GetTransparentPartition().ToConfig();
    }

    auto transparentOutputs = passes::setupTransparentPass(
        *m_framegraph,
        m_device,
        detailOutputs,
        transparentConfig,
        width, height,
        m_passStates->transparent
    );

    passes::ExposureConfig exposureConfig = passes::GetDefaultExposureConfig();
    auto exposureOutput = passes::setupExposurePass(
        *m_framegraph,
        m_device,
        transparentOutputs.albedo,
        exposureConfig,
        Device.fTimeDelta,
        width,
        height,
        m_passStates->exposure
    );

    m_exposureTexture = exposureOutput.exposureTexture;

    auto sceneWithUI = passes::setupUIPass(
        *m_framegraph,
        transparentOutputs.albedo,
        width,
        height
    );

    sceneWithUI = passes::setupTextPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height,
        m_passStates->uiText
    );

    sceneWithUI = passes::setupCursorPass(
        *m_framegraph,
        sceneWithUI,
        width,
        height
    );

    auto ldrOutput = passes::setupTonemapPass(
        *m_framegraph,
        sceneWithUI,
        exposureOutput.exposureTexture,
        backbufferHandle,
        width,
        height,
        m_passStates->tonemap,
        &m_passStates->exposure
    );

    m_framegraph->GetRTRegistry().RegisterRT("rt_SceneColor", skyColorHandle);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Depth", depthBuffer);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Normal", transparentOutputs.normal);
    m_framegraph->GetRTRegistry().RegisterRT("rt_Exposure", exposureOutput.exposureTexture);

    if (m_statsOverlay && psDeviceFlags.test(rsStatistic) && m_inspectorPreview)
    {
        auto rtNames = m_framegraph->GetRTRegistry().GetAllNames();
        m_statsOverlay->SetInspectorRTList(rtNames);

        auto selectedName = m_statsOverlay->GetSelectedRTName();
        if (selectedName.size() > 0)
        {
            auto selectedHandle = m_framegraph->GetRTRegistry().TryGetRT(selectedName.c_str());
            if (selectedHandle.is_valid())
            {
                framegraph::ResourceDesc previewDesc;
                previewDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                previewDesc.width = 512;
                previewDesc.height = 512;
                previewDesc.format = nvrhi::Format::RGBA16_FLOAT;
                previewDesc.isUAV = true;
                previewDesc.isImported = true;
                previewDesc.debugName = "debug_preview";
                auto previewHandle = m_framegraph->ImportTexture(
                    "debug_preview", m_inspectorPreview.Get(), previewDesc);

                struct DebugPreviewData {
                    framegraph::VirtualResourceHandle source;
                    framegraph::VirtualResourceHandle dest;
                    ng::RenderDevice* device;
                    u32 sourceW, sourceH;
                    int channelMode;
                    int mipLevel;
                };

                auto& previewData = m_framegraph->addCallbackPass<DebugPreviewData>(
                    "Debug Preview",
                    [&, selectedHandle, previewHandle](framegraph::FrameGraph& builder, framegraph::PassHandle pass, DebugPreviewData& data) {
                        data.device = m_device;
                        data.channelMode = m_statsOverlay ? m_statsOverlay->GetChannelMode() : 0;
                        data.mipLevel = m_statsOverlay ? m_statsOverlay->GetSelectedMipLevel() : 0;
                        data.source = selectedHandle;
                        data.dest = previewHandle;
                        auto& srcDesc = builder.GetResourceDesc(selectedHandle);
                        data.sourceW = std::max(1u, srcDesc.width >> data.mipLevel);
                        data.sourceH = std::max(1u, srcDesc.height >> data.mipLevel);
                        if (m_statsOverlay) {
                            m_statsOverlay->SetSelectedRTMipCount(srcDesc.mipLevels);
                            m_statsOverlay->SetSelectedRTSize(srcDesc.width, srcDesc.height);
                        }
                        builder.PassRead(pass, selectedHandle, framegraph::ResourceState::ShaderResource);
                        builder.PassWrite(pass, previewHandle, framegraph::ResourceState::UnorderedAccess);
                    },
                    [](const DebugPreviewData& data, const framegraph::FrameGraph& fg, ng::RenderContext* ctx) {
                        static ref_cs s_debugPreviewCS;
                        static nvrhi::ComputePipelineHandle s_pipeline;
                        static nvrhi::BindingLayoutHandle s_layout;
                        static nvrhi::BufferHandle s_cb;
                        static bool s_init = false;

                        nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                        if (!s_init) {
                            s_debugPreviewCS.create("debug_preview");
                            if (!s_debugPreviewCS || !s_debugPreviewCS->nvrhiShader) return;

                            nvrhi::BindingLayoutDesc layoutDesc;
                            layoutDesc.visibility = nvrhi::ShaderType::Compute;
                            layoutDesc.bindings = {
                                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                                nvrhi::BindingLayoutItem::Texture_SRV(0),
                                nvrhi::BindingLayoutItem::Texture_UAV(0),
                            };
                            s_layout = nvDevice->createBindingLayout(layoutDesc);

                            nvrhi::ComputePipelineDesc pipeDesc;
                            pipeDesc.CS = s_debugPreviewCS->nvrhiShader;
                            pipeDesc.bindingLayouts = { s_layout };
                            s_pipeline = nvDevice->createComputePipeline(pipeDesc);

                            nvrhi::BufferDesc cbDesc;
                            cbDesc.debugName = "DebugPreviewCB";
                            cbDesc.byteSize = 32;
                            cbDesc.isConstantBuffer = true;
                            cbDesc.isVolatile = true;
                            cbDesc.maxVersions = 16;
                            cbDesc.keepInitialState = true;
                            cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                            s_cb = nvDevice->createBuffer(cbDesc);

                            s_init = true;
                        }

                        if (!s_pipeline) return;

                        nvrhi::ITexture* srcTex = fg.GetPhysicalTexture(data.source);
                        nvrhi::ITexture* dstTex = fg.GetPhysicalTexture(data.dest);
                        if (!srcTex || !dstTex) return;

                        struct {
                            u32 outputW, outputH;
                            u32 sourceW, sourceH;
                            u32 mode;
                            u32 mipLevel;
                            u32 pad[2];
                        } cb;
                        cb.outputW = 512; cb.outputH = 512;
                        cb.sourceW = data.sourceW; cb.sourceH = data.sourceH;
                        cb.mode = (u32)data.channelMode;
                        cb.mipLevel = (u32)data.mipLevel;
                        cb.pad[0] = cb.pad[1] = 0;

                        nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                        cmdList->writeBuffer(s_cb, &cb, sizeof(cb));

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, s_cb),
                            nvrhi::BindingSetItem::Texture_SRV(0, srcTex),
                            nvrhi::BindingSetItem::Texture_UAV(0, dstTex),
                        };
                        auto bindings = nvDevice->createBindingSet(bindDesc, s_layout);
                        if (!bindings) return;

                        ctx->SetComputePipeline(s_pipeline.Get());
                        ctx->SetComputeBindingSet(0, bindings.Get());
                        ctx->Dispatch((512 + 7) / 8, (512 + 7) / 8, 1);
                    }
                );
            }
        }
    }

    ng::ImGuiRendererNVRHI* imguiRenderer = RImplementation.GetImGuiRendererNVRHI();
    auto finalOutput = passes::setupImGuiPass(
        *m_framegraph,
        ldrOutput,
        imguiRenderer,
        width,
        height
    );

    m_finalOutput = finalOutput;

    {
        nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

        if (!m_prevFrameDepth || m_prevFrameWidth != width || m_prevFrameHeight != height) {
            nvrhi::TextureDesc prevDepthDesc;
            prevDepthDesc.width = width;
            prevDepthDesc.height = height;
            prevDepthDesc.format = nvrhi::Format::D32;
            prevDepthDesc.isShaderResource = true;
            prevDepthDesc.debugName = "PrevFrameDepth";
            prevDepthDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            prevDepthDesc.keepInitialState = true;

            m_prevFrameDepth = nvDevice->createTexture(prevDepthDesc);
            m_prevFrameWidth = width;
            m_prevFrameHeight = height;
            if (m_prevFrameDepth)
                Msg("* [TemporalHiZ] Created persistent depth buffer: %dx%d", width, height);
        }

        if (m_prevFrameDepth) {
            framegraph::ResourceDesc prevDepthImportDesc;
            prevDepthImportDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            prevDepthImportDesc.debugName = "rt_PrevDepthCopyDest";
            prevDepthImportDesc.width = width;
            prevDepthImportDesc.height = height;
            prevDepthImportDesc.format = nvrhi::Format::D32;
            prevDepthImportDesc.isDepthStencil = true;
            prevDepthImportDesc.isImported = true;
            prevDepthImportDesc.isTransient = false;

            auto prevDepthCopyDest = m_framegraph->ImportTexture("rt_PrevDepthCopyDest", m_prevFrameDepth, prevDepthImportDesc);

            auto finalDepth = transparentOutputs.depth;
            framegraph::PassHandle depthCopyPass = m_framegraph->AddPass("DepthCopy");
            m_framegraph->PassRead(depthCopyPass, finalDepth, framegraph::ResourceState::CopySource);
            m_framegraph->PassWrite(depthCopyPass, prevDepthCopyDest, framegraph::ResourceState::CopyDest);
            m_framegraph->SetPassCallback(depthCopyPass,
                [finalDepth, prevDepthCopyDest](ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
                    nvrhi::ITexture* src = fg.GetPhysicalTexture(finalDepth);
                    nvrhi::ITexture* dst = fg.GetPhysicalTexture(prevDepthCopyDest);
                    if (src && dst)
                        ctx.GetCommandList()->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
                }
            );
        }
    }
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

void FrameGraphRenderer::RenderHUD() {
}

bool FrameGraphRenderer::ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable, bool isStatic) {
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

    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

    u32 visualType = visual->getType();
    if (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM) {
        u16 renderMode = 0;
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            renderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            renderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    GeometryBatch batch;

    batch.vertexBuffer = nvrhiVB;
    batch.indexBuffer = nvrhiIB;

    bool isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    if (isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_PM) {
            const FSlideWindowItem& swi = static_cast<CSkeletonX_PM*>(visual)->GetSWI();
            if (swi.sw && swi.count > 0) {
                const FSlideWindow& sw = swi.sw[0];
                batch.indexCount = sw.num_tris * 3;
                batch.startIndex = sw.offset;
            } else {
                batch.indexCount = meshVisual->iCount;
                batch.startIndex = 0;
            }
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = 0;
        }
        batch.baseVertex = 0;
    } else if (visualType == MT_PROGRESSIVE) {
        const FSlideWindowItem& swi = static_cast<FProgressive*>(visual)->GetSWI();
        if (swi.sw && swi.count > 0) {
            const FSlideWindow& sw = swi.sw[0];
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else if (visualType == MT_TREE_PM) {
        const FSlideWindowItem* pSWI = static_cast<FTreeVisual_PM*>(visual)->GetSWI();
        if (pSWI && pSWI->sw && pSWI->count > 0) {
            const FSlideWindow& sw = pSWI->sw[0];
            batch.indexCount = sw.num_tris * 3;
            batch.startIndex = meshVisual->iBase + sw.offset;
        } else {
            batch.indexCount = meshVisual->iCount;
            batch.startIndex = meshVisual->iBase;
        }
        batch.baseVertex = meshVisual->vBase;
    } else {
        batch.indexCount = meshVisual->iCount;
        batch.startIndex = meshVisual->iBase;
        batch.baseVertex = meshVisual->vBase;
    }
    batch.vertexStride = meshVisual->vStride;

    batch.worldMatrix = worldTransform;

    batch.visual = visual;

    batch.renderable = renderable;

    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
    batch.isStatic = isStatic;

    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    if (visualType == MT_TREE_ST || visualType == MT_TREE_PM) {
        batch.worldBoundsCenter = visual->vis.sphere.P;
        batch.worldBoundsRadius = visual->vis.sphere.R;
    } else {
        worldTransform.transform_tiny(batch.worldBoundsCenter, visual->vis.sphere.P);
        batch.worldBoundsRadius = visual->vis.sphere.R;
    }

    float distSQ = Device.vCameraPosition.distance_to_sqr(batch.worldBoundsCenter) + EPS;
    batch.ssa = batch.worldBoundsRadius / distSQ;

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
        Msg("! [ProcessVisualGeometry] ERROR: Created batch with null buffers! VB=%p, IB=%p",
            nvrhiVB.Get(), nvrhiIB.Get());
        return false;
    }

    if (m_materialCache) {
        if (m_materialCache->IsTerrainMaterial(visual)) {
            batch.isTerrain = true;
            batch.terrainMaterialID = m_materialCache->PreRegisterTerrainMaterial(visual);
        } else {
            batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
        }
    }

    if (m_gpuCullingManager && m_gpuCullingManager->AreMegaBuffersReady()) {
        batch.megaBufferAlloc = m_gpuCullingManager->GetMeshAllocation(
            meshVisual->vbPoolID, batch.baseVertex, meshVisual->vCount,
            meshVisual->ibPoolID, batch.startIndex, batch.indexCount,
            meshVisual->useAlternativeGeom
        );

        static int s_allocDebug = 0;
        if (s_allocDebug < 10 && !batch.megaBufferAlloc.valid) {
            Msg("! [MegaBuffer] Invalid alloc: vbPool=%u, vBase=%u, vCount=%u, ibPool=%u, iBase=%u, iCount=%u, alt=%d",
                meshVisual->vbPoolID, batch.baseVertex, meshVisual->vCount,
                meshVisual->ibPoolID, batch.startIndex, batch.indexCount,
                meshVisual->useAlternativeGeom ? 1 : 0);
            s_allocDebug++;
        }
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

    nvrhi::BufferHandle nvrhiVB = geom->vb;
    nvrhi::BufferHandle nvrhiIB = geom->ib;

    if (!nvrhiVB || !nvrhiIB)
        return false;

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

    u32 visualType = visual->getType();
    batch.isSkinned = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);

    if (batch.isSkinned) {
        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            batch.skinningRenderMode = static_cast<CSkeletonX_ST*>(visual)->RenderMode;
        } else {
            batch.skinningRenderMode = static_cast<CSkeletonX_PM*>(visual)->RenderMode;
        }
    }

    if (m_materialCache) {
        batch.bindlessMaterialID = m_materialCache->PreRegisterBindlessMaterial(visual);
    }

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
    u32 particleCount = 0;
    shared_str textureName;

    if (vType == MT_PARTICLE_EFFECT) {
        RENDER_NAMESPACE::PS::CParticleEffect* pEffect =
            static_cast<RENDER_NAMESPACE::PS::CParticleEffect*>(visual);
        isHUDParticle = pEffect->GetHudMode();

        PAPI::Particle* particles = nullptr;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

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
            pV->CalculateBones_InvalidateFG();
            pV->CalculateBonesFG(TRUE);

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

    auto& dsgraph = RImplementation.get_imm_context();
    u32 submittedStatic = 0;

    if (!m_staticBatchesCached && !dsgraph.Sectors.empty()) {
        Msg("* [GeomCache] Building static geometry cache from %zu sectors...", dsgraph.Sectors.size());

        xr_vector<dxRender_Visual*> staticVisuals;
        xr_set<dxRender_Visual*> uniqueVisuals;

        for (CSector* sector : dsgraph.Sectors) {
            if (sector && sector->root()) {
                ExtractStaticLeafVisuals(sector->root(), staticVisuals);
            }
        }

        for (dxRender_Visual* v : staticVisuals) {
            uniqueVisuals.insert(v);
        }

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

        const auto& allBatches = m_geometryCollector->GetBatches();
        m_cachedStaticBatches.assign(allBatches.begin() + batchCountBefore, allBatches.end());
        m_staticBatchesCached = true;

        Msg("* [GeomCache] Cached %zu static batches from %zu unique visuals (total sectors: %zu)",
            m_cachedStaticBatches.size(), uniqueVisuals.size(), dsgraph.Sectors.size());
    }
    else if (m_staticBatchesCached) {
        for (const auto& batch : m_cachedStaticBatches) {
            m_geometryCollector->Submit(batch);
            submittedStatic++;
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

}

void FrameGraphRenderer::RouteBatchesToPasses() {
    auto& batches = m_geometryCollector->GetBatchesMutable();

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
}

}
