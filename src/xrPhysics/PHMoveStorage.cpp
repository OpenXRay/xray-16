#include "stdafx.h"
#include "phmovestorage.h"
#pragma warning(disable:4995)
#pragma warning(disable:4267)
#include "ode/src/collision_kernel.h"
#pragma warning(default:4995)
#pragma warning(default:4267)
struct dxGeomTransform : public dxGeom {
	dxGeom *obj;		// object that is being transformed
	int cleanup;		// 1 to destroy obj when destroyed
	int infomode;		// 1 to put Tx geom in dContactGeom g1

	// cached final object transform (body tx + relative tx). this is set by
	// computeAABB(), and it is valid while the AABB is valid.
	dVector3 final_pos;
	dMatrix3 final_R;
	dxGeomTransform::dxGeomTransform (dSpaceID space) : dxGeom (space,1)
	{
		type = dGeomTransformClass;
		obj = 0;
		cleanup = 0;
		infomode = 0;
		dSetZero (final_pos,4);
		dRSetIdentity (final_R);
	}
};
void CPHPositionsPairs::Positions(const Fvector* &p0,const  Fvector* &p1)
{
	CODEGeom	*g=*geom;
	if(g->is_transformed_bt())
	{
		g->geometry_transform()->recomputeAABB();
		p0=(const Fvector*)dGeomGetUserData(g->geom())->last_pos;
		p1=(const Fvector*)((dxGeomTransform*)g->geometry_transform())->final_pos;
	}
	else 
	{
		p1=(const Fvector*)dGeomGetPosition(g->geometry_transform());
		p0=(const Fvector*)dGeomGetUserData(g->geometry_transform())->last_pos;
	}

}