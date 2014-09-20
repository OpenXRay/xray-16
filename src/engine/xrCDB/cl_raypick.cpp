#include "stdafx.h"
#pragma hdrstop

#include "cl_RAPID.h"
#include "cl_defs.h"
#include "cl_intersect.h"

#ifdef ENGINE_BUILD
#define R_BEGIN Device.Statistic->clRAY.Begin()
#define R_END Device.Statistic->clRAY.End()
#else
#define R_BEGIN
#define R_END
#endif


namespace RAPID {
	void XRCollide::add_raypick(const raypick_info& rp_inf)
	{
		RayContact.push_back(rp_inf);
		if (min_raypick_id>=0){
			VERIFY(min_raypick_id<int(RayContact.size()));
			if (RayContact[min_raypick_id].range>rp_inf.range) min_raypick_id = RayContact.size()-1;
		}else{
			min_raypick_id = RayContact.size()-1;
		}
	}
	
	//----------------------------------------------------------------------------
	IC DWORD& IR(float &x) { return (DWORD&)x; }
	IC BOOL TestAABB(const Fvector& bMax, const Fvector& rP, const Fvector& rD, Fvector& coord)
	{
    #ifdef _EDITOR
    	Fbox		BB;
        BB.set		(-bMax.x,-bMax.y,-bMax.z,bMax.x,bMax.y,bMax.z);
        return 		BB.Pick2(rP,rD,coord);
    #else
		BOOL		Inside = TRUE;
		Fvector		MaxT,bMin;
		MaxT.set	(-1.f,-1.f,-1.f);
		bMin.set	(-bMax.x,-bMax.y,-bMax.z);

		// Find candidate planes.
		if(rP[0] < bMin[0]) {
			Inside		= FALSE;
			coord[0]	= bMin[0];
			MaxT[0]		= (bMin[0] - rP[0]) / rD[0];	// Calculate T distances to candidate planes
		} else if(rP[0] > bMax[0]) {
			Inside		= FALSE;
			coord[0]	= bMax[0];
			MaxT[0]		= (bMax[0] - rP[0]) / rD[0];	// Calculate T distances to candidate planes
		}
		if(rP[1] < bMin[1]) {
			Inside		= FALSE;
			coord[1]	= bMin[1];
			MaxT[1]		= (bMin[1] - rP[1]) / rD[1];	// Calculate T distances to candidate planes
		} else if(rP[1] > bMax[1]) {
			Inside		= FALSE;
			coord[1]	= bMax[1];
			MaxT[1]		= (bMax[1] - rP[1]) / rD[1];	// Calculate T distances to candidate planes
		}
		if(rP[2] < bMin[2]) {
			Inside		= FALSE;
			coord[2]	= bMin[2];
			MaxT[2]		= (bMin[2] - rP[2]) / rD[2];	// Calculate T distances to candidate planes
		} else if(rP[2] > bMax[2]) {
			Inside		= FALSE;
			coord[2]	= bMax[2];
			MaxT[2]		= (bMax[2] - rP[2]) / rD[2];	// Calculate T distances to candidate planes
		}

		// Ray rP inside bounding box
		if(Inside)		{ coord.set	(rP); return true; }

		// Get largest of the maxT's for final choice of intersection
		DWORD WhichPlane = 0;
		if(MaxT[1] > MaxT[0])			WhichPlane = 1;
		if(MaxT[2] > MaxT[WhichPlane])	WhichPlane = 2;

		// Check final candidate actually inside box
		if(IR(MaxT[WhichPlane])&0x80000000) return false;

		switch (WhichPlane) {
		case 0:
			// 1 & 2
			coord[1] = rP[1] + MaxT[0] * rD[1];				// 1 1 0 1
			if(fabsf(coord[1]) > bMax[1])	return false;
			coord[2] = rP[2] + MaxT[0] * rD[2];				// 2 2 0 2
			if(fabsf(coord[2]) > bMax[2])	return false;
			return true;
		case 1:
			// 0 & 2
			coord[0] = rP[0] + MaxT[1] * rD[0];				// 0 0 1 0
			if(fabsf(coord[0]) > bMax[0])	return false;
			coord[2] = rP[2] + MaxT[1] * rD[2];				// 2 2 1 2
			if(fabsf(coord[2]) > bMax[2])	return false;
			return true;
		case 2:
			// 0 & 1
			coord[0] = rP[0] + MaxT[2] * rD[0];				// 0 0 2 0
			if(fabsf(coord[0]) > bMax[0])	return false;
			coord[1] = rP[1] + MaxT[2] * rD[1];				// 1 1 2 1
			if(fabsf(coord[1]) > bMax[1])	return false;
			return true;
		default:
			NODEFAULT;
			#ifdef DEBUG
			return false;
			#endif
		}
	#endif
	}

	void XRCollide::raypick_fast		(const box *B, const Fvector& rC, const Fvector& rD)
	{
		if ((ray_flags&RAY_ONLYFIRST) && (RayContact.size()>1)) return;
//		if (!B) return;

		// 1. XForm ray from parent to local space
		Fvector C,D,P;
		B->pR.MTxV		(D,rD);
		P.sub			(rC,B->pT);
		B->pR.MTxV		(C,P);
		
		// 2. Actual ray/aabb test
		if (TestAABB(B->d,C,D,P)) 
		{
			if (P.distance_to_sqr(C)<rmodel_range_sq)
			{
				if (B->leaf())
				{
					// 3. Test triangle(s)
					for(int i=0; i<B->num_tris; i++)
					{
						raypick_info	rp_inf;
						rp_inf.id		= B->tri_index[i];
						rp_inf.range	= 0;
						tri&			T = model1->tris[rp_inf.id];
						if (TestRayTri(rmodel_C,rmodel_D, T.verts, rp_inf.u, rp_inf.v, rp_inf.range, ray_flags&RAY_CULL))
						{
							if (rp_inf.range>0) {
								if (rmodel_L2W) {
									rmodel_L2W->transform_tiny(rp_inf.p[0], *T.verts[0]);
									rmodel_L2W->transform_tiny(rp_inf.p[1], *T.verts[1]);
									rmodel_L2W->transform_tiny(rp_inf.p[2], *T.verts[2]);
								} else {
									rp_inf.p[0].set(*T.verts[0]);
									rp_inf.p[1].set(*T.verts[1]);
									rp_inf.p[2].set(*T.verts[2]);
								}
								add_raypick	(rp_inf);
							}
						}
					}
				} else {
					// 4. Traverse chields
					raypick_fast(B->P,C,D);
					raypick_fast(B->N,C,D);
				}
			}
		}
	}
	void XRCollide::raypick_fast_nearest(const box *B, const Fvector& rC, const Fvector& rD)
	{
		// 1. XForm ray from parent to local space
		Fvector C,D,P;
		B->pR.MTxV		(D,rD);
		P.sub			(rC,B->pT);
		B->pR.MTxV		(C,P);
		
		// 2. Actual ray/aabb test
		if (TestAABB(B->d,C,D,P)) 
		{
			if (P.distance_to_sqr(C)<rmodel_range_sq)
			{
				if (B->leaf())
				{
					// 3. Test triangle(s)
					for(int i=0; i<B->num_tris; i++){
						raypick_info	rp_inf;
						rp_inf.id		= B->tri_index[i];
						rp_inf.range	= 0;
						tri&			T = model1->tris[rp_inf.id];
						if (TestRayTri(rmodel_C,rmodel_D, T.verts, rp_inf.u, rp_inf.v, rp_inf.range, ray_flags&RAY_CULL))
						{
							if (rp_inf.range>0) {
								if (rmodel_L2W) {
									rmodel_L2W->transform_tiny(rp_inf.p[0], *T.verts[0]);
									rmodel_L2W->transform_tiny(rp_inf.p[1], *T.verts[1]);
									rmodel_L2W->transform_tiny(rp_inf.p[2], *T.verts[2]);
								} else {
									rp_inf.p[0].set(*T.verts[0]);
									rp_inf.p[1].set(*T.verts[1]);
									rp_inf.p[2].set(*T.verts[2]);
								}
								if (rp_inf.range<rmodel_range)
								{
									min_raypick_id	= RayContact.size();
									rmodel_range	= rp_inf.range;
									rmodel_range_sq	= rp_inf.range*rp_inf.range;
									RayContact.push_back(rp_inf);
								}
							}
						}
					}
				} else {
					// 4. Traverse chields
					raypick_fast_nearest(B->P,C,D);
					raypick_fast_nearest(B->N,C,D);
				}
			}
		}
	}
	
	void XRCollide::RayPick( const Fmatrix* parent, const Model *o1, const Fvector& C, const Fvector& D, float max_range)
	{
		R_BEGIN;
		if (parent) {
			Fmatrix					rXForm;
			rXForm.invert			(*parent);		// create W2L xform
			rXForm.transform_dir	(rmodel_D,D);	// convert ray W2L
			rXForm.transform_tiny	(rmodel_C,C);
		} else {
			rmodel_D.set			(D);
			rmodel_C.set			(C);
		}
		rmodel_range			= max_range;
		rmodel_range_sq			= max_range*max_range;
		rmodel_L2W				= (Fmatrix*)parent;
		
		// reset the report fields
		min_raypick_id			= -1;
		model1					= o1;
		RayContact.clear		();
		
		// make the call
		if (ray_flags&RAY_ONLYNEAREST)	raypick_fast_nearest(o1->b, rmodel_C, rmodel_D);
		else							raypick_fast		(o1->b, rmodel_C, rmodel_D);
		R_END;
	}
}
