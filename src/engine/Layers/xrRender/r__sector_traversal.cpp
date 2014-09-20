#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"
#include "fvf.h"

CPortalTraverser	PortalTraverser;

CPortalTraverser::CPortalTraverser	()
{
	i_marker			=	0xffffffff;
}

#ifdef DEBUG
xr_vector<IRender_Sector*>				dbg_sectors;
#endif

void CPortalTraverser::traverse			(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options)
{
	Fmatrix			m_viewport_01	= {
		1.f/2.f,			0.0f,				0.0f,		0.0f,
		0.0f,				-1.f/2.f,			0.0f,		0.0f,
		0.0f,				0.0f,				1.0f,		0.0f,
		1.f/2.f + 0 + 0,	1.f/2.f + 0 + 0,	0.0f,		1.0f
	};

	if (options & VQ_FADE)			{
		f_portals.clear		();
		f_portals.reserve	(16);
	}

	VERIFY				(start);
	i_marker			++;
	i_options			= options;
	i_vBase				= vBase;
	i_mXFORM			= mXFORM;
	i_mXFORM_01.mul		(m_viewport_01,mXFORM);
	i_start				= (CSector*)start;
	r_sectors.clear		();
	_scissor			scissor;
	scissor.set			(0,0,1,1);
	scissor.depth		= 0;
	i_start->traverse	(F,scissor);

	if (options & VQ_SCISSOR)		{
		// dbg_sectors					= r_sectors;
		// merge scissor info
		for (u32 s=0; s<r_sectors.size(); s++)
		{
			CSector*	S		= (CSector*)r_sectors[s];
			S->r_scissor_merged.invalidate	();
			S->r_scissor_merged.depth		= flt_max;
			for (u32 it=0; it<S->r_scissors.size(); it++)
			{
				S->r_scissor_merged.merge(S->r_scissors[it]);
				if (S->r_scissors[it].depth < S->r_scissor_merged.depth)
					S->r_scissor_merged.depth = S->r_scissors[it].depth;
			}
		}
	}
}

void CPortalTraverser::fade_portal	(CPortal* _p, float ssa)
{
	f_portals.push_back				(mk_pair(_p,ssa));
}
void CPortalTraverser::initialize	()
{
	f_shader.create					("portal");
	f_geom.create					(FVF::F_L, RCache.Vertex.Buffer(), 0);
}
void CPortalTraverser::destroy		()
{
	f_geom.destroy					();
	f_shader.destroy				();
}
ICF		bool	psort_pred			(const std::pair<CPortal*, float>& _1, const std::pair<CPortal*, float>& _2)
{
	float		d1		= PortalTraverser.i_vBase.distance_to_sqr(_1.first->S.P);
	float		d2		= PortalTraverser.i_vBase.distance_to_sqr(_2.first->S.P);
	return		d2>d1;	// descending, back to front
}
extern float r_ssaDISCARD			;
extern float r_ssaLOD_A, r_ssaLOD_B ;
void CPortalTraverser::fade_render	()
{
	if (f_portals.empty())			return;

	// re-sort, back to front
	std::sort						(f_portals.begin(),f_portals.end(),psort_pred);
	
	// calc poly-count
	u32		_pcount					= 0;
	for		(u32 _it = 0; _it<f_portals.size(); _it++)	_pcount	+= f_portals[_it].first->getPoly().size()-2;

	// fill buffers
	u32			_offset				= 0;
	FVF::L*		_v					= (FVF::L*)RCache.Vertex.Lock(_pcount*3,f_geom.stride(),_offset);
	float		ssaRange			= r_ssaLOD_A - r_ssaLOD_B;
	Fvector		_ambient_f			= g_pGamePersistent->Environment().CurrentEnv->ambient;
	u32			_ambient			= color_rgba_f	(_ambient_f.x,_ambient_f.y,_ambient_f.z,0);
	for (u32 _it = 0; _it<f_portals.size(); _it++)
	{
		std::pair<CPortal*, float>&	fp		= f_portals	[_it]	;
		CPortal*					_P		= fp.first	;
		float						_ssa	= fp.second	;
		float		ssaDiff					= _ssa-r_ssaLOD_B	;
		float		ssaScale				= ssaDiff/ssaRange	;
		int			iA						= iFloor((1-ssaScale)*255.5f);	clamp(iA,0,255);
		u32							_clr	= subst_alpha(_ambient,u32(iA));	

		// fill polys
		u32			_polys					= _P->getPoly().size()-2;
		for			(u32 _pit=0; _pit<_polys; _pit++)	{
			_v->set	(_P->getPoly()[0],		_clr);	_v++;
			_v->set (_P->getPoly()[_pit+1],_clr);	_v++;
			_v->set (_P->getPoly()[_pit+2],_clr);	_v++;
		}
	}
	RCache.Vertex.Unlock			(_pcount*3,f_geom.stride());

	// render
	RCache.set_xform_world			(Fidentity);
	RCache.set_Shader				(f_shader);
	RCache.set_Geometry				(f_geom);
	RCache.set_CullMode				(CULL_NONE);
	RCache.Render					(D3DPT_TRIANGLELIST,_offset,_pcount);
	RCache.set_CullMode				(CULL_CCW);

	// cleanup
	f_portals.clear					();
}

#ifdef DEBUG
void CPortalTraverser::dbg_draw		()
{
	RCache.OnFrameEnd		();
	RCache.set_xform_world	(Fidentity);
	RCache.set_xform_view	(Fidentity);
	RCache.set_xform_project(Fidentity);
	for (u32 s=0; s<dbg_sectors.size(); s++)	{
		CSector*	S		= (CSector*)dbg_sectors[s];
		FVF::L		verts	[5];
		Fbox2		bb		= S->r_scissor_merged;
		bb.min.x			= bb.min.x * 2 - 1;
		bb.max.x			= bb.max.x * 2 - 1;
		bb.min.y			= (1-bb.min.y) * 2 - 1;
		bb.max.y			= (1-bb.max.y) * 2 - 1;

		verts[0].set(bb.min.x,bb.min.y,EPS,0xffffffff);
		verts[1].set(bb.max.x,bb.min.y,EPS,0xffffffff);
		verts[2].set(bb.max.x,bb.max.y,EPS,0xffffffff);
		verts[3].set(bb.min.x,bb.max.y,EPS,0xffffffff);
		verts[4].set(bb.min.x,bb.min.y,EPS,0xffffffff);
		RCache.dbg_Draw		(D3DPT_LINESTRIP,verts,4);
	}
}
#endif
