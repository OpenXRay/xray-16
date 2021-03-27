#pragma once

#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/r__occlusion.h"
#include <Layers/xrRender/r__sync_point.h>

#include "Layers/xrRender/PSLibrary.h"

#include "r2_types.h"
#include "gl_rendertarget.h"

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
#include "xrCore/FMesh.hpp"

class dxRender_Visual;

// definition
class CRender : public D3DXRenderBase
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

        u32 dx10_msaa : 1; //	DX10.0 path
        u32 dx10_msaa_hybrid : 1; //	DX10.0 main path with DX10.1 A-test msaa allowed
        u32 dx10_msaa_opt : 1; //	DX10.1 path
        u32 dx10_gbuffer_opt : 1; //
        u32 dx10_sm4_1 : 1; //	DX10.1 path
        u32 dx10_msaa_alphatest : 2; //	A-test mode
        u32 dx10_msaa_samples : 4;

        u32 dx10_minmax_sm : 2;
        u32 dx10_minmax_sm_screenarea_threshold;

        u32 dx11_enable_tessellation : 1;

        u32 forcegloss : 1;
        u32 forceskinw : 1;
        float forcegloss_v;
    } o;

    struct RenderR4Statistics
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

        RenderR4Statistics() { FrameStart(); }
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
    RenderR4Statistics Stats;
    // Sector detection and visibility
    CSector* pLastSector;
    Fvector vLastCameraPos;
    u32 uLastLTRACK;
    xr_vector<IRender_Portal*> Portals;
    xr_vector<IRender_Sector*> Sectors;
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

    shared_str c_sbase;
    shared_str c_lmaterial;
    float o_hemi;
    float o_hemi_cube[CROS_impl::NUM_FACES];
    float o_sun;
    R_sync_point q_sync_point;

    bool m_bMakeAsyncSS;
    bool m_bFirstFrameAfterReset; // Determines weather the frame is the first after resetting device.
    xr_vector<sun::cascade> m_sun_cascades;


private:
    // Loading / Unloading
    void LoadBuffers(CStreamReader* fs, bool alternative);
    void LoadVisuals(IReader* fs);
    void LoadLights(IReader* fs);
    void LoadPortals(IReader* fs);
    void LoadSectors(IReader* fs);
    void LoadSWIs(CStreamReader* fs);
    void Load3DFluid();

public:
    IRender_Sector* rimp_detectSector(Fvector& P, Fvector& D);
    void render_main(Fmatrix& mCombined, bool _fportals);
    void render_forward();
    void render_smap_direct(Fmatrix& mCombined);
    void render_indirect(light* L);
    void render_lights(light_Package& LP);
    void render_sun();
    void render_sun_near();
    void render_sun_filtered();
    void render_menu();
    void render_rain();

    void render_sun_cascade(u32 cascade_ind);
    void init_cacades();
    void render_sun_cascades();

    ShaderElement* rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq);
    ShaderElement* rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq);
    VertexElement* getVB_Format(int id, bool alternative = false);
    VertexStagingBuffer* getVB(int id, bool alternative = false);
    IndexStagingBuffer* getIB(int id, bool alternative = false);
    FSlideWindowItem* getSWI(int id);
    IRender_Portal* getPortal(int id);
    IRender_Sector* getSectorActive();
    IRenderVisual* model_CreatePE(LPCSTR name);
    IRender_Sector* detectSector(const Fvector& P, Fvector& D);
    int translateSector(IRender_Sector* pSector);

    // HW-occlusion culling
    u32 occq_begin(u32& ID) { return HWOCC.occq_begin(ID); }
    void occq_end(u32& ID) { HWOCC.occq_end(ID); }
    R_occlusion::occq_result occq_get(u32& ID) { return HWOCC.occq_get(ID); }

    ICF void apply_object(IRenderable* O)
    {
        if (!O || !O->renderable_ROS())
            return;

        CROS_impl& LT = *(CROS_impl*)O->renderable_ROS();
        LT.update_smooth(O);
        o_hemi = 0.75f * LT.get_hemi();
        //o_hemi						= 0.5f*LT.get_hemi			()	;
        o_sun = 0.75f * LT.get_sun();
        CopyMemory(o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES*sizeof(float));
    }

    void apply_lmaterial()
    {
        R_constant* C = RCache.get_c(c_sbase)._get(); // get sampler
        if (!C)
            return;
        VERIFY (RC_dest_sampler == C->destination);
        VERIFY (RC_sampler == C->type);
        CTexture* T = RCache.get_ActiveTexture(u32(C->samp.index));
        VERIFY (T);
        float mtl = T->m_material;
#ifdef	DEBUG
        if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))
            mtl=ps_r2_gmaterial;
#endif
        RCache.hemi.set_material(o_hemi, o_sun, 0, (mtl + .5f) / 4.f);
        RCache.hemi.set_pos_faces(o_hemi_cube[CROS_impl::CUBE_FACE_POS_X],
                                  o_hemi_cube[CROS_impl::CUBE_FACE_POS_Y],
                                  o_hemi_cube[CROS_impl::CUBE_FACE_POS_Z]);
        RCache.hemi.set_neg_faces(o_hemi_cube[CROS_impl::CUBE_FACE_NEG_X],
                                  o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Y],
                                  o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Z]);
    }

public:
    // feature level
    GenerationLevel GetGeneration() const override { return GENERATION_R2; }
    virtual BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::OpenGL; }

    bool is_sun_static() override { return o.sunstatic; }
    u32 get_dx_level() override { return /*HW.pDevice1?0x000A0001:*/0x000A0000; }

    // Loading / Unloading
    void create() override;
    void destroy() override;
    void reset_begin() override;
    void reset_end() override;

    void level_Load(IReader*) override;
    void level_Unload() override;

    GLuint texture_load(LPCSTR fname, u32& msize, GLenum& ret_desc);
    HRESULT shader_compile(
        pcstr name,
        IReader* fs,
        pcstr pFunctionName,
        pcstr pTarget,
        u32 Flags,
        void*& result) override;

    // Information
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
    LPCSTR getShaderPath() override { return "gl" DELIMITER; }
    virtual ref_shader getShader(int id);
    IRender_Sector* getSector(int id) override;
    IRenderVisual* getVisual(int id) override;
    IRender_Sector* detectSector(const Fvector& P) override;
    IRender_Target* getTarget() override;

    // Main
    void flush() override;
    void add_Occluder(Fbox2& bb_screenspace) override; // mask screen region as oclluded
    void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& m) override; // add visual leaf	(no culling performed at all)
    void add_Geometry(IRenderVisual* V, const CFrustum& view) override; // add visual(s)	(all culling performed)

    // wallmarks
    virtual void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void clear_static_wallmarks() override;
    virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    virtual void add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
                                      const Fvector& dir, float size);
    void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
                              const Fvector& dir, float size) override;

    //
    virtual IBlender* blender_create(CLASS_ID cls);
    virtual void blender_destroy(IBlender* &);

    //
    IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    void ros_destroy(IRender_ObjectSpecific* &) override;

    // Lighting
    IRender_Light* light_create() override;
    IRender_Glow* glow_create() override;

    // Models
    IRenderVisual* model_CreateParticles(LPCSTR name) override;
    virtual IRender_DetailModel* model_CreateDM(IReader* F);
    IRenderVisual* model_Create(LPCSTR name, IReader* data = nullptr) override;
    IRenderVisual* model_CreateChild(LPCSTR name, IReader* data) override;
    IRenderVisual* model_Duplicate(IRenderVisual* V) override;
    void model_Delete(IRenderVisual* & V, bool bDiscard) override;
    virtual void model_Delete(IRender_DetailModel* & F);
    void model_Logging(bool bEnable) override { Models->Logging(bEnable); }
    void models_Prefetch() override;
    void models_Clear(bool b_complete) override;

    // Occlusion culling
    bool occ_visible(vis_data& V) override;
    bool occ_visible(Fbox& B) override;
    bool occ_visible(sPoly& P) override;

    // Main
    void BeforeFrame() override;

    void Calculate() override;
    void Render() override;
    void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = nullptr) override;
    void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) override;
    void ScreenshotAsyncBegin() override;
    void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override;
    void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Procedure is called before world render and post-effects
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Procedure is called after world render and before UI

    void ObtainRequiredWindowFlags(u32& windowFlags) override;
    RenderContext GetCurrentContext() const override;
    void MakeContextCurrent(RenderContext context) override;

    // Render mode
    void rmNear() override;
    void rmFar() override;
    void rmNormal() override;

    u32 active_phase() override { return phase; }

    // Constructor/destructor/loader
    CRender();
    virtual ~CRender();

    void addShaderOption(pcstr name, pcstr value);
    void clearAllShaderOptions() { m_ShaderOptions.clear(); }

private:
    xr_string m_ShaderOptions;

protected:
    void ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer) override;

private:
    FS_FileSet m_file_set;
};

extern CRender RImplementation;
