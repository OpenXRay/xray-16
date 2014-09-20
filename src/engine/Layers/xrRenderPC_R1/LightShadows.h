// LightShadows.h: interface for the CLightShadows class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIGHTSHADOWS_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_)
#define AFX_LIGHTSHADOWS_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_
#pragma once

#include "../xrRender/light.h"
#include "../xrRender/r__dsgraph_types.h"

class	CLightShadows			
{
private:
	//
	typedef	R_dsgraph::_MatrixItem	NODE;		
	struct	caster			{
		IRenderable*		O;
		Fvector				C;
		float				D;
		xr_vector<NODE>		nodes;
	};
	struct	shadow			{
#ifdef	DEBUG
		float				dbg_HAT;
#endif
		IRenderable*		O;
		int					slot;
		Fvector				C;
		Fmatrix				M;
		light*				L;
		float				E;
	};
	struct	tess_tri		{
		Fvector				v[3];
		Fvector				N;
	};
public:
	struct	cache_item		{
		IRenderable*		O;
		Fvector				Op;
		light*				L;
		Fvector				Lp;
		u32					time;
		tess_tri*			tris;
		u32					tcnt;
	
		cache_item()		{ O=0; L=0; tris=0; }
	};
private:
	IRenderable*			current;
	xr_vector<caster*>		casters_pool;
	xr_vector<caster*>		casters;
	xr_vector<shadow>		shadows;
	xr_vector<tess_tri>		tess;
	xr_vector<cache_item>	cache;
	xrXRC					xrc;

	ref_rt					RT;
	ref_rt					RT_temp;
	ref_shader				sh_BlurTR;
	ref_shader				sh_BlurRT;
	ref_geom				geom_Blur;
	ref_shader				sh_World;
	ref_geom				geom_World;
	ref_shader				sh_Screen;
	ref_geom				geom_Screen;
private:
public:
	void					set_object		(IRenderable*	O);
	void					add_element		(NODE&			N);
	void					calculate		();
	void					render			();

	CLightShadows			();
	~CLightShadows			();
};

#endif // !defined(AFX_LIGHTSHADOWS_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_)
