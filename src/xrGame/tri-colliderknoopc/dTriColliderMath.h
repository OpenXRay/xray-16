#ifndef D_TRI_COLLIDER_MATH_H
#define D_TRI_COLLIDER_MATH_H
#include "__aabb_tri.h"
#include "../ode_include.h"
#include "../mathutils.h"
#include "dcTriangle.h"

inline bool  TriContainPoint(const dReal* v0,const dReal* v1,const dReal* v2,
							 const dReal* triSideAx0,const dReal* triSideAx1,const dReal* triSideAx2,
							 const dReal* triAx, const dReal* pos){
								 dVector3 cross0, cross1, cross2;
								 dCROSS(cross0,=,triAx,triSideAx0);
								 if(dDOT(cross0,pos)<dDOT(cross0,v0))return false;
								 dCROSS(cross1,=,triAx,triSideAx1);
								 if(dDOT(cross1,pos)<dDOT(cross1,v1))return false;
								 dCROSS(cross2,=,triAx,triSideAx2);
								 if(dDOT(cross2,pos)<dDOT(cross2,v2))return false;
								 return true;
							 }

inline bool  TriContainPoint(const dReal* v0,const dReal* v1,const dReal* v2,const dReal* triAx,const dReal* triSideAx0,const dReal* triSideAx1, const dReal* pos){


	dVector3 triSideAx2={v0[0]-v2[0],v0[1]-v2[1],v0[2]-v2[2]};
	return TriContainPoint(v0,v1,v2,triSideAx0,triSideAx1,triSideAx2,triAx,pos);
}


inline bool  TriContainPoint(const dReal* v0,const dReal* v1,const dReal* v2, const dReal* pos){


	dVector3 triSideAx0={v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
	dVector3 triSideAx1={v2[0]-v1[0],v2[1]-v1[1],v2[2]-v1[2]};
	dVector3 triSideAx2={v0[0]-v2[0],v0[1]-v2[1],v0[2]-v2[2]};

	dVector3 triAx;
	dCROSS(triAx,=,triSideAx0,triSideAx1);
	return TriContainPoint(v0,v1,v2,triSideAx0,triSideAx1,triSideAx2,triAx,pos);


}

inline bool  TriPlaneContainPoint(const dReal* v0,const dReal* v1,const dReal* v2, const dReal* pos){


	 dVector3 triSideAx0={v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
	 dVector3 triSideAx1={v2[0]-v1[0],v2[1]-v1[1],v2[2]-v1[2]};

	 dVector3 triAx;
	 dCROSS(triAx,=,triSideAx0,triSideAx1);


	 if(dDOT(triAx,pos)-dDOT(triAx,v0)>0.f) return true;
	 else									 return false;


 }

 inline bool  TriPlaneContainPoint(const dReal* triAx,const dReal* v0, const dReal* pos){


	 if(dDOT(triAx,pos)-dDOT(triAx,v0)>0.f) return true;
	 else									 return false;


 }

 inline bool TriPlaneContainPoint(Triangle* T)
 {
	 return T->dist>0.f;
 }

 inline void PlanePoint(const Triangle& tri,const dReal* from,const dReal* to,float from_dist,dReal* point)
 {
	 dVector3	dir		=	{to[0]-from[0],to[1]-from[1],to[2]-from[2]}	;
	 dReal		cosinus	=	(tri.dist-from_dist)							;
	 VERIFY2(cosinus<0.f,"wrong positions")								;
	 dReal mul=(from_dist)/cosinus								;
	 dir[0]*=mul;	dir[1]*=mul;	dir[2]*=mul							;
	 point[0]=from[0]-dir[0]												;	
	 point[1]=from[1]-dir[1]												;	
	 point[2]=from[2]-dir[2]												;

 }


 ICF	void InitTriangle(CDB::TRI* XTri,Triangle& triangle,const Point* VRT)
 {
	 dVectorSub(triangle.side0,VRT[1],VRT[0])				;
	 dVectorSub(triangle.side1,VRT[2],VRT[1])				;
	 triangle.T=XTri											;
	 dCROSS(triangle.norm,=,triangle.side0,triangle.side1)	;
	 cast_fv(triangle.norm).normalize()						;
	 triangle.pos=dDOT(VRT[0],triangle.norm)					;
 }
 ICF	void InitTriangle(CDB::TRI* XTri,Triangle& triangle,const Fvector*	 V_array)
 {
	 const Point vertices[3]={Point((dReal*)&V_array[XTri->verts[0]]),Point((dReal*)&V_array[XTri->verts[1]]),Point((dReal*)&V_array[XTri->verts[2]])};
	 InitTriangle(XTri,triangle,vertices);
 }

 ICF	void CalculateTri(CDB::TRI* XTri,const float* pos,Triangle& triangle,const Fvector* V_array)
 {
	 InitTriangle(XTri,triangle,V_array);
	 triangle.dist=dDOT(pos,triangle.norm)-triangle.pos;
 }

 ICF	void CalculateTri(CDB::TRI* XTri,const float* pos,Triangle& triangle,const Point* VRT)
 {
	 InitTriangle(XTri,triangle,VRT);
	 triangle.dist=dDOT(pos,triangle.norm)-triangle.pos;
 }
 #endif