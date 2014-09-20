#include "stdafx.h"
#include "compiler.h"
#include "cl_intersect.h"

// MagicFM
#pragma warning(disable:4995)
#include <freemagic/MgcAppr3DPlaneFit.h>
#pragma warning(default:4995)

const float	RCAST_DepthValid = 0.2f;

IC void BoxQuery(Fbox& BB, bool exact)
{
	if (exact) 		XRC.box_options	(CDB::OPT_FULL_TEST);
	else			XRC.box_options	(0);
	Fvector			C,D;
	BB.get_CD		(C,D);
	XRC.box_query	(&Level,C,D);
}

struct tri {
	Fvector v[3];
};

BOOL	ValidNode(vertex& N)
{
	// *** Query and cache polygons for ray-casting
	Fvector	PointUp;		PointUp.set(N.Pos);		PointUp.y	+= RCAST_Depth/2;
	Fvector	PointDown;		PointDown.set(N.Pos);	PointDown.y	-= RCAST_Depth/2;

	Fbox	BB;				BB.set	(PointUp,PointUp);		BB.grow(g_params.fPatchSize/2);	// box 1
	Fbox	B2;				B2.set	(PointDown,PointDown);	B2.grow(g_params.fPatchSize/2);	// box 2
	BB.merge(B2			);
	BoxQuery(BB,false	);
	u32	dwCount = XRC.r_count();
	if (dwCount==0)	{
		Log("chasm1");
		return FALSE;			// chasm?
	}

	// *** Transfer triangles and compute sector
	R_ASSERT(dwCount<RCAST_MaxTris);
	static svector<tri,RCAST_MaxTris> tris;		tris.clear();
	for (u32 i=0; i<dwCount; i++)
	{
		tri&		D = tris.last();
		CDB::RESULT&rp = XRC.r_begin()[i];
		*(Level.get_tris()+XRC.r_begin()[i].id);

		D.v[0].set	(rp.verts[0]);
		D.v[1].set	(rp.verts[1]);
		D.v[2].set	(rp.verts[2]);

		Fvector		N;
		N.mknormal	(D.v[0],D.v[1],D.v[2]);
		if (N.y<=0)	continue;

		tris.inc	();
	}
	if (tris.size()==0)	{
		Log("chasm2");
		return FALSE;			// chasm?
	}

	// *** Perform ray-casts and calculate sector
	Fvector P,D,PLP; D.set(0,-1,0);

	float coeff = 0.5f*g_params.fPatchSize/float(RCAST_Count);

	int num_successed_rays = 0;
	for (int x=-RCAST_Count; x<=RCAST_Count; x++) 
	{
		P.x = N.Pos.x + coeff*float(x);
		for (int z=-RCAST_Count; z<=RCAST_Count; z++) {
			P.z = N.Pos.z + coeff*float(z);
			P.y = N.Pos.y;
			N.Plane.intersectRayPoint(P,D,PLP);	// "project" position
			P.y = PLP.y+RCAST_DepthValid/2;

			float	tri_min_range	= flt_max;
			int		tri_selected	= -1;
			float	range = 0.f,u,v;
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
				if (range<RCAST_DepthValid)	num_successed_rays++;

			}
		}
	}
	if (float(num_successed_rays)/float(RCAST_Total) < 0.5f) {
		Msg		("Floating node.");
		return	FALSE;
	}
	return TRUE;
}

#define		merge(pt)	if (fsimilar(P.y,REF.y,0.5f)) { c++; pt.add(P); }

void	xrSmoothNodes()
{
	Nodes	smoothed;	smoothed.reserve(g_nodes.size());
	Marks	mark;		mark.assign(g_nodes.size(),false);

	int inv_count = 0;
	for (u32 i=0; i<g_nodes.size(); i++)
	{
		vertex& N = g_nodes[i];

		Fvector	P1,P2,P3,P4,P,REF;
		int		c;

		// smooth point LF
		{
			bool	bCorner = false;

			c=1;	N.PointLF(REF);	P1.set(REF);
			if (N.nLeft()!=InvalidNode) {
				vertex& L = g_nodes[N.nLeft()];

				L.PointFR(P);	merge(P1);
				if (L.nForward()!=InvalidNode) {
					bCorner = true;
					vertex& C = g_nodes[L.nForward()];

					C.PointRB(P);	merge(P1);
				}
			}
			if (N.nForward()!=InvalidNode) {
				vertex& F = g_nodes[N.nForward()];

				F.PointBL(P);	merge(P1);
				if ((!bCorner) && (F.nLeft()!=InvalidNode)) {
					bCorner = true;

					vertex& C = g_nodes[F.nLeft()];
					C.PointRB(P);	merge(P1);
				}
			}
			R_ASSERT(c<=4);
			P1.div(float(c));
		}

		// smooth point FR
		{
			bool	bCorner = false;

			c=1;	N.PointFR(REF); P2.set(REF);
			if (N.nForward()!=InvalidNode) {
				vertex& F = g_nodes[N.nForward()];

				F.PointRB(P);	merge(P2);
				if (F.nRight()!=InvalidNode) {
					bCorner = true;
					vertex& C = g_nodes[F.nRight()];

					C.PointBL(P);	merge(P2);
				}
			}
			if (N.nRight()!=InvalidNode) {
				vertex& R = g_nodes[N.nRight()];

				R.PointLF(P);	merge(P2);
				if ((!bCorner) && (R.nForward()!=InvalidNode)) {
					bCorner = true;

					vertex& C = g_nodes[R.nForward()];
					C.PointBL(P);	merge(P2);
				}
			}
			R_ASSERT(c<=4);
			P2.div(float(c));
		}

		// smooth point RB
		{
			bool	bCorner = false;

			c=1;	N.PointRB(REF); P3.set(REF);
			if (N.nRight()!=InvalidNode) {
				vertex& R = g_nodes[N.nRight()];

				R.PointBL(P);	merge(P3);
				if (R.nBack()!=InvalidNode) {
					bCorner = true;
					vertex& C = g_nodes[R.nBack()];

					C.PointLF(P);	merge(P3);
				}
			}
			if (N.nBack()!=InvalidNode) {
				vertex& B = g_nodes[N.nBack()];

				B.PointFR(P);	merge(P3);
				if ((!bCorner) && (B.nRight()!=InvalidNode)) {
					bCorner = true;

					vertex& C = g_nodes[B.nRight()];
					C.PointLF(P);	merge(P3);
				}
			}
			R_ASSERT(c<=4);
			P3.div(float(c));
		}

		// smooth point BL
		{
			bool	bCorner = false;

			c=1;	N.PointBL(REF); P4.set(REF);
			if (N.nBack()!=InvalidNode) {
				vertex& B = g_nodes[N.nBack()];

				B.PointLF(P);	merge(P4);
				if (B.nLeft()!=InvalidNode) {
					bCorner = true;
					vertex& C = g_nodes[B.nLeft()];

					C.PointFR(P);	merge(P4);
				}
			}
			if (N.nLeft()!=InvalidNode) {
				vertex& L = g_nodes[N.nLeft()];

				L.PointRB(P);	merge(P4);
				if ((!bCorner) && (L.nBack()!=InvalidNode)) {
					bCorner = true;

					vertex& C = g_nodes[L.nBack()];
					C.PointFR(P);	merge(P4);
				}
			}
			R_ASSERT(c<=4);
			P4.div(float(c));
		}

		// align plane
		Fvector data[4]; data[0]=P1; data[1]=P2; data[2]=P3; data[3]=P4;
		Fvector vOffs,vNorm,D;
		vNorm.set(N.Plane.n);
		vOffs.set(N.Pos);
		Mgc::OrthogonalPlaneFit(
			4,(Mgc::Vector3*)data,
			*((Mgc::Vector3*)&vOffs),
			*((Mgc::Vector3*)&vNorm)
		);
		if (vNorm.y<0) vNorm.invert();

		// create _new node
		vertex NEW = N;
		NEW.Plane.build	(vOffs,vNorm);
		D.set			(0,1,0);
		N.Plane.intersectRayPoint(N.Pos,D,NEW.Pos);	// "project" position
		smoothed.push_back	(NEW);

		// verify placement
		/*
		mark[i]			= !!ValidNode	(NEW);

		if (!mark[i])	inv_count++;.
		*/
	}
	g_nodes = smoothed;

	if (inv_count) Msg("%d invalid nodes detected",inv_count);
}
