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
struct CompiledLevelShader;
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

    using VertexDeclarator = ::xray::render::fg::VertexDeclarator;



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
        return BufferPool.fastGeomLoaded;
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
    xray::render::CompiledLevelShader* getCompiledShader(int id);
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
    void clearAllShaderOptions();
    void PrintFailedShadersSummary();

    void RequestGrassInteraction(const Fvector& world_pos, float radius, float strength, uint8_t type = 0) override;

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
