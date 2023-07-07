//---------------------------------------------------------------------------
#ifndef renderH
#define renderH

#include "..\..\..\xrCDB\frustum.h"
#include "..\..\..\xrEngine\vis_common.h"

#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/IRenderDetailModel.h"
#include "Layers/xrRender/DetailModel.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/SkeletonCustom.h"

#include "xrEngine/Render.h"

static constexpr auto c_sbase = "s_base";

// definition (Renderer)
class CRenderTarget final : public IRender_Target
{
public:
	u32 get_width(CBackend&) override { return EDevice.m_RenderWidth; }
	u32 get_height(CBackend&) override { return EDevice.m_RenderHeight; }

	// fake for interface
    void set_blur(float f) override {}
    void set_gray(float f) override {}
    void set_duality_h(float f) override {}
    void set_duality_v(float f) override {}
    void set_noise(float f) override {}
    void set_noise_scale(float f) override {}
    void set_noise_fps(float f) override {}
    void set_color_base(u32 f) override {}
    void set_color_gray(u32 f) override {}
    void set_color_add(const Fvector& f) override {}
    void set_cm_imfluence(float f) override {}
    void set_cm_interpolate(float f) override {}
    void set_cm_textures(const shared_str& tex0, const shared_str& tex1) override {}
};

class ECORE_API CRender : public IRender
{
	CRenderTarget *Target;
	Fmatrix current_matrix;

    CBackend cmd_list;

public:
    // Dynamic geometry streams
    _VertexStream Vertex;
    _IndexStream Index;

    IndexStagingBuffer QuadIB;
    IndexBufferHandle old_QuadIB;

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
        u32 ffp : 1;
    } o = {};

public:
	// Occlusion culling
    bool occ_visible(Fbox &B) override;
    bool occ_visible(sPoly &P) override;
    bool occ_visible(vis_data &P) override;

	// Constructor/destructor
	CRender();
    ~CRender() override;

	void shader_option_skinning(u32 mode) { m_skinning = mode; }

	void Initialize();
	void ShutDown();

	void OnDeviceCreate(pcstr shName = nullptr) override;
	void OnDeviceDestroy(bool bKeepTextures = false) override;

	void Calculate() override;
	void Render() override;
    void RenderMenu() override;

	void set_Transform(Fmatrix *M);
	void add_Visual(IRenderVisual *visual);

	virtual ref_shader getShader(int id);
	CRenderTarget *getTarget() override { return Target; }
	//.	virtual IRender_Target*	getTarget		(){return Target;}

	void reset_begin() override;
	void reset_end() override;
    IRenderVisual* model_Create(pcstr name, IReader* data = 0) override;
    IRenderVisual* model_CreateChild(pcstr name, IReader* data) override;
	virtual IRenderVisual* model_CreatePE(pcstr name);
    IRenderVisual* model_CreateParticles(pcstr name) override;

	virtual IRender_DetailModel *model_CreateDM(IReader *R);
    IRenderVisual *model_Duplicate(IRenderVisual *V) override;
    void model_Delete(IRenderVisual *&V, bool bDiscard = TRUE) override;
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
    GenerationLevel GetGeneration() const override { return GENERATION_R1; }
    bool is_sun_static() override { return true; };
    BackendAPI GetBackendAPI() const override { return IRender::BackendAPI::D3D9; }
    u32 get_dx_level() override { return 0x00090000; }

	virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm){};
	virtual void add_SkeletonWallmark(const Fmatrix *xf, CKinematics *obj, ref_shader &sh, const Fvector &start, const Fvector &dir, float size){};

	// Render mode
    void rmNear(CBackend& cmd_list) override;
    void rmFar(CBackend& cmd_list) override;
    void rmNormal(CBackend& cmd_list) override;

	void apply_lmaterial() {}

    LPCSTR getShaderPath() override
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

    HRESULT shader_compile(
		pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result) override;

	// fake for interface
    void create() override {}
    void destroy() override {}
    void Destroy() override {}

    void level_Load(IReader* fs) override {}
    void level_Unload() override {}

    void DumpStatistics(IGameFont& font, IPerformanceAlert* alert) override {}

    IRenderVisual* getVisual(int id) override { return nullptr; }

    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* V, Fmatrix& m) override {}
    void add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override {}
    void add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V) override {}
    void clear_static_wallmarks() override {}

    void add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start,
        const Fvector& dir, float size) override {}

    IRender_ObjectSpecific* ros_create(IRenderable* parent) override { return nullptr; }
    void ros_destroy(IRender_ObjectSpecific*&) override {}

    IRender_Light* light_create() override { return nullptr; }
    IRender_Glow* glow_create() override { return nullptr; }

    void model_Logging(bool bEnable) override {}
    void models_Prefetch() override {}
    void models_Clear(bool b_complete) override {}

    void BeforeWorldRender() override {} //--#SM+#-- Перед рендерингом мира
    void AfterWorldRender() override {} //--#SM+#-- После рендеринга мира (до UI)

    void Screenshot(ScreenshotMode mode = SM_NORMAL, pcstr name = 0) override {}
    void Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer) override {}
    void ScreenshotAsyncBegin() override {}
    void ScreenshotAsyncEnd(CMemoryWriter& memory_writer) override {}

	// Gamma correction functions
    void setGamma(float fGamma) override {}
    void setBrightness(float fGamma) override {}
    void setContrast(float fGamma) override {}
    void updateGamma() override {}

    void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override {}

	// Init
    void ObtainRequiredWindowFlags(u32& windowFlags) override {}
    void SetupStates() override {}
    void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override {}

	// Overdraw
    void overdrawBegin() override {}
    void overdrawEnd() override {}

	// Resources control
    void DeferredLoad(bool E) override {}
    void ResourcesDeferredUpload() override {}
    void ResourcesDeferredUnload() override {}
    void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) override {}
    void ResourcesDestroyNecessaryTextures() override {}
    void ResourcesStoreNecessaryTextures() override {}
    void ResourcesDumpMemoryUsage() override {}

	// HWSupport
    bool HWSupportsShaderYUV2RGB() override { return false; }

	// Device state
    DeviceState GetDeviceState() override { return DeviceState::Normal; }
    bool GetForceGPU_REF() override { return false; }
    u32 GetCacheStatPolys() override { return 0; }
    void BeforeRender() override {}
    void Begin() override {}
    void Clear() override {}
    void End() override {}
    void ClearTarget() override {}
    void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) override {}
    void OnAssetsChanged() override {}

    RenderContext GetCurrentContext() const override { return RenderContext::NoContext; }
    void MakeContextCurrent(RenderContext context) override {}

    CBackend& get_imm_command_list() override
    {
        return cmd_list;
    }

    void CreateQuadIB();

protected:
    void ScreenshotImpl(ScreenshotMode mode, pcstr name, CMemoryWriter* memory_writer) override {}
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
