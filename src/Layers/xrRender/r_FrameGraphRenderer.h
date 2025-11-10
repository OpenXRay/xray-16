#pragma once

#include "xrEngine/IFrameGraphRender.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderPhaseCache.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
#include "Layers/xrRender/FrameGraphPasses/HUDPass.h"
#include "Layers/xrRender/FrameGraphPasses/ParticlePass.h"
#include "Layers/xrRender/FrameGraphPasses/LightingPass.h"
#include "Layers/xrRender/FrameGraphPasses/TonemapPass.h"
#include "Layers/xrRender/FrameGraphPasses/UIPass.h"
#include "Layers/xrRender/FrameGraphPasses/TextPass.h"
#include "Layers/xrRender/FrameGraphPasses/MenuDistortPass.h"
#include "Layers/xrRender/FrameGraphPasses/MenuCompositePass.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

struct Fmatrix;
struct ImDrawData;

namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
}

namespace xray::render::ng {
    class ImGuiRendererNVRHI;
}

namespace xray::render {
using RENDER_NAMESPACE::dxRender_Visual;

class FrameGraphRenderer: public IFrameGraphRender, public pureRender {
public:
    FrameGraphRenderer();
    ~FrameGraphRenderer();

    bool Initialize(ng::RenderDevice* device);
    void Shutdown();

    void Render() override;
    void RenderMenu() override;
    void SetEnabled(bool enabled) override { m_enabled = enabled; }
    bool IsEnabled() const override { return m_enabled; }

    void OnRender();

    void RenderImGui(ImDrawData* drawData, ng::ImGuiRendererNVRHI* imguiRenderer);

    ng::RenderContext* GetRenderContext() const { return m_renderContext.get(); }

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

private:
    bool m_enabled = false;
    ng::RenderDevice* m_device = nullptr;

    xr_unique_ptr<framegraph::FrameGraph> m_framegraph;

    xr_unique_ptr<framegraph::ShaderPhaseCache> m_shaderPhaseCache;

    framegraph::VirtualResourceHandle m_finalOutput;

    ng::TextureHandle m_native_Position;
    ng::TextureHandle m_native_Normal;
    ng::TextureHandle m_native_Albedo;
    ng::TextureHandle m_native_Depth;
    ng::TextureHandle m_native_Accumulator;
    ng::TextureHandle m_native_Generic_0;
    ng::TextureHandle m_native_Generic_1;
    ng::TextureHandle m_native_Generic_2;

    ng::TextureHandle m_native_MenuMain;
    ng::TextureHandle m_native_MenuDistort;

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

    xr_unique_ptr<passes::GBufferPass> m_gbufferPass;
    xr_unique_ptr<passes::HUDPass> m_hudPass;
    xr_unique_ptr<passes::ParticlePass> m_particlePass;
    xr_unique_ptr<passes::LightingPass> m_lightingPass;
    xr_unique_ptr<passes::TonemapPass> m_tonemapPass;

    xr_unique_ptr<passes::UIPass> m_uiPass;
    xr_unique_ptr<passes::TextPass> m_textPass;
    xr_unique_ptr<passes::MenuDistortPass> m_menuDistortPass;
    xr_unique_ptr<passes::MenuCompositePass> m_menuCompositePass;

    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    xr_vector<GeometryBatch> m_hudBatches;

    xr_vector<passes::ParticleBatch> m_worldParticleBatches;
    xr_vector<passes::ParticleBatch> m_hudParticleBatches;

    xr_unique_ptr<ng::RenderContext> m_renderContext;

    xr_map<ID3D11Buffer*, nvrhi::BufferHandle> m_bufferHandleCache;

    Stats m_stats;

    xr_vector<ISpatial*> m_lstRenderables;

    void SetupFrame();

    void BuildFrameGraphStructure();

    void SetupFrameGraphPasses();

    void PresentToBackbuffer();

    void CollectVisibleGeometry();

    void RenderHUD();

    framegraph::VirtualResourceHandle CreateRT(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format,
        bool isDepthStencil = false
    );

    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr);
    bool ProcessHudGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr);
    bool ProcessParticleGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr, bool isHUD = false);
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
