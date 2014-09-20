#pragma once

#include "../xrRender/r__dsgraph_structure.h"

#include "../xrRender/PSLibrary.h"

#include "../xrRender/hom.h"
#include "../xrRender/detailmanager.h"
#include "glowmanager.h"
#include "../xrRender/wallmarksengine.h"
#include "fstaticrender_rendertarget.h"
#include "../xrRender/modelpool.h"

#include "lightShadows.h"
#include "lightProjector.h"
#include "lightPPA.h"
#include "../xrRender/light_DB.h"
#include "../../xrEngine/fmesh.h"

class dxRender_Visual;

// definition
class CRender													:	public R_dsgraph_structure
{
public:
	enum	{
		PHASE_NORMAL,
		PHASE_POINT,
		PHASE_SPOT
	};
	struct		_options	{
		u32		vis_intersect		: 1;	// config
		u32		distortion			: 1;	// run-time modified
		u32		color_mapping		: 1;	// true if SM 1.4 and higher
		u32		disasm				: 1;	// config
		u32		forceskinw			: 1;	// config
		u32		no_detail_textures	: 1;	// config
	}			o;
	struct		_stats		{
		u32		o_queries,	o_culled;
	}			stats;
public:
	// Sector detection and visibility
	CSector*													pLastSector;
	Fvector														vLastCameraPos;
	u32															uLastLTRACK;
	xr_vector<IRender_Portal*>									Portals;
	xr_vector<IRender_Sector*>									Sectors;
	xrXRC														Sectors_xrc;
	CDB::MODEL*													rmPortals;
	CHOM														HOM;
//.	R_occlusion													HWOCC;
	
	// Global containers
	xr_vector<FSlideWindowItem>									SWIs;
	xr_vector<ref_shader>										Shaders;
	typedef svector<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1>		VertexDeclarator;
	xr_vector<VertexDeclarator>									DCL;
	xr_vector<IDirect3DVertexBuffer9*>							VB;
	xr_vector<IDirect3DIndexBuffer9*>							IB;
	xr_vector<dxRender_Visual*>									Visuals;
	CPSLibrary													PSLibrary;

	CLight_DB*													L_DB;
	CLightR_Manager*											L_Dynamic;
	CLightShadows*												L_Shadows;
	CLightProjector*											L_Projector;
	CGlowManager*												L_Glows;
	CWallmarksEngine*											Wallmarks;
	CDetailManager*												Details;
	CModelPool*													Models;

	CRenderTarget*												Target;			// Render-target

	// R1-specific global constants
	Fmatrix														r1_dlight_tcgen			;
	light*														r1_dlight_light			;
	float														r1_dlight_scale			;
	cl_light_PR													r1_dlight_binder_PR		;
	cl_light_C													r1_dlight_binder_color	;
	cl_light_XFORM												r1_dlight_binder_xform	;
	shared_str													c_ldynamic_props		;
	bool														m_bMakeAsyncSS;
	bool														m_bFirstFrameAfterReset;	// Determines weather the frame is the first after resetting device.

private:
	// Loading / Unloading
	void								LoadBuffers				(CStreamReader	*fs);
	void								LoadVisuals				(IReader *fs);
	void								LoadLights				(IReader *fs);
	void								LoadSectors				(IReader *fs);
	void								LoadSWIs				(CStreamReader	*fs);

	BOOL								add_Dynamic				(dxRender_Visual	*pVisual, u32 planes);		// normal processing
	void								add_Static				(dxRender_Visual	*pVisual, u32 planes);
	void								add_leafs_Dynamic		(dxRender_Visual	*pVisual);					// if detected node's full visibility
	void								add_leafs_Static		(dxRender_Visual	*pVisual);					// if detected node's full visibility

public:
	ShaderElement*						rimp_select_sh_static	(dxRender_Visual	*pVisual, float cdist_sq);
	ShaderElement*						rimp_select_sh_dynamic	(dxRender_Visual	*pVisual, float cdist_sq);
	D3DVERTEXELEMENT9*					getVB_Format			(int id);
	IDirect3DVertexBuffer9*				getVB					(int id);
	IDirect3DIndexBuffer9*				getIB					(int id);
	FSlideWindowItem*					getSWI					(int id);
	IRender_Portal*						getPortal				(int id);
	IRender_Sector*						getSectorActive			();
	IRenderVisual*						model_CreatePE			(LPCSTR			name);
	void								ApplyBlur4				(FVF::TL4uv*	dest, u32 w, u32 h, float k);
	void								apply_object			(IRenderable*	O);
	IC void								apply_lmaterial			()				{};
public:
	// feature level
	virtual	GenerationLevel			get_generation			()	{ return IRender_interface::GENERATION_R1; }
	virtual DWORD					get_dx_level			()	{ return 0x00090000; }

	virtual bool					is_sun_static			() {return true;}

	// Loading / Unloading
	virtual	void					create					();
	virtual	void					destroy					();
	virtual	void					reset_begin				();
	virtual	void					reset_end				();

	virtual	void					level_Load				(IReader*);
	virtual void					level_Unload			();
	
	virtual IDirect3DBaseTexture9*	texture_load			(LPCSTR	fname, u32& msize);
	virtual HRESULT					shader_compile			(
		LPCSTR							name,
		DWORD const*                    pSrcData,
		UINT                            SrcDataLen,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		void*&							result
	);

	// Information
	virtual void					Statistics				(CGameFont* F);
	virtual LPCSTR					getShaderPath			()									{ return "r1\\";	}
	virtual ref_shader				getShader				(int id);
	virtual IRender_Sector*			getSector				(int id);
	virtual IRenderVisual*			getVisual				(int id);
	virtual IRender_Sector*			detectSector			(const Fvector& P);
	int								translateSector			(IRender_Sector* pSector);
	virtual IRender_Target*			getTarget				();
	
	// Main 
	virtual void					flush					();
	virtual void					set_Object				(IRenderable*		O	);
	virtual	void					add_Occluder			(Fbox2&	bb_screenspace	);			// mask screen region as oclluded
	virtual void					add_Visual				(IRenderVisual*	V	);			// add visual leaf (no culling performed at all)
	virtual void					add_Geometry			(IRenderVisual*	V	);			// add visual(s)	(all culling performed)

	// wallmarks
	virtual void					add_StaticWallmark		(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					clear_static_wallmarks	();
	virtual void					add_SkeletonWallmark	(intrusive_ptr<CSkeletonWallmark> wm);
	virtual void					add_SkeletonWallmark	(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
	virtual void					add_SkeletonWallmark		(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size);
	
	//
	virtual IBlender*				blender_create			(CLASS_ID cls);
	virtual void					blender_destroy			(IBlender* &);

	//
	virtual IRender_ObjectSpecific*	ros_create				(IRenderable* parent);
	virtual void					ros_destroy				(IRender_ObjectSpecific* &);

	// Particle library
	virtual CPSLibrary*				ps_library				(){return &PSLibrary;}

	// Lighting
	virtual IRender_Light*			light_create			();
	virtual IRender_Glow*			glow_create				();
	
	// Models
	virtual IRenderVisual*			model_CreateParticles	(LPCSTR name);
	virtual IRender_DetailModel*	model_CreateDM			(IReader*F);
	virtual IRenderVisual*			model_Create			(LPCSTR name, IReader*data=0);
	virtual IRenderVisual*			model_CreateChild		(LPCSTR name, IReader*data);
	virtual IRenderVisual*			model_Duplicate			(IRenderVisual*	V);
	virtual void					model_Delete			(IRenderVisual* &	V, BOOL bDiscard);
	virtual void 					model_Delete			(IRender_DetailModel* & F);
	virtual void					model_Logging			(BOOL bEnable)				{ Models->Logging(bEnable);	}
	virtual void					models_Prefetch			();
	virtual void					models_Clear			(BOOL b_complete);
	
	// Occlusion culling
	virtual BOOL					occ_visible				(vis_data&	V);
	virtual BOOL					occ_visible				(Fbox&		B);
	virtual BOOL					occ_visible				(sPoly&		P);
	
	// Main
	virtual void					Calculate				();
	virtual void					Render					();
	virtual void					Screenshot				(ScreenshotMode mode=SM_NORMAL, LPCSTR name = 0);
	virtual void					Screenshot				(ScreenshotMode mode, CMemoryWriter& memory_writer);
	virtual void					ScreenshotAsyncBegin	();
	virtual void					ScreenshotAsyncEnd		(CMemoryWriter& memory_writer);
	virtual void	_BCL			OnFrame					();
	
	// Render mode
	virtual void					rmNear					();
	virtual void					rmFar					();
	virtual void					rmNormal				();

	// Constructor/destructor/loader
	CRender							();
	virtual ~CRender				();
protected:
	virtual	void					ScreenshotImpl			(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer);

private:
	FS_FileSet						m_file_set;
};

extern CRender						RImplementation;
