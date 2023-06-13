#pragma once

#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/r__occlusion.h"
#include "Layers/xrRender/r__sync_point.h"

#include "Layers/xrRender/PSLibrary.h"

#include "r2_types.h"

#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/DetailManager.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/WallmarksEngine.h"

#include "SMAP_Allocator.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/Light_Render_Direct.h"
#include "Layers/xrRender/LightTrack.h"
#include "Layers/xrRender/r_sun_cascades.h"

#include "xrEngine/IRenderable.h"
#include "xrCore/Threading/TaskManager.hpp"
#include "xrCore/FMesh.hpp"

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

        main_task = &TaskScheduler->CreateTask("phase_calculate", { this, &i_render_phase::calculate_task });

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

    void calculate_task(Task&, void*)
    {
        calculate();

        if (o.mt_draw_enabled)
        {
            draw_task = &TaskScheduler->AddTask(*main_task, "phase_render", { this, &i_render_phase::render_task });
        }
    }

    void render_task(Task&, void*)
    {
        render();
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
    void render() override
    {
        if (!o.active)
            return;

        render_sun_near();
        render_sun();
        render_sun_filtered();
    }

    void render_sun() const;
    void render_sun_near();
    void render_sun_filtered() const;

    xr_vector<sun::cascade> m_sun_cascades;
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
        u32 instanced_details : 1;

        float forcegloss_v;

        // Yohji - New shader support
        u32 new_shader_support : 1;
    } o;

    struct RenderR2Statistics
    {
        u32 l_total;
        u32 l_visible;
        u32 l_shadowed;
        u32 l_unshadowed;
        s32 s_used;
        s32 s_merged;
        s32 s_finalclip;
        u32 ic_total;
        u32 ic_culled;

        RenderR2Statistics() { FrameStart(); }
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

public:
    RenderR2Statistics Stats;
    // Sector detection and visibility
    IRender_Sector::sector_id_t last_sector_id{IRender_Sector::INVALID_SECTOR_ID};
    u32 uLastLTRACK;
    xrXRC Sectors_xrc;
    CDB::MODEL* rmPortals;
    CHOM HOM;
    Task* ProcessHOMTask;
    R_occlusion HWOCC;

    // Global vertex-buffer container
    xr_vector<FSlideWindowItem> SWIs;
    xr_vector<ref_shader> Shaders;
    typedef svector<VertexElement, MAXD3DDECLLENGTH + 1> VertexDeclarator;
    xr_vector<VertexDeclarator> nDC, xDC;
    xr_vector<VertexStagingBuffer> nVB, xVB;
    xr_vector<IndexStagingBuffer> nIB, xIB;
    xr_vector<dxRender_Visual*> Visuals;
    CPSLibrary PSLibrary;

    CDetailManager* Details;
    CModelPool* Models;
    CWallmarksEngine* Wallmarks;

    CRenderTarget* Target; // Render-target

    CLight_DB Lights;
    CLight_Compute_XFORM_and_VIS LR;
    xr_vector<light*> Lights_LastFrame;
    SMAP_Allocator LP_smap_pool;
    light_Package LP_normal;
    light_Package LP_pending;

    xr_vector<Fbox3> main_coarse_structure;

    R_sync_point q_sync_point;

    bool m_bMakeAsyncSS;
    bool m_bFirstFrameAfterReset{}; // Determines weather the frame is the first after resetting device.

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

public:
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
    auto get_largest_sector() const { return largest_sector_id; }
    ShaderElement* rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    ShaderElement* rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    VertexElement* getVB_Format(int id, bool alternative = false);
    VertexStagingBuffer* getVB(int id, bool alternative = false);
    IndexStagingBuffer* getIB(int id, bool alternative = false);
    FSlideWindowItem* getSWI(int id);
    IRenderVisual* model_CreatePE(LPCSTR name);

    // HW-occlusion culling
    u32 occq_begin(u32& ID) { return HWOCC.occq_begin(ID); }
    void occq_end(u32& ID) { HWOCC.occq_end(ID); }
    auto occq_get(u32& ID) { return HWOCC.occq_get(ID); }

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

#if defined(USE_DX9)
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
    u32 get_dx_level() override { return 0x00090000; }
    pcstr getShaderPath() override { return "r2\\"; }
#elif defined(USE_DX11)
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D11; }
    u32 get_dx_level() override { return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 ? 0x000A0001 : 0x000A0000; }
    pcstr getShaderPath() override
    {
        return o.new_shader_support ? "r5\\" : "r3\\";
    }
#elif defined(USE_OGL)
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::OpenGL; }
    u32 get_dx_level() override { return /*HW.pDevice1?0x000A0001:*/0x000A0000; }
    pcstr getShaderPath() override { return "gl\\"; }
#else
#   error No graphics API selected or enabled!
#endif

    // Loading / Unloading
    void OnDeviceCreate(pcstr shName) override;
    void create() override;
    void destroy() override;
    void reset_begin() override;
    void reset_end() override;

    void level_Load(IReader*) override;
    void level_Unload() override;

#if defined(USE_DX9) || defined(USE_DX11)
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
    ref_shader getShader(int id);
    IRenderVisual* getVisual(int id) override;
    IRender_Target* getTarget() override;

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
    void model_Logging(bool bEnable) override { Models->Logging(bEnable); }
    void models_Prefetch() override;
    void models_Clear(bool b_complete) override;

    // Occlusion culling
    bool occ_visible(vis_data& V) override;
    bool occ_visible(Fbox& B) override;
    bool occ_visible(sPoly& P) override;

    // Main
    void BeforeRender() override;

    void Calculate() override;
    void Render() override;
    void RenderMenu() override;

    void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = nullptr) override;
    void ScreenshotAsyncBegin() override;
    void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override;
    void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Procedure is called before world render and post-effects
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Procedure is called after world render and before UI

#ifdef USE_OGL
    RenderContext GetCurrentContext() const override;
    void MakeContextCurrent(RenderContext context) override;
#endif

    // Render mode
    void rmNear(CBackend& cmd_list) override;
    void rmFar(CBackend& cmd_list) override;
    void rmNormal(CBackend& cmd_list) override;

    // Constructor/destructor/loader
    CRender();
    ~CRender() override;

#if defined(USE_DX9)
    // nothing
#elif defined(USE_DX11)
    void addShaderOption(pcstr name, pcstr value);
    void clearAllShaderOptions() { m_ShaderOptions.clear(); }

private:
    xr_vector<D3D_SHADER_MACRO> m_ShaderOptions;
#elif defined(USE_OGL)
    void addShaderOption(pcstr name, pcstr value);
    void clearAllShaderOptions() { m_ShaderOptions.clear(); }

private:
    xr_string m_ShaderOptions;
#else
#   error No graphics API selected or enabled!
#endif

private:
    IRender_Sector::sector_id_t largest_sector_id{ IRender_Sector::INVALID_SECTOR_ID };
};

extern CRender RImplementation;
