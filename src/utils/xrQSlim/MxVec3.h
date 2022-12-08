#ifndef MXVEC3_INCLUDED // -*- C++ -*-
#define MXVEC3_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  3D Vector class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxVec3.h,v 1.10 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/

#include "vec3.h"

#ifdef MXGL_INCLUDED
inline void glV(const Vec3& v) { glVertex3d(v[X], v[Y], v[Z]); }
inline void glN(const Vec3& v) { glNormal3d(v[X], v[Y], v[Z]); }
inline void glC(const Vec3& v) { glColor3d(v[X], v[Y], v[Z]); }
#endif

// MXVEC3_INCLUDED
#endif
