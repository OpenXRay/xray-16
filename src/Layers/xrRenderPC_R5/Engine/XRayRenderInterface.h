#pragma once
class XRayShaderElement;
class XRayRenderInterface : public IRender
{
public:
	XRayRenderInterface();

 virtual GenerationLevel get_generation() ;

    virtual bool is_sun_static() ;
    virtual DWORD get_dx_level() ;

    // Loading / Unloading
    virtual void create() ;
    virtual void destroy() ;
    virtual void reset_begin() ;
    virtual void reset_end() ;

    BENCH_SEC_SCRAMBLEVTBL1
    BENCH_SEC_SCRAMBLEVTBL3

    virtual void level_Load(IReader* fs) ;
    virtual void level_Unload() ;

    // virtual IDirect3DBaseTexture9* texture_load (LPCSTR fname, u32& msize) ;
    void shader_option_skinning(s32 mode) { m_skinning = mode; }
    virtual HRESULT shader_compile(
        LPCSTR name, IReader* fs, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags, void*& result) ;

    // Information
    virtual void DumpStatistics(IGameFont& font, IPerformanceAlert* alert) ;

    virtual LPCSTR getShaderPath() ;
    // virtual ref_shader getShader (int id) ;
    virtual IRender_Sector* getSector(int id) ;
    virtual IRenderVisual* getVisual(int id) ;
    virtual IRender_Sector* detectSector(const Fvector& P) ;
    virtual IRender_Target* getTarget() ;

    // Main
    virtual void flush() ;
    virtual void set_Object(IRenderable* O) ;
    virtual void add_Occluder(Fbox2& bb_screenspace) ; // mask screen region as oclluded (-1..1, -1..1)
    virtual void add_Visual(
        IRenderable* root, IRenderVisual* V, Fmatrix& m) ; // add visual leaf (no culling performed at all)
    virtual void add_Geometry(IRenderVisual* V, const CFrustum& view) ; // add visual(s) (all culling performed)
    // virtual void add_StaticWallmark (ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)=0;
    virtual void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) ;
    // Prefer this function when possible
    virtual void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) ;
    virtual void clear_static_wallmarks() ;
    // virtual void add_SkeletonWallmark (intrusive_ptr<CSkeletonWallmark> wm) ;
    // virtual void add_SkeletonWallmark (const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
    // const Fvector& dir, float size)=0;
    // Prefer this function when possible
    virtual void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) ;

    // virtual IBlender* blender_create (CLASS_ID cls) ;
    // virtual void blender_destroy (IBlender* &) ;

    virtual IRender_ObjectSpecific* ros_create(IRenderable* parent) ;
    virtual void ros_destroy(IRender_ObjectSpecific*&) ;

    // Lighting/glowing
    virtual IRender_Light* light_create() ;
    virtual void light_destroy(IRender_Light* p_){};
    virtual IRender_Glow* glow_create() ;
    virtual void glow_destroy(IRender_Glow* p_){};

    // Models
    virtual IRenderVisual* model_CreateParticles(LPCSTR name) ;
    // virtual IRender_DetailModel* model_CreateDM (IReader* F) ;
    // virtual IRenderDetailModel* model_CreateDM (IReader* F) ;
    // virtual IRenderVisual* model_Create (LPCSTR name, IReader* data=0) ;
    virtual IRenderVisual* model_Create(LPCSTR name, IReader* data = 0) ;
    virtual IRenderVisual* model_CreateChild(LPCSTR name, IReader* data) ;
    virtual IRenderVisual* model_Duplicate(IRenderVisual* V) ;
    // virtual void model_Delete (IRenderVisual* & V, BOOL bDiscard=FALSE) ;
    virtual void model_Delete(IRenderVisual*& V, BOOL bDiscard = FALSE) ;
    // virtual void model_Delete (IRender_DetailModel* & F) ;
    virtual void model_Logging(BOOL bEnable) ;
    virtual void models_Prefetch() ;
    virtual void models_Clear(BOOL b_complete) ;

    // Occlusion culling
    virtual BOOL occ_visible(vis_data& V) ;
    virtual BOOL occ_visible(Fbox& B) ;
    virtual BOOL occ_visible(sPoly& P) ;

    // Main
    virtual void Calculate() ;
    virtual void Render() ;

    virtual void BeforeWorldRender() ; //--#SM+#-- Перед рендерингом мира
    virtual void AfterWorldRender() ; //--#SM+#-- После рендеринга мира (до UI)

    virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = 0) ;
    virtual void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) ;
    virtual void ScreenshotAsyncBegin() ;
    virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) ;

    // Render mode
    virtual void rmNear() ;
    virtual void rmFar() ;
    virtual void rmNormal() ;
    virtual u32 active_phase() ;

    // Constructor/destructor

protected:
    virtual void ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer) ;

public:
    //	Gamma correction functions
    virtual void setGamma(float fGamma) ;
    virtual void setBrightness(float fGamma) ;
    virtual void setContrast(float fGamma) ;
    virtual void updateGamma() ;

    //	Destroy
    virtual void OnDeviceDestroy(bool bKeepTextures) ;
    virtual void Destroy() ;
    virtual void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) ;

    //	Init
    virtual void SetupStates() ;
    virtual void OnDeviceCreate(LPCSTR shName) ;
    virtual void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) ;
    virtual void SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF) ;

    //	Overdraw
    virtual void overdrawBegin() ;
    virtual void overdrawEnd() ;

    //	Resources control
    virtual void DeferredLoad(bool E) ;
    virtual void ResourcesDeferredUpload() ;
    virtual void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) ;
    virtual void ResourcesDestroyNecessaryTextures() ;
    virtual void ResourcesStoreNecessaryTextures() ;
    virtual void ResourcesDumpMemoryUsage() ;

    //	HWSupport
    virtual bool HWSupportsShaderYUV2RGB() ;

    //	Device state
    virtual DeviceState GetDeviceState() ;
    virtual bool GetForceGPU_REF() ;
    virtual u32 GetCacheStatPolys() ;
    virtual void BeforeFrame() ;
    virtual void Begin() ;
    virtual void Clear() ;
    virtual void End() ;
    virtual void ClearTarget() ;
    virtual void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) ;
    virtual void OnAssetsChanged() ;

    virtual void ObtainRequiredWindowFlags(u32& windowFlags) ;
    virtual void MakeContextCurrent(RenderContext context) ;
	
};
 extern XRayRenderInterface GRenderInterface;
