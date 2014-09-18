// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 ()
{
    // the vector is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (Real fX, Real fY)
{
    m_afTuple[0] = fX;
    m_afTuple[1] = fY;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Vector2& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,2*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Vector<2,Real>& rkV)
{
    memcpy(m_afTuple,(const Real*)rkV,2*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator= (const Vector2& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,2*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator= (const Vector<2,Real>& rkV)
{
    memcpy(m_afTuple,(const Real*)rkV,2*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::X () const
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector2<Real>::X ()
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Y () const
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector2<Real>::Y ()
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::Perp () const
{
    return Vector2(m_afTuple[1],-m_afTuple[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::UnitPerp () const
{
    Vector2 kPerp(m_afTuple[1],-m_afTuple[0]);
    kPerp.Normalize();
    return kPerp;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Kross (const Vector2& rkV) const
{
    return m_afTuple[0]*rkV.m_afTuple[1] - m_afTuple[1]*rkV.m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::Cross (const Vector2&) const
{
    return Vector2(m_afTuple[1],-m_afTuple[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::UnitCross (const Vector2&) const
{
    Vector2 kPerp(m_afTuple[1],-m_afTuple[0]);
    kPerp.Normalize();
    return kPerp;
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::Orthonormalize (Vector2& rkU, Vector2& rkV)
{
    // If the input vectors are v0 and v1, then the Gram-Schmidt
    // orthonormalization produces vectors u0 and u1 as follows,
    //
    //   u0 = v0/|v0|
    //   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
    //
    // where |A| indicates length of vector A and A*B indicates dot
    // product of vectors A and B.

    // compute u0
    rkU.Normalize();

    // compute u1
    Real fDot0 = rkU.Dot(rkV); 
    rkV -= fDot0*rkU;
    rkV.Normalize();
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::GenerateOrthonormalBasis (Vector2& rkU, Vector2& rkV,
    bool bUnitLengthV)
{
    if ( !bUnitLengthV )
        rkV.Normalize();

    rkU = rkV.Perp();
}
//----------------------------------------------------------------------------
