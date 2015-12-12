#pragma once

#include "xrEngine/Render.h"
#include "xrCDB/ISpatial.h"
#include "r__dsgraph_types.h"
#include "r__sector.h"
#include "xr_effgamma.h"

// feedback	for receiving visuals
class	R_feedback
{
public:
	virtual		void	rfeedback_static	(dxRender_Visual*	V)		= 0;
};

// Common part of interface implementation for all D3D renderers
class D3DXRenderBase : public IRender, public pureFrame
{
public:
	IRenderable*												val_pObject;
	Fmatrix*													val_pTransform;
	BOOL														val_bHUD;
	BOOL														val_bInvisible;
	BOOL														val_bRecordMP;		// record nearest for multi-pass
	R_feedback*													val_feedback;		// feedback for geometry being rendered
	u32															val_feedback_breakp;// breakpoint
	xr_vector<Fbox3,render_alloc<Fbox3> >*						val_recorder;		// coarse structure recorder
	u32															phase;
	u32															marker;
	bool														pmask		[2]		;
	bool														pmask_wmark			;
public:
	// Dynamic scene graph
	//R_dsgraph::mapNormal_T										mapNormal	[2]		;	// 2==(priority/2)
	R_dsgraph::mapNormalPasses_T								mapNormalPasses	[2]	;	// 2==(priority/2)
	//R_dsgraph::mapMatrix_T										mapMatrix	[2]		;
	R_dsgraph::mapMatrixPasses_T								mapMatrixPasses	[2]	;
	R_dsgraph::mapSorted_T										mapSorted;
	R_dsgraph::mapHUD_T											mapHUD;
	R_dsgraph::mapLOD_T											mapLOD;
	R_dsgraph::mapSorted_T										mapDistort;

#if RENDER!=R_R1
	R_dsgraph::mapSorted_T										mapWmark;			// sorted
	R_dsgraph::mapSorted_T										mapEmissive;
	R_dsgraph::mapSorted_T										mapHUDEmissive;
#endif

	// Runtime structures 
	xr_vector<R_dsgraph::mapNormalVS::TNode*,render_alloc<R_dsgraph::mapNormalVS::TNode*> >				nrmVS;
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
	xr_vector<R_dsgraph::mapNormalGS::TNode*,render_alloc<R_dsgraph::mapNormalGS::TNode*> >				nrmGS;
#endif	//	USE_DX10
	xr_vector<R_dsgraph::mapNormalPS::TNode*,render_alloc<R_dsgraph::mapNormalPS::TNode*> >				nrmPS;
	xr_vector<R_dsgraph::mapNormalCS::TNode*,render_alloc<R_dsgraph::mapNormalCS::TNode*> >				nrmCS;
	xr_vector<R_dsgraph::mapNormalStates::TNode*,render_alloc<R_dsgraph::mapNormalStates::TNode*> >		nrmStates;
	xr_vector<R_dsgraph::mapNormalTextures::TNode*,render_alloc<R_dsgraph::mapNormalTextures::TNode*> >	nrmTextures;
	xr_vector<R_dsgraph::mapNormalTextures::TNode*,render_alloc<R_dsgraph::mapNormalTextures::TNode*> >	nrmTexturesTemp;

	xr_vector<R_dsgraph::mapMatrixVS::TNode*,render_alloc<R_dsgraph::mapMatrixVS::TNode*> >				matVS;
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
	xr_vector<R_dsgraph::mapMatrixGS::TNode*,render_alloc<R_dsgraph::mapMatrixGS::TNode*> >				matGS;
#endif	//	USE_DX10
	xr_vector<R_dsgraph::mapMatrixPS::TNode*,render_alloc<R_dsgraph::mapMatrixPS::TNode*> >				matPS;
	xr_vector<R_dsgraph::mapMatrixCS::TNode*,render_alloc<R_dsgraph::mapMatrixCS::TNode*> >				matCS;
	xr_vector<R_dsgraph::mapMatrixStates::TNode*,render_alloc<R_dsgraph::mapMatrixStates::TNode*> >		matStates;
	xr_vector<R_dsgraph::mapMatrixTextures::TNode*,render_alloc<R_dsgraph::mapMatrixTextures::TNode*> >	matTextures;
	xr_vector<R_dsgraph::mapMatrixTextures::TNode*,render_alloc<R_dsgraph::mapMatrixTextures::TNode*> >	matTexturesTemp;

    xr_vector<R_dsgraph::_LodItem, render_alloc<R_dsgraph::_LodItem> > lstLODs;
    xr_vector<int, render_alloc<int> > lstLODgroups;
    xr_vector<ISpatial*> lstRenderables;
    xr_vector<ISpatial*> lstSpatial;
    xr_vector<dxRender_Visual*, render_alloc<dxRender_Visual*>> lstVisuals;
    xr_vector<dxRender_Visual*, render_alloc<dxRender_Visual*>> lstRecorded;

	u32	counter_S;
	u32	counter_D;
	BOOL b_loaded;

public:
    friend class CSkeletonX; // Stats.Skinning
    friend class CKinematics; // Stats.Animation
    RenderStatistics BasicStats;

public:
	virtual void set_Transform(Fmatrix *M) override
    {
        VERIFY(M);
        val_pTransform = M;
    }
	virtual void set_HUD(BOOL V) override { val_bHUD = V; }
	virtual BOOL get_HUD() override { return val_bHUD; }
	virtual void set_Invisible(BOOL V) override { val_bInvisible = V; }

	void set_Feedback(R_feedback *V, u32 id)
	{
	    val_feedback_breakp = id;
        val_feedback = V;
	}
	void set_Recorder(xr_vector<Fbox3, render_alloc<Fbox3>> *dest)
	{
	    val_recorder = dest;
        if (dest)
            dest->clear();
	}
	void get_Counters(u32 &s, u32 &d)
	{
	    s = counter_S;
        d = counter_D;
	}
	void clear_Counters() { counter_S = counter_D = 0; }
public:
	D3DXRenderBase();

	void r_dsgraph_destroy()
	{
		nrmVS.clear				();
		nrmPS.clear				();
		nrmCS.clear				();
		nrmStates.clear			();
		nrmTextures.clear		();
		nrmTexturesTemp.clear	();

		matVS.clear				();
		matPS.clear				();
		matCS.clear				();
		matStates.clear			();
		matTextures.clear		();
		matTexturesTemp.clear	();

		lstLODs.clear			();
		lstLODgroups.clear		();
		lstRenderables.clear	();
		lstSpatial.clear		();
		lstVisuals.clear		();

		lstRecorded.clear		();

		//mapNormal[0].destroy	();
		//mapNormal[1].destroy	();
		//mapMatrix[0].destroy	();
		//mapMatrix[1].destroy	();
		for (int i=0; i<SHADER_PASSES_MAX; ++i)
		{
			mapNormalPasses[0][i].destroy	();
			mapNormalPasses[1][i].destroy	();
			mapMatrixPasses[0][i].destroy	();
			mapMatrixPasses[1][i].destroy	();
		}
		mapSorted.destroy		();
		mapHUD.destroy			();
		mapLOD.destroy			();
		mapDistort.destroy		();

#if RENDER!=R_R1
		mapWmark.destroy		();
		mapEmissive.destroy		();
#endif
	}

	void r_pmask(bool _1, bool _2, bool _wm=false)
	{
	    pmask[0] = _1;
        pmask[1] = _2;
        pmask_wmark = _wm;
	}

	void r_dsgraph_insert_dynamic(dxRender_Visual *pVisual, Fvector &Center);
	void r_dsgraph_insert_static(dxRender_Visual *pVisual);
    // render primitives
	void r_dsgraph_render_graph(u32	_priority,	bool _clear = true);
	void r_dsgraph_render_hud();
	void r_dsgraph_render_hud_ui();
	void r_dsgraph_render_lods(bool	_setup_zb,	bool _clear);
	void r_dsgraph_render_sorted();
	void r_dsgraph_render_emissive();
	void r_dsgraph_render_wmarks();
	void r_dsgraph_render_distort();
	void r_dsgraph_render_subspace(IRender_Sector *_sector, CFrustum *_frustum, Fmatrix &mCombined, Fvector &_cop, BOOL _dynamic, BOOL _precise_portals = FALSE);
	void r_dsgraph_render_subspace(IRender_Sector *_sector, Fmatrix &mCombined, Fvector &_cop, BOOL _dynamic, BOOL _precise_portals = FALSE);
	void r_dsgraph_render_R1_box(IRender_Sector *_sector, Fbox &_bb, int _element);
	virtual u32 memory_usage() override { return g_render_allocator.get_allocated_size(); }

    virtual void Copy(IRender &_in) override;
    //	Gamma correction functions
    virtual void setGamma(float fGamma) override;
    virtual void setBrightness(float fGamma) override;
    virtual void setContrast(float fGamma) override;
    virtual void updateGamma() override;
    //	Destroy
    virtual void OnDeviceDestroy(bool bKeepTextures) override;
    virtual void ValidateHW() override;
    virtual void DestroyHW() override;
    virtual void Reset(HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2) override;
    //	Init
    virtual void SetupStates() override;
    virtual void OnDeviceCreate(const char *shName) override;
    virtual void Create(HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2, bool) override;
    virtual void SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF) override;
    //	Overdraw
    virtual void overdrawBegin() override;
    virtual void overdrawEnd() override;
    //	Resources control
    virtual void DeferredLoad(bool E) override;
    virtual void ResourcesDeferredUpload() override;
    virtual void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) override;
    virtual void ResourcesDestroyNecessaryTextures() override;
    virtual void ResourcesStoreNecessaryTextures() override;
    virtual void ResourcesDumpMemoryUsage() override;
    //	HWSupport
    virtual bool HWSupportsShaderYUV2RGB() override;
    //	Device state
    virtual DeviceState GetDeviceState() override;
    virtual bool GetForceGPU_REF() override;
    virtual u32 GetCacheStatPolys() override;
    virtual void Begin() override;
    virtual void Clear() override;
    virtual void End() override;
    virtual void ClearTarget() override;
    virtual void SetCacheXform(Fmatrix &mView, Fmatrix &mProject) override;
    virtual void OnAssetsChanged() override;
    virtual void DumpStatistics(class IGameFont &font, class IPerformanceAlert *alert) override;

public:
    CResourceManager *Resources;
    ref_shader m_WireShader;
    ref_shader m_SelectionShader;

private:
#ifndef USE_OGL
	CGammaControl m_Gamma;
#endif // !USE_OGL
};
