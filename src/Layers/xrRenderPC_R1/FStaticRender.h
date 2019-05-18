#pragma once
#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/DetailManager.h"
#include "GlowManager.h"
#include "Layers/xrRender/WallmarksEngine.h"
#include "FStaticRender_RenderTarget.h"
#include "Layers/xrRender/ModelPool.h"
#include "LightShadows.h"
#include "LightProjector.h"
#include "LightPPA.h"
#include "Layers/xrRender/Light_DB.h"
#include "xrCore/FMesh.hpp"

class dxRender_Visual;

class CRender : public D3DXRenderBase
{
public:
    enum
    {
        PHASE_NORMAL,
        PHASE_POINT,
        PHASE_SPOT
    };

    struct _options
    {
        u32 vis_intersect : 1; // config
        u32 distortion : 1; // run-time modified
        u32 color_mapping : 1; // true if SM 1.4 and higher
        u32 disasm : 1; // config
        u32 forceskinw : 1; // config
        u32 no_detail_textures : 1; // config
    } o;

public:
    // Sector detection and visibility
    CSector* pLastSector;
    Fvector vLastCameraPos;
    u32 uLastLTRACK;
    xr_vector<IRender_Portal*> Portals;
    xr_vector<IRender_Sector*> Sectors;
    xrXRC Sectors_xrc;
    CDB::MODEL* rmPortals;
    CHOM HOM;

    // Global containers
    xr_vector<FSlideWindowItem> SWIs;
    xr_vector<ref_shader> Shaders;
    typedef svector<D3DVERTEXELEMENT9, MAXD3DDECLLENGTH + 1> VertexDeclarator;
    xr_vector<VertexDeclarator> nDC, xDC;
    xr_vector<ID3DVertexBuffer*> nVB, xVB;
    xr_vector<ID3DIndexBuffer*> nIB, xIB;
    xr_vector<dxRender_Visual*> Visuals;
    CPSLibrary PSLibrary;
    CLight_DB Lights;
    CLightR_Manager* L_Dynamic;
    CLightShadows* L_Shadows;
    CLightProjector* L_Projector;
    CGlowManager* L_Glows;
    CWallmarksEngine* Wallmarks;
    CDetailManager* Details;
    CModelPool* Models;
    CRenderTarget* Target; // Render-target

    // R1-specific global constants
    Fmatrix r1_dlight_tcgen;
    light* r1_dlight_light;
    float r1_dlight_scale;
    cl_light_PR r1_dlight_binder_PR;
    cl_light_C r1_dlight_binder_color;
    cl_light_XFORM r1_dlight_binder_xform;
    shared_str c_ldynamic_props;
    bool m_bMakeAsyncSS;
    bool m_bFirstFrameAfterReset; // Determines weather the frame is the first after resetting device.

private:
    // Loading / Unloading
    void LoadBuffers(CStreamReader* fs, bool alternative = false);
    void LoadVisuals(IReader* fs);
    void LoadLights(IReader* fs);
    void LoadSectors(IReader* fs);
    void LoadSWIs(CStreamReader* fs);
    BOOL add_Dynamic(dxRender_Visual* pVisual, u32 planes); // normal processing
    void add_Static(dxRender_Visual* pVisual, u32 planes);
    void add_leafs_Dynamic(dxRender_Visual* pVisual); // if detected node's full visibility
    void add_leafs_Static(dxRender_Visual* pVisual); // if detected node's full visibility

public:
    ShaderElement* rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq);
    ShaderElement* rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq);
    D3DVERTEXELEMENT9* getVB_Format(int id, bool alternative = false);
    ID3DVertexBuffer* getVB(int id, bool alternative = false);
    ID3DIndexBuffer* getIB(int id, bool alternative = false);
    FSlideWindowItem* getSWI(int id);
    IRender_Portal* getPortal(int id);
    IRender_Sector* getSectorActive();
    IRenderVisual* model_CreatePE(LPCSTR name);
    void ApplyBlur4(FVF::TL4uv* dest, u32 w, u32 h, float k);
    void apply_object(IRenderable* O);
    void apply_lmaterial(){};

public:
    // feature level
    virtual GenerationLevel get_generation() override { return IRender::GENERATION_R1; }
    virtual DWORD get_dx_level() override { return 0x00090000; }
    virtual bool is_sun_static() override { return true; }
    // Loading / Unloading
    virtual void create() override;
    virtual void destroy() override;
    virtual void reset_begin() override;
    virtual void reset_end() override;
    virtual void level_Load(IReader* fs) override;
    virtual void level_Unload() override;

    virtual IDirect3DBaseTexture9* texture_load(LPCSTR fname, u32& msize);
    virtual HRESULT shader_compile(LPCSTR name, IReader* fs, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags,
        void*& result) override;

    // Information
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
    virtual LPCSTR getShaderPath() override { return "r1" DELIMITER ""; }
    virtual ref_shader getShader(int id);
    virtual IRender_Sector* getSector(int id) override;
    virtual IRenderVisual* getVisual(int id) override;
    virtual IRender_Sector* detectSector(const Fvector& P) override;
    int translateSector(IRender_Sector* pSector);
    virtual IRender_Target* getTarget() override;

    // Main
    virtual void flush() override;
    virtual void set_Object(IRenderable* O) override;
    virtual void add_Occluder(Fbox2& bb_screenspace) override; // mask screen region as oclluded
    virtual void add_Visual(IRenderVisual* V) override; // add visual leaf (no culling performed at all)
    virtual void add_Geometry(IRenderVisual* V) override; // add visual(s)	(all culling performed)

    // wallmarks
    virtual void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    virtual void add_StaticWallmark(
        IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    virtual void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    virtual void clear_static_wallmarks() override;
    virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    virtual void add_SkeletonWallmark(
        const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
    virtual void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) override;

    //
    virtual IBlender* blender_create(CLASS_ID cls);
    virtual void blender_destroy(IBlender*&);

    //
    virtual IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    virtual void ros_destroy(IRender_ObjectSpecific*&) override;

    // Particle library
    virtual CPSLibrary* ps_library() { return &PSLibrary; }
    // Lighting
    virtual IRender_Light* light_create() override;
    virtual IRender_Glow* glow_create() override;

    // Models
    virtual IRenderVisual* model_CreateParticles(LPCSTR name) override;
    virtual IRender_DetailModel* model_CreateDM(IReader* F);
    virtual IRenderVisual* model_Create(LPCSTR name, IReader* data = nullptr) override;
    virtual IRenderVisual* model_CreateChild(LPCSTR name, IReader* data) override;
    virtual IRenderVisual* model_Duplicate(IRenderVisual* V) override;
    virtual void model_Delete(IRenderVisual*& V, BOOL bDiscard) override;
    virtual void model_Delete(IRender_DetailModel*& F);
    virtual void model_Logging(BOOL bEnable) override { Models->Logging(bEnable); }
    virtual void models_Prefetch() override;
    virtual void models_Clear(BOOL b_complete) override;

    // Occlusion culling
    virtual BOOL occ_visible(vis_data& V) override;
    virtual BOOL occ_visible(Fbox& B) override;
    virtual BOOL occ_visible(sPoly& P) override;

    // Main
    void BeforeFrame() override;

    virtual void Calculate() override;
    virtual void Render() override;
    virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = nullptr) override;
    virtual void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) override;
    virtual void ScreenshotAsyncBegin() override;
    virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override;
    virtual void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Вызывается перед началом рендера мира и пост-эффектов
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Вызывается после рендера мира и перед UI

    // Render mode
    virtual void rmNear() override;
    virtual void rmFar() override;
    virtual void rmNormal() override;

    u32 active_phase() override { return phase; }

    // Constructor/destructor/loader
    CRender();
    virtual ~CRender();

protected:
    virtual void ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer) override;

private:
    FS_FileSet m_file_set;
};

extern CRender RImplementation;
