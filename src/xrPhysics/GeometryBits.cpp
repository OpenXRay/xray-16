#include "stdafx.h"

#include "GeometryBits.h"

#include "phworld.h"
#include "geometry.h"

static enum geom_category {
	gct_static		= 1<< 0,
	gct_dynamic		= 1<< 1
};


void CPHGeometryBits::init_geom( CODEGeom &g )
{

}

void CPHGeometryBits::init_geom( CPHMesh &g  )
{
	dGeomSetCategoryBits( g.GetGeom(), gct_static );
}
void CPHGeometryBits::set_ignore_static( CODEGeom &g  )
{
	dGeomSetCollideBits( g.geometry_transform(), dGeomGetCollideBits(g.geometry_transform()) & ~gct_static ); 
}