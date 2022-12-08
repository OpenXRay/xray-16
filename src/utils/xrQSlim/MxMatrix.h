#ifndef MXMATRIX_INCLUDED // -*- C++ -*-
#define MXMATRIX_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  Generic n-dimensional matrix class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxMatrix.h,v 1.8 1998/10/26 21:09:10 garland Exp $

 ************************************************************************/

#include "MxVector.h"

#define __T float
#include "mixmops.h"

#define __T double
#include "mixmops.h"

////////////////////////////////////////////////////////////////////////
//
// Matrix class
//

#include "MxBlock2.h"

class MxMatrix : public MxBlock2<double>
{
public:
    MxMatrix(unsigned int n) : MxBlock2<double>(n, n) { *this = 0.0; }
    MxMatrix(const MxMatrix& m) : MxBlock2<double>(m.dim(), m.dim()) { copy(m); }
    MxMatrix& operator=(const MxMatrix& m)
    {
        copy(m);
        return *this;
    }
    MxMatrix& operator=(double d)
    {
        mxm_set(*this, d, dim());
        return *this;
    }

    unsigned int dim() const { return width(); }
    MxMatrix& operator+=(const MxMatrix& m)
    {
        mxm_addinto(*this, m, dim());
        return *this;
    }
    MxMatrix& operator-=(const MxMatrix& m)
    {
        mxm_subfrom(*this, m, dim());
        return *this;
    }
    MxMatrix& operator*=(double d)
    {
        mxm_scale(*this, d, dim());
        return *this;
    }
    MxMatrix& operator/=(double d)
    {
        mxm_invscale(*this, d, dim());
        return *this;
    }

    MxVector operator*(const MxVector& v) const
    {
        MxVector r(dim());
        mxm_xform(r, *this, v, dim());
        return r;
    }

    double invert(MxMatrix& inv) const { return mxm_invert(inv, *this, dim()); }
};

/*
inline ostream& operator<<(ostream& out, const MxMatrix& a)
{
    return mxm_write(out, a, a.dim());
}
*/
// MXMATRIX_INCLUDED
#endif
