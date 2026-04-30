// xrRender/r_FrameGraphRenderer.h
#pragma once

#include "xrEngine/IFrameGraphRender.h"
#include "xrEngine/Render.h"
#include "Layers/xrRender/FGRenderBase.h"
#include "xrCDB/xrXRC.h"
#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/r__occlusion.h"
#include "Layers/xrRender/Light_Render_Direct.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender_R2/SMAP_Allocator.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderPhaseCache.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/Profiler/StatsOverlay.h"

// Forward declarations
struct ImDrawData;

namespace CDB { class MODEL; }
class xrXRC;

namespace xray::render::fg {
    class dxRender_Visual;
    class RTAccelStructManager;
    class CDetailManager;
    class CWallmarksEngine;
    class CRenderTarget;
    class light;
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

extern xr_vector<xr_string> g_failedShaders;

// Forward declarations
class GeometryCollector;
class MaterialCache;

struct CompiledLevelShader {
    shared_str shaderName;
    shared_str textureName;
    nvrhi::ShaderHandle vsHandle;
    nvrhi::ShaderHandle psHandle;
    xr_unique_ptr<framegraph::ExtractedReflection> vsReflection;
    xr_unique_ptr<framegraph::ExtractedReflection> psReflection;
    MaterialSystem::MaterialInfo materialInfo;
    struct PrecompiledPSOs {
        struct PSOVariant {
            u32 vertexFormatID;
            RenderPassType passType;
            fg::PipelineState* pso;
            MaterialPSO* materialPSO;
        };
        xr_vector<PSOVariant> variants;
        xr_map<u64, MaterialPSO*> psoCache;
    };
    PrecompiledPSOs precompiledPSOs;
};

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

// ══════════════════════════════════════════════════════════
//  FRAMEGRAPH RENDERER
// ══════════════════════════════════════════════════════════

class FrameGraphRenderer: public IFrameGraphRender, public xray::render::fg::FGRenderBase {
public:
    FrameGraphRenderer();
    ~FrameGraphRenderer();

    GenerationLevel GetGeneration() const override { return IRender::GENERATION_R2; }
    IRender::BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D11; }
    bool is_sun_static() override { return false; }
    u32 get_dx_level() override { return 0x000B0000; }

    void create() override;
    void destroy() override;
    void reset_begin() override;
    void reset_end() override;
    void rmNear(fg::CBackend& cmd_list);
    void rmFar(fg::CBackend& cmd_list);
    void rmNormal(fg::CBackend& cmd_list);

    void level_Load(IReader* fs) override;
    void level_Unload() override;
    HRESULT shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result) override;
    void addShaderOption(pcstr name, pcstr value);
    void clearAllShaderOptions() { m_ShaderOptions.clear(); }
private:
    using VertexDeclarator = ::xray::render::fg::VertexDeclarator;
    void LoadBuffers(CStreamReader* fs, bool alternative);
    void LoadVisuals(IReader* fs);
    void LoadLights(IReader* fs);
    void LoadSectors(IReader* fs);
    void LoadSWIs(CStreamReader* fs);
    void CompileLevelShader(u32 shaderID, const char* shaderName, const char* textureName);
    void PrecompileLevelPSOs();
    bool IsVertexFormatCompatible(const VertexDeclarator& decl, const framegraph::ExtractedReflection* vsReflection);
    bool MatchesSemanticName(const fg::VertexElement& elem, const xr_string& semanticName);
    bool IsFormatCompatible(u8 d3dFormat, nvrhi::Format nvrhiFormat);
    u32 GetVertexStride(u32 vertexFormatID);
    bool CreatePrecompiledPSO(u32 shaderID, u32 vertexFormatID, RenderPassType passType,
                               nvrhi::Format colorFormat, nvrhi::Format depthFormat, MaterialCache* materialCache);
    void SetupDepthState(RenderPassType passType, const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc);
    void SetupBlendState(const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc);
public:

    pcstr getShaderPath() override { return "r5\\"; }
    IRenderVisual* getVisual(int id) override;

    void add_Visual(u32, IRenderable* root, IRenderVisual* V, Fmatrix& m) override;
    void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void clear_static_wallmarks() override;
    void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size) override;

    IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    void ros_destroy(IRender_ObjectSpecific*& p) override;
    IRender_Light* light_create() override;
    IRender_Glow* glow_create() override;

    IRenderVisual* model_CreateParticles(pcstr name) override;
    IRenderVisual* model_Create(pcstr name, IReader* data) override;
    IRenderVisual* model_CreateChild(pcstr name, IReader* data) override;
    IRenderVisual* model_Duplicate(IRenderVisual* V) override;
    void model_Delete(IRenderVisual*& V, bool bDiscard) override;
    void model_Logging(bool bEnable) override;
    void models_Prefetch() override;
    void models_Clear(bool b_complete) override;

    bool occ_visible(vis_data& V) override;
    bool occ_visible(Fbox& B) override;
    bool occ_visible(sPoly& P) override;

    void Calculate() override;
    void BeforeWorldRender() override {}
    void AfterWorldRender() override {}

    void OnFrame() override;
    void OnCameraUpdated() override;
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    void Screenshot(IRender::ScreenshotMode mode, pcstr name) override;
    void SetPostProcessParams(const SPPInfo&) override;
    void RequestGrassInteraction(const Fvector&, float, float, uint8_t) override;

    // Initialize
    bool Initialize(fg::RenderDevice* device);
    void Shutdown();

    // IFrameGraphRender interface
    void Render() override;
    void RenderMenu() override;
    void RenderStatsOverlay() override;
    void SetEnabled(bool enabled) override { m_enabled = enabled; }
    bool IsEnabled() const override { return m_enabled; }

    // Render ImGui onto final output (called after FrameGraph execution)
    void RenderImGui(ImDrawData* drawData, fg::ImGuiRendererNVRHI* imguiRenderer);

    // Access RenderContext
    fg::RenderContext* GetRenderContext() const { return m_renderContext.get(); }

    // Statistics
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

    // Profiler access
    xray::profiler::GPUProfiler* GetGPUProfiler() const { return m_gpuProfiler.get(); }
    xray::profiler::StatsOverlay* GetStatsOverlay() const { return m_statsOverlay.get(); }
    void ToggleStatsOverlay() { if (m_statsOverlay) m_statsOverlay->ToggleVisible(); }

    // Accessors for lambda passes to access shared infrastructure (override IFrameGraphRender)
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

    // Smoke trail interface
    void UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode) override;
    void NotifySmokeShot() override;
    fg::passes::SmokeTrailManager* GetSmokeTrailManager() const { return m_smokeTrailManager.get(); }

    // GPU Culling Manager accessor (for level loading integration)
    fg::GPUCullingManager* GetGPUCullingManager() const { return m_gpuCullingManager.get(); }

    // Detail Manager accessor (for level loading integration)
    fg::FGDetailManager* GetDetailManager() const { return m_detailManager.get(); }

    // Decal Manager accessor (for wallmark routing)
    fg::decals::DecalManager* GetDecalManager() const { return m_decalManager.get(); }
    fg::decals::OverlayManager* GetOverlayManager() const { return m_overlayManager.get(); }

public:
    struct _options
    {
        u32 bug : 1;

        u32 ssao_blur_on : 1;
        u32 ssao_opt_data : 1;
        u32 ssao_half_data : 1;
        u32 ssao_hbao : 1;
        u32 ssao_hdao : 1;
        u32 ssao_ultra : 1;
        u32 hbao_vectorized : 1;

        u32 rain_smapsize : 16;
        u32 smapsize : 16;
        u32 depth16 : 1;
        u32 mrt : 1;
        u32 mrtmixdepth : 1;
        u32 fp16_filter : 1;
        u32 fp16_blend : 1;
        u32 albedo_wo : 1;
        u32 HW_smap : 1;
        u32 HW_smap_PCF : 1;
        u32 HW_smap_FETCH4 : 1;

        u32 HW_smap_FORMAT : 32;

        u32 nvstencil : 1;
        u32 nvdbt : 1;

        u32 nullrt : 1;
        u32 ffp : 1;

        u32 distortion : 1;
        u32 distortion_enabled : 1;
        u32 mblur : 1;

        u32 sunfilter : 1;
        u32 sunstatic : 1;
        u32 sjitter : 1;
        u32 noshadows : 1;
        u32 Tshadows : 1;
        u32 oldshadowcascades : 1;
        u32 disasm : 1;
        u32 advancedpp : 1;
        u32 volumetricfog : 1;

        u32 msaa : 1;
        u32 msaa_hybrid : 1;
        u32 msaa_opt : 1;
        u32 gbuffer_opt : 1;
        u32 dx11_sm4_1 : 1;
        u32 msaa_alphatest : 2;
        u32 msaa_samples : 4;

        u32 minmax_sm : 2;
        u32 minmax_sm_screenarea_threshold;

        u32 tessellation : 1;

        u32 forcegloss : 1;
        u32 forceskinw : 1;

        u32 mt_calculate : 1;
        u32 mt_render : 1;

        u32 support_rt_arrays : 1;

        float forcegloss_v;
    } o;

    struct Statistics
    {
        u32 l_total{ 0 };
        u32 l_visible{ 0 };
        u32 l_shadowed{ 0 };
        u32 l_unshadowed{ 0 };
        s32 s_used{ 0 };
        s32 s_merged{ 0 };
        s32 s_finalclip{ 0 };
        u32 ic_total{ 0 };
        u32 ic_culled{ 0 };

        void FrameStart()
        {
            l_total = 0;
            l_visible = 0;
            l_shadowed = 0;
            l_unshadowed = 0;
            s_used = 0;
            s_merged = 0;
            s_finalclip = 0;
            ic_total = 0;
            ic_culled = 0;
        }
        void FrameEnd() {}
    };

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
    fg::CHOM m_HOM;
    fg::R_occlusion m_HWOCC;
    Statistics m_Stats;
    fg::CLight_Compute_XFORM_and_VIS m_LR;
    xr_vector<fg::light*> m_Lights_LastFrame;
    fg::SMAP_Allocator m_LP_smap_pool;
    fg::CRenderTarget* m_pTarget{ nullptr };
    fg::CPSLibrary m_PSLibrary;
#if defined(USE_DX11)
    xr_vector<D3D_SHADER_MACRO> m_ShaderOptions;
#elif defined(USE_OGL)
    xr_string m_ShaderOptions;
#endif

    xr_vector<CompiledLevelShader> m_CompiledLevelShaders;

private:
    bool m_enabled = false;
    fg::RenderDevice* m_device = nullptr;
    framegraph::ShaderLoader* m_shaderLoader = nullptr;
    fg::ImGuiRendererNVRHI* m_imguiRendererNVRHI = nullptr;

    xr_unique_ptr<framegraph::Blackboard> m_blackboard;

    // FrameGraph
    xr_unique_ptr<framegraph::FrameGraph> m_framegraph;

    // Shader phase cache (Week 16 - for precompilation phase detection)
    xr_unique_ptr<framegraph::ShaderPhaseCache> m_shaderPhaseCache;

    // Final output texture (for copying to backbuffer)
    framegraph::VirtualResourceHandle m_finalOutput;

    // ═══════════════════════════════════════════════════
    //  NATIVE NVRHI RENDER TARGETS (Phase 1 Migration)
    // ═══════════════════════════════════════════════════
    // Created once via NativeRTFactory, imported into FrameGraph

    // Native handles (owned by FGResourceManager)
    fg::TextureHandle m_native_Position;
    fg::TextureHandle m_native_Normal;
    fg::TextureHandle m_native_Albedo;
    fg::TextureHandle m_native_Depth;
    fg::TextureHandle m_native_Accumulator;
    fg::TextureHandle m_native_Generic_0;
    fg::TextureHandle m_native_Generic_1;
    fg::TextureHandle m_native_Generic_2;

    // Menu-specific native RTs (for main menu rendering pipeline)
    fg::TextureHandle m_native_MenuMain;      // Main UI RT (replaces rt_Generic_0 in menu)
    fg::TextureHandle m_native_MenuDistort;   // Distortion mask RT (replaces rt_Generic_1 in menu)
    fg::TextureHandle m_native_FinalComposite; // Final composited output (scene + UI)

    // FrameGraph virtual handles (imported from native RTs)
    framegraph::VirtualResourceHandle m_rt_Position;
    framegraph::VirtualResourceHandle m_rt_Normal;
    framegraph::VirtualResourceHandle m_rt_Albedo;
    framegraph::VirtualResourceHandle m_rt_Depth;
    framegraph::VirtualResourceHandle m_rt_Accumulator;
    framegraph::VirtualResourceHandle m_rt_Generic_0;
    framegraph::VirtualResourceHandle m_rt_Generic_1;
    framegraph::VirtualResourceHandle m_rt_Generic_2;
    framegraph::VirtualResourceHandle m_backbuffer;

    // Menu-specific virtual handles
    framegraph::VirtualResourceHandle m_rt_MenuMain;      // Main UI rendering
    framegraph::VirtualResourceHandle m_rt_MenuDistort;   // Distortion mask
    framegraph::VirtualResourceHandle m_rt_FinalComposite; // Final composited output (scene + UI)

    // Exposure texture (1x1 R32_FLOAT) for sky and tonemap passes
    framegraph::VirtualResourceHandle m_exposureTexture;

    // Hi-Z pyramid (R32_FLOAT with mip chain) for GPU occlusion culling
    // Generated from depth prepass, used by GPU culling and froxel volumetrics
    framegraph::VirtualResourceHandle m_hizPyramid;

    // ═══════════════════════════════════════════════════
    //  TEMPORAL HI-Z (Previous Frame Depth Reuse)
    // ═══════════════════════════════════════════════════
    // Instead of depth prepass, reuse previous frame's depth for Hi-Z
    // Eliminates double vertex processing cost (~1.5-2ms savings)
    nvrhi::TextureHandle m_prevFrameDepth;
    nvrhi::TextureHandle m_normals[2];
    nvrhi::TextureHandle m_worldPos[2];
    u32 m_pingPongIndex = 0;

    Fmatrix m_prevViewProj;                       // Previous frame's view-projection
    Fvector m_prevCameraPos;                      // Previous frame's camera position
    bool m_hasPrevFrameData = false;              // Valid previous frame exists
    u32 m_prevFrameWidth = 0;                     // Previous frame resolution
    u32 m_prevFrameHeight = 0;

    // Debug preview texture for Render Inspector (persistent, not part of framegraph)
    nvrhi::TextureHandle m_inspectorPreview;

    // Passes
    //xr_unique_ptr<passes::GBufferPass> m_gbufferPass;
    //xr_unique_ptr<passes::HUDPass> m_hudPass;
    //xr_unique_ptr<passes::ParticlePass> m_particlePass;
    //xr_unique_ptr<passes::LightingPass> m_lightingPass;
    //xr_unique_ptr<passes::TonemapPass> m_tonemapPass;

    // UI rendering passes (5-step pipeline - works for menu AND in-game)
    //xr_unique_ptr<passes::UIPass> m_uiPass;                   // Step 1: Render UI sprites/widgets
    //xr_unique_ptr<passes::TextPass> m_textPass;               // Step 2: Render text/fonts on top
    //xr_unique_ptr<passes::CursorPass> m_cursorPass;           // Step 3: Render cursor on top of all
    //xr_unique_ptr<passes::MenuDistortPass> m_menuDistortPass; // Step 4: Render distortion mask
    //xr_unique_ptr<passes::MenuCompositePass> m_menuCompositePass; // Step 5: Composite to output

    // Geometry collector
    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    // Material cache (for shader/PSO management)
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_geometryVCBPool;
    xr_unique_ptr<MaterialCache> m_materialCache;

    // GPU Culling Manager (Phase 3.5: Hi-Z occlusion culling)
    xr_unique_ptr<fg::GPUCullingManager> m_gpuCullingManager;

    // Detail Manager (Framegraph: grass/vegetation rendering)
    xr_unique_ptr<fg::FGDetailManager> m_detailManager;

    // Decal Manager (screen-space box decals replacing legacy wallmarks)
    xr_unique_ptr<fg::decals::DecalManager> m_decalManager;

    // Overlay Manager (per-NPC UV-space overlay textures for baked decals)
    xr_unique_ptr<fg::decals::OverlayManager> m_overlayManager;

    // Smoke Trail Manager (GPU weapon muzzle smoke)
    xr_unique_ptr<fg::passes::SmokeTrailManager> m_smokeTrailManager;

    // Ray Tracing acceleration structures (for path tracer)
    xr_unique_ptr<fg::RTAccelStructManager> m_rtAccelMgr;
    u32 m_ptSampleIndex = 0;
    Fvector m_ptPrevCameraPos = {0, 0, 0};
    Fvector m_ptPrevCameraDir = {0, 0, 0};
    int m_ptPrevBounces = 0;
    bool m_ptWasEnabled = false;

    // UI rendering infrastructure (shared by UI/Text/Cursor passes)
    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_uiVCBPool;
    xr_unique_ptr<MaterialCache> m_uiMaterialCache;

    // Text rendering infrastructure (shared by TextPass lambda)
    xr_unique_ptr<MaterialCache> m_textMaterialCache;
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_textVCBPool;

    // HUD geometry (separate from world geometry)
    xr_vector<GeometryBatch> m_hudBatches;

    // Particle systems (collected during same spatial query as geometry)
    xr_vector<fg::passes::ParticleBatch> m_worldParticleBatches;  // World-space particles
    xr_vector<fg::passes::ParticleBatch> m_hudParticleBatches;    // HUD particles (need FOV adjustment)

    // ═══════════════════════════════════════════════════════
    //  STATIC GEOMETRY CACHE (collected once, reused every frame)
    // ═══════════════════════════════════════════════════════
    // Static geometry from sector hierarchies doesn't change - cache it!
    // Only dynamic objects (from spatial DB) need per-frame collection
    xr_vector<GeometryBatch> m_cachedStaticBatches;
    bool m_staticBatchesCached = false;

    // RenderContext for execution
    xr_unique_ptr<fg::RenderContext> m_renderContext;

    // Buffer handle cache (D3D11 buffer ptr → NVRHI handle)
    xr_map<ID3D11Buffer*, nvrhi::BufferHandle> m_bufferHandleCache;

    // Statistics
    Stats m_stats;

    // Profiler (GPU timing + ImGui overlay)
    xr_unique_ptr<xray::profiler::GPUProfiler> m_gpuProfiler;
    xr_unique_ptr<xray::profiler::StatsOverlay> m_statsOverlay;

    // ═══════════════════════════════════════════════════
    //  CACHED SPATIAL QUERIES (like R_dsgraph_structure)
    // ═══════════════════════════════════════════════════
    // Populated once per frame, reused across passes
    xr_vector<ISpatial*> m_lstRenderables;

    // Frame setup
    void SetupFrame();

    // FrameGraph passes (called per-frame in Render)
    void SetupFrameGraphPasses();

    // Visibility & culling (CPU-based for now, will move to GPU later)
    void CollectVisibleGeometry();

    // Helper: Create render target (DRY helper for BuildFrameGraph)
    framegraph::VirtualResourceHandle CreateRT(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format,
        bool isDepthStencil = false
    );

    // Helper functions for geometry collection
    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr, bool isStatic = false);
    bool ProcessHudGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr);
    bool ProcessParticleGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform, IRenderable* renderable = nullptr, bool isHUD = false);
    void ProcessSingleParticleEffect(fg::PS::CParticleEffect* pEffect, const Fmatrix& worldTransform, IRenderable* renderable, bool isHUD);
    void ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs);

    // ═══════════════════════════════════════════════════
    //  DYNAMIC PASS ROUTING (Week 16)
    // ═══════════════════════════════════════════════════

    // Pass registry entry
    struct PassEntry {
        framegraph::RenderPhase phase;
        xr_unique_ptr<framegraph::IPass> pass;
        xr_vector<GeometryBatch*> assignedBatches;
    };

    // Active passes for this frame (dynamically created)
    xr_vector<PassEntry> m_activePasses;

    // Scan materials to determine required phases
    xr_set<framegraph::RenderPhase> ScanRequiredPhases() const;

    // Create passes based on required phases
    void CreatePhasePass(framegraph::RenderPhase phase);

    // Route batches to appropriate passes
    void RouteBatchesToPasses();

    // Create all required passes dynamically
    void CreateAllRequiredPasses();

    public:

        // ═══════════════════════════════════════════════════
        //  GAME OBJECT RENDERING CALLBACK INTEGRATION
        // ═══════════════════════════════════════════════════
        // Called by game objects via CRender::add_Visual() during renderable_Render()
        // This is the bridge that allows game objects to add their visuals, attachments, and HUD items
        void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& xform);
};

} // namespace xray::render
