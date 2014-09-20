// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLBOX3H
#define WMLBOX3H

#include "WmlVector3.h"

namespace Wml
{

template <class Real>
class WML_ITEM Box3
{
public:
    Box3 ();

    Vector3<Real>& Center ();
    const Vector3<Real>& Center () const;

    Vector3<Real>& Axis (int i);
    const Vector3<Real>& Axis (int i) const;
    Vector3<Real>* Axes ();
    const Vector3<Real>* Axes () const;

    Real& Extent (int i);
    const Real& Extent (int i) const;
    Real* Extents ();
    const Real* Extents () const;

    void ComputeVertices (Vector3<Real> akVertex[8]) const;

protected:
    Vector3<Real> m_kCenter;
    Vector3<Real> m_akAxis[3];
    Real m_afExtent[3];
};

typedef Box3<float> Box3f;
typedef Box3<double> Box3d;

}

#endif
