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
template <int N, class Real>
Vector<N,Real>::Vector ()
{
    // For efficiency in construction of large arrays of vectors, the
    // default constructor does not initialize the vector.
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>::Vector (const Real* afTuple)
{
    memcpy(m_afTuple,afTuple,N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>::Vector (const Vector& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>::operator const Real* () const
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>::operator Real* ()
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Vector<N,Real>::operator[] (int i) const
{
    assert( 0 <= i && i < N );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real& Vector<N,Real>::operator[] (int i)
{
    assert( 0 <= i && i < N );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>& Vector<N,Real>::operator= (const Vector& rkV)
{
    memcpy(m_afTuple,rkV.m_afTuple,N*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator== (const Vector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,N*sizeof(Real)) == 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator!= (const Vector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,N*sizeof(Real)) != 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
int Vector<N,Real>::CompareArrays (const Vector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator< (const Vector& rkV) const
{
    return CompareArrays(rkV) < 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator<= (const Vector& rkV) const
{
    return CompareArrays(rkV) <= 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator> (const Vector& rkV) const
{
    return CompareArrays(rkV) > 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Vector<N,Real>::operator>= (const Vector& rkV) const
{
    return CompareArrays(rkV) >= 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Vector<N,Real>::operator+ (const Vector& rkV) const
{
    Vector<N,Real> kSum;
    for (int i = 0; i < N; i++)
        kSum.m_afTuple[i] = m_afTuple[i] + rkV.m_afTuple[i];
    return kSum;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Vector<N,Real>::operator- (const Vector& rkV) const
{
    Vector<N,Real> kDiff;
    for (int i = 0; i < N; i++)
        kDiff.m_afTuple[i] = m_afTuple[i] - rkV.m_afTuple[i];
    return kDiff;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Vector<N,Real>::operator* (Real fScalar) const
{
    Vector<N,Real> kProd;
    for (int i = 0; i < N; i++)
        kProd.m_afTuple[i] = fScalar*m_afTuple[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Vector<N,Real>::operator/ (Real fScalar) const
{
    Vector<N,Real> kQuot;
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < N; i++)
            kQuot.m_afTuple[i] = fInvScalar*m_afTuple[i];
    }
    else
    {
        for (i = 0; i < N; i++)
            kQuot.m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Vector<N,Real>::operator- () const
{
    Vector<N,Real> kNeg;
    for (int i = 0; i < N; i++)
        kNeg.m_afTuple[i] = -m_afTuple[i];
    return kNeg;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> operator* (Real fScalar, const Vector<N,Real>& rkV)
{
    Vector<N,Real> kProd;
    for (int i = 0; i < N; i++)
        kProd[i] = fScalar*rkV[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>& Vector<N,Real>::operator+= (const Vector& rkV)
{
    for (int i = 0; i < N; i++)
        m_afTuple[i] += rkV.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>& Vector<N,Real>::operator-= (const Vector& rkV)
{
    for (int i = 0; i < N; i++)
        m_afTuple[i] -= rkV.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>& Vector<N,Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < N; i++)
        m_afTuple[i] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real>& Vector<N,Real>::operator/= (Real fScalar)
{
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < N; i++)
            m_afTuple[i] *= fInvScalar;
    }
    else
    {
        for (i = 0; i < N; i++)
            m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Vector<N,Real>::Length () const
{
    Real fSqrLen = (Real)0.0;
    for (int i = 0; i < N; i++)
        fSqrLen += m_afTuple[i]*m_afTuple[i];
    return Math<Real>::Sqrt(fSqrLen);
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Vector<N,Real>::SquaredLength () const
{
    Real fSqrLen = (Real)0.0;
    for (int i = 0; i < N; i++)
        fSqrLen += m_afTuple[i]*m_afTuple[i];
    return fSqrLen;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Vector<N,Real>::Dot (const Vector& rkV) const
{
    Real fDot = (Real)0.0;
    for (int i = 0; i < N; i++)
        fDot += m_afTuple[i]*rkV.m_afTuple[i];
    return fDot;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Vector<N,Real>::Normalize ()
{
    Real fLength = Length();
    int i;

    if ( fLength > Math<Real>::EPSILON )
    {
        Real fInvLength = ((Real)1.0)/fLength;
        for (i = 0; i < N; i++)
            m_afTuple[i] *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        for (i = 0; i < N; i++)
            m_afTuple[i] = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
