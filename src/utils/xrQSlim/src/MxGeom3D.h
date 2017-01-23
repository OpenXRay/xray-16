#ifndef MXGEOM3D_INCLUDED // -*- C++ -*-
#define MXGEOM3D_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  Handy 3D geometrical primitives

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxGeom3D.h,v 1.14 2000/12/14 17:47:43 garland Exp $

 ************************************************************************/

#include "geom3d.h"
#include "MxVec3.h"
#include "MxVec4.h"

extern double triangle_project_point(const Vec3& v0, const Vec3& v1,
				     const Vec3& v2, const Vec3& v,
				     Vec3 *bary=NULL);

extern void mx3d_box_corners(const Vec3& min, const Vec3& max, Vec3 *v);

class MxBounds
{
public:

    bool is_initialized;
    Vec3 min, max;
    Vec3 center;
    double radius;
    unsigned int points;

    void reset();
    void add_point(const double *v, bool will_update=true);
    void add_point(const float *v, bool will_update=true);
    void complete();
    void merge(const MxBounds&);

    MxBounds() { reset(); }
};

class MxPlane3
{
private:
    float p[4];

public:
    MxPlane3() { }
    MxPlane3(const float *p0) { for(unsigned int i=0; i<4; i++) p[i]=p0[i]; }
    MxPlane3(const MxPlane3& n) { *this = n; }

    inline MxPlane3& operator=(const MxPlane3& n);

    operator const float*() const { return p; }
    operator       float*()       { return p; }

#ifdef __GNUC__
    float&       operator[](int i)       { return p[i]; }
    const float& operator[](int i) const { return p[i]; }
#endif
};

inline MxPlane3& MxPlane3::operator=(const MxPlane3& n)
{
    for(unsigned int i=0; i<4; i++) p[i]=n.p[i];
    return *this;
}

// MXGEOM3D_INCLUDED
#endif
