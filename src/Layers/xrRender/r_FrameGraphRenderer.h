#pragma once

#include "xrEngine/IFrameGraphRender.h"
#include "xrEngine/Render.h"
#include "xrCDB/xrXRC.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderPhaseCache.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/Profiler/StatsOverlay.h"

struct ImDrawData;

namespace CDB { class MODEL; }
class xrXRC;

namespace xray::render::fg {
    class dxRender_Visual;
    class RTAccelStructManager;
    class CDetailManager;
    class CWallmarksEngine;
    namespace PS {
        class CParticleEffect;
    }
    namespace decals {
        class DecalManager;
        class OverlayManager;
    }
}

namespace xray::render::framegraph {
    class Blackboard;
    class ShaderLoader;
}

namespace xray::render::fg::passes {
    struct ParticleBatch;
    class SmokeTrailManager;
}

namespace xray::render::fg {
    class ImGuiRendererNVRHI;
}

namespace xray::render {
using fg::dxRender_Visual;

class GeometryCollector;
class MaterialCache;

namespace fg {
    class GPUCullingManager;
    class FGDetailManager;
}

namespace ui {
    class UIRenderCollector;
    class NVRHIUIRenderer;
}

namespace framegraph {
    class VolatileConstantBufferPool;
}

class FrameGraphRenderer: public IFrameGraphRender{
public:
    FrameGraphRenderer();
    ~FrameGraphRenderer();

    bool Initialize(fg::RenderDevice* device);
    void Shutdown();

    void Render() override;
    void RenderMenu() override;
    void RenderStatsOverlay() override;
    void SetEnabled(bool enabled) override { m_enabled = enabled; }
    bool IsEnabled() const override { return m_enabled; }

    void RenderImGui(ImDrawData* drawData, fg::ImGuiRendererNVRHI* imguiRenderer);

    fg::RenderContext* GetRenderContext() const { return m_renderContext.get(); }

    struct Stats {
        float totalFrameMs = 0.0f;
        float gbufferMs = 0.0f;
        float lightingMs = 0.0f;
        float tonemapMs = 0.0f;
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void PrintStats() const;

    xray::profiler::GPUProfiler* GetGPUProfiler() const { return m_gpuProfiler.get(); }
    xray::profiler::StatsOverlay* GetStatsOverlay() const { return m_statsOverlay.get(); }
    void ToggleStatsOverlay() { if (m_statsOverlay) m_statsOverlay->ToggleVisible(); }

    fg::RenderDevice* GetRenderDevice() const override { return m_device; }
    framegraph::ShaderLoader* GetShaderLoader() const override { return m_shaderLoader; }
    fg::ImGuiRendererNVRHI* GetImGuiRendererNVRHI() const override { return m_imguiRendererNVRHI; }
    void SetImGuiRendererNVRHI(fg::ImGuiRendererNVRHI* r) { m_imguiRendererNVRHI = r; }
    MaterialCache* GetMaterialCache() const override { return m_materialCache.get(); }
    MaterialCache* GetUIMaterialCache() const override { return m_uiMaterialCache.get(); }
    ui::UIRenderCollector* GetUICollector() const override { return m_uiCollector.get(); }
    ui::NVRHIUIRenderer* GetUIRenderer() const override { return m_uiRenderer.get(); }
    MaterialCache* GetTextMaterialCache() const override { return m_textMaterialCache.get(); }
    framegraph::VolatileConstantBufferPool* GetTextVCBPool() const { return m_textVCBPool.get(); }

    void UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode) override;
    void NotifySmokeShot() override;
    fg::passes::SmokeTrailManager* GetSmokeTrailManager() const { return m_smokeTrailManager.get(); }

    fg::GPUCullingManager* GetGPUCullingManager() const { return m_gpuCullingManager.get(); }

    fg::FGDetailManager* GetDetailManager() const { return m_detailManager.get(); }

    fg::decals::DecalManager* GetDecalManager() const { return m_decalManager.get(); }
    fg::decals::OverlayManager* GetOverlayManager() const { return m_overlayManager.get(); }

public:
    CDB::MODEL* m_pRmPortals{ nullptr };
    xrXRC m_Sectors_xrc{ "render" };
    IRender_Sector::sector_id_t m_last_sector_id{ IRender_Sector::INVALID_SECTOR_ID };
    IRender_Sector::sector_id_t m_largest_sector_id{ IRender_Sector::INVALID_SECTOR_ID };
    u32 m_uLastLTRACK{ 0 };
    Task* m_pProcessHOMTask{ nullptr };
    bool m_bFirstFrameAfterReset{ false };
    xr_vector<Fbox3> m_main_coarse_structure;
    fg::CDetailManager* m_pDetailManager{ nullptr };
    fg::CWallmarksEngine* m_pWallmarksEngine{ nullptr };

private:
    bool m_enabled = false;
    fg::RenderDevice* m_device = nullptr;
    framegraph::ShaderLoader* m_shaderLoader = nullptr;
    fg::ImGuiRendererNVRHI* m_imguiRendererNVRHI = nullptr;

    xr_unique_ptr<framegraph::Blackboard> m_blackboard;

    xr_unique_ptr<framegraph::FrameGraph> m_framegraph;

    xr_unique_ptr<framegraph::ShaderPhaseCache> m_shaderPhaseCache;

    framegraph::VirtualResourceHandle m_finalOutput;

    fg::TextureHandle m_native_Position;
    fg::TextureHandle m_native_Normal;
    fg::TextureHandle m_native_Albedo;
    fg::TextureHandle m_native_Depth;
    fg::TextureHandle m_native_Accumulator;
    fg::TextureHandle m_native_Generic_0;
    fg::TextureHandle m_native_Generic_1;
    fg::TextureHandle m_native_Generic_2;

    fg::TextureHandle m_native_MenuMain;
    fg::TextureHandle m_native_MenuDistort;
    fg::TextureHandle m_native_FinalComposite;

    framegraph::VirtualResourceHandle m_rt_Position;
    framegraph::VirtualResourceHandle m_rt_Normal;
    framegraph::VirtualResourceHandle m_rt_Albedo;
    framegraph::VirtualResourceHandle m_rt_Depth;
    framegraph::VirtualResourceHandle m_rt_Accumulator;
    framegraph::VirtualResourceHandle m_rt_Generic_0;
    framegraph::VirtualResourceHandle m_rt_Generic_1;
    framegraph::VirtualResourceHandle m_rt_Generic_2;
    framegraph::VirtualResourceHandle m_backbuffer;

    framegraph::VirtualResourceHandle m_rt_MenuMain;
    framegraph::VirtualResourceHandle m_rt_MenuDistort;
    framegraph::VirtualResourceHandle m_rt_FinalComposite;

    framegraph::VirtualResourceHandle m_exposureTexture;

    framegraph::VirtualResourceHandle m_hizPyramid;

    nvrhi::TextureHandle m_prevFrameDepth;
    nvrhi::TextureHandle m_normals[2];
    nvrhi::TextureHandle m_worldPos[2];
    u32 m_pingPongIndex = 0;

    Fmatrix m_prevViewProj;
    Fvector m_prevCameraPos;
    bool m_hasPrevFrameData = false;
    u32 m_prevFrameWidth = 0;
    u32 m_prevFrameHeight = 0;

    nvrhi::TextureHandle m_inspectorPreview;

    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_geometryVCBPool;
    xr_unique_ptr<MaterialCache> m_materialCache;

    xr_unique_ptr<fg::GPUCullingManager> m_gpuCullingManager;

    xr_unique_ptr<fg::FGDetailManager> m_detailManager;

    xr_unique_ptr<fg::decals::DecalManager> m_decalManager;

    xr_unique_ptr<fg::decals::OverlayManager> m_overlayManager;

    xr_unique_ptr<fg::passes::SmokeTrailManager> m_smokeTrailManager;

    xr_unique_ptr<fg::RTAccelStructManager> m_rtAccelMgr;
    u32 m_ptSampleIndex = 0;
    Fvector m_ptPrevCameraPos = {0, 0, 0};
    Fvector m_ptPrevCameraDir = {0, 0, 0};
    int m_ptPrevBounces = 0;
    bool m_ptWasEnabled = false;

    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_uiVCBPool;
    xr_unique_ptr<MaterialCache> m_uiMaterialCache;

    xr_unique_ptr<MaterialCache> m_textMaterialCache;
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_textVCBPool;

    xr_vector<GeometryBatch> m_hudBatches;

    xr_vector<fg::passes::ParticleBatch> m_worldParticleBatches;
    xr_vector<fg::passes::ParticleBatch> m_hudParticleBatches;

    xr_vector<GeometryBatch> m_cachedStaticBatches;
    bool m_staticBatchesCached = false;

    xr_unique_ptr<fg::RenderContext> m_renderContext;

    xr_map<ID3D11Buffer*, nvrhi::BufferHandle> m_bufferHandleCache;

    Stats m_stats;

    xr_unique_ptr<xray::profiler::GPUProfiler> m_gpuProfiler;
    xr_unique_ptr<xray::profiler::StatsOverlay> m_statsOverlay;

    xr_vector<ISpatial*> m_lstRenderables;

    void SetupFrame();

    void SetupFrameGraphPasses();

    void CollectVisibleGeometry();

    framegraph::VirtualResourceHandle CreateRT(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format,
        bool isDepthStencil = false
    );

    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr, bool isStatic = false);
    bool ProcessHudGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr);
    bool ProcessParticleGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr, bool isHUD = false);
    void ProcessSingleParticleEffect(fg::PS::CParticleEffect* pEffect, const Fmatrix& worldTransform, IRenderable* renderable, bool isHUD);
    void ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs);

    struct PassEntry {
        framegraph::RenderPhase phase;
        xr_unique_ptr<framegraph::IPass> pass;
        xr_vector<GeometryBatch*> assignedBatches;
    };

    xr_vector<PassEntry> m_activePasses;

    xr_set<framegraph::RenderPhase> ScanRequiredPhases() const;

    void CreatePhasePass(framegraph::RenderPhase phase);

    void RouteBatchesToPasses();

    void CreateAllRequiredPasses();

    public:

        void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& xform);
};

}
