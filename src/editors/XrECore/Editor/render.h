//---------------------------------------------------------------------------
#ifndef renderH
#define renderH

#include "..\..\..\xrCDB\frustum.h"
#include "..\..\..\XrEngine\vis_common.h"

#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/IRenderDetailModel.h"
#include "Layers/xrRender/DetailModel.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/SkeletonCustom.h"

#include "xrEngine/Render.h"

// definition (Renderer)
class CRenderTarget : public IRender_Target
{
public:
	virtual u32 get_width() { return EDevice.m_RenderWidth; }
	virtual u32 get_height() { return EDevice.m_RenderHeight; }

	// fake for interface
	virtual void set_blur(float f) {}
	virtual void set_gray(float f) {}
	virtual void set_duality_h(float f) {}
	virtual void set_duality_v(float f) {}
	virtual void set_noise(float f) {}
	virtual void set_noise_scale(float f) {}
	virtual void set_noise_fps(float f) {}
	virtual void set_color_base(u32 f) {}
	virtual void set_color_gray(u32 f) {}
	virtual void set_color_add(const Fvector& f) {}
	virtual void set_cm_imfluence(float f) {}
	virtual void set_cm_interpolate(float f) {}
	virtual void set_cm_textures(const shared_str& tex0, const shared_str& tex1) {}
};

class ECORE_API CRender : public IRender
{
	CRenderTarget *Target;
	Fmatrix current_matrix;

public:
	// options
	s32 m_skinning;

	// Data
	CFrustum ViewBase;
	CPSLibrary PSLibrary;
    bool m_hq_skinning = false;

    shared_str c_ssky0;
    shared_str c_ssky1;
    shared_str c_sclouds0;
    shared_str c_sclouds1;

	CModelPool *Models;
    IRender::RenderStatistics BasicStats;
    // TODO: hack
    CResourceManager* Resources;

    struct _options
    {
        u32 vis_intersect : 1; // config
        u32 distortion : 1; // run-time modified
        u32 color_mapping : 1; // true if SM 1.4 and higher
        u32 disasm : 1; // config
        u32 forceskinw : 1; // config
        u32 no_detail_textures : 1; // config
        u32 no_ram_textures : 1; // don't keep textures in RAM
    } o = {};

public:
	// Occlusion culling
	virtual bool occ_visible(Fbox &B);
	virtual bool occ_visible(sPoly &P);
	virtual bool occ_visible(vis_data &P);

	// Constructor/destructor
	CRender();
	virtual ~CRender();

	void shader_option_skinning(u32 mode) { m_skinning = mode; }

	void Initialize();
	void ShutDown();

	void OnDeviceCreate(pcstr shName = nullptr);
	void OnDeviceDestroy(bool bKeepTextures = false);

	void Calculate();
	void Render();

	void set_Transform(Fmatrix *M);
	void add_Visual(IRenderVisual *visual);

	virtual ref_shader getShader(int id);
	CRenderTarget *getTarget() { return Target; }
	//.	virtual IRender_Target*	getTarget		(){return Target;}

	void reset_begin();
	void reset_end();
	virtual IRenderVisual* model_Create(pcstr name, IReader* data = 0);
	virtual IRenderVisual* model_CreateChild(pcstr name, IReader* data);
	virtual IRenderVisual* model_CreatePE(pcstr name);
	virtual IRenderVisual* model_CreateParticles(pcstr name);

	virtual IRender_DetailModel *model_CreateDM(IReader *R);
	virtual IRenderVisual *model_Duplicate(IRenderVisual *V);
	virtual void model_Delete(IRenderVisual *&V, bool bDiscard = TRUE);
	virtual void model_Delete(IRender_DetailModel *&F)
	{
		if (F)
		{
			CDetail *D = (CDetail *)F;
			D->Unload();
			xr_delete(D);
			F = NULL;
		}
	}
	void model_Render(IRenderVisual *m_pVisual, const Fmatrix &mTransform, int priority, bool strictB2F, float m_fLOD);
	void model_RenderSingle(IRenderVisual *m_pVisual, const Fmatrix &mTransform, float m_fLOD);
	virtual GenerationLevel GetGeneration() const { return GENERATION_R1; }
	virtual bool is_sun_static() { return true; };
	virtual BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
	virtual u32 get_dx_level() override { return 0x00090000; }

	virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm){};
	virtual void add_SkeletonWallmark(const Fmatrix *xf, CKinematics *obj, ref_shader &sh, const Fvector &start, const Fvector &dir, float size){};

	// Render mode
	virtual void rmNear();
	virtual void rmFar();
	virtual void rmNormal();

	void apply_lmaterial() {}

	virtual LPCSTR getShaderPath()
	{
#ifndef _EDITOR
		return "R1\\";
#else
		return "editor\\";
#endif
	}

	virtual HRESULT CompileShader(
		LPCSTR pSrcData,
		UINT SrcDataLen,
		void *pDefines,
		void *pInclude,
		LPCSTR pFunctionName,
		LPCSTR pTarget,
		DWORD Flags,
		void *ppShader,
		void *ppErrorMsgs,
		void *ppConstantTable);

	virtual IDirect3DBaseTexture9 *texture_load(LPCSTR fname, u32 &mem_size);

	virtual HRESULT shader_compile(
		pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result);

	// fake for interface
	virtual void create() {}
	virtual void destroy() {}
	virtual void Destroy() {}

	virtual void level_Load(IReader* fs) {}
	virtual void level_Unload() {}

	virtual void DumpStatistics(IGameFont& font, IPerformanceAlert* alert) {}

	virtual IRender_Sector* getSector(int id) { return nullptr; }
	virtual IRenderVisual* getVisual(int id) { return nullptr; }
	virtual IRender_Sector* detectSector(const Fvector& P) { return nullptr; }

	virtual void flush() {}
	virtual void add_Occluder(Fbox2& bb_screenspace) {}
	virtual void add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& m) {}
	virtual void add_Geometry(IRenderVisual* V, const CFrustum& view) {}
	virtual void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) {}
	virtual void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) {}
	virtual void clear_static_wallmarks() {}

	virtual void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) {}

	virtual IRender_ObjectSpecific* ros_create(IRenderable* parent) { return nullptr; }
	virtual void ros_destroy(IRender_ObjectSpecific*&) {}

	virtual IRender_Light* light_create() { return nullptr; }
	virtual IRender_Glow* glow_create() { return nullptr; }

	virtual void model_Logging(bool bEnable) {}
	virtual void models_Prefetch() {}
	virtual void models_Clear(bool b_complete) {}

	virtual void BeforeWorldRender() {} //--#SM+#-- Перед рендерингом мира
	virtual void AfterWorldRender() {} //--#SM+#-- После рендеринга мира (до UI)

	virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = 0) {}
	virtual void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) {}
	virtual void ScreenshotAsyncBegin() {}
	virtual void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) {}

	virtual u32 active_phase() { return 0; }

	// Gamma correction functions
	virtual void setGamma(float fGamma) {}
	virtual void setBrightness(float fGamma) {}
	virtual void setContrast(float fGamma) {}
	virtual void updateGamma() {}

	virtual void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) {}

	// Init
	virtual void ObtainRequiredWindowFlags(u32& windowFlags) {}
	virtual void SetupStates() {}
	virtual void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) {}

	// Overdraw
	virtual void overdrawBegin() {}
	virtual void overdrawEnd() {}

	// Resources control
	virtual void DeferredLoad(bool E) {}
	virtual void ResourcesDeferredUpload(){}
	virtual void ResourcesDeferredUnload(){}
	virtual void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) {}
	virtual void ResourcesDestroyNecessaryTextures() {}
	virtual void ResourcesStoreNecessaryTextures() {}
	virtual void ResourcesDumpMemoryUsage() {}

	// HWSupport
	virtual bool HWSupportsShaderYUV2RGB() { return false; }

	// Device state
	virtual DeviceState GetDeviceState() { return DeviceState::Normal; }
	virtual bool GetForceGPU_REF() { return false; }
	virtual u32 GetCacheStatPolys() { return 0; }
	virtual void BeforeRender() {}
	virtual void Begin() {}
	virtual void Clear() {}
	virtual void End() {}
	virtual void ClearTarget() {}
	virtual void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) {}
	virtual void OnAssetsChanged() {}

	virtual RenderContext GetCurrentContext() const { return RenderContext::NoContext; }
	virtual void MakeContextCurrent(RenderContext context) {}

protected:
	virtual void ScreenshotImpl(ScreenshotMode mode, pcstr name, CMemoryWriter* memory_writer) {}
};

IC float CalcSSA(Fvector &C, float R)
{
	float distSQ = EDevice.m_Camera.GetPosition().distance_to_sqr(C);
	return R * R / distSQ;
}

extern ECORE_API CRender RImplementation;
extern ECORE_API CRender* Render;
//.extern ECORE_API CRender*	Render;

#endif
