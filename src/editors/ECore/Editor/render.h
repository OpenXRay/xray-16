//---------------------------------------------------------------------------
#ifndef renderH
#define renderH

#include "..\..\xrCDB\frustum.h"
#include "vis_common.h"

#include "..\..\Layers\xrRender\blenders\blender.h"
#include "..\..\Layers\xrRender\blenders\blender_clsid.h"
#include "..\..\Layers\xrRender\xrRender_console.h"
#include "..\..\Layers\xrRender\PSLibrary.h"
#include "..\..\Layers\xrRender\IRenderDetailModel.H"
#include "..\..\Layers\xrRender\DetailModel.H"
#include "..\..\Layers\xrRender\ModelPool.h"
#include "..\..\Layers\xrRender\SkeletonCustom.h"
#include "..\..\Include/xrAPI/xrAPI.h"


// definition (Renderer)
class CRenderTarget /*:public IRender_Target*/
{
public:
	virtual u32			get_width			()				{ return EDevice.dwWidth;	}
	virtual u32			get_height			()				{ return EDevice.dwHeight;	}
};

class IRender_interface{
public:
	enum GenerationLevel
	{
		GENERATION_R1				= 81,
		GENERATION_DX81				= 81,
		GENERATION_R2				= 90,
		GENERATION_DX90				= 90,
		GENERATION_forcedword		= u32(-1)
	};
	// feature level
	virtual	GenerationLevel	get_generation			()=0;
};

class	ECORE_API CRender: public IRender_interface
{
    CRenderTarget*			Target;
    Fmatrix					current_matrix;

public:
	// options
	s32						m_skinning;

	// Data
	CFrustum				ViewBase;
	CPSLibrary				PSLibrary;

    CModelPool*				Models;
public:
	// Occlusion culling
	virtual BOOL			occ_visible		(Fbox&	B);
	virtual BOOL			occ_visible		(sPoly& P);
	virtual BOOL			occ_visible		(vis_data& P);

	// Constructor/destructor
							CRender			();
	virtual 				~CRender		();

    void					shader_option_skinning	(u32 mode)									{ m_skinning=mode;	}

	void 					Initialize		();
	void 					ShutDown		();

	void					OnDeviceCreate	();
	void					OnDeviceDestroy	();

    void					Calculate		();
    void					Render	 		();

	void					set_Transform	(Fmatrix* M);
	void					add_Visual   	(IRenderVisual* visual);

	virtual ref_shader		getShader		(int id);
	CRenderTarget*			getTarget		(){return Target;}
//.	virtual IRender_Target*	getTarget		(){return Target;}


    void					reset_begin				();
    void					reset_end				();
	virtual IRenderVisual*	model_Create			(LPCSTR name, IReader* data=0);
	virtual IRenderVisual*	model_CreateChild		(LPCSTR name, IReader* data);
	virtual IRenderVisual*	model_CreatePE			(LPCSTR name);
	virtual IRenderVisual*	model_CreateParticles	(LPCSTR name);

    virtual IRender_DetailModel*	model_CreateDM		(IReader* R);
	virtual IRenderVisual*	model_Duplicate			(IRenderVisual* V);
	virtual void			model_Delete				(IRenderVisual* &	V, BOOL bDiscard=TRUE);
    virtual void			model_Delete				(IRender_DetailModel* & F)
    {
        if (F)
        {
            CDetail*	D	= (CDetail*)F;
            D->Unload		();
            xr_delete		(D);
            F				= NULL;
        }
    }
	void 					model_Render			(IRenderVisual* m_pVisual, const Fmatrix& mTransform, int priority, bool strictB2F, float m_fLOD);
	void 					model_RenderSingle		(IRenderVisual* m_pVisual, const Fmatrix& mTransform, float m_fLOD);
	virtual	GenerationLevel	get_generation			(){return GENERATION_R1;}
	virtual bool			is_sun_static			() {return true;};

	virtual void			add_SkeletonWallmark	(intrusive_ptr<CSkeletonWallmark> wm){};
	virtual void			add_SkeletonWallmark	(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size){};

	// Render mode
	virtual void			rmNear					();
	virtual void			rmFar					();
	virtual void			rmNormal				();

	void 					apply_lmaterial			(){}
    
    virtual LPCSTR			getShaderPath			(){return "R1\\";}

	virtual HRESULT			CompileShader			(
		LPCSTR          	pSrcData,
		UINT                SrcDataLen,
		void*				pDefines,
		void*				pInclude,
		LPCSTR              pFunctionName,
		LPCSTR              pTarget,
		DWORD               Flags,
		void*				ppShader,
		void*				ppErrorMsgs,
		void*				ppConstantTable);

	virtual IDirect3DBaseTexture9*	texture_load			(LPCSTR	fname, u32& mem_size);
	virtual HRESULT					shader_compile			(
		LPCSTR							name,
		LPCSTR                          pSrcData,
		UINT                            SrcDataLen,
		void*							pDefines,
		void*							pInclude,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		void*							ppShader,
		void*							ppErrorMsgs,
		void*							ppConstantTable);
};

IC  float   CalcSSA(Fvector& C, float R)
{
    float distSQ  = EDevice.m_Camera.GetPosition().distance_to_sqr(C);
    return  R*R/distSQ;
}
extern ECORE_API CRender  	RImplementation;
//.extern ECORE_API CRender*	Render;

#endif
