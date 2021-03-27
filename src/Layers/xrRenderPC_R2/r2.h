#pragma once

#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/r__occlusion.h"
#include "Layers/xrRender/r__sync_point.h"

#include "Layers/xrRender/PSLibrary.h"

#include "r2_types.h"
#include "r2_rendertarget.h"

#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/DetailManager.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/WallmarksEngine.h"

#include "smap_allocator.h"
#include "Layers/xrRender/light_db.h"
#include "Layers/xrRender/light_render_direct.h"
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

    struct _options
    {
        u32 bug : 1;

        u32 ssao_blur_on : 1;
        u32 ssao_opt_data : 1;
        u32 ssao_half_data : 1;
        u32 ssao_hdao : 1;
        u32 ssao_hbao : 1;

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
        u32 no_ram_textures : 1; // don't keep textures in RAM

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

        u32 dx10_msaa : 1; // DX10.0 path
        u32 dx10_msaa_opt : 1; // DX10.1 path
        u32 dx10_gbuffer_opt : 1;
        u32 dx10_msaa_samples : 4;

        u32 forcegloss : 1;
        u32 forceskinw : 1;
        float forcegloss_v;
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
    void render_sun_cascade(u32 cascade_ind);
    void init_cacades();
    void render_sun_cascades();

public:
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
    IC u32 occq_begin(u32& ID) { return HWOCC.occq_begin(ID); }
    IC void occq_end(u32& ID) { HWOCC.occq_end(ID); }
    IC u32 occq_get(u32& ID) { return HWOCC.occq_get(ID); }

    ICF void apply_object(IRenderable* O)
    {
        if (!O || !O->renderable_ROS())
            return;

        CROS_impl& LT = *static_cast<CROS_impl*>(O->renderable_ROS());
        LT.update_smooth(O);
        o_hemi = 0.75f * LT.get_hemi();
        o_sun = 0.75f * LT.get_sun();
        CopyMemory(o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES * sizeof(float));
    }

    IC void apply_lmaterial()
    {
        R_constant* C = RCache.get_c(c_sbase)._get(); // get sampler
        if (!C)
            return;
        VERIFY(RC_dest_sampler == C->destination);
        VERIFY(RC_sampler == C->type);
        CTexture* T = RCache.get_ActiveTexture(u32(C->samp.index));
        VERIFY(T);
        float mtl = T->m_material;
#ifdef DEBUG
        if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))
            mtl = ps_r2_gmaterial;
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
    virtual GenerationLevel GetGeneration() const override { return IRender::GENERATION_R2; }
    virtual BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
    virtual bool is_sun_static() { return o.sunstatic; }
    virtual u32 get_dx_level() { return 0x00090000; }

    // Loading / Unloading
    virtual void create();
    virtual void destroy();
    virtual void reset_begin();
    virtual void reset_end();

    virtual void level_Load(IReader*);
    virtual void level_Unload();

    virtual IDirect3DBaseTexture9* texture_load(LPCSTR fname, u32& msize);
    virtual HRESULT shader_compile(
        pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result);

    // Information
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
    virtual LPCSTR getShaderPath() { return "r2" DELIMITER ""; }
    virtual ref_shader getShader(int id);
    virtual IRender_Sector* getSector(int id);
    virtual IRenderVisual* getVisual(int id);
    virtual IRender_Sector* detectSector(const Fvector& P);
    virtual IRender_Target* getTarget();

    // Main
    virtual void flush();
    virtual void add_Occluder(Fbox2& bb_screenspace); // mask screen region as oclluded
    void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& m) override; // add visual leaf	(no culling performed at all)
    void add_Geometry(IRenderVisual* V, const CFrustum& view) override; // add visual(s)	(all culling performed)

    // wallmarks
    virtual void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    virtual void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    virtual void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    virtual void clear_static_wallmarks();
    virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    virtual void add_SkeletonWallmark(
        const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
    virtual void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size);

    //
    virtual IBlender* blender_create(CLASS_ID cls);
    virtual void blender_destroy(IBlender*&);

    //
    virtual IRender_ObjectSpecific* ros_create(IRenderable* parent);
    virtual void ros_destroy(IRender_ObjectSpecific*&);

    // Lighting
    virtual IRender_Light* light_create();
    virtual IRender_Glow* glow_create();

    // Models
    virtual IRenderVisual* model_CreateParticles(LPCSTR name);
    virtual IRender_DetailModel* model_CreateDM(IReader* F);
    virtual IRenderVisual* model_Create(LPCSTR name, IReader* data = 0);
    virtual IRenderVisual* model_CreateChild(LPCSTR name, IReader* data);
    virtual IRenderVisual* model_Duplicate(IRenderVisual* V);
    virtual void model_Delete(IRenderVisual*& V, bool bDiscard);
    virtual void model_Delete(IRender_DetailModel*& F);
    virtual void model_Logging(bool bEnable) { Models->Logging(bEnable); }
    virtual void models_Prefetch();
    virtual void models_Clear(bool b_complete);

    // Occlusion culling
    virtual bool occ_visible(vis_data& V);
    virtual bool occ_visible(Fbox& B);
    virtual bool occ_visible(sPoly& P);

    // Main
    void BeforeFrame() override;

    virtual void Calculate();
    virtual void Render();
    virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = 0);
    virtual void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer);
    virtual void ScreenshotAsyncBegin();
    virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer);
    virtual void OnFrame();

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Вызывается перед началом рендера мира и пост-эффектов
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Вызывается после рендера мира и перед UI

    // Render mode
    virtual void rmNear();
    virtual void rmFar();
    virtual void rmNormal();

    u32 active_phase() override { return phase; }

    // Constructor/destructor/loader
    CRender();
    virtual ~CRender();

protected:
    virtual void ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer);

private:
    FS_FileSet m_file_set;
};

extern CRender RImplementation;
