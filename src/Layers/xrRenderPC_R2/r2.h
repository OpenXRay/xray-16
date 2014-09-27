#pragma once

#include "../xrRender/r__dsgraph_structure.h"
#include "../xrRender/r__occlusion.h"

#include "../xrRender/PSLibrary.h"

#include "r2_types.h"
#include "r2_rendertarget.h"

#include "../xrRender/hom.h"
#include "../xrRender/detailmanager.h"
#include "../xrRender/modelpool.h"
#include "../xrRender/wallmarksengine.h"

#include "smap_allocator.h"
#include "../xrRender/light_db.h"
#include "light_render_direct.h"
#include "../xrRender/LightTrack.h"
#include "../xrRender/r_sun_cascades.h"

#include "../../xrEngine/irenderable.h"
#include "../../xrEngine/fmesh.h"


class dxRender_Visual;

// definition
class CRender													:	public R_dsgraph_structure
{
public:
	enum
	{
		PHASE_NORMAL	= 0,	// E[0]
		PHASE_SMAP		= 1,	// E[1]
	};

public:
	struct		_options	{
		u32		bug					: 1;
		
		u32		ssao_blur_on		: 1;
		u32		ssao_opt_data		: 1;
		u32		ssao_half_data		: 1;
		u32		ssao_hbao			: 1;

		u32		smapsize			: 16;
		u32		depth16				: 1;
		u32		mrt					: 1;
		u32		mrtmixdepth			: 1;
		u32		fp16_filter			: 1;
		u32		fp16_blend			: 1;
		u32		albedo_wo			: 1;						// work-around albedo on less capable HW
		u32		HW_smap				: 1;
		u32		HW_smap_PCF			: 1;
		u32		HW_smap_FETCH4		: 1;

		u32		HW_smap_FORMAT		: 32;

		u32		nvstencil			: 1;
		u32		nvdbt				: 1;

		u32		nullrt				: 1;

		u32		distortion			: 1;
		u32		distortion_enabled	: 1;
		u32		mblur				: 1;

		u32		sunfilter			: 1;
		u32		sunstatic			: 1;
		u32		sjitter				: 1;
		u32		noshadows			: 1;
		u32		Tshadows			: 1;						// transluent shadows
		u32		disasm				: 1;
		u32		advancedpp			: 1;	//	advanced post process (DOF, SSAO, volumetrics, etc.)

		u32		forcegloss			: 1;
		u32		forceskinw			: 1;
		float	forcegloss_v		;
	}			o;
	struct		_stats		{
		u32		l_total,	l_visible;
		u32		l_shadowed,	l_unshadowed;
		s32		s_used,		s_merged,	s_finalclip;
		u32		o_queries,	o_culled;
		u32		ic_total,	ic_culled;
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
	R_occlusion													HWOCC;

	// Global vertex-buffer container
	xr_vector<FSlideWindowItem>									SWIs;
	xr_vector<ref_shader>										Shaders;
	typedef svector<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1>		VertexDeclarator;
	xr_vector<VertexDeclarator>									nDC,xDC;
	xr_vector<IDirect3DVertexBuffer9*>							nVB,xVB;
	xr_vector<IDirect3DIndexBuffer9*>							nIB,xIB;
	xr_vector<dxRender_Visual*>									Visuals;
	CPSLibrary													PSLibrary;

	CDetailManager*												Details;
	CModelPool*													Models;
	CWallmarksEngine*											Wallmarks;

	CRenderTarget*												Target;			// Render-target

	CLight_DB													Lights;
	CLight_Compute_XFORM_and_VIS								LR;
	xr_vector<light*>											Lights_LastFrame;
	SMAP_Allocator												LP_smap_pool;
	light_Package												LP_normal;
	light_Package												LP_pending;

	xr_vector<Fbox3,render_alloc<Fbox3> >						main_coarse_structure;

	shared_str													c_sbase			;
	shared_str													c_lmaterial		;
	float														o_hemi			;
	float														o_hemi_cube[CROS_impl::NUM_FACES]	;
	float														o_sun			;
	IDirect3DQuery9*											q_sync_point[CHWCaps::MAX_GPUS];
	u32															q_sync_count	;

	bool														m_bMakeAsyncSS;
	bool														m_bFirstFrameAfterReset;	// Determines weather the frame is the first after resetting device.

	xr_vector<sun::cascade>										m_sun_cascades;

private:
	// Loading / Unloading
	void							LoadBuffers					(CStreamReader	*fs,	BOOL	_alternative);
	void							LoadVisuals					(IReader	*fs);
	void							LoadLights					(IReader	*fs);
	void							LoadPortals					(IReader	*fs);
	void							LoadSectors					(IReader	*fs);
	void							LoadSWIs					(CStreamReader	*fs);

	BOOL							add_Dynamic					(dxRender_Visual*pVisual, u32 planes);		// normal processing
	void							add_Static					(dxRender_Visual*pVisual, u32 planes);
	void							add_leafs_Dynamic			(dxRender_Visual*pVisual);					// if detected node's full visibility
	void							add_leafs_Static			(dxRender_Visual*pVisual);					// if detected node's full visibility

public:
	IRender_Sector*					rimp_detectSector			(Fvector& P, Fvector& D);
	void							render_main					(Fmatrix& mCombined, bool _fportals);
	void							render_forward				();
	void							render_smap_direct			(Fmatrix& mCombined);
	void							render_indirect				(light*			L	);
	void							render_lights				(light_Package& LP	);
	void							render_sun					();
	void							render_sun_near				();
	void							render_sun_filtered			();
	void							render_menu					();
	void							render_sun_cascade			(u32 cascade_ind);
	void							init_cacades				();
	void							render_sun_cascades			();

public:
	ShaderElement*					rimp_select_sh_static		(dxRender_Visual	*pVisual, float cdist_sq);
	ShaderElement*					rimp_select_sh_dynamic		(dxRender_Visual	*pVisual, float cdist_sq);
	D3DVERTEXELEMENT9*				getVB_Format				(int id, BOOL	_alt=FALSE);
	IDirect3DVertexBuffer9*			getVB						(int id, BOOL	_alt=FALSE);
	IDirect3DIndexBuffer9*			getIB						(int id, BOOL	_alt=FALSE);
	FSlideWindowItem*				getSWI						(int id);
	IRender_Portal*					getPortal					(int id);
	IRender_Sector*					getSectorActive				();
	IRenderVisual*					model_CreatePE				(LPCSTR name);
	IRender_Sector*					detectSector				(const Fvector& P, Fvector& D);
	int								translateSector				(IRender_Sector* pSector);

	// HW-occlusion culling
	IC u32							occq_begin					(u32&	ID		)	{ return HWOCC.occq_begin	(ID);	}
	IC void							occq_end					(u32&	ID		)	{ HWOCC.occq_end	(ID);			}
	IC u32							occq_get					(u32&	ID		)	{ return HWOCC.occq_get		(ID);	}

	ICF void						apply_object				(IRenderable*	O)
	{
		if (0==O)					return;
		if (0==O->renderable_ROS())	return;
		CROS_impl& LT				= *((CROS_impl*)O->renderable_ROS());
		LT.update_smooth			(O)								;
		o_hemi						= 0.75f*LT.get_hemi			()	;
		o_sun						= 0.75f*LT.get_sun			()	;
		CopyMemory(o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES*sizeof(float));
	}
	IC void							apply_lmaterial				()
	{
		R_constant*		C	= &*RCache.get_c	(c_sbase);		// get sampler
		if (0==C)			return;
		VERIFY				(RC_dest_sampler	== C->destination);
		VERIFY				(RC_sampler			== C->type);
		CTexture*		T	= RCache.get_ActiveTexture	(u32(C->samp.index));
		VERIFY				(T);
		float	mtl			= T->m_material;
#ifdef	DEBUG
		if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))	mtl=ps_r2_gmaterial;
#endif
		RCache.hemi.set_material (o_hemi,o_sun,0,(mtl+.5f)/4.f);
		RCache.hemi.set_pos_faces(o_hemi_cube[CROS_impl::CUBE_FACE_POS_X],
								  o_hemi_cube[CROS_impl::CUBE_FACE_POS_Y],
								  o_hemi_cube[CROS_impl::CUBE_FACE_POS_Z]);
		RCache.hemi.set_neg_faces	(o_hemi_cube[CROS_impl::CUBE_FACE_NEG_X],
								 o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Y],
								 o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Z]);
	}

public:
	// feature level
	virtual	GenerationLevel			get_generation			()	{ return IRender_interface::GENERATION_R2; }

	virtual bool					is_sun_static			()	{ return o.sunstatic;}
	virtual DWORD					get_dx_level			()	{ return 0x00090000;}

	// Loading / Unloading
	virtual void					create						();
	virtual void					destroy						();
	virtual	void					reset_begin					();
	virtual	void					reset_end					();

	virtual	void					level_Load					(IReader*);
	virtual void					level_Unload				();

	virtual IDirect3DBaseTexture9*	texture_load			(LPCSTR	fname, u32& msize);
	virtual HRESULT					shader_compile			(
		LPCSTR							name,
		DWORD const*					pSrcData,
		UINT                            SrcDataLen,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		void*&							result);

	// Information
	virtual void					Statistics					(CGameFont* F);
	virtual LPCSTR					getShaderPath				()									{ return "r2\\";	}
	virtual ref_shader				getShader					(int id);
	virtual IRender_Sector*			getSector					(int id);
	virtual IRenderVisual*			getVisual					(int id);
	virtual IRender_Sector*			detectSector				(const Fvector& P);
	virtual IRender_Target*			getTarget					();

	// Main 
	virtual void					flush						();
	virtual void					set_Object					(IRenderable*		O	);
	virtual	void					add_Occluder				(Fbox2&	bb_screenspace	);			// mask screen region as oclluded
	virtual void					add_Visual					(IRenderVisual*	V	);			// add visual leaf	(no culling performed at all)
	virtual void					add_Geometry				(IRenderVisual*	V	);			// add visual(s)	(all culling performed)

	// wallmarks
	virtual void					add_StaticWallmark			(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					clear_static_wallmarks		();
	virtual void					add_SkeletonWallmark		(intrusive_ptr<CSkeletonWallmark> wm);
	virtual void					add_SkeletonWallmark		(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
	virtual void					add_SkeletonWallmark		(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size);

	//
	virtual IBlender*				blender_create				(CLASS_ID cls);
	virtual void					blender_destroy				(IBlender* &);

	//
	virtual IRender_ObjectSpecific*	ros_create					(IRenderable*		parent);
	virtual void					ros_destroy					(IRender_ObjectSpecific* &);

	// Lighting
	virtual IRender_Light*			light_create				();
	virtual IRender_Glow*			glow_create					();

	// Models
	virtual IRenderVisual*			model_CreateParticles		(LPCSTR name);
	virtual IRender_DetailModel*	model_CreateDM				(IReader* F);
	virtual IRenderVisual*			model_Create				(LPCSTR name, IReader* data=0);
	virtual IRenderVisual*			model_CreateChild			(LPCSTR name, IReader* data);
	virtual IRenderVisual*			model_Duplicate				(IRenderVisual*	V);
	virtual void					model_Delete				(IRenderVisual* &	V, BOOL bDiscard);
	virtual void 					model_Delete				(IRender_DetailModel* & F);
	virtual void					model_Logging				(BOOL bEnable)				{ Models->Logging(bEnable);	}
	virtual void					models_Prefetch				();
	virtual void					models_Clear				(BOOL b_complete);

	// Occlusion culling
	virtual BOOL					occ_visible					(vis_data&	V);
	virtual BOOL					occ_visible					(Fbox&		B);
	virtual BOOL					occ_visible					(sPoly&		P);

	// Main
	virtual void					Calculate					();
	virtual void					Render						();
	virtual void					Screenshot					(ScreenshotMode mode=SM_NORMAL, LPCSTR name = 0);
	virtual void					Screenshot					(ScreenshotMode mode, CMemoryWriter& memory_writer);
	virtual void					ScreenshotAsyncBegin		();
	virtual void					ScreenshotAsyncEnd			(CMemoryWriter& memory_writer);
	virtual void	_BCL			OnFrame						();

	// Render mode
	virtual void					rmNear						();
	virtual void					rmFar						();
	virtual void					rmNormal					();

	// Constructor/destructor/loader
	CRender							();
	virtual ~CRender				();
protected:
	virtual	void					ScreenshotImpl				(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer);

private:
	FS_FileSet						m_file_set;
};

extern CRender						RImplementation;
