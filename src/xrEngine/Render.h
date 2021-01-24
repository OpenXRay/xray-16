#pragma once

#include "xrEngine/Engine.h"
#include "xrCDB/Frustum.h"
#include "vis_common.h"
#include "Include/xrRender/FactoryPtr.h"
#include "xrCore/xr_resource.h"

class IUIShader;
typedef FactoryPtr<IUIShader> wm_shader;
// refs
class ENGINE_API IRenderable;
struct ENGINE_API FSlideWindowItem;

// fwd. decl.
struct SDL_Window;
class IRenderVisual;
class IKinematics;
class IGameFont;
class IPerformanceAlert;
template <class T> class _box2; typedef _box2<float> Fbox2;
struct Fcolor;
class IReader;
class CMemoryWriter;

#ifndef _EDITOR
extern const float fLightSmoothFactor;
#else
const float fLightSmoothFactor = 4.f;
#endif
//////////////////////////////////////////////////////////////////////////
// definition (Dynamic Light)
class ENGINE_API IRender_Light : public xr_resource
{
public:
    enum LT
    {
        DIRECT = 0,
        POINT = 1,
        SPOT = 2,
        OMNIPART = 3,
        REFLECTED = 4,
    };

public:
    virtual void set_type(LT type) = 0;
    virtual void set_active(bool) = 0;
    virtual bool get_active() = 0;
    virtual void set_shadow(bool) = 0;
    virtual void set_volumetric(bool) = 0;
    virtual void set_volumetric_quality(float) = 0;
    virtual void set_volumetric_intensity(float) = 0;
    virtual void set_volumetric_distance(float) = 0;
    virtual void set_indirect(bool){};
    virtual void set_position(const Fvector& P) = 0;
    virtual void set_rotation(const Fvector& D, const Fvector& R) = 0;
    virtual void set_cone(float angle) = 0;
    virtual void set_range(float R) = 0;
    virtual void set_virtual_size(float R) = 0;
    virtual void set_texture(pcstr name) = 0;
    virtual void set_color(const Fcolor& C) = 0;
    virtual void set_color(float r, float g, float b) = 0;
    virtual void set_hud_mode(bool b) = 0;
    virtual bool get_hud_mode() = 0;
    virtual ~IRender_Light();
};
struct ENGINE_API resptrcode_light : public resptr_base<IRender_Light>
{
    void destroy() { _set(NULL); }
};
typedef resptr_core<IRender_Light, resptrcode_light> ref_light;

//////////////////////////////////////////////////////////////////////////
// definition (Dynamic Glow)
class ENGINE_API IRender_Glow : public xr_resource
{
public:
    virtual void set_active(bool) = 0;
    virtual bool get_active() = 0;
    virtual void set_position(const Fvector& P) = 0;
    virtual void set_direction(const Fvector& P) = 0;
    virtual void set_radius(float R) = 0;
    virtual void set_texture(pcstr name) = 0;
    virtual void set_color(const Fcolor& C) = 0;
    virtual void set_color(float r, float g, float b) = 0;
    virtual ~IRender_Glow();
};
struct ENGINE_API resptrcode_glow : public resptr_base<IRender_Glow>
{
    void destroy() { _set(NULL); }
};
typedef resptr_core<IRender_Glow, resptrcode_glow> ref_glow;

//////////////////////////////////////////////////////////////////////////
// definition (Per-object render-specific data)
class ENGINE_API IRender_ObjectSpecific
{
public:
    enum mode
    {
        TRACE_LIGHTS = (1 << 0),
        TRACE_SUN = (1 << 1),
        TRACE_HEMI = (1 << 2),
        TRACE_ALL = (TRACE_LIGHTS | TRACE_SUN | TRACE_HEMI),
    };

public:
    virtual void force_mode(u32 mode) = 0;
    virtual float get_luminocity() = 0;
    virtual float get_luminocity_hemi() = 0;
    virtual float* get_luminocity_hemi_cube() = 0;

    virtual ~IRender_ObjectSpecific(){};
};

//////////////////////////////////////////////////////////////////////////
// definition (Portal)
class ENGINE_API IRender_Portal
{
public:
    virtual ~IRender_Portal(){};
};

//////////////////////////////////////////////////////////////////////////
// definition (Sector)
class ENGINE_API IRender_Sector
{
public:
    virtual ~IRender_Sector(){};
};

//////////////////////////////////////////////////////////////////////////
// definition (Target)
class ENGINE_API IRender_Target
{
public:
    virtual void set_blur(float f) = 0;
    virtual void set_gray(float f) = 0;
    virtual void set_duality_h(float f) = 0;
    virtual void set_duality_v(float f) = 0;
    virtual void set_noise(float f) = 0;
    virtual void set_noise_scale(float f) = 0;
    virtual void set_noise_fps(float f) = 0;
    virtual void set_color_base(u32 f) = 0;
    virtual void set_color_gray(u32 f) = 0;
    // virtual void set_color_add (u32 f) = 0;
    virtual void set_color_add(const Fvector& f) = 0;
    virtual u32 get_width() = 0;
    virtual u32 get_height() = 0;
    virtual void set_cm_imfluence(float f) = 0;
    virtual void set_cm_interpolate(float f) = 0;
    virtual void set_cm_textures(const shared_str& tex0, const shared_str& tex1) = 0;
    virtual ~IRender_Target(){};
};

enum class DeviceState
{
    Normal = 0,
    Lost,
    NeedReset
};

class ENGINE_API IRender
{
public:
    enum GenerationLevel
    {
        GENERATION_R1 = 1,
        GENERATION_R2 = 2,
        GENERATION_forcedword = u32(-1)
    };

    enum class BackendAPI : u32
    {
        D3D9,
        D3D10,
        D3D11,
        OpenGL
    };

    enum ScreenshotMode
    {
        SM_NORMAL = 0, // jpeg, name ignored
        SM_FOR_CUBEMAP = 1, // tga, name used as postfix
        SM_FOR_GAMESAVE = 2, // dds/dxt1,name used as full-path
        SM_FOR_LEVELMAP = 3, // tga, name used as postfix (level_name)
        SM_FOR_MPSENDING = 4,
        SM_forcedword = u32(-1)
    };

    enum RenderContext
    {
        NoContext = -1,
        PrimaryContext,
        HelperContext
    };

    class ENGINE_API ScopedContext
    {
        RenderContext previousContext;

    public:
        ScopedContext(RenderContext context);
        ~ScopedContext();
    };

    struct RenderStatistics
    {
        CStatTimer Culling; // portal traversal, frustum culling, entities "renderable_Render"
        CStatTimer Animation; // skeleton calculation
        CStatTimer Primitives; // actual primitive rendering
        CStatTimer Wait; // ...waiting something back (queries results, etc.)
        CStatTimer WaitS; // ...frame-limit sync
        CStatTimer RenderTargets; // ...render-targets
        CStatTimer Skinning; // ...skinning
        CStatTimer DetailVisibility; // ...details visibility detection
        CStatTimer DetailRender; // ...details rendering
        CStatTimer DetailCache; // ...details slot cache access
        u32 DetailCount; // ...number of DT-elements
        CStatTimer Wallmarks; // ...wallmark sorting, rendering
        u32 StaticWMCount; // ...number of static wallmark
        u32 DynamicWMCount; // ...number of dynamic wallmark
        u32 WMTriCount; // ...number of wallmark tri
        CStatTimer HUD; // ...hud rendering
        CStatTimer Glows; // ...glows vis-testing,sorting,render
        CStatTimer Lights; // ...d-lights building/rendering
        CStatTimer Projectors; // ...projectors building
        CStatTimer ShadowsCalc; // ...shadows building
        CStatTimer ShadowsRender; // ...shadows render
        u32 OcclusionQueries;
        u32 OcclusionCulled;

        void FrameStart()
        {
            Culling.FrameStart();
            Animation.FrameStart();
            Primitives.FrameStart();
            Wait.FrameStart();
            WaitS.FrameStart();
            RenderTargets.FrameStart();
            Skinning.FrameStart();
            DetailVisibility.FrameStart();
            DetailRender.FrameStart();
            DetailCache.FrameStart();
            DetailCount = 0;
            Wallmarks.FrameStart();
            StaticWMCount = 0;
            DynamicWMCount = 0;
            WMTriCount = 0;
            HUD.FrameStart();
            Glows.FrameStart();
            Lights.FrameStart();
            Projectors.FrameStart();
            ShadowsCalc.FrameStart();
            ShadowsRender.FrameStart();
            OcclusionQueries = 0;
            OcclusionCulled = 0;
        }

        void FrameEnd()
        {
            Culling.FrameEnd();
            Animation.FrameEnd();
            Primitives.FrameEnd();
            Wait.FrameEnd();
            WaitS.FrameEnd();
            RenderTargets.FrameEnd();
            Skinning.FrameEnd();
            DetailVisibility.FrameEnd();
            DetailRender.FrameEnd();
            DetailCache.FrameEnd();
            Wallmarks.FrameEnd();
            HUD.FrameEnd();
            Glows.FrameEnd();
            Lights.FrameEnd();
            Projectors.FrameEnd();
            ShadowsCalc.FrameEnd();
            ShadowsRender.FrameEnd();
        }
    };

public:
    // options
    bool m_hq_skinning;
    s32 m_skinning;
    s32 m_MSAASample;

    BENCH_SEC_SCRAMBLEMEMBER1

    // data
    CFrustum ViewBase;

public:
    // feature level
    virtual GenerationLevel GetGeneration() const = 0;
    bool GenerationIsR1() const { return GetGeneration() == GENERATION_R1; }
    bool GenerationIsR2() const { return GetGeneration() == GENERATION_R2; }
    bool GenerationIsR2OrHigher() const { return GetGeneration() >= GENERATION_R2; }

    virtual BackendAPI GetBackendAPI() const = 0;

    virtual bool is_sun_static() = 0;
    virtual u32 get_dx_level() = 0;

    // Loading / Unloading
    virtual void create() = 0;
    virtual void destroy() = 0;
    virtual void reset_begin() = 0;
    virtual void reset_end() = 0;

    BENCH_SEC_SCRAMBLEVTBL1
    BENCH_SEC_SCRAMBLEVTBL3

    virtual void level_Load(IReader* fs) = 0;
    virtual void level_Unload() = 0;

    // virtual IDirect3DBaseTexture9* texture_load (pcstr fname, u32& msize) = 0;
    void shader_option_skinning(s32 mode) { m_skinning = mode; }
    virtual HRESULT shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags,
        void*& result) = 0;

    // Information
    virtual void DumpStatistics(IGameFont& font, IPerformanceAlert* alert) = 0;

    virtual pcstr getShaderPath() = 0;
    // virtual ref_shader getShader (int id) = 0;
    virtual IRender_Sector* getSector(int id) = 0;
    virtual IRenderVisual* getVisual(int id) = 0;
    virtual IRender_Sector* detectSector(const Fvector& P) = 0;
    virtual IRender_Target* getTarget() = 0;

    // Main
    virtual void flush() = 0;
    virtual void add_Occluder(Fbox2& bb_screenspace) = 0; // mask screen region as oclluded (-1..1, -1..1)
    virtual void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& m) = 0; // add visual leaf (no culling performed at all)
    virtual void add_Geometry(IRenderVisual* V, const CFrustum& view) = 0; // add visual(s) (all culling performed)
    // virtual void add_StaticWallmark (ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)=0;
    virtual void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) = 0;
    // Prefer this function when possible
    virtual void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) = 0;
    virtual void clear_static_wallmarks() = 0;
    // virtual void add_SkeletonWallmark (intrusive_ptr<CSkeletonWallmark> wm) = 0;
    // virtual void add_SkeletonWallmark (const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
    // const Fvector& dir, float size)=0;
    // Prefer this function when possible
    virtual void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) = 0;

    // virtual IBlender* blender_create (CLASS_ID cls) = 0;
    // virtual void blender_destroy (IBlender* &) = 0;

    virtual IRender_ObjectSpecific* ros_create(IRenderable* parent) = 0;
    virtual void ros_destroy(IRender_ObjectSpecific*&) = 0;

    // Lighting/glowing
    virtual IRender_Light* light_create() = 0;
    virtual void light_destroy(IRender_Light* p_){};
    virtual IRender_Glow* glow_create() = 0;
    virtual void glow_destroy(IRender_Glow* p_){};

    // Models
    virtual IRenderVisual* model_CreateParticles(pcstr name) = 0;
    // virtual IRender_DetailModel* model_CreateDM (IReader* F) = 0;
    // virtual IRenderDetailModel* model_CreateDM (IReader* F) = 0;
    // virtual IRenderVisual* model_Create (pcstr name, IReader* data=0) = 0;
    virtual IRenderVisual* model_Create(pcstr name, IReader* data = 0) = 0;
    virtual IRenderVisual* model_CreateChild(pcstr name, IReader* data) = 0;
    virtual IRenderVisual* model_Duplicate(IRenderVisual* V) = 0;
    // virtual void model_Delete (IRenderVisual* & V, bool bDiscard=false) = 0;
    virtual void model_Delete(IRenderVisual*& V, bool bDiscard = false) = 0;
    // virtual void model_Delete (IRender_DetailModel* & F) = 0;
    virtual void model_Logging(bool bEnable) = 0;
    virtual void models_Prefetch() = 0;
    virtual void models_Clear(bool b_complete) = 0;

    // Occlusion culling
    virtual bool occ_visible(vis_data& V) = 0;
    virtual bool occ_visible(Fbox& B) = 0;
    virtual bool occ_visible(sPoly& P) = 0;

    // Main
    virtual void Calculate() = 0;
    virtual void Render() = 0;

    virtual void BeforeWorldRender() = 0; //--#SM+#-- Перед рендерингом мира
    virtual void AfterWorldRender() = 0; //--#SM+#-- После рендеринга мира (до UI)

    virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = 0) = 0;
    virtual void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) = 0;
    virtual void ScreenshotAsyncBegin() = 0;
    virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) = 0;

    // Render mode
    virtual void rmNear() = 0;
    virtual void rmFar() = 0;
    virtual void rmNormal() = 0;
    virtual u32 active_phase () = 0;

    // Constructor/destructor
    virtual ~IRender() {}

protected:
    virtual void ScreenshotImpl(ScreenshotMode mode, pcstr name, CMemoryWriter* memory_writer) = 0;

public:
    //	Gamma correction functions
    virtual void setGamma(float fGamma) = 0;
    virtual void setBrightness(float fGamma) = 0;
    virtual void setContrast(float fGamma) = 0;
    virtual void updateGamma() = 0;

    //	Destroy
    virtual void OnDeviceDestroy(bool bKeepTextures) = 0;
    virtual void Destroy() = 0;
    virtual void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) = 0;

    //	Init
    virtual void SetupStates() = 0;
    virtual void OnDeviceCreate(pcstr shName) = 0;
    virtual void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) = 0;
    virtual void SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF) = 0;

    //	Overdraw
    virtual void overdrawBegin() = 0;
    virtual void overdrawEnd() = 0;

    //	Resources control
    virtual void DeferredLoad(bool E) = 0;
    virtual void ResourcesDeferredUpload() = 0;
    virtual void ResourcesDeferredUnload() = 0;
    virtual void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) = 0;
    virtual void ResourcesDestroyNecessaryTextures() = 0;
    virtual void ResourcesStoreNecessaryTextures() = 0;
    virtual void ResourcesDumpMemoryUsage() = 0;

    //	HWSupport
    virtual bool HWSupportsShaderYUV2RGB() = 0;

    //	Device state
    virtual DeviceState GetDeviceState() = 0;
    virtual bool GetForceGPU_REF() = 0;
    virtual u32 GetCacheStatPolys() = 0;
    virtual void BeforeFrame() = 0;
    virtual void Begin() = 0;
    virtual void Clear() = 0;
    virtual void End() = 0;
    virtual void ClearTarget() = 0;
    virtual void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) = 0;
    virtual void OnAssetsChanged() = 0;

    virtual void ObtainRequiredWindowFlags(u32& windowFlags) = 0;
    virtual RenderContext GetCurrentContext() const = 0;
    virtual void MakeContextCurrent(RenderContext context) = 0;
};
