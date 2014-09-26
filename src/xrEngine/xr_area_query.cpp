#include "stdafx.h"
#include "xr_area.h"
#include "xr_object.h"

using namespace	collide;

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
