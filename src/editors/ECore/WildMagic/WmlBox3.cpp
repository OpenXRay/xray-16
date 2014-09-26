// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.
#include "stdafx.h"
#pragma hdrstop

#include "WmlBox3.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
Box3<Real>::Box3 ()
{
    // no initialization for efficiency
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Box3<Real>::Center ()
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Box3<Real>::Center () const
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Box3<Real>::Axis (int i)
{
    assert( 0 <= i && i < 3 );
    return m_akAxis[i];
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Box3<Real>::Axis (int i) const
{
    assert( 0 <= i && i < 3 );
    return m_akAxis[i];
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>* Box3<Real>::Axes ()
{
    return m_akAxis;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* Box3<Real>::Axes () const
{
    return m_akAxis;
}
//----------------------------------------------------------------------------
template <class Real>
Real& Box3<Real>::Extent (int i)
{
    assert( 0 <= i && i < 3 );
    return m_afExtent[i];
}
//----------------------------------------------------------------------------
template <class Real>
const Real& Box3<Real>::Extent (int i) const
{
    assert( 0 <= i && i < 3 );
    return m_afExtent[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real* Box3<Real>::Extents ()
{
    return m_afExtent;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* Box3<Real>::Extents () const
{
    return m_afExtent;
}
//----------------------------------------------------------------------------
template <class Real>
void Box3<Real>::ComputeVertices (Vector3<Real> akVertex[8]) const
{
    Vector3<Real> akEAxis[3];
    akEAxis[0] = m_akAxis[0]*m_afExtent[0];
    akEAxis[1] = m_akAxis[1]*m_afExtent[1];
    akEAxis[2] = m_akAxis[2]*m_afExtent[2];

    akVertex[0] = m_kCenter - akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[1] = m_kCenter + akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[2] = m_kCenter + akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[3] = m_kCenter - akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[4] = m_kCenter - akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[5] = m_kCenter + akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[6] = m_kCenter + akEAxis[0] + akEAxis[1] + akEAxis[2];
    akVertex[7] = m_kCenter - akEAxis[0] + akEAxis[1] + akEAxis[2];
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template WML_ITEM class Box3<float>;
template WML_ITEM class Box3<double>;
}
//----------------------------------------------------------------------------
