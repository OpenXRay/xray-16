#include "stdafx.h"


//#include "build.h"
#include "xrMU_Model.h"
//#include "xrLC_GlobalData.h"
#include "light_point.h"
//#include "xrDeflector.h"
#include "../../xrcdb/xrcdb.h"
#include "../shader_xrlc.h"
#include "mu_model_face.h"
#include "xrface.h"
#include "xrLC_GlobalData.h"

void LightPoint(CDB::COLLIDER* DB, CDB::MODEL* MDL, base_color_c &C, Fvector &P, Fvector &N, base_lighting& lights, u32 flags, Face* skip);
union var
{
	int		i;
	float	f;
	bool	b;

	operator float()			{ return f; }
	operator int()				{ return i; }
	operator bool()				{ return b; }

	var& operator = (float _f)	{ f=_f;	return *this; }
	var& operator = (int _i)	{ i=_i;	return *this; }
	var& operator = (bool _b)	{ b=_b;	return *this; }

	var()  						{ }
	var(float _f) : f(_f) 		{ }
	var(int _i)	: i(_i)			{ }
	var(bool _b) : b(_b)		{ }
};

/*
var		test;

test	= 0.f;
int	k	= test;

test	= true;
float f = test;

float x = 10.f;
var _x	= var(x);
*/

//-----------------------------------------------------------------------
void xrMU_Model::calc_lighting	(xr_vector<base_color>& dest, const Fmatrix& xform, CDB::MODEL* MDL, base_lighting& lights, u32 flags)
{
	// trans-map
	typedef	xr_multimap<float,v_vertices>	mapVert;
	typedef	mapVert::iterator				mapVertIt;
	mapVert									g_trans;
	u32										I;

	// trans-epsilons
	const float eps			= EPS_L;
	const float eps2		= 2.f*eps;

	// calc pure rotation matrix
	Fmatrix Rxform,tmp,R;
	R.set						(xform	);
	R.translate_over			(0,0,0	);
	tmp.transpose				(R		);
	Rxform.invert				(tmp	);

	// Perform lighting
	CDB::COLLIDER				DB;
	DB.ray_options				(0);

	// Disable faces if needed
	/*
	BOOL bDisableFaces			= flags&LP_UseFaceDisable;
	if	(bDisableFaces)
		for (I=0; I<m_faces.size(); I++)	m_faces[I]->flags.bDisableShadowCast	= true;
	*/

	// Perform lighting
	for (I = 0; I<m_vertices.size(); I++)
	{
		_vertex*	V			= m_vertices[I];

		// Get ambient factor
		float		v_amb		= 0.f;
		float		v_trans		= 0.f;
		for (u32 f=0; f<V->m_adjacents.size(); f++)
		{
			_face*	F			=	V->m_adjacents[f];
			v_amb				+=	F->Shader().vert_ambient;
			v_trans				+=	F->Shader().vert_translucency;
		}
		v_amb					/=	float(V->m_adjacents.size());
		v_trans					/=	float(V->m_adjacents.size());
		float v_inv				=	1.f-v_amb;

		base_color_c			vC;
		Fvector					vP,vN;
		xform.transform_tiny	(vP,V->P);
		Rxform.transform_dir	(vN,V->N);
		exact_normalize			(vN); 

		// multi-sample
		const int n_samples		= (g_params().m_quality==ebqDraft)?1:6;
		for (u32 sample=0; sample<(u32)n_samples; sample++)
		{
			float				a	= 0.2f * float(sample) / float(n_samples);
			Fvector				P,N;
			N.random_dir		(vN,deg2rad(30.f));
			P.mad				(vP,N,a);
			LightPoint			(&DB, MDL, vC, P, N, lights, flags, 0);
		}
		vC.scale				(n_samples);
		vC._tmp_				=	v_trans;
		if (flags&LP_dont_hemi) ;
		else					vC.hemi	+=	v_amb;
		V->C._set				(vC);

		// Search
		const float key			= V->P.x;
		mapVertIt	it			= g_trans.lower_bound	(key);
		mapVertIt	it2			= it;

		// Decrement to the start and inc to end
		while (it!=g_trans.begin() && ((it->first+eps2)>key)) it--;
		while (it2!=g_trans.end() && ((it2->first-eps2)<key)) it2++;
		if (it2!=g_trans.end())	it2++;

		// Search
		BOOL	found = FALSE;
		for (; it!=it2; it++)
		{
			v_vertices&	VL		= it->second;
			_vertex* Front		= VL.front();
			R_ASSERT			(Front);
			if (Front->P.similar(V->P,eps))
			{
				found				= TRUE;
				VL.push_back		(V);
			}
		}

		// Register
		if (!found)				{
			mapVertIt	ins			= g_trans.insert(mk_pair(key,v_vertices()));
			ins->second.reserve		(32);
			ins->second.push_back	(V);
		}
	}

	// Enable faces if needed
	/*
	if	(bDisableFaces)
		for (I=0; I<m_faces.size(); I++)	m_faces[I]->flags.bDisableShadowCast	= true;
	*/

	// Process all groups
	for (mapVertIt it=g_trans.begin(); it!=g_trans.end(); it++)
	{
		// Unique
		v_vertices&	VL		= it->second;
		std::sort			(VL.begin(),VL.end());
		VL.erase			(std::unique(VL.begin(),VL.end()),VL.end());

		// Calc summary color
		base_color_c	C;
		for (int v=0; v<int(VL.size()); v++)
		{
			base_color_c	vC;
			VL[v]->C._get	(vC);
			C.max			(vC);
		}

		// Calculate final vertex color
		for (u32 v=0; v<int(VL.size()); v++)
		{
			base_color_c		vC;
			VL[v]->C._get		(vC);

			// trans-level
			float	level		= vC._tmp_;

			// 
			base_color_c		R;
			R.lerp				(vC,C,level);
			R.max				(vC);
			R.mul				(.5f);
			VL[v]->C._set		(R);
		}
	}

	// Transfer colors to destination
	dest.resize				(m_vertices.size());
	for (I = 0; I<m_vertices.size(); I++)
	{
		Fvector		ptPos	= m_vertices[I]->P;
		base_color	ptColor	= m_vertices[I]->C;
		dest[I]				= ptColor;
	}
}

void xrMU_Model::calc_lighting	()
{
	// BB
	Fbox			BB; 
	BB.invalidate	();
	for (v_vertices_it vit=m_vertices.begin(); vit!=m_vertices.end(); vit++)
		BB.modify	((*vit)->P);

	// Export CForm
	CDB::CollectorPacked	CL	(BB,(u32)m_vertices.size(),(u32)m_faces.size());
	export_cform_rcast		(CL,Fidentity);

	CDB::MODEL*				M	= xr_new<CDB::MODEL>	();
	M->build				(CL.getV(),(u32)CL.getVS(),CL.getT(),(u32)CL.getTS());

	calc_lighting			(color,Fidentity,M,inlc_global_data()->L_static(),LP_dont_rgb+LP_dont_sun);

	xr_delete				(M);

	clMsg					("model '%s' - REF_lighted.",*m_name);
}



