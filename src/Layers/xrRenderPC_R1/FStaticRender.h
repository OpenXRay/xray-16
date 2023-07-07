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

class CRender final : public D3DXRenderBase
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
        u32 no_ram_textures : 1; // don't keep textures in RAM
        u32 ffp : 1; // don't use shaders, only fixed-function pipeline or software processing
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
    GenerationLevel GetGeneration() const override { return IRender::GENERATION_R1; }
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
    u32 get_dx_level() override { return 0x00090000; }
    bool is_sun_static() override { return true; }
    // Loading / Unloading
    void create() override;
    void destroy() override;
    void reset_begin() override;
    void reset_end() override;
    void level_Load(IReader* fs) override;
    void level_Unload() override;

    ID3DBaseTexture* texture_load(LPCSTR fname, u32& msize);
    HRESULT shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags,
        void*& result) override;

    // Information
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
    LPCSTR getShaderPath() override { return "r1" DELIMITER ""; }
    ref_shader getShader(int id);
    IRenderVisual* getVisual(int id) override;
    IRender_Target* getTarget() override;

    // Main
    void set_Object(IRenderable* O, u32 phase);
    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m) override; // add visual leaf (no culling performed at all)

    // wallmarks
    void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
    void add_StaticWallmark(
        IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override;
    void clear_static_wallmarks() override;
    void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    void add_SkeletonWallmark(
        const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
    void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) override;

    //
    IBlender* blender_create(CLASS_ID cls);
    void blender_destroy(IBlender*&);

    //
    IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    void ros_destroy(IRender_ObjectSpecific*&) override;

    // Particle library
    CPSLibrary* ps_library() { return &PSLibrary; }
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

    void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = nullptr) override;
    void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) override;
    void ScreenshotAsyncBegin() override;
    void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override;
    void OnFrame() override;

    void BeforeWorldRender() override; //--#SM+#-- +SecondVP+ Вызывается перед началом рендера мира и пост-эффектов
    void AfterWorldRender() override;  //--#SM+#-- +SecondVP+ Вызывается после рендера мира и перед UI

    // Render mode
    void rmNear(CBackend& cmd_list) override;
    void rmFar(CBackend& cmd_list) override;
    void rmNormal(CBackend& cmd_list) override;

    // Constructor/destructor/loader
    CRender();
    ~CRender() override;

protected:
    void ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer) override;
};

extern CRender RImplementation;
