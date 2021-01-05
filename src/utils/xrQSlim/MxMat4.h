#ifndef MXMAT4_INCLUDED // -*- C++ -*-
#define MXMAT4_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  4x4 Matrix class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxMat4.h,v 1.12 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/

#include "mat4.h"

extern bool jacobi(const Mat4& m, Vec4& vals, Vec4 vecs[4]);

#ifdef MXGL_INCLUDED
inline void glGetMatrix(Mat4& m, GLenum which = GL_MODELVIEW_MATRIX)
{
    Mat4 tmp;
    glGetDoublev(which, &tmp(0, 0));
    m = transpose(tmp);
}

inline void glLoadMatrix(const Mat4& m)
{
    Mat4 tmp = transpose(m);
    glLoadMatrixd(&tmp(0, 0));
}

inline void glMultMatrix(const Mat4& m)
{
    Mat4 tmp = transpose(m);
    glMultMatrixd(&tmp(0, 0));
}
#endif

// MXMAT4_INCLUDED
#endif
