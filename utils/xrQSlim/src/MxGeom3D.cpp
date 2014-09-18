/************************************************************************

  Handy 3D geometrical primitives

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxGeom3D.cxx,v 1.13 2000/12/14 17:47:43 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxGeom3D.h"

void mx3d_box_corners(const Vec3& min, const Vec3& max, Vec3 *v)
{
    v[0] = min;
    v[1] = Vec3(min[0], max[1], min[2]);
    v[2] = Vec3(max[0], min[1], min[2]);
    v[3] = Vec3(max[0], max[1], min[2]);
    v[4] = Vec3(min[0], min[1], max[2]);
    v[5] = Vec3(min[0], max[1], max[2]);
    v[6] = Vec3(max[0], min[1], max[2]);
    v[7] = max;
}

////////////////////////////////////////////////////////////////////////
//
// Basic bounding volumes.
// This has been imported pretty much directly from GFX
//

void MxBounds::reset()
{
    center[0] = center[1] = center[2] = 0.0;
    radius = 0.0;

    points = 0;
    is_initialized = false;
}

void MxBounds::add_point(const double *v, bool will_update)
{
    if( !is_initialized )
    {
	min[0] = max[0] = v[0];
	min[1] = max[1] = v[1];
	min[2] = max[2] = v[2];
	is_initialized = true;
    }
    else
    {
	if( v[0] < min[0] ) min[0] = v[0];
	if( v[1] < min[1] ) min[1] = v[1];
	if( v[2] < min[2] ) min[2] = v[2];

	if( v[0] > max[0] ) max[0] = v[0];
	if( v[1] > max[1] ) max[1] = v[1];
	if( v[2] > max[2] ) max[2] = v[2];
    }

    if( will_update )
    {
	center += Vec3(v);
	points++;
    }
}

void MxBounds::add_point(const float *v, bool will_update)
{
    if( !is_initialized )
    {
	min[0] = max[0] = v[0];
	min[1] = max[1] = v[1];
	min[2] = max[2] = v[2];
	is_initialized = true;
    }
    else
    {
	if( v[0] < min[0] ) min[0] = v[0];
	if( v[1] < min[1] ) min[1] = v[1];
	if( v[2] < min[2] ) min[2] = v[2];

	if( v[0] > max[0] ) max[0] = v[0];
	if( v[1] > max[1] ) max[1] = v[1];
	if( v[2] > max[2] ) max[2] = v[2];
    }

    if( will_update )
    {
	center += Vec3(v);
	points++;
    }
}

void MxBounds::complete()
{
    center /= (double)points;

    Vec3 R1 = max-center;
    Vec3 R2 = min-center;

    radius = _max(norm(R1), norm(R2));
}

void MxBounds::merge(const MxBounds& b)
{
    add_point(b.min, false);
    add_point(b.max, false);
    points += b.points;

    Vec3 dC = b.center - center;
    double dist = norm(dC);

    if( dist + b.radius > radius )
    {
	// New sphere does not lie within old sphere
	center += b.center;
	center /= 2;

	dist /= 2;
	radius = _max(dist+radius, dist+b.radius);
    }
}
