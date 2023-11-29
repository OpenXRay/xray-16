#pragma once
#include "Layers/xrRender/D3DXRenderBase.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/HOM.h"
#include "Layers/xrRender/DetailManager.h"
#include "GlowManager.h"
#include "Layers/xrRender/WallmarksEngine.h"
#include "FStaticRender_Types.h"
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
        PHASE_SMAP,
        PHASE_POINT = PHASE_SMAP,
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
        u32 ffp : 1; // don't use shaders, only fixed-function pipeline or software processing
        u32 new_shader_support : 1; // always disabled for r1
    } o;

public:
    // Sector detection and visibility
    IRender_Sector::sector_id_t last_sector_id{IRender_Sector::INVALID_SECTOR_ID};
    Fvector vLastCameraPos;
    u32 uLastLTRACK;
    xrXRC Sectors_xrc;
    CDB::MODEL* rmPortals;
    Task* ProcessHOMTask;
    CHOM HOM;

    // Global containers
    xr_vector<FSlideWindowItem> SWIs;
    xr_vector<ref_shader> Shaders;
    typedef svector<VertexElement, MAXD3DDECLLENGTH + 1> VertexDeclarator;
    xr_vector<VertexDeclarator> nDC, xDC;
    xr_vector<VertexStagingBuffer> nVB, xVB;
    xr_vector<IndexStagingBuffer> nIB, xIB;
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

    bool m_bMakeAsyncSS;
    bool m_bFirstFrameAfterReset; // Determines weather the frame is the first after resetting device.

private:
    // Loading / Unloading
    void LoadBuffers(CStreamReader* fs, bool alternative = false);
    void LoadVisuals(IReader* fs);
    void LoadLights(IReader* fs);
    void LoadSectors(IReader* fs);
    void LoadSWIs(CStreamReader* fs);

public:
    ShaderElement* rimp_select_sh_static(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    ShaderElement* rimp_select_sh_dynamic(dxRender_Visual* pVisual, float cdist_sq, u32 phase);
    VertexElement* getVB_Format(int id, bool alternative = false);
    VertexStagingBuffer* getVB(int id, bool alternative = false);
    IndexStagingBuffer* getIB(int id, bool alternative = false);
    FSlideWindowItem* getSWI(int id);
    IRenderVisual* model_CreatePE(LPCSTR name);
    void ApplyBlur2(FVF::TL2uv* dest, u32 size) const;
    void ApplyBlur4(FVF::TL4uv* dest, u32 w, u32 h, float k) const;
    void apply_object(CBackend& cmd_list, IRenderable* O);

public:
    // feature level
    virtual GenerationLevel GetGeneration() const override { return IRender::GENERATION_R1; }
    virtual BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
    virtual u32 get_dx_level() override { return 0x00090000; }
    virtual bool is_sun_static() override { return true; }
    // Loading / Unloading
    virtual void OnDeviceCreate(pcstr shName) override;
    virtual void create() override;
    virtual void destroy() override;
    virtual void reset_begin() override;
    virtual void reset_end() override;
    virtual void level_Load(IReader* fs) override;
    virtual void level_Unload() override;

    ID3DBaseTexture* texture_load(LPCSTR fname, u32& msize);
    virtual HRESULT shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags,
        void*& result) override;

    // Information
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
    virtual LPCSTR getShaderPath() override { return "r1" DELIMITER ""; }
    virtual ref_shader getShader(int id);
    virtual IRenderVisual* getVisual(int id) override;
    virtual IRender_Target* getTarget() override;

    // Main
    void set_Object(IRenderable* O, u32 phase);
    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m) override; // add visual leaf (no culling performed at all)

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
    virtual void model_Delete(IRenderVisual*& V, bool bDiscard) override;
    virtual void model_Delete(IRender_DetailModel*& F);
    virtual void model_Logging(bool bEnable) override { Models->Logging(bEnable); }
    virtual void models_Prefetch() override;
    virtual void models_Clear(bool b_complete) override;

    // Occlusion culling
    virtual bool occ_visible(vis_data& V) override;
    virtual bool occ_visible(Fbox& B) override;
    virtual bool occ_visible(sPoly& P) override;

    // Main
    void BeforeRender() override;

    void Calculate() override;
    void Render() override;
    void RenderMenu() override;

    virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = nullptr) override;
    virtual void ScreenshotAsyncBegin() override;
    virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override;
    virtual void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Вызывается перед началом рендера мира и пост-эффектов
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Вызывается после рендера мира и перед UI

    // Render mode
    void rmNear(CBackend& cmd_list) override;
    void rmFar(CBackend& cmd_list) override;
    void rmNormal(CBackend& cmd_list) override;

    // Constructor/destructor/loader
    CRender();
    virtual ~CRender();
};

extern CRender RImplementation;
