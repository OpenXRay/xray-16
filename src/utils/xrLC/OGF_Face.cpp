#include "stdafx.h"
#include "build.h"
#include "ogf_face.h"
//#include "std_classes.h"
#include "../../xrcore/fs.h"
#include "../../xrEngine/fmesh.h"
#include "xrOcclusion.h"


using namespace std;

void set_status(char* N, int id, int f, int v)
{
	string1024 status_str;

	xr_sprintf	(status_str,"Model #%4d [F:%5d, V:%5d]: %s...",id,f,v,N);
	Status	(status_str);
	clMsg	(status_str);
}

BOOL OGF_Vertex::similar(OGF* ogf, OGF_Vertex& V)
{
	const float ntb		= _cos	(deg2rad(5.f));
	if (!P.similar(V.P)) 		return FALSE;
	if (!N.similar(V.N)) 		return FALSE;
	if (!T.similar(V.T)) 		return FALSE;
	if (!B.similar(V.B)) 		return FALSE;
	
	R_ASSERT(UV.size()==V.UV.size());
	for (u32 i=0; i<V.UV.size(); i++) {
		OGF_Texture *T = &*ogf->textures.begin()+i;
		b_texture	*B = T->pBuildSurface;
		float		eu = 1.f/float(B->dwWidth );
		float		ev = 1.f/float(B->dwHeight);
		if (!UV[i].similar(V.UV[i],eu,ev)) return FALSE;
	}
	return TRUE;
}
void OGF_Vertex::dump	(u32 id)
{
//	Msg	("%d: ");
}
BOOL x_vertex::similar	(OGF* ogf, x_vertex& V)
{
	return P.similar(V.P);
}
u16 OGF::x_BuildVertex	(x_vertex& V1)
{
	for (itXV it=fast_path_data.vertices.begin(); it!=fast_path_data.vertices.end(); it++)
		if (it->similar(this,V1)) return u16(it-fast_path_data.vertices.begin());
	fast_path_data.vertices.push_back	(V1);
	return (u32)			fast_path_data.vertices.size()-1;
}
u16 OGF::_BuildVertex	(OGF_Vertex& V1)
{
	try {
		for (itOGF_V it=data.vertices.begin(); it!=data.vertices.end(); it++)
		{
			if (it->similar(this,V1)) return u16(it-data.vertices.begin());
		}
	} catch (...) { clMsg("* ERROR: OGF::_BuildVertex");	}

	data.vertices.push_back	(V1);
	return (u32)data.vertices.size()-1;
}
void OGF::x_BuildFace	(OGF_Vertex& V1, OGF_Vertex& V2, OGF_Vertex& V3, bool _tc_)
{
	if (_tc_)	return	;	// make empty-list for stuff that has relevant TCs
	x_face	F;
	u32		VertCount	= (u32)fast_path_data.vertices.size();
	F.v[0]	= x_BuildVertex(x_vertex(V1));
	F.v[1]	= x_BuildVertex(x_vertex(V2));
	F.v[2]	= x_BuildVertex(x_vertex(V3));
	if (!F.Degenerate()) {
		fast_path_data.faces.push_back(F);
	} else {
		if (fast_path_data.vertices.size()>VertCount) 
			fast_path_data.vertices.erase(fast_path_data.vertices.begin()+VertCount,fast_path_data.vertices.end());
	}
}
void OGF::_BuildFace	(OGF_Vertex& V1, OGF_Vertex& V2, OGF_Vertex& V3, bool _tc_)
{
	OGF_Face			F;
	u32		VertCount	= (u32)data.vertices.size();
	F.v[0]	= _BuildVertex(V1);
	F.v[1]	= _BuildVertex(V2);
	F.v[2]	= _BuildVertex(V3);
	if (!F.Degenerate()) {
		for (itOGF_F I=data.faces.begin(); I!=data.faces.end(); I++)		if (I->Equal(F)) return;
		data.faces.push_back	(F);
		x_BuildFace		(V1,V2,V3,_tc_);
	} else {
		if (data.vertices.size()>VertCount) 
				data.vertices.erase(data.vertices.begin()+VertCount,data.vertices.end());
	}
}
BOOL OGF::dbg_SphereContainsVertex(Fvector& c, float R)
{
	Fsphere	S;	S.set(c,R);
	for (u32 it=0; it<data.vertices.size(); it++)
		if (S.contains(data.vertices[it].P))	return	TRUE;
	return FALSE	;
}

void OGF::adjacent_select	(xr_vector<u32>& dest, xr_vector<bool>& vmark, xr_vector<bool>& fmark)
{
	// 0. Search for the group
	for (u32 fit=0; fit<data.faces.size(); fit++)	{
		OGF_Face&	F		= data.faces	[fit];
		if (fmark[fit])		continue;			// already registered

		// new face - if empty - just put it in, else check connectivity
		if (dest.empty())	{
			fmark[fit]		= true	;
			dest.push_back	(F.v[0]);	vmark[F.v[0]]=true;
			dest.push_back	(F.v[1]);	vmark[F.v[1]]=true;
			dest.push_back	(F.v[2]);	vmark[F.v[2]]=true;
		} else {
			// check connectivity
			BOOL	bConnected	=	FALSE;
			for (u32 vid=0; vid<3; vid++)	{
				u32		id = F.v	[vid];	// search in already registered verts
				for (u32 sid=0; sid<dest.size(); sid++)
				{
					if (id==dest[sid])	{
						bConnected	= TRUE;	// this face shares at least one vertex with already selected faces
						break;
					}
				}
				if (bConnected)	break;
			}
			if (bConnected)		{
				// add this face's vertices
				fmark[fit]	= true	;
				if (!vmark[F.v[0]])	{ dest.push_back	(F.v[0]);	vmark[F.v[0]]=true; }
				if (!vmark[F.v[1]])	{ dest.push_back	(F.v[1]);	vmark[F.v[1]]=true; }
				if (!vmark[F.v[2]])	{ dest.push_back	(F.v[2]);	vmark[F.v[2]]=true; }
			}
		}
	}
}

void OGF::Optimize	()
{
	// Real optimization
	//////////////////////////////////////////////////////////////////////////
	// x-vertices
	try {
		if (fast_path_data.vertices.size() && fast_path_data.faces.size())
		{
			try {
				VERIFY	(fast_path_data.vertices.size()	<= data.vertices.size()	);
				VERIFY	(fast_path_data.faces.size()		== data.faces.size()		);
			} catch(...) {
				Msg	("* ERROR: optimize: x-geom : verify: failed");
			}

			// Optimize texture coordinates
			/*
			Fvector2 Tdelta;
			try {
				// 1. Calc bounds
				Fvector2 Tmin,Tmax;
				Tmin.set(flt_max,flt_max);
				Tmax.set(flt_min,flt_min);
				for (u32 j=0; j<x_vertices.size(); j++)			{
					x_vertex& V = x_vertices[j];
					//Tmin.min	(V.UV);
					//Tmax.max	(V.UV);
				}
				Tdelta.x = floorf((Tmax.x-Tmin.x)/2+Tmin.x);
				Tdelta.y = floorf((Tmax.y-Tmin.y)/2+Tmin.y);
			} catch(...) {
				Msg	("* ERROR: optimize: x-geom : bounds: failed");
			}

			// 2. Recalc UV mapping
			try {
				for (u32 i=0; i<x_vertices.size(); i++)
					x_vertices[i].UV.sub	(Tdelta);
			} catch(...) {
				Msg	("* ERROR: optimize: x-geom : recalc : failed");
			}
			*/
		}
	} catch(...) {
		Msg	("* ERROR: optimize: x-geom : failed");
	}

	//////////////////////////////////////////////////////////////////////////
	// Detect relevant number of UV pairs
	try {
		R_ASSERT			(data.vertices.size());
		dwRelevantUV		= data.vertices.front().UV.size();
		const Shader_xrLC*	SH	= pBuild->shaders().Get(pBuild->materials()[material].reserved);
		if (!SH->flags.bOptimizeUV)		return;
	} catch(...) {
		Msg	("* ERROR: optimize: std-geom : find relevant UV");
	}

	// Build p-rep
	/*
	typedef xr_vector<u32>	flist	;
	xr_vector<flist>		prep	;	prep.resize(vertices.size());
	for (u32 fit=0; fit<faces.size(); fit++)	{
		OGF_Face&	F		= faces	[fit];
		prep[F.v[0]].push_back		(fit);
		prep[F.v[1]].push_back		(fit);
		prep[F.v[2]].push_back		(fit);
	}
	*/

	// Optimize texture coordinates
	xr_vector<bool>	vmarker;	vmarker.assign	(data.vertices.size(),false);
	xr_vector<bool>	fmarker;	fmarker.assign	(data.faces.size(),false);

	for (;;)	{
		// 0. Search for the group
		xr_vector<u32>	selection		;
		for (;;)	{
			u32		_old	= selection.size();
			adjacent_select	(selection,vmarker,fmarker);
			u32		_new	= selection.size();
			if (_old==_new)	break;		// group selected !
		}
		if (selection.empty())		break;

		// 1. Calc bounds
		Fvector2 Tdelta;
		try {
			Fvector2 Tmin,Tmax;
			Tmin.set(flt_max,flt_max);
			Tmax.set(flt_min,flt_min);
			for (u32 j=0; j<selection.size(); j++)
			{
				OGF_Vertex& V = data.vertices[selection[j]];
				Tmin.min(V.UV[0]);
				Tmax.max(V.UV[0]);
			}
			Tdelta.x = floorf((Tmax.x-Tmin.x)/2+Tmin.x);
			Tdelta.y = floorf((Tmax.y-Tmin.y)/2+Tmin.y);
		} catch(...) {
			Msg	("* ERROR: optimize: std-geom : delta UV");
		}

		// 2. Recalc UV mapping
		try {
			for (u32 i=0; i<selection.size(); i++)
				data.vertices[selection[i]].UV[0].sub(Tdelta);
		} catch(...) {
			Msg	("* ERROR: optimize: std-geom : recalc UV");
		}
		selection.clear	();
	}
}



// Make Progressive
xrCriticalSection			progressive_cs
#ifdef PROFILE_CRITICAL_SECTIONS
	(MUTEX_PROFILE_ID(progressive_cs))
#endif // PROFILE_CRITICAL_SECTIONS
;
void OGF::MakeProgressive	(float metric_limit)
{
	// test
	// there is no-sense to simplify small models
	// for batch size 50,100,200 - we are CPU-limited anyway even on nv30
	// for nv40 and up the better guess will probably be around 500
	if (data.faces.size()<c_PM_FaceLimit)		return		;	

//. AlexMX added for draft build mode
	if (g_params().m_quality==ebqDraft)		return		;

	progressive_cs.Enter	();

	//////////////////////////////////////////////////////////////////////////
	// NORMAL
	vecOGF_V	_saved_vertices		=	data.vertices	;
	vecOGF_F	_saved_faces		=	data.faces		;

	{
		// prepare progressive geom
		VIPM_Init				();
		//clMsg("--- append v start .");
		for (u32 v_idx=0;  v_idx<data.vertices.size(); v_idx++)	
			VIPM_AppendVertex	(data.vertices[v_idx].P,	data.vertices[v_idx].UV[0]					);
		//clMsg("--- append f start .");
		for (u32 f_idx=0;  f_idx<data.faces.size();    f_idx++)	
			VIPM_AppendFace		(data.faces[f_idx].v[0],	data.faces[f_idx].v[1],	data.faces[f_idx].v[2]	);
		//clMsg("--- append end.");

		// Convert
		VIPM_Result*	VR		= 0;
		try						{
						VR		= VIPM_Convert			(u32(25),1.f,1);
		} catch (...)			{
			progressive_clear	()		;
			clMsg				("* mesh simplification failed: access violation");
		}
		if (0==VR)				{
			progressive_clear	()		;
			clMsg				("* mesh simplification failed");
		}
		while (VR && VR->swr_records.size()>0)	{
			// test metric
			u32		_full	=	data.vertices.size	()		;
			u32		_remove	=	VR->swr_records.size()	;
			u32		_simple	=	_full - _remove			;
			float	_metric	=	float(_remove)/float(_full);
			if		(_metric<metric_limit)		{
				progressive_clear				()		;
				clMsg	("* mesh simplified from [%4dv] to [%4dv], nf[%4d] ==> em[%0.2f]-discarded",_full,_simple,VR->indices.size()/3,metric_limit);
				break									;
			} else {
				clMsg	("* mesh simplified from [%4dv] to [%4dv], nf[%4d] ==> em[%0.2f]-accepted", _full,_simple,VR->indices.size()/3,metric_limit);
			}

			// OK
			// Permute vertices
			for(u32 i=0; i<data.vertices.size(); i++)
				data.vertices[VR->permute_verts[i]]=_saved_vertices[i];

			// Fill indices
			data.faces.resize			(VR->indices.size()/3);
			for (u32 f_idx=0; f_idx<data.faces.size(); f_idx++){
				data.faces[f_idx].v[0]	= VR->indices[f_idx*3+0];
				data.faces[f_idx].v[1]	= VR->indices[f_idx*3+1];
				data.faces[f_idx].v[2]	= VR->indices[f_idx*3+2];
			}
			// Fill SWR
			data.m_SWI.count				= VR->swr_records.size();
			data.m_SWI.sw				= xr_alloc<FSlideWindow>(data.m_SWI.count);
			for (u32 swr_idx=0; swr_idx!=data.m_SWI.count; swr_idx++){
				FSlideWindow& dst	= data.m_SWI.sw[swr_idx];
				VIPM_SWR& src		= VR->swr_records[swr_idx];
				dst.num_tris		= src.num_tris;
				dst.num_verts		= src.num_verts;
				dst.offset			= src.offset;
			}

			break	;
		}
		// cleanup
		VIPM_Destroy			();
	}

	//////////////////////////////////////////////////////////////////////////
	// FAST-PATH
	if (progressive_test() && fast_path_data.vertices.size() && fast_path_data.faces.size())
	{
		// prepare progressive geom
		VIPM_Init				();
		Fvector2				zero; zero.set		(0,0);
		for (u32 v_idx=0;  v_idx<fast_path_data.vertices.size(); v_idx++)	VIPM_AppendVertex	(fast_path_data.vertices[v_idx].P,	zero						);
		for (u32 f_idx=0;  f_idx<fast_path_data.faces.size();    f_idx++)	VIPM_AppendFace		(fast_path_data.faces[f_idx].v[0],	fast_path_data.faces[f_idx].v[1],	fast_path_data.faces[f_idx].v[2]	);

		VIPM_Result*	VR		= 0;
		try						{
			VR		= VIPM_Convert			(u32(25),1.f,1);
		} catch (...)			{
			data.faces				= _saved_faces		;
			data.vertices			= _saved_vertices	;
			progressive_clear	()		;
			clMsg				("* X-mesh simplification failed: access violation");
		}
		if (0==VR)				{
			data.faces				= _saved_faces		;
			data.vertices			= _saved_vertices	;
			progressive_clear	()		;
			clMsg				("* X-mesh simplification failed");
		} else {
			// Convert
			/*
			VIPM_Result*	VR		= VIPM_Convert		(u32(25),1.f,1);
			VERIFY			(VR->swr_records.size()>0)	;
			*/

			// test metric
			u32		_full	=	data.vertices.size	()		;
			u32		_remove	=	VR->swr_records.size()	;
			u32		_simple	=	_full - _remove			;
			float	_metric	=	float(_remove)/float(_full);
			clMsg	("X mesh simplified from [%4dv] to [%4dv], nf[%4d]",_full,_simple,VR ? VR->indices.size()/3 : 0);

			// OK
			vec_XV					vertices_saved;

			// Permute vertices
			vertices_saved			= fast_path_data.vertices;
			for(u32 i=0; i<fast_path_data.vertices.size(); i++)
				fast_path_data.vertices[VR->permute_verts[i]]=vertices_saved[i];

			// Fill indices
			fast_path_data.faces.resize			(VR->indices.size()/3);
			for (u32 f_idx=0; f_idx<fast_path_data.faces.size(); f_idx++){
				fast_path_data.faces[f_idx].v[0]	= VR->indices[f_idx*3+0];
				fast_path_data.faces[f_idx].v[1]	= VR->indices[f_idx*3+1];
				fast_path_data.faces[f_idx].v[2]	= VR->indices[f_idx*3+2];
			}

			// Fill SWR
			fast_path_data.m_SWI.count				= VR->swr_records.size();
			fast_path_data.m_SWI.sw					= xr_alloc<FSlideWindow>(fast_path_data.m_SWI.count);
			for (u32 swr_idx=0; swr_idx!=fast_path_data.m_SWI.count; swr_idx++){
				FSlideWindow& dst	= fast_path_data.m_SWI.sw[swr_idx];
				VIPM_SWR& src		= VR->swr_records[swr_idx];
				dst.num_tris		= src.num_tris;
				dst.num_verts		= src.num_verts;
				dst.offset			= src.offset;
			}
		}

		// cleanup
		VIPM_Destroy			();
	}

	progressive_cs.Leave	();
}

void OGF_Base::Save	(IWriter &fs)
{
}

// Represent a node as HierrarhyVisual
void OGF_Node::Save	(IWriter &fs)
{
	OGF_Base::Save		(fs);

	// Header
	fs.open_chunk		(OGF_HEADER);
	ogf_header H;
	H.format_version	= xrOGF_FormatVersion;
	H.type				= MT_HIERRARHY;
	H.shader_id			= 0;
	H.bb.min			= bbox.min;
	H.bb.max			= bbox.max;
	H.bs.c				= C;
	H.bs.r				= R;
	fs.w				(&H,sizeof(H));
	fs.close_chunk		();

	// Children
	fs.open_chunk		(OGF_CHILDREN_L);
	fs.w_u32			((u32)chields.size());
	fs.w				(&*chields.begin(),(u32)chields.size()*sizeof(u32));
	fs.close_chunk		();
}

extern u16	RegisterShader	(LPCSTR T);

void OGF_LOD::Save		(IWriter &fs)
{
	OGF_Base::Save		(fs);

	// Header
	ogf_header			H;
	string1024			sid;
	strconcat			(sizeof(sid),sid,
		pBuild->shader_render[pBuild->materials()[lod_Material].shader].name,
		"/",
		pBuild->textures()[pBuild->materials()[lod_Material].surfidx].name
		);
	fs.open_chunk		(OGF_HEADER);
	H.format_version	= xrOGF_FormatVersion;
	H.type				= MT_LOD;
	H.shader_id			= RegisterShader(sid);
	H.bb.min			= bbox.min;
	H.bb.max			= bbox.max;
	H.bs.c				= C;
	H.bs.r				= R;
	fs.w				(&H,sizeof(H));
	fs.close_chunk		();

	// Chields
	fs.open_chunk		(OGF_CHILDREN_L);
	fs.w_u32			((u32)chields.size());
	fs.w				(&*chields.begin(),(u32)chields.size()*sizeof(u32));
	fs.close_chunk		();

	// Lod-def
	fs.open_chunk		(OGF_LODDEF2);
	fs.w				(lod_faces,sizeof(lod_faces));
	fs.close_chunk		();
}
