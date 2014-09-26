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
Vector3<Real>::Vector3 ()
{
    // the vector is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (Real fX, Real fY, Real fZ)
{
    m_afTuple[0] = fX;
    m_afTuple[1] = fY;
    m_afTuple[2] = fZ;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (const Vector3& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,3*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (const Vector<3,Real>& rkV)
{
    memcpy(m_afTuple,(const Real*)rkV,3*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator= (const Vector3& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,3*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator= (const Vector<3,Real>& rkV)
{
    memcpy(m_afTuple,(const Real*)rkV,3*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::X () const
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector3<Real>::X ()
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::Y () const
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector3<Real>::Y ()
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::Z () const
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector3<Real>::Z ()
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::Cross (const Vector3& rkV) const
{
    return Vector3(
        m_afTuple[1]*rkV.m_afTuple[2] - m_afTuple[2]*rkV.m_afTuple[1],
        m_afTuple[2]*rkV.m_afTuple[0] - m_afTuple[0]*rkV.m_afTuple[2],
        m_afTuple[0]*rkV.m_afTuple[1] - m_afTuple[1]*rkV.m_afTuple[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::UnitCross (const Vector3& rkV) const
{
    Vector3 kCross(
        m_afTuple[1]*rkV.m_afTuple[2] - m_afTuple[2]*rkV.m_afTuple[1],
        m_afTuple[2]*rkV.m_afTuple[0] - m_afTuple[0]*rkV.m_afTuple[2],
        m_afTuple[0]*rkV.m_afTuple[1] - m_afTuple[1]*rkV.m_afTuple[0]);
    kCross.Normalize();
    return kCross;
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::Orthonormalize (Vector3& rkU, Vector3& rkV, Vector3& rkW)
{
    // If the input vectors are v0, v1, and v2, then the Gram-Schmidt
    // orthonormalization produces vectors u0, u1, and u2 as follows,
    //
    //   u0 = v0/|v0|
    //   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
    //   u2 = (v2-(u0*v2)u0-(u1*v2)u1)/|v2-(u0*v2)u0-(u1*v2)u1|
    //
    // where |A| indicates length of vector A and A*B indicates dot
    // product of vectors A and B.

    // compute u0
    rkU.Normalize();

    // compute u1
    Real fDot0 = rkU.Dot(rkV); 
    rkV -= fDot0*rkU;
    rkV.Normalize();

    // compute u2
    Real fDot1 = rkV.Dot(rkW);
    fDot0 = rkU.Dot(rkW);
    rkW -= fDot0*rkU + fDot1*rkV;
    rkW.Normalize();
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::Orthonormalize (Vector3 akV[/*3*/])
{
    Orthonormalize(akV[0],akV[1],akV[2]);
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::GenerateOrthonormalBasis (Vector3& rkU, Vector3& rkV,
    Vector3& rkW, bool bUnitLengthW)
{
    if ( !bUnitLengthW )
        rkW.Normalize();

    Real fInvLength;

    if ( Math<Real>::FAbs(rkW.m_afTuple[0]) >=
         Math<Real>::FAbs(rkW.m_afTuple[1]) )
    {
        // W.x or W.z is the largest magnitude component, swap them
        fInvLength = Math<Real>::InvSqrt(rkW.m_afTuple[0]*rkW.m_afTuple[0] +
            rkW.m_afTuple[2]*rkW.m_afTuple[2]);
        rkU.m_afTuple[0] = -rkW.m_afTuple[2]*fInvLength;
        rkU.m_afTuple[1] = (Real)0.0;
        rkU.m_afTuple[2] = +rkW.m_afTuple[0]*fInvLength;
    }
    else
    {
        // W.y or W.z is the largest magnitude component, swap them
        fInvLength = Math<Real>::InvSqrt(rkW.m_afTuple[1]*rkW.m_afTuple[1] +
            rkW.m_afTuple[2]*rkW.m_afTuple[2]);
        rkU.m_afTuple[0] = (Real)0.0;
        rkU.m_afTuple[1] = +rkW.m_afTuple[2]*fInvLength;
        rkU.m_afTuple[2] = -rkW.m_afTuple[1]*fInvLength;
    }

    rkV = rkW.Cross(rkU);
}
//----------------------------------------------------------------------------
