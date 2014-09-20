#include "stdafx.h"
#include "xrLightVertex.h"
#include "xrThread.h"
#include "xrface.h"
#include "xrLC_GlobalData.h"
#include "light_point.h"

#include "../../xrcdb/xrCDB.h"
//-----------------------------------------------------------------------
typedef	xr_multimap<float,vecVertex>	mapVert;
typedef	mapVert::iterator				mapVertIt;
mapVert*								g_trans;
xrCriticalSection						g_trans_CS
#ifdef PROFILE_CRITICAL_SECTIONS
	(MUTEX_PROFILE_ID(g_trans_CS))
#endif // PROFILE_CRITICAL_SECTIONS
;
extern XRLC_LIGHT_API void		LightPoint		(CDB::COLLIDER* DB, CDB::MODEL* MDL, base_color_c &C, Fvector &P, Fvector &N, base_lighting& lights, u32 flags, Face* skip);

void	g_trans_register_internal		(Vertex* V)
{
	R_ASSERT	(V);

	const float eps		= EPS_L;
	const float eps2	= 2.f*eps;
	
	// Search
	const float key		= V->P.x;
	mapVertIt	it		= g_trans->lower_bound	(key);
	mapVertIt	it2		= it;

	// Decrement to the start and inc to end
	while (it!=g_trans->begin() && ((it->first+eps2)>key)) it--;
	while (it2!=g_trans->end() && ((it2->first-eps2)<key)) it2++;
	if (it2!=g_trans->end())	it2++;
	
	// Search
	for (; it!=it2; it++)
	{
		vecVertex&	VL		= it->second;
		Vertex* Front		= VL.front();
		R_ASSERT			(Front);
		if (Front->P.similar(V->P,eps))
		{
			VL.push_back		(V);
			return;
		}
	}

	// Register
	mapVertIt	ins			= g_trans->insert(mk_pair(key,vecVertex()));
	ins->second.reserve		(32);
	ins->second.push_back	(V);
}
void	g_trans_register	(Vertex* V)
{
	g_trans_CS.Enter			();
	g_trans_register_internal	(V);
	g_trans_CS.Leave			();
}

//////////////////////////////////////////////////////////////////////////
const u32				VLT_END		= u32(-1);
class CVertexLightTasker
{
	xrCriticalSection	cs;
	volatile u32		index;	
public:
	CVertexLightTasker	() : index(0)
#ifdef PROFILE_CRITICAL_SECTIONS
		,cs(MUTEX_PROFILE_ID(CVertexLightTasker))
#endif // PROFILE_CRITICAL_SECTIONS
	{};
	
	void	init		()
	{
		index			= 0;
	}

	u32		get			()
	{
		cs.Enter		();
		u32 _res		=	index;
		if (_res>=lc_global_data()->g_vertices().size())	_res	=	VLT_END;
		else							index	+=	1;
		cs.Leave		();
		return			_res;
	}
};
CVertexLightTasker		VLT;

bool GetTranslucency(const Vertex* V,float &v_trans )
{
	// Get transluency factor
			
	bool		bVertexLight= FALSE;
	u32 		L_flags		= 0;
	for (u32 f=0; f<V->m_adjacents.size(); ++f)
	{
		Face*	F								=	V->m_adjacents[f];
		v_trans									+=	F->Shader().vert_translucency;
		if	(F->Shader().flags.bLIGHT_Vertex)	
			bVertexLight		= TRUE;
	}
	v_trans				/=	float(V->m_adjacents.size());
	return bVertexLight;
}

class CVertexLightThread : public CThread
{
public:
	CVertexLightThread(u32 ID) : CThread(ID)
	{
		thMessages	= FALSE;
	}
	virtual void		Execute	()
	{
		u32	counter		= 0;
		for (;; counter++)
		{
			u32 id				= VLT.get();
			if (id==VLT_END)	break;

			Vertex* V		= lc_global_data()->g_vertices()[id];

			R_ASSERT		(V);

			float		v_trans		= 0.f;

			if (GetTranslucency( V, v_trans ))	
			{
				base_color_c		vC, old;
				V->C._get			(old);

				CDB::COLLIDER	DB;
				DB.ray_options	(0);
				LightPoint			(&DB, lc_global_data()->RCAST_Model(), vC, V->P, V->N, lc_global_data()->L_static(), (lc_global_data()->b_nosun()?LP_dont_sun:0)|LP_dont_hemi, 0);
				vC._tmp_			= v_trans;
				vC.mul				(.5f);
				vC.hemi				= old.hemi;			// preserve pre-calculated hemisphere
				V->C._set			(vC);

				g_trans_register	(V);
			}

			thProgress			= float(counter) / float(lc_global_data()->g_vertices().size());
		}
	}
};
namespace lc_net{
void RunLightVertexNet();
}
#define NUM_THREADS			4
void LightVertex	( bool net )
{
	g_trans				= xr_new<mapVert>	();

	// Start threads, wait, continue --- perform all the work
	Status				("Calculating...");
	if( !net )
	{
		CThreadManager		Threads;
		VLT.init			();
		CTimer	start_time;	start_time.Start();				
		for (u32 thID=0; thID<NUM_THREADS; thID++)	Threads.start(xr_new<CVertexLightThread>(thID));
		Threads.wait		();
		clMsg				("%f seconds",start_time.GetElapsed_sec());
	} else
	{
		lc_net::RunLightVertexNet();
	}
	// Process all groups
	Status				("Transluenting...");
	for (mapVertIt it=g_trans->begin(); it!=g_trans->end(); it++)
	{
		// Unique
		vecVertex&	VL	= it->second;
		std::sort		(VL.begin(),VL.end());
		VL.erase		(std::unique(VL.begin(),VL.end()),VL.end());

		// Calc summary color
		base_color_c	C;
		for (u32 v=0; v<VL.size(); v++)
		{
			base_color_c	cc;	VL[v]->C._get(cc);
			C.max			(cc);
		}

		// Calculate final vertex color
		for (u32 v=0; v<VL.size(); v++)
		{
			base_color_c		vC;
			VL[v]->C._get		(vC);

			// trans-level
			float	level		= vC._tmp_;

			// 
			base_color_c		R;
			R.lerp				(vC,C,level);
			R.max				(vC);
			VL[v]->C._set		(R);
		}
	}
	xr_delete	(g_trans);
	Status				("Wating...");
}
