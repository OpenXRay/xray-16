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
Matrix4<Real>::Matrix4 ()
{
    // the matrix is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (const Matrix4& rkM)
{
    memcpy(m_afEntry,rkM.m_afEntry,16*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (const Matrix<4,Real>& rkM)
{
    memcpy(m_afEntry,(const Real*)rkM,16*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator= (const Matrix4& rkM)
{
    memcpy(m_afEntry,rkM.m_afEntry,16*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator= (const Matrix<4,Real>& rkM)
{
    memcpy(m_afEntry,(const Real*)rkM,16*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (Real fM00, Real fM01, Real fM02, Real fM03,
    Real fM10, Real fM11, Real fM12, Real fM13, Real fM20, Real fM21,
    Real fM22, Real fM23, Real fM30, Real fM31, Real fM32, Real fM33)
{
    m_afEntry[ 0] = fM00;
    m_afEntry[ 1] = fM01;
    m_afEntry[ 2] = fM02;
    m_afEntry[ 3] = fM03;
    m_afEntry[ 4] = fM10;
    m_afEntry[ 5] = fM11;
    m_afEntry[ 6] = fM12;
    m_afEntry[ 7] = fM13;
    m_afEntry[ 8] = fM20;
    m_afEntry[ 9] = fM21;
    m_afEntry[10] = fM22;
    m_afEntry[11] = fM23;
    m_afEntry[12] = fM30;
    m_afEntry[13] = fM31;
    m_afEntry[14] = fM32;
    m_afEntry[15] = fM33;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (const Real afEntry[16], bool bRowMajor)
{
    if ( bRowMajor )
    {
        memcpy(m_afEntry,afEntry,16*sizeof(Real));
    }
    else
    {
        m_afEntry[ 0] = afEntry[ 0];
        m_afEntry[ 1] = afEntry[ 4];
        m_afEntry[ 2] = afEntry[ 8];
        m_afEntry[ 3] = afEntry[12];
        m_afEntry[ 4] = afEntry[ 1];
        m_afEntry[ 5] = afEntry[ 5];
        m_afEntry[ 6] = afEntry[ 9];
        m_afEntry[ 7] = afEntry[13];
        m_afEntry[ 8] = afEntry[ 2];
        m_afEntry[ 9] = afEntry[ 6];
        m_afEntry[10] = afEntry[10];
        m_afEntry[11] = afEntry[14];
        m_afEntry[12] = afEntry[ 3];
        m_afEntry[13] = afEntry[ 7];
        m_afEntry[14] = afEntry[11];
        m_afEntry[15] = afEntry[15];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::Inverse () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];

    Real fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    if ( Math<Real>::FAbs(fDet) <= Math<Real>::EPSILON )
        return Matrix4::ZERO;

    Matrix4 kInv;
    kInv[0][0] = + m_afEntry[ 5]*fB5 - m_afEntry[ 6]*fB4 + m_afEntry[ 7]*fB3;
    kInv[1][0] = - m_afEntry[ 4]*fB5 + m_afEntry[ 6]*fB2 - m_afEntry[ 7]*fB1;
    kInv[2][0] = + m_afEntry[ 4]*fB4 - m_afEntry[ 5]*fB2 + m_afEntry[ 7]*fB0;
    kInv[3][0] = - m_afEntry[ 4]*fB3 + m_afEntry[ 5]*fB1 - m_afEntry[ 6]*fB0;
    kInv[0][1] = - m_afEntry[ 1]*fB5 + m_afEntry[ 2]*fB4 - m_afEntry[ 3]*fB3;
    kInv[1][1] = + m_afEntry[ 0]*fB5 - m_afEntry[ 2]*fB2 + m_afEntry[ 3]*fB1;
    kInv[2][1] = - m_afEntry[ 0]*fB4 + m_afEntry[ 1]*fB2 - m_afEntry[ 3]*fB0;
    kInv[3][1] = + m_afEntry[ 0]*fB3 - m_afEntry[ 1]*fB1 + m_afEntry[ 2]*fB0;
    kInv[0][2] = + m_afEntry[13]*fA5 - m_afEntry[14]*fA4 + m_afEntry[15]*fA3;
    kInv[1][2] = - m_afEntry[12]*fA5 + m_afEntry[14]*fA2 - m_afEntry[15]*fA1;
    kInv[2][2] = + m_afEntry[12]*fA4 - m_afEntry[13]*fA2 + m_afEntry[15]*fA0;
    kInv[3][2] = - m_afEntry[12]*fA3 + m_afEntry[13]*fA1 - m_afEntry[14]*fA0;
    kInv[0][3] = - m_afEntry[ 9]*fA5 + m_afEntry[10]*fA4 - m_afEntry[11]*fA3;
    kInv[1][3] = + m_afEntry[ 8]*fA5 - m_afEntry[10]*fA2 + m_afEntry[11]*fA1;
    kInv[2][3] = - m_afEntry[ 8]*fA4 + m_afEntry[ 9]*fA2 - m_afEntry[11]*fA0;
    kInv[3][3] = + m_afEntry[ 8]*fA3 - m_afEntry[ 9]*fA1 + m_afEntry[10]*fA0;

    Real fInvDet = ((Real)1.0)/fDet;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
            kInv[iRow][iCol] *= fInvDet;
    }

    return kInv;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::Adjoint () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];

    Matrix4 kAdj;
    kAdj[0][0] = + m_afEntry[ 5]*fB5 - m_afEntry[ 6]*fB4 + m_afEntry[ 7]*fB3;
    kAdj[1][0] = - m_afEntry[ 4]*fB5 + m_afEntry[ 6]*fB2 - m_afEntry[ 7]*fB1;
    kAdj[2][0] = + m_afEntry[ 4]*fB4 - m_afEntry[ 5]*fB2 + m_afEntry[ 7]*fB0;
    kAdj[3][0] = - m_afEntry[ 4]*fB3 + m_afEntry[ 5]*fB1 - m_afEntry[ 6]*fB0;
    kAdj[0][1] = - m_afEntry[ 1]*fB5 + m_afEntry[ 2]*fB4 - m_afEntry[ 3]*fB3;
    kAdj[1][1] = + m_afEntry[ 0]*fB5 - m_afEntry[ 2]*fB2 + m_afEntry[ 3]*fB1;
    kAdj[2][1] = - m_afEntry[ 0]*fB4 + m_afEntry[ 1]*fB2 - m_afEntry[ 3]*fB0;
    kAdj[3][1] = + m_afEntry[ 0]*fB3 - m_afEntry[ 1]*fB1 + m_afEntry[ 2]*fB0;
    kAdj[0][2] = + m_afEntry[13]*fA5 - m_afEntry[14]*fA4 + m_afEntry[15]*fA3;
    kAdj[1][2] = - m_afEntry[12]*fA5 + m_afEntry[14]*fA2 - m_afEntry[15]*fA1;
    kAdj[2][2] = + m_afEntry[12]*fA4 - m_afEntry[13]*fA2 + m_afEntry[15]*fA0;
    kAdj[3][2] = - m_afEntry[12]*fA3 + m_afEntry[13]*fA1 - m_afEntry[14]*fA0;
    kAdj[0][3] = - m_afEntry[ 9]*fA5 + m_afEntry[10]*fA4 - m_afEntry[11]*fA3;
    kAdj[1][3] = + m_afEntry[ 8]*fA5 - m_afEntry[10]*fA2 + m_afEntry[11]*fA1;
    kAdj[2][3] = - m_afEntry[ 8]*fA4 + m_afEntry[ 9]*fA2 - m_afEntry[11]*fA0;
    kAdj[3][3] = + m_afEntry[ 8]*fA3 - m_afEntry[ 9]*fA1 + m_afEntry[10]*fA0;

    return kAdj;
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix4<Real>::Determinant () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];
    Real fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    return fDet;
}
//----------------------------------------------------------------------------
