#include "pch.h"
void XRayRenderInterface::level_Load(IReader*)
{
}

void XRayRenderInterface::level_Unload()
{}

HRESULT XRayRenderInterface::shader_compile(
    LPCSTR name, IReader* fs, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags, void*& result)
{
    return E_NOTIMPL;
}



void XRayRenderInterface::DumpStatistics(IGameFont& font, IPerformanceAlert* alert) {}

LPCSTR XRayRenderInterface::getShaderPath() {
	return "r5";
}

IRender_Sector* XRayRenderInterface::getSector(int id)
{
	return nullptr;
}

IRenderVisual* XRayRenderInterface::getVisual(int id)
{
	return nullptr;
}

IRender_Sector* XRayRenderInterface::detectSector(const Fvector& P)
{
	return nullptr;
}

IRender_Target* XRayRenderInterface::getTarget()
{
	return 0;
}


void XRayRenderInterface::flush()
{
}

void XRayRenderInterface::set_Object(IRenderable* O)
{
}

void XRayRenderInterface::add_Occluder(Fbox2& bb_screenspace)
{}

void XRayRenderInterface::add_Visual(IRenderable* root, IRenderVisual* V, Fmatrix& m) {}

void XRayRenderInterface::add_Geometry(IRenderVisual* V, const CFrustum& view) {}


void XRayRenderInterface::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
}

void XRayRenderInterface::add_StaticWallmark(IWallMarkArray* pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
}

void XRayRenderInterface::clear_static_wallmarks()
{
}

void XRayRenderInterface::add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray* pArray, const Fvector& start, const Fvector& dir, float size)
{
}

IRender_ObjectSpecific* XRayRenderInterface::ros_create(IRenderable* parent)
{
	return nullptr;
}

void XRayRenderInterface::ros_destroy(IRender_ObjectSpecific*&)
{
}

IRender_Light* XRayRenderInterface::light_create()
{
	return nullptr;
}

IRender_Glow* XRayRenderInterface::glow_create()
{
	return nullptr;
}

IRenderVisual* XRayRenderInterface::model_CreateParticles(LPCSTR name)
{
	return nullptr;
}

IRenderVisual* XRayRenderInterface::model_Create(LPCSTR name, IReader* data)
{
	return nullptr;
}

IRenderVisual* XRayRenderInterface::model_CreateChild(LPCSTR name, IReader* data)
{
	return nullptr;
}

IRenderVisual* XRayRenderInterface::model_Duplicate(IRenderVisual* V)
{
	return nullptr;
}

void XRayRenderInterface::model_Delete(IRenderVisual*& V, BOOL bDiscard)
{
}

void XRayRenderInterface::model_Logging(BOOL bEnable)
{
}

void XRayRenderInterface::models_Prefetch()
{
}

void XRayRenderInterface::models_Clear(BOOL b_complete)
{
}

BOOL XRayRenderInterface::occ_visible(vis_data& V)
{
	return 0;
}

BOOL XRayRenderInterface::occ_visible(Fbox& B)
{
	return 0;
}

BOOL XRayRenderInterface::occ_visible(sPoly& P)
{
	return 0;
}

void XRayRenderInterface::Calculate()
{
}

void XRayRenderInterface::Render()
{
//	if (g_pGamePersistent)	g_pGamePersistent->OnRenderPPUI_main();
}

void XRayRenderInterface::Screenshot(ScreenshotMode mode, LPCSTR name)
{
}

void XRayRenderInterface::Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer)
{
}

void XRayRenderInterface::ScreenshotAsyncBegin()
{
}

void XRayRenderInterface::ScreenshotAsyncEnd(CMemoryWriter& memory_writer)
{
}

void XRayRenderInterface::rmNear()
{
}

void XRayRenderInterface::rmFar()
{
}

void XRayRenderInterface::rmNormal()
{
}


void XRayRenderInterface::BeforeWorldRender()
{
}

void XRayRenderInterface::AfterWorldRender()
{
}


u32 XRayRenderInterface::active_phase()
{
	return u32();
}

void XRayRenderInterface::ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer)
{}

void XRayRenderInterface::setGamma(float fGamma) {}

void XRayRenderInterface::setBrightness(float fGamma) {}

void XRayRenderInterface::setContrast(float fGamma) {}

void XRayRenderInterface::updateGamma() {}

void XRayRenderInterface::OnDeviceDestroy(bool bKeepTextures) {}

void XRayRenderInterface::Destroy() {}

void XRayRenderInterface::Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) {}

void XRayRenderInterface::SetupStates() {}

void XRayRenderInterface::OnDeviceCreate(LPCSTR shName) {}

void XRayRenderInterface::Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) {}

void XRayRenderInterface::SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF) {}

void XRayRenderInterface::overdrawBegin() {}

void XRayRenderInterface::overdrawEnd() {}

void XRayRenderInterface::DeferredLoad(bool E) {}

void XRayRenderInterface::ResourcesDeferredUpload() {}

void XRayRenderInterface::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) {}

void XRayRenderInterface::ResourcesDestroyNecessaryTextures() {}

void XRayRenderInterface::ResourcesStoreNecessaryTextures() {}

void XRayRenderInterface::ResourcesDumpMemoryUsage() {}

bool XRayRenderInterface::HWSupportsShaderYUV2RGB() { return false; }

DeviceState XRayRenderInterface::GetDeviceState() { return DeviceState(); }

bool XRayRenderInterface::GetForceGPU_REF() { return false; }

u32 XRayRenderInterface::GetCacheStatPolys() { return u32(); }

void XRayRenderInterface::BeforeFrame() {}

void XRayRenderInterface::Begin() {}

void XRayRenderInterface::Clear() {}

void XRayRenderInterface::End() {}

void XRayRenderInterface::ClearTarget() {}

void XRayRenderInterface::SetCacheXform(Fmatrix& mView, Fmatrix& mProject) {}

void XRayRenderInterface::OnAssetsChanged() {}

void XRayRenderInterface::ObtainRequiredWindowFlags(u32& windowFlags) {}

void XRayRenderInterface::MakeContextCurrent(RenderContext context) {}


XRayRenderInterface::XRayRenderInterface() {}

XRayRenderInterface::GenerationLevel XRayRenderInterface::get_generation() { return GenerationLevel(); }


bool XRayRenderInterface::is_sun_static()
{
	return false;
}

DWORD XRayRenderInterface::get_dx_level()
{
	return 12;
}

void XRayRenderInterface::create()
{
	
}

void XRayRenderInterface::destroy()
{
}

void XRayRenderInterface::reset_begin()
{
}

void XRayRenderInterface::reset_end()
{

}
XRayRenderInterface GRenderInterface;
