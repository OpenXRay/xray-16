// DynamicHeightMap.cpp: implementation of the CDynamicHeightMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DynamicHeightMap.h"
#include "../xrEngine/cl_intersect.h"

const int	tasksPerFrame	= 1;
const float limit_up		= 100.f;
const float limit_down		= 20.f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHM_Static::CHM_Static()
{
	// Initialize slots
	Slot*	slt = pool;
	for (u32 i=0; i<dhm_matrix; ++i)
		for (u32 j=0; j<dhm_matrix; ++j, ++slt)
			data	[i][j]	= slt;

	// Center
	c_x=c_z=0;
	task.clear();
}

void CHM_Static::Update	()
{
	Fvector&	view	= Device.vCameraPosition;
	int			v_x		= iFloor(view.x/dhm_size);
	int			v_z		= iFloor(view.z/dhm_size);
	
	// *****	SCROLL
	if (v_x!=c_x)	{
		if (v_x>c_x)	{
			// scroll matrix to left
			++c_x;
			for (int z=0; z<dhm_matrix; ++z)
			{
				Slot*	S	= data[z][0];
				if (S->bReady)	{	S->bReady = FALSE; task.push_back(S); }
				for (int x=1; x<dhm_matrix; ++x)	data[z][x-1] = data[z][x];
				data[z][dhm_matrix-1] = S;
				S->set	(c_x-dhm_line+dhm_matrix-1, c_z-dhm_line+z);
			}
		} else {
			// scroll matrix to right
			--c_x;
			for (int z=0; z<dhm_matrix; ++z)
			{
				Slot*	S	= data[z][dhm_matrix-1];
				if (S->bReady)	{	S->bReady = FALSE; task.push_back(S); }
				for (int x=dhm_matrix-1; x>0; --x)	data[z][x] = data[z][x-1]; 
				data[z][0]	= S;
				S->set	(c_x-dhm_line+0,c_z-dhm_line+z);
			}
		}
	}
	if (v_z!=c_z)	{
		if (v_z>c_z)	{
			// scroll matrix down a bit
			++c_z;
			for (int x=0; x<dhm_matrix; ++x)
			{
				Slot*	S	= data[dhm_matrix-1][x];
				if (S->bReady)	{	S->bReady = FALSE; task.push_back(S); }
				for (int z=dhm_matrix-1; z>0; --z)	data[z][x] = data[z-1][x];
				data[0][x]	= S;
				S->set	(c_x-dhm_line+x,c_z-dhm_line+0);
			}
		} else {
			// scroll matrix up
			--c_z;
			for (int x=0; x<dhm_matrix; ++x)
			{
				Slot*	S = data[0][x];
				if (S->bReady)	{	S->bReady = FALSE; task.push_back(S); }
				for (int z=0; z<dhm_matrix; ++z)	data[z-1][x] = data[z][x];
				data[dhm_matrix-1][x]	= S;
				S->set	(c_x-dhm_line+x,c_z-dhm_line+dhm_matrix-1);
			}
		}
	}
	
	// *****	perform TASKs
	for (int taskid=0; (taskid<tasksPerFrame) && (!task.empty()); ++taskid)
	{
		Slot*	S	= task.back	();	task.pop_back();
		S->bReady	= TRUE;

		// Build BBox
		Fbox				bb;
		bb.min.set			(S->x*dhm_size,		view.y-limit_down,	S->z*dhm_size);
		bb.max.set			(bb.min.x+dhm_size,	view.y+limit_up,	bb.min.z+dhm_size);
		bb.grow				(EPS_L);
		
		// Select polygons
		XRC.BBoxMode		(0); // BBOX_TRITEST
		XRC.BBoxCollide		(precalc_identity,g_pGameLevel->ObjectSpace.GetStaticModel(),precalc_identity,bb);
		u32	triCount	= XRC.GetBBoxContactCount();
		if (0==triCount)	{
			S->clear	();
			continue;
		}
		
		// Cull polys
		RAPID::tri* tris	= g_pGameLevel->ObjectSpace.GetStaticTris();
		Fvector		vecUP;	vecUP.set(0,1,0);
		for (u32 tid=0; tid<triCount; ++tid)
		{
			RAPID::tri&	T		= tris[XRC.BBoxContact[tid].id];
			Poly		P;
			Fvector		N;
			P.v[0].set	(*T.verts[0]);	P.v[1].set	(*T.verts[1]);	P.v[2].set	(*T.verts[2]);
			N.mknormal	(P.v[0],P.v[1],P.v[2]);
			if (N.dotproduct(vecUP)<=0)	continue;
			polys.push_back		(P);
		}
		
		// Perform testing
		for (int z=0; z<dhm_precision; ++z)
		{
			for (int x=0; x<dhm_precision; ++x)
			{
				float	rx	= (float(x)/float(dhm_precision))*dhm_size + bb.min.x;
				float	rz	= (float(z)/float(dhm_precision))*dhm_size + bb.min.z;
				float	ry	= bb.min.y-5;
				Fvector pos; pos.set(rx,bb.max.y,rz);
				Fvector	dir; dir.set(0,-1,0);
				
				float	r_u,r_v,r_range;
				for (u32 tid=0; tid<polys.size(); ++tid)
				{
					if (RAPID::TestRayTri(pos,dir,polys[tid].v,r_u,r_v,r_range,TRUE))
					{
						if (r_range>=0)	{
							float y_test	= pos.y - r_range;
							if (y_test>ry)	ry = y_test;
						}
					}
				}
				S->data[z][x] = ry;
			}
		}
	}
}

float CHM_Static::Query	(float x, float z)
{
	// base slot
	int			v_x		= iFloor(x/dhm_size);
	int			v_z		= iFloor(z/dhm_size);
	int			dx		= v_x - c_x;
	int			dz		= v_z - c_z;
	int			gx		= dx  - dhm_line;	clamp(gx,0,dhm_matrix-1);
	int			gz		= dz  - dhm_line;	clamp(gz,0,dhm_matrix-1);
	Slot*		S		= data[gz][gx];
	
	// precision
	float		ostX	= x-v_x*dhm_size;
	float		ostZ	= z-v_z*dhm_size;
	int			px		= iFloor(dhm_precision*ostX/dhm_size);	clamp(px,0,dhm_precision-1);
	int			pz		= iFloor(dhm_precision*ostZ/dhm_size);	clamp(pz,0,dhm_precision-1);
	return		S->data	[pz][px];
}

//
void	CHM_Dynamic::Update	()
{
}
float	CHM_Dynamic::Query	(float x, float z)
{
	return flt_min;
}

//
float	CHeightMap::Query	(float x, float z)
{
	if (dwFrame!=Device.dwFrame)
	{
		dwFrame = Device.dwFrame;
		hm_static.Update	();
		hm_dynamic.Update	();
	}
	float q1 = hm_static.Query(x,z);
	float q2 = hm_dynamic.Query(x,z);
	return _max(q1,q2);
}
