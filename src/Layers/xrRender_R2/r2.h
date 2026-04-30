#pragma once

#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/r__buffer_pool.h"
#include "Layers/xrRender/r__occlusion.h"

#include "Layers/xrRender/PSLibrary.h"

#include "r2_types.h"

#include "Layers/xrRender/HOM.h"
#include "Layers/xrRenderDX11/DetailManager.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/WallmarksEngine.h"

#include "SMAP_Allocator.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/Light_Render_Direct.h"
#include "Layers/xrRender/LightTrack.h"
#include "Layers/xrRender/r_sun_cascades.h"

#include "xrEngine/IRenderable.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCore/Threading/TaskManager.hpp"
#include "xrCore/FMesh.hpp"

// D3D12: Need full MaterialSystem definition for MaterialInfo
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::fg
{
class RenderDevice;
class ImGuiRendererNVRHI;
}

namespace xray::render
{
class FrameGraphRenderer;
struct MaterialPSO;
}

namespace xray::render::fg
{
class PipelineState;
}

namespace xray::render::framegraph
{
class ShaderLoader;
struct ExtractedReflection;
}

namespace xray::render::fg
{
class CRenderTarget;
class dxRender_Visual;

// TODO: move it into separate file.
struct i_render_phase
{
    explicit i_render_phase(const xr_string& name_in)
        : name(name_in)
    {
        o.active = false;
        o.mt_calc_enabled = false;
        o.mt_draw_enabled = false;
    }

    virtual ~i_render_phase() = default;

    ICF void run()
    {
        if (!o.active)
            return;

        main_task = &TaskScheduler->CreateTask([this]
        {
            calculate();

            if (o.mt_draw_enabled)
            {
                draw_task = &TaskScheduler->AddTask(*main_task, [this]
                {
                    render();
                });
            }
        });

        if (o.mt_calc_enabled)
        {
            TaskScheduler->PushTask(*main_task);
        }
        else
        {
            TaskScheduler->RunTask(*main_task);
        }
    }

    ICF void sync()
    {
        if (main_task)
            TaskScheduler->Wait(*main_task);
        main_task = nullptr;

        if (o.mt_draw_enabled && draw_task)
        {
            // draw task should be finished as sub task of main
            VERIFY(draw_task->IsFinished());
            draw_task = nullptr;
        }
        else
        {
            render();
        }

        flush();

        o.active = false;
    }

    virtual void init() = 0;
    virtual void calculate() = 0;
    virtual void render() = 0;
    virtual void flush() {}

    struct options_t
    {
        u32 active : 1;
        u32 mt_calc_enabled : 1;
        u32 mt_draw_enabled : 1;
    } o;
    Task* main_task{ nullptr };
    Task* draw_task{ nullptr };
    xr_string name{ "<UNKNOWN>" };
};

struct render_main : public i_render_phase
{
    render_main() : i_render_phase("main_render") {}

    void init() override;
    void calculate() override;
    void render() override;
};

struct render_rain : public i_render_phase
{
    render_rain() : i_render_phase("rain_render") {}

    void init() override;
    void calculate() override;
    void render() override;
    void flush() override;

    light RainLight;
    u32 context_id{ R_dsgraph_structure::INVALID_CONTEXT_ID };
    float rain_factor{ 0.0f };
};

struct render_sun : public i_render_phase
{
    render_sun() : i_render_phase("sun_render") {}

    void init() override;
    void calculate() override;
    void render() override;
    void flush() override;

    void accumulate_cascade(u32 cascade_ind);

    sun::cascade m_sun_cascades[R__NUM_SUN_CASCADES];
    light* sun{ nullptr };
    bool need_to_render_sunshafts{ false };
    bool last_cascade_chain_mode{ false };

    u32 contexts_ids[R__NUM_SUN_CASCADES];
};

struct render_sun_old : public i_render_phase
{
    render_sun_old() : i_render_phase("sun_render_old") {}

    void init() override;
    void calculate() override {}
    void render() override;
    void flush() override;

    void render_sun();
    void render_sun_near();
    void render_sun_filtered() const;

    xr_vector<sun::cascade> m_sun_cascades;
    xr_vector<Fbox> s_casters;
    light* sun{ nullptr };
    u32 context_id{ R_dsgraph_structure::INVALID_CONTEXT_ID };
};
//----

// definition
class CRender final : public D3DXRenderBase
{
public:
    enum
    {
        PHASE_NORMAL = 0, // E[0]
        PHASE_SMAP = 1, // E[1]
    };

    enum
    {
        MSAA_ATEST_NONE = 0x0, //	Hi bit - DX10.1 mode
        MSAA_ATEST_DX10_0_ATOC = 0x1, //	Lo bit - ATOC mode
        MSAA_ATEST_DX10_1_NATIVE = 0x2,
        MSAA_ATEST_DX10_1_ATOC = 0x3,
    };

    enum
    {
        MMSM_OFF = 0,
        MMSM_ON,
        MMSM_AUTO,
        MMSM_AUTODETECT
    };

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
        u32 albedo_wo : 1; // work-around albedo on less capable HW
        u32 HW_smap : 1;
        u32 HW_smap_PCF : 1;
        u32 HW_smap_FETCH4 : 1;

        u32 HW_smap_FORMAT : 32;

        u32 nvstencil : 1;
        u32 nvdbt : 1;

        u32 nullrt : 1;
        u32 ffp : 1; // don't use shaders, only fixed-function pipeline or software processing

        u32 distortion : 1;
        u32 distortion_enabled : 1;
        u32 mblur : 1;

        u32 sunfilter : 1;
        u32 sunstatic : 1;
        u32 sjitter : 1;
        u32 noshadows : 1;
        u32 Tshadows : 1; // transluent shadows
        u32 oldshadowcascades : 1;
        u32 disasm : 1;
        u32 advancedpp : 1; //	advanced post process (DOF, SSAO, volumetrics, etc.)
        u32 volumetricfog : 1;

        u32 msaa : 1; // DX10.0 path
        u32 msaa_hybrid : 1; // DX10.0 main path with DX10.1 A-test msaa allowed
        u32 msaa_opt : 1; // DX10.1 path
        u32 gbuffer_opt : 1;
        u32 dx11_sm4_1 : 1; // DX10.1 path
        u32 msaa_alphatest : 2; //	A-test mode
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

public:

    xr_vector<FSlideWindowItem>& SWIs = ::xray::render::fg::BufferPool.SWIs;

    // ═══════════════════════════════════════════════════
    //  D3D12: CompiledLevelShader (replaces ref_shader + ShaderNameEntry)
    // ═══════════════════════════════════════════════════
    struct CompiledLevelShader {
        // Metadata
        shared_str shaderName;
        shared_str textureName;

        // NVRHI shader handles (replaces ref_shader)
        nvrhi::ShaderHandle vsHandle;
        nvrhi::ShaderHandle psHandle;

        // Reflection data (needed for PSO creation)
        xr_unique_ptr<framegraph::ExtractedReflection> vsReflection;
        xr_unique_ptr<framegraph::ExtractedReflection> psReflection;

        // Material info (alpha test, transparency, etc.)
        MaterialSystem::MaterialInfo materialInfo;

        // Precompiled PSOs (indexed by vertex format + pass type)
        struct PrecompiledPSOs {
            struct PSOVariant {
                u32 vertexFormatID;
                RenderPassType passType;
                fg::PipelineState* pso;
                MaterialPSO* materialPSO;
            };

            xr_vector<PSOVariant> variants;
            xr_map<u64, MaterialPSO*> psoCache;  // hash(vertexFormatID, passType) → MaterialPSO*
        };

        PrecompiledPSOs precompiledPSOs;
    };

    xr_vector<CompiledLevelShader> CompiledLevelShaders;  // D3D12: Replaces legacy Shaders + ShaderNames

    using VertexDeclarator = ::xray::render::fg::VertexDeclarator;
    xr_vector<VertexDeclarator>&    nDC = ::xray::render::fg::BufferPool.nDC;
    xr_vector<VertexDeclarator>&    xDC = ::xray::render::fg::BufferPool.xDC;
    xr_vector<VertexStagingBuffer>& nVB = ::xray::render::fg::BufferPool.nVB;
    xr_vector<VertexStagingBuffer>& xVB = ::xray::render::fg::BufferPool.xVB;
    xr_vector<IndexStagingBuffer>&  nIB = ::xray::render::fg::BufferPool.nIB;
    xr_vector<IndexStagingBuffer>&  xIB = ::xray::render::fg::BufferPool.xIB;
    xr_vector<dxRender_Visual*>&    Visuals = ::xray::render::fg::BufferPool.Visuals;
    CPSLibrary PSLibrary;


    CRenderTarget* Target; // Render-target



    bool& m_fast_geom_loaded = ::xray::render::fg::BufferPool.fastGeomLoaded;

private:
    // Loading / Unloading
    void LoadBuffers(CStreamReader* fs, bool alternative);
    void LoadVisuals(IReader* fs);
    void LoadLights(IReader* fs);
    void LoadSectors(IReader* fs);
    void LoadSWIs(CStreamReader* fs);
#if RENDER != R_R2
    void Load3DFluid();
#endif

    // D3D12: Shader compilation and PSO precompilation (private helpers)
    void CompileLevelShader(u32 shaderID, const char* shaderName, const char* textureName);
    void PrecompileLevelPSOs();
    bool IsVertexFormatCompatible(const VertexDeclarator& decl, const framegraph::ExtractedReflection* vsReflection);
    bool MatchesSemanticName(const VertexElement& elem, const xr_string& semanticName);
    bool IsFormatCompatible(u8 d3dFormat, nvrhi::Format nvrhiFormat);
    u32 GetVertexStride(u32 vertexFormatID);
    bool CreatePrecompiledPSO(u32 shaderID, u32 vertexFormatID, RenderPassType passType,
                               nvrhi::Format colorFormat, nvrhi::Format depthFormat, MaterialCache* materialCache);
    void SetupDepthState(RenderPassType passType, const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc);
    void SetupBlendState(const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc);

public:
    // D3D12: Public PSO cache key computation (used by MaterialCache)

    void render_forward();
    void render_indirect(light* L) const;
    void render_lights(light_Package& LP);

    render_main r_main;
#if RENDER != R_R2
    render_rain r_rain;
#endif

    render_sun r_sun;
    render_sun_old r_sun_old;

public:
    ShaderElement* rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    ShaderElement* rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    VertexElement*       getVB_Format(int id, bool alternative = false) { return ::xray::render::fg::BufferPool.getVB_Format(id, alternative); }
    VertexStagingBuffer* getVB(int id, bool alternative = false)        { return ::xray::render::fg::BufferPool.getVB(id, alternative); }
    IndexStagingBuffer*  getIB(int id, bool alternative = false)        { return ::xray::render::fg::BufferPool.getIB(id, alternative); }
    FSlideWindowItem*    getSWI(int id)                                 { return ::xray::render::fg::BufferPool.getSWI(id); }
    IRenderVisual* model_CreatePE(LPCSTR name);

    u32 occq_begin(u32& ID);
    void occq_end(u32& ID);
    R_occlusion::occq_result occq_get(u32& ID);

    ICF void apply_object(CBackend& cmd_list, IRenderable* O)
    {
        if (!O || !O->renderable_ROS())
            return;

        CROS_impl& LT = *(CROS_impl*)O->renderable_ROS();
        LT.update_smooth(O);
        cmd_list.o_hemi = 0.75f * LT.get_hemi();
        // o_hemi						= 0.5f*LT.get_hemi			()	;
        cmd_list.o_sun = 0.75f * LT.get_sun();
        CopyMemory(cmd_list.o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES * sizeof(float));
    }

public:
    // feature level
    GenerationLevel GetGeneration() const override { return IRender::GENERATION_R2; }
    bool is_sun_static() override { return o.sunstatic; }

#if defined(USE_DX11)
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D11; }
    u32 get_dx_level() override { return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 ? 0x000A0001 : 0x000A0000; }
    pcstr getShaderPath() override { return "r5\\"; }
#elif defined(USE_OGL)
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::OpenGL; }
    u32 get_dx_level() override { return /*HW.pDevice1?0x000A0001:*/0x000A0000; }
    pcstr getShaderPath() override { return "gl\\"; }
#else
#   error No graphics API selected or enabled!
#endif

    [[nodiscard]]
    bool IsFastGeomSupported() const
    {
        return m_fast_geom_loaded;
    }

    // Loading / Unloading
    void create() override;
    void destroy() override;
    void reset_begin() override;
    void reset_end() override;

    void level_Load(IReader*) override;
    void level_Unload() override;

#if defined(USE_DX11)
    ID3DBaseTexture* texture_load(pcstr fname, u32& msize);
#elif defined(USE_OGL)
    GLuint           texture_load(pcstr fname, u32& msize, GLenum& ret_desc);
#else
#   error No graphics API selected or enabled!
#endif

    HRESULT shader_compile(pcstr name, IReader* fs,
        pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result) override;

    // Information
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    // D3D12/NVRHI: New shader access (replaces getShader/getShaderNames)
    CompiledLevelShader* getCompiledShader(int id);
    bool getShaderHandles(int id, nvrhi::ShaderHandle& outVS, nvrhi::ShaderHandle& outPS);

    // Legacy D3D11: Old shader access
    ref_shader getShader(int id);
    bool getShaderNames(int id, shared_str& outShaderName, shared_str& outTextureName);

    IRenderVisual* getVisual(int id) override;

    // Main
    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m) override; // add visual leaf	(no culling performed at all)
    // wallmarks
    void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void clear_static_wallmarks() override;
    void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    void add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
                              const Fvector& dir, float size);
    void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
                              const Fvector& dir, float size) override;

    //
    IBlender* blender_create(CLASS_ID cls);
    void blender_destroy(IBlender*&);

    //
    IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    void ros_destroy(IRender_ObjectSpecific*&) override;

    // Lighting
    IRender_Light* light_create() override;
    IRender_Glow* glow_create() override;

    // Models
    IRenderVisual* model_CreateParticles(LPCSTR name) override;
    IRender_DetailModel* model_CreateDM(IReader* F);
    IRenderVisual* model_Create(LPCSTR name, IReader* data = nullptr) override;
    IRenderVisual* model_CreateChild(LPCSTR name, IReader* data) override;
    IRenderVisual* model_Duplicate(IRenderVisual* V) override;
    void model_Delete(IRenderVisual*& V, bool bDiscard) override;
    void model_Delete(IRender_DetailModel*& F);
    void model_Logging(bool bEnable) override { g_pModelPool->Logging(bEnable); }
    void models_Prefetch() override;
    void models_Clear(bool b_complete) override;

    // Occlusion culling
    bool occ_visible(vis_data& V) override;
    bool occ_visible(Fbox& B) override;
    bool occ_visible(sPoly& P) override;

    // Main
    void OnCameraUpdated() override;

    void Calculate() override;
    void Render() override;
    void RenderMenu() override;

    void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = nullptr) override;
    void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Procedure is called before world render and post-effects
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Procedure is called after world render and before UI

    void SetPostProcessParams(const SPPInfo& ppi) override;

#ifdef USE_OGL
    RenderContext GetCurrentContext() const override;
    void MakeContextCurrent(RenderContext context) override;
#endif

    // Render mode
    void rmNear(CBackend& cmd_list);
    void rmFar(CBackend& cmd_list);
    void rmNormal(CBackend& cmd_list);

    // Constructor/destructor/loader
    CRender();
    ~CRender() override;

    void addShaderOption(pcstr name, pcstr value);
    void clearAllShaderOptions() { m_ShaderOptions.clear(); }
    void PrintFailedShadersSummary();

    void RequestGrassInteraction(const Fvector& world_pos, float radius, float strength, uint8_t type = 0) override;

private:
#if defined(USE_DX11)
    xr_vector<D3D_SHADER_MACRO> m_ShaderOptions;
#elif defined(USE_OGL)
    xr_string m_ShaderOptions;
#else
#   error No graphics API selected or enabled!
#endif

public:
#if defined(USE_DX11) && RENDER == R_R4
    xray::render::fg::RenderDevice* m_renderDevice{ nullptr };

    IRenderBackend* m_backend{ nullptr };

    xray::render::FrameGraphRenderer* m_framegraphRenderer{ nullptr };

    framegraph::ShaderLoader* GetShaderLoader() const override;

    void RenderStatsOverlay() override;
    void SetEnabled(bool enabled) override;
    bool IsEnabled() const override;
    xray::render::fg::RenderDevice* GetRenderDevice() const override;
    xray::render::fg::ImGuiRendererNVRHI* GetImGuiRendererNVRHI() const override;
    xray::render::MaterialCache* GetMaterialCache() const override;
    xray::render::MaterialCache* GetUIMaterialCache() const override;
    xray::render::ui::UIRenderCollector* GetUICollector() const override;
    xray::render::ui::NVRHIUIRenderer* GetUIRenderer() const override;
    xray::render::MaterialCache* GetTextMaterialCache() const override;
    void UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode) override;
    void NotifySmokeShot() override;
#endif
};

extern CRender RImplementation;
} // namespace xray::render::fg
