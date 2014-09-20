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
GVector<Real>::GVector (int iSize)
{
    if ( iSize > 0 )
    {
        m_iSize = iSize;
        m_afTuple = new Real[m_iSize];
        memset(m_afTuple,0,m_iSize*sizeof(Real));
    }
    else
    {
        m_iSize = 0;
        m_afTuple = 0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>::GVector (int iSize, const Real* afTuple)
{
    if ( iSize > 0 )
    {
        m_iSize = iSize;
        m_afTuple = new Real[m_iSize];
        memcpy(m_afTuple,afTuple,m_iSize*sizeof(Real));
    }
    else
    {
        m_iSize = 0;
        m_afTuple = 0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>::GVector (const GVector& rkV)
{
    m_iSize = rkV.m_iSize;
    if ( m_iSize > 0 )
    {
        m_afTuple = new Real[m_iSize];
        memcpy(m_afTuple,rkV.m_afTuple,m_iSize*sizeof(Real));
    }
    else
    {
        m_afTuple = 0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>::~GVector ()
{
    delete[] m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
void GVector<Real>::SetSize (int iSize)
{
    delete[] m_afTuple;
    if ( iSize > 0 )
    {
        m_iSize = iSize;
        m_afTuple = new Real[m_iSize];
        memset(m_afTuple,0,m_iSize*sizeof(Real));
    }
    else
    {
        m_iSize = 0;
        m_afTuple = 0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
int GVector<Real>::GetSize () const
{
    return m_iSize;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>::operator const Real* () const
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>::operator Real* ()
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
Real GVector<Real>::operator[] (int i) const
{
    assert( 0 <= i && i < m_iSize );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real& GVector<Real>::operator[] (int i)
{
    assert( 0 <= i && i < m_iSize );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>& GVector<Real>::operator= (const GVector& rkV)
{
    if ( rkV.m_iSize > 0 )
    {
        if ( m_iSize != rkV.m_iSize )
        {
            delete[] m_afTuple;
            m_iSize = rkV.m_iSize;
            m_afTuple = new Real[m_iSize];
        }
        memcpy(m_afTuple,rkV.m_afTuple,m_iSize*sizeof(Real));
    }
    else
    {
        delete[] m_afTuple;
        m_iSize = 0;
        m_afTuple = 0;
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator== (const GVector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,m_iSize*sizeof(Real)) == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator!= (const GVector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,m_iSize*sizeof(Real)) != 0;
}
//----------------------------------------------------------------------------
template <class Real>
int GVector<Real>::CompareArrays (const GVector& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,m_iSize*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator< (const GVector& rkV) const
{
    return CompareArrays(rkV) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator<= (const GVector& rkV) const
{
    return CompareArrays(rkV) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator> (const GVector& rkV) const
{
    return CompareArrays(rkV) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool GVector<Real>::operator>= (const GVector& rkV) const
{
    return CompareArrays(rkV) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> GVector<Real>::operator+ (const GVector& rkV) const
{
    GVector<Real> kSum(m_iSize);
    for (int i = 0; i < m_iSize; i++)
        kSum.m_afTuple[i] = m_afTuple[i] + rkV.m_afTuple[i];
    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> GVector<Real>::operator- (const GVector& rkV) const
{
    GVector<Real> kDiff(m_iSize);
    for (int i = 0; i < m_iSize; i++)
        kDiff.m_afTuple[i] = m_afTuple[i] - rkV.m_afTuple[i];
    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> GVector<Real>::operator* (Real fScalar) const
{
    GVector<Real> kProd(m_iSize);
    for (int i = 0; i < m_iSize; i++)
        kProd.m_afTuple[i] = fScalar*m_afTuple[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> GVector<Real>::operator/ (Real fScalar) const
{
    GVector<Real> kQuot(m_iSize);
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < m_iSize; i++)
            kQuot.m_afTuple[i] = fInvScalar*m_afTuple[i];
    }
    else
    {
        for (i = 0; i < m_iSize; i++)
            kQuot.m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> GVector<Real>::operator- () const
{
    GVector<Real> kNeg(m_iSize);
    for (int i = 0; i < m_iSize; i++)
        kNeg.m_afTuple[i] = -m_afTuple[i];
    return kNeg;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> Wml::operator* (Real fScalar, const GVector<Real>& rkV)
{
    GVector<Real> kProd(rkV.GetSize());
    for (int i = 0; i < rkV.GetSize(); i++)
        kProd[i] = fScalar*rkV[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>& GVector<Real>::operator+= (const GVector& rkV)
{
    for (int i = 0; i < m_iSize; i++)
        m_afTuple[i] += rkV.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>& GVector<Real>::operator-= (const GVector& rkV)
{
    for (int i = 0; i < m_iSize; i++)
        m_afTuple[i] -= rkV.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>& GVector<Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < m_iSize; i++)
        m_afTuple[i] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real>& GVector<Real>::operator/= (Real fScalar)
{
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < m_iSize; i++)
            m_afTuple[i] *= fInvScalar;
    }
    else
    {
        for (i = 0; i < m_iSize; i++)
            m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real GVector<Real>::Length () const
{
    Real fSqrLen = (Real)0.0;
    for (int i = 0; i < m_iSize; i++)
        fSqrLen += m_afTuple[i]*m_afTuple[i];
    return Math<Real>::Sqrt(fSqrLen);
}
//----------------------------------------------------------------------------
template <class Real>
Real GVector<Real>::SquaredLength () const
{
    Real fSqrLen = (Real)0.0;
    for (int i = 0; i < m_iSize; i++)
        fSqrLen += m_afTuple[i]*m_afTuple[i];
    return fSqrLen;
}
//----------------------------------------------------------------------------
template <class Real>
Real GVector<Real>::Dot (const GVector& rkV) const
{
    Real fDot = (Real)0.0;
    for (int i = 0; i < m_iSize; i++)
        fDot += m_afTuple[i]*rkV.m_afTuple[i];
    return fDot;
}
//----------------------------------------------------------------------------
template <class Real>
Real GVector<Real>::Normalize ()
{
    Real fLength = Length();
    int i;

    if ( fLength > Math<Real>::EPSILON )
    {
        Real fInvLength = ((Real)1.0)/fLength;
        for (i = 0; i < m_iSize; i++)
            m_afTuple[i] *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        for (i = 0; i < m_iSize; i++)
            m_afTuple[i] = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
