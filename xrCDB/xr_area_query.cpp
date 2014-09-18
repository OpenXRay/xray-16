#include "stdafx.h"
#include "xr_area.h"
#include "frustum.h"

#include "../xrCore/_vector3d_ext.h"

using namespace	collide;

bool CObjectSpace::BoxQuery	(Fvector const & 		box_center, 
							 Fvector const & 		box_z_axis,
							 Fvector const & 		box_y_axis,
							 Fvector const & 		box_sizes, 
							 xr_vector<Fvector> *	out_tris)
{
	Fvector z_axis			=	box_z_axis;
	z_axis.normalize			();
	Fvector y_axis			=	box_y_axis;
	y_axis.normalize			();
	Fvector x_axis;
	x_axis.crossproduct			(box_y_axis, box_z_axis).normalize();

	Fplane planes[6];
	enum { left_plane, right_plane, top_plane, bottom_plane, front_plane, near_plane };
	
	planes[left_plane].build	(box_center - (x_axis * (box_sizes.x * 0.5f)), - x_axis); 
	planes[right_plane].build	(box_center + (x_axis * (box_sizes.x * 0.5f)),   x_axis); 
	planes[top_plane].build		(box_center + (y_axis * (box_sizes.y * 0.5f)),   y_axis); 
	planes[bottom_plane].build	(box_center - (y_axis * (box_sizes.y * 0.5f)), - y_axis);
	planes[front_plane].build	(box_center - (z_axis * (box_sizes.z * 0.5f)), - z_axis); 
	planes[near_plane].build	(box_center + (z_axis * (box_sizes.z * 0.5f)),   z_axis);

	CFrustum	frustum;
	frustum.CreateFromPlanes	(planes, sizeof(planes) / sizeof(planes[0]));

	xrc.frustum_options			(CDB::OPT_FULL_TEST);
	xrc.frustum_query			(&Static, frustum);

	if ( out_tris )
	{
		for ( CDB::RESULT*	result	=	xrc.r_begin(); 
							result != 	xrc.r_end(); 
							++result )
		{
			out_tris->push_back	(result->verts[0]);
			out_tris->push_back	(result->verts[1]);
			out_tris->push_back	(result->verts[2]);
		}
	}

	return						!!xrc.r_count();
}


/*
const u32	clStatic		= clQUERY_STATIC+clGET_TRIS;

void CObjectSpace::BoxQuery	(collide::rq_results& r_dest, const Fbox& B, const Fmatrix& M, u32 flags)
{
	Fvector		bc,bd;
	Fbox		xf; 
	xf.xform	(B,M);
	xf.get_CD	(bc,bd);

	q_result.Clear	();
	xrc.box_options	(
		(flags&clCOARSE?0:CDB::OPT_FULL_TEST)|
		(flags&clQUERY_ONLYFIRST?CDB::OPT_ONLYFIRST:0)
		);

	if ((flags&clStatic) == clStatic)
	{
		xrc.box_query	(&Static, bc, bd);
		if (xrc.r_count())
		{
			CDB::RESULT* it	=xrc.r_begin();
			CDB::RESULT* end=xrc.r_end	();
			for (; it!=end; it++)
				q_result.AddTri(&Static.get_tris() [it->id],Static.get_verts());
		}
	};

	if (flags&clQUERY_DYNAMIC)
	{
		// Traverse object database
		g_SpatialSpace->q_box	(r_spatial,0,STYPE_COLLIDEABLE,bc,bd);

		// Determine visibility for dynamic part of scene
		for (u32 o_it=0; o_it<r_spatial.size(); o_it++)
		{
			ISpatial*	spatial						= r_spatial[o_it];
			CObject*	collidable					= spatial->dcast_CObject	();
			if			(0==collidable)				continue;
			collidable->collidable.model->_BoxQuery	(B,M,flags);
		}
	};
}
*/
