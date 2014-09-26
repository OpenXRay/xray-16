#include "stdafx.h"
#include "compiler.h"

#include "cl_intersect.h"

#include "motion_simulator.h"

#pragma warning(disable:4995)
#include <freemagic/MgcAppr3DPlaneFit.h>
#pragma warning(default:4995)

IC void SnapXZ	(Fvector&	V)
{
	V.x = snapto(V.x,g_params.fPatchSize);
	V.z = snapto(V.z,g_params.fPatchSize);
}

IC void BoxQuery(Fbox& BB, bool exact)
{
	if (exact) 		XRC.box_options	(CDB::OPT_FULL_TEST);
	else			XRC.box_options	(0);
	Fvector			C,D;
	BB.get_CD		(C,D);
	XRC.box_query	(&Level,C,D);
}

struct tri	{
	Fvector v[3];
	u32	sector;
	Fvector	N;
};

const float RCAST_VALID = 0.55f;
BOOL	CreateNode(Fvector& vAt, vertex& N)
{
	// *** Query and cache polygons for ray-casting
	Fvector	PointUp;		PointUp.set(vAt);	PointUp.y	+= RCAST_Depth;		SnapXZ	(PointUp);
	Fvector	PointDown;		PointDown.set(vAt);	PointDown.y	-= RCAST_Depth;		SnapXZ	(PointDown);

	Fbox	BB;				BB.set	(PointUp,PointUp);		BB.grow(g_params.fPatchSize/2);	// box 1
	Fbox	B2;				B2.set	(PointDown,PointDown);	B2.grow(g_params.fPatchSize/2);	// box 2
	BB.merge(B2			);
	BoxQuery(BB,false	);
	u32	dwCount = XRC.r_count();
	if (dwCount==0)	{
//		Log("chasm1");
		return FALSE;			// chasm?
	}

	// *** Transfer triangles and compute sector
	R_ASSERT(dwCount<RCAST_MaxTris);
	static svector<tri,RCAST_MaxTris> tris;		tris.clear();
	for (u32 i=0; i<dwCount; i++)
	{
		tri&		D = tris.last();
		CDB::RESULT	&rp = XRC.r_begin()[i];
		CDB::TRI&	T = *(Level.get_tris()+rp.id);

		D.v[0].set	(rp.verts[0]);
		D.v[1].set	(rp.verts[1]);
		D.v[2].set	(rp.verts[2]);
		D.sector	= T.sector;
		D.N.mknormal(D.v[0],D.v[1],D.v[2]);
		if (D.N.y<=0)	continue;

		tris.inc	();
	}
	if (tris.size()==0)	{
//		Log("chasm2");
		return FALSE;			// chasm?
	}

	// *** Perform ray-casts and calculate sector
	WORD Sector = 0xfffe;	// mark as first time

	static svector<Fvector,RCAST_Total>	points;		points.clear();
	static svector<Fvector,RCAST_Total>	normals;	normals.clear();
	Fvector P,D; D.set(0,-1,0);

	float coeff = 0.5f*g_params.fPatchSize/float(RCAST_Count);

	for (int x=-RCAST_Count; x<=RCAST_Count; x++) 
	{
		P.x = vAt.x + coeff*float(x);
		for (int z=-RCAST_Count; z<=RCAST_Count; z++) {
			P.z = vAt.z + coeff*float(z);
			P.y = vAt.y + 10.f;

			float	tri_min_range	= flt_max;
			int		tri_selected	= -1;
			float	range,u,v;
			for (i=0; i<u32(tris.size()); i++) 
			{
				if (CDB::TestRayTri(P,D,tris[i].v,u,v,range,false)) 
				{
					if (range<tri_min_range) {
						tri_min_range	= range;
						tri_selected	= i;
					}
				}
			}
			if (tri_selected>=0) {
				P.y -= tri_min_range;
				points.push_back(P);
				normals.push_back(tris[tri_selected].N);
				WORD TS = WORD(tris[tri_selected].sector);
				if (Sector==0xfffe)	Sector = TS;
				else 				if (Sector!=TS) Sector=InvalidSector;
			}
		}
	}
	if (points.size()<3) {
//		Msg		("Failed to create node at [%f,%f,%f].",vAt.x,vAt.y,vAt.z);
		return	FALSE;
	}
	if (float(points.size())/float(RCAST_Total) < 0.7f) {
//		Msg		("Partial chasm at [%f,%f,%f].",vAt.x,vAt.y,vAt.z);
		return	FALSE;
	}

	// *** Calc normal
	Fvector vNorm;
	vNorm.set(0,0,0);
	for (u32 n=0; n<normals.size(); n++)
		vNorm.add(normals[n]);
	vNorm.div(float(normals.size()));
	vNorm.normalize();
	/*
	{
		// second algorithm (Magic)
		Fvector N,O;
		N.set(vNorm);
		O.set(points[0]);
		Mgc::OrthogonalPlaneFit(
			points.size(),(Mgc::Vector3*)points.begin(),
			*((Mgc::Vector3*)&O),
			*((Mgc::Vector3*)&N)
		);
		if (N.y<0) N.invert();
		N.normalize();
		vNorm.lerp(vNorm,N,.3f);
		vNorm.normalize();
	}
	*/

 
	// *** Align plane
	Fvector vOffs;
	vOffs.set(0,-1000,0);
	Fplane PL; 	PL.build(vOffs,vNorm);
	for (u32 p=0; p<points.size(); p++)
	{
		float dist = PL.classify(points[p]);
		if (dist>0) {
			vOffs = points[p];
			PL.build(vOffs,vNorm);
		}
	}

	// *** Create node and register it
	N.Sector		=Sector;						// sector
	N.Plane.build	(vOffs,vNorm);					// build plane
	D.set			(0,1,0);
	N.Plane.intersectRayPoint(PointDown,D,N.Pos);	// "project" position

	// *** Validate results
	vNorm.set(0,1,0);
	if (vNorm.dotproduct(N.Plane.n)<_cos(deg2rad(60.f)))  return FALSE;

	float y_old = vAt.y;
	float y_new = N.Pos.y;
	if (y_old>y_new) {
		// down
		if (y_old-y_new > g_params.fCanDOWN ) return FALSE;
	} else {
		// up
		if (y_new-y_old > g_params.fCanUP	) return FALSE;
	}
 
	// *** Validate plane
	{
		Fvector PLP; D.set(0,-1,0);
		int num_successed_rays = 0;
		for (int x=-RCAST_Count; x<=RCAST_Count; x++) 
		{
			P.x = N.Pos.x + coeff*float(x);
			for (int z=-RCAST_Count; z<=RCAST_Count; z++) {
				P.z = N.Pos.z + coeff*float(z);
				P.y = N.Pos.y;
				N.Plane.intersectRayPoint(P,D,PLP);	// "project" position
				P.y = PLP.y+RCAST_VALID*0.01f;
				
				float	tri_min_range	= flt_max;
				int		tri_selected	= -1;
				float	range,u,v;
				for (i=0; i<float(tris.size()); i++) 
				{
					if (CDB::TestRayTri(P,D,tris[i].v,u,v,range,false)) 
					{
						if (range<tri_min_range) {
							tri_min_range	= range;
							tri_selected	= i;
						}
					}
				}
				if (tri_selected>=0) {
					if (tri_min_range<RCAST_VALID) num_successed_rays++;
				}
			}
		}
		float perc = float(num_successed_rays)/float(RCAST_Total);
		if (perc < 0.5f) {
			//			Msg		("Floating node.");
			return	FALSE;
		}
	}

	// *** Mask check
	// ???

	return TRUE;
}

const int		HDIM_X = 128;
const int		HDIM_Z = 128;

DEF_VECTOR		(vecDW,u32);

static vecDW*	HASH[HDIM_X+1][HDIM_Z+1];

void	hash_Initialize ()
{
	for (int i=0; i<=HDIM_X; i++)
	{
		for (int j=0; j<HDIM_Z; j++)
		{
			HASH[i][j]	= xr_new<vecDW>();
			HASH[i][j]->reserve	(64);
		}
	}
}
void	hash_Destroy	()
{
	for (int i=0; i<=HDIM_X; i++)
	{
		for (int j=0; j<HDIM_Z; j++)
		{
			xr_delete	(HASH[i][j]);
		}
	}
}

vecDW&	HashMap	(Fvector& V)
{
	// Calculate offset,scale,epsilon
	Fvector				VMmin,	VMscale, VMeps, scale;

	Fbox				bb = LevelBB;
	VMscale.set			(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
	VMmin.set			(bb.min);
	VMeps.set			(float(VMscale.x/HDIM_X/2.f),float(0),float(VMscale.z/HDIM_Z/2.f));
	VMeps.x				= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
	VMeps.y				= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
	VMeps.z				= (VMeps.z<EPS_L)?VMeps.z:EPS_L;
	scale.set			(float(HDIM_X),float(0),float(HDIM_Z));
	scale.div			(VMscale);

	// Hash
	u32 ix,iz;
	ix = iFloor((V.x-VMmin.x)*scale.x);
	iz = iFloor((V.z-VMmin.z)*scale.z);
	R_ASSERT(ix<=HDIM_X && iz<=HDIM_Z);
	return *HASH[ix][iz];
}

void	RegisterNode(vertex& N)
{
	u32 ID = g_nodes.size();
	g_nodes.push_back(N);

	HashMap(N.Pos).push_back(ID);
}

u32	FindNode(Fvector& vAt)
{
	float eps	= 0.05f;
	vecDW& V	= HashMap(vAt);

	for (vecDW_it I=V.begin(); I!=V.end(); I++)
	{
		vertex& N = g_nodes[*I];
		if (vAt.similar(N.Pos,eps)) return *I;
	}
	return InvalidNode;
}
 
BOOL	CanTravel(Fvector& _from, Fvector& _at)
{
	float eps	= 0.1f;
	float eps_y = g_params.fPatchSize*1.5f; // * tan(56) = 1.5
	Fvector Result; float radius = g_params.fPatchSize/_sqrt(2.f);

	// 1
	msimulator_Simulate(Result,_from,_at,radius,0.7f);
	BOOL b1 = fsimilar(Result.x,_at.x,eps)&&fsimilar(Result.z,_at.z,eps)&&fsimilar(Result.y,_at.y,eps_y);
	if (b1) return TRUE;

	// 2
	msimulator_Simulate(Result,_from,_at,radius,2.f);
	BOOL b2 = fsimilar(Result.x,_at.x,eps)&&fsimilar(Result.z,_at.z,eps)&&fsimilar(Result.y,_at.y,eps_y);
	if (b2) return TRUE;

	return FALSE;
}

u32 BuildNode(Fvector& vFrom, Fvector& vAt)	// return node's index
{
	// *** Test if we can travel this path
	SnapXZ			(vAt);

	if (!CanTravel(vFrom, vAt))	return InvalidNode;

	// *** set up xr_new<node
	vertex N;
	if (CreateNode(vAt,N)) {
		//*** check if similar node exists
		u32	old		= FindNode(N.Pos);
		if (old==InvalidNode)	
		{
			// register xr_new<node
			RegisterNode(N);
			return g_nodes.size()-1;
		} else {
			// where already was node - return it
			return old;
		}
	} else return InvalidNode;
}

#if 0
#define VPUSH(a)	a.x,a.y,a.z
void xrBuildNodes()
{
	// begin
	XRC.box_options	(CDB::OPT_FULL_TEST);
	XRC.ray_options	(CDB::OPT_CULL | CDB::OPT_ONLYNEAREST);
	g_nodes.reserve	(1024*1024);

	// Initialize hash
	hash_Initialize ();

	for (u32 em=0; em<Emitters.size(); em++) 
	{
		// Align emitter
		Fvector			Pos = Emitters[em];
		SnapXZ			(Pos);
		Pos.y			+=1;
		Fvector			Dir; Dir.set(0,-1,0);
		
		XRC.ray_query	(&Level,Pos,Dir,3);
		if (XRC.r_count()==0) {
			Msg		("Can't align emitter");
			abort	();
		} else {
			CDB::RESULT& R = *XRC.r_begin();
			Pos.y = Pos.y - R.range;
		}
		
		// Build first node
		int oldcount = g_nodes.size();
		int start = BuildNode		(Pos,Pos);
		if (start==InvalidNode)		continue;
		if (start<oldcount)			continue;

		// Estimate nodes
		Fvector	LevelSize;
		LevelBB.getsize				(LevelSize);
		u32	estimated_nodes		= iFloor(LevelSize.x/g_params.fPatchSize)*iFloor(LevelSize.z/g_params.fPatchSize);
		
		// General cycle
		for (u32 i=0; i<g_nodes.size(); i++)
		{
			// left 
			if (g_nodes[i].n1==UnkonnectedNode)
			{
				Pos.set			(g_nodes[i].Pos);
				Pos.x			-=	g_params.fPatchSize;
				int	id			=	BuildNode(g_nodes[i].Pos,Pos);
				g_nodes[i].n1	=	id;
			}
			// fwd
			if (g_nodes[i].n2==UnkonnectedNode)
			{
				Pos.set			(g_nodes[i].Pos);
				Pos.z			+=	g_params.fPatchSize;
				int id			=	BuildNode(g_nodes[i].Pos,Pos);
				g_nodes[i].n2	=	id;
			}
			// right
			if (g_nodes[i].n3==UnkonnectedNode)
			{
				Pos.set			(g_nodes[i].Pos);
				Pos.x			+=	g_params.fPatchSize;
				int id			=	BuildNode(g_nodes[i].Pos,Pos);
				g_nodes[i].n3	=	id;
			}
			// back
			if (g_nodes[i].n4==UnkonnectedNode)
			{
				Pos.set			(g_nodes[i].Pos);
				Pos.z			-=	g_params.fPatchSize;
				int	id			=	BuildNode(g_nodes[i].Pos,Pos);
				g_nodes[i].n4	=	id;
			}
			if (i%512==0) {
				Status("%d / %d nodes created.",g_nodes.size()-i,g_nodes.size());

				float	p1	= float(i)/float(g_nodes.size());
				float	p2	= float(g_nodes.size())/estimated_nodes;
				float	p	= 0.1f*p1+0.9f*p2;

				clamp	(p,0.f,1.f);
				Progress(p);
			}
		}
	}
	Msg("Freeing memory...");
	hash_Destroy	();
}
#endif