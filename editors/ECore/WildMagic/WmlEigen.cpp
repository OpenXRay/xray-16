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

#include "WmlEigen.h"
#include "WmlMath.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::Eigen (int iSize)
    :
    m_kMat(iSize,iSize)
{
    assert( iSize >= 2 );
    m_iSize = iSize;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = ((iSize % 2) == 0);
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::Eigen (const Matrix2<Real>& rkM)
    :
    m_kMat(2,2,(const Real*)rkM)
{
    m_iSize = 2;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = true;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::Eigen (const Matrix3<Real>& rkM)
    :
    m_kMat(3,3,(const Real*)rkM)
{
    m_iSize = 3;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = false;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::Eigen (const Matrix4<Real>& rkM)
    :
    m_kMat(4,4,(const Real*)rkM)
{
    m_iSize = 4;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = true;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::Eigen (const GMatrix<Real>& rkM)
    :
    m_kMat(rkM)
{
    m_iSize = rkM.GetRows();
    assert( m_iSize >= 2 && (rkM.GetColumns() == m_iSize) );
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = ((m_iSize % 2) == 0);
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>::~Eigen ()
{
    delete[] m_afSubd;
    delete[] m_afDiag;
}
//----------------------------------------------------------------------------
template <class Real>
Real& Eigen<Real>::operator() (int iRow, int iCol)
{
    return m_kMat[iRow][iCol];
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>& Eigen<Real>::operator= (const Matrix2<Real>& rkM)
{
    m_kMat.SetMatrix(2,2,(const Real*)rkM);
    m_iSize = 2;
    delete[] m_afDiag;
    delete[] m_afSubd;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = true;

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>& Eigen<Real>::operator= (const Matrix3<Real>& rkM)
{
    m_kMat.SetMatrix(3,3,(const Real*)rkM);
    m_iSize = 3;
    delete[] m_afDiag;
    delete[] m_afSubd;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = false;

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>& Eigen<Real>::operator= (const Matrix4<Real>& rkM)
{
    m_kMat.SetMatrix(4,4,(const Real*)rkM);
    m_iSize = 4;
    delete[] m_afDiag;
    delete[] m_afSubd;
    m_afDiag = new Real[m_iSize];
    m_afSubd = new Real[m_iSize];

    // set according to the parity of the number of Householder reflections
    m_bIsRotation = true;

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Eigen<Real>& Eigen<Real>::operator= (const GMatrix<Real>& rkM)
{
    m_kMat = rkM;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Eigen<Real>::GetEigenvalue (int i) const
{
    return m_afDiag[i];
}
//----------------------------------------------------------------------------
template <class Real>
const Real* Eigen<Real>::GetEigenvalues () const
{
    return m_afDiag;
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::GetEigenvector (int i, Vector2<Real>& rkV) const
{
    assert( m_iSize == 2 );
    if ( m_iSize == 2 )
    {
        for (int iRow = 0; iRow < m_iSize; iRow++)
            rkV[iRow] = m_kMat[iRow][i];
    }
    else
    {
        rkV = Vector2<Real>::ZERO;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::GetEigenvector (int i, Vector3<Real>& rkV) const
{
    assert( m_iSize == 3 );
    if ( m_iSize == 3 )
    {
        for (int iRow = 0; iRow < m_iSize; iRow++)
            rkV[iRow] = m_kMat[iRow][i];
    }
    else
    {
        rkV = Vector3<Real>::ZERO;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::GetEigenvector (int i, Vector4<Real>& rkV) const
{
    assert( m_iSize == 4 );
    if ( m_iSize == 4 )
    {
        for (int iRow = 0; iRow < m_iSize; iRow++)
            rkV[iRow] = m_kMat[iRow][i];
    }
    else
    {
        rkV = Vector4<Real>::ZERO;
    }
}
//----------------------------------------------------------------------------
template <class Real>
GVector<Real> Eigen<Real>::GetEigenvector (int i) const
{
    return m_kMat.GetColumn(i);
}
//----------------------------------------------------------------------------
template <class Real>
const GMatrix<Real>& Eigen<Real>::GetEigenvectors () const
{
    return m_kMat;
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::Tridiagonal2 ()
{
    // matrix is already tridiagonal
    m_afDiag[0] = m_kMat[0][0];
    m_afDiag[1] = m_kMat[1][1];
    m_afSubd[0] = m_kMat[0][1];
    m_afSubd[1] = (Real)0.0;
    m_kMat[0][0] = (Real)1.0;
    m_kMat[0][1] = (Real)0.0;
    m_kMat[1][0] = (Real)0.0;
    m_kMat[1][1] = (Real)1.0;
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::Tridiagonal3 ()
{
    Real fM00 = m_kMat[0][0];
    Real fM01 = m_kMat[0][1];
    Real fM02 = m_kMat[0][2];
    Real fM11 = m_kMat[1][1];
    Real fM12 = m_kMat[1][2];
    Real fM22 = m_kMat[2][2];

    m_afDiag[0] = fM00;
    m_afSubd[2] = (Real)0.0;
    if ( fM02 != (Real)0.0 )
    {
        Real fLength = Math<Real>::Sqrt(fM01*fM01+fM02*fM02);
        Real fInvLength = ((Real)1.0)/fLength;
        fM01 *= fInvLength;
        fM02 *= fInvLength;
        Real fQ = ((Real)2.0)*fM01*fM12+fM02*(fM22-fM11);
        m_afDiag[1] = fM11+fM02*fQ;
        m_afDiag[2] = fM22-fM02*fQ;
        m_afSubd[0] = fLength;
        m_afSubd[1] = fM12-fM01*fQ;
        m_kMat[0][0] = (Real)1.0;
        m_kMat[0][1] = (Real)0.0;
        m_kMat[0][2] = (Real)0.0;
        m_kMat[1][0] = (Real)0.0;
        m_kMat[1][1] = fM01;
        m_kMat[1][2] = fM02;
        m_kMat[2][0] = (Real)0.0;
        m_kMat[2][1] = fM02;
        m_kMat[2][2] = -fM01;
    }
    else
    {
        m_afDiag[1] = fM11;
        m_afDiag[2] = fM22;
        m_afSubd[0] = fM01;
        m_afSubd[1] = fM12;
        m_kMat[0][0] = (Real)1.0;
        m_kMat[0][1] = (Real)0.0;
        m_kMat[0][2] = (Real)0.0;
        m_kMat[1][0] = (Real)0.0;
        m_kMat[1][1] = (Real)1.0;
        m_kMat[1][2] = (Real)0.0;
        m_kMat[2][0] = (Real)0.0;
        m_kMat[2][1] = (Real)0.0;
        m_kMat[2][2] = (Real)1.0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::Tridiagonal4 ()
{
    // save matrix M
    Real fM00 = m_kMat[0][0];
    Real fM01 = m_kMat[0][1];
    Real fM02 = m_kMat[0][2];
    Real fM03 = m_kMat[0][3];
    Real fM11 = m_kMat[1][1];
    Real fM12 = m_kMat[1][2];
    Real fM13 = m_kMat[1][3];
    Real fM22 = m_kMat[2][2];
    Real fM23 = m_kMat[2][3];
    Real fM33 = m_kMat[3][3];

    m_afDiag[0] = fM00;
    m_afSubd[3] = (Real)0.0;

    m_kMat[0][0] = (Real)1.0;
    m_kMat[0][1] = (Real)0.0;
    m_kMat[0][2] = (Real)0.0;
    m_kMat[0][3] = (Real)0.0;
    m_kMat[1][0] = (Real)0.0;
    m_kMat[2][0] = (Real)0.0;
    m_kMat[3][0] = (Real)0.0;

    Real fLength, fInvLength;

    if ( fM02 != (Real)0.0 || fM03 != (Real)0.0 )
    {
        Real fQ11, fQ12, fQ13;
        Real fQ21, fQ22, fQ23;
        Real fQ31, fQ32, fQ33;

        // build column Q1
        fLength = Math<Real>::Sqrt(fM01*fM01+fM02*fM02+fM03*fM03);
        fInvLength = ((Real)1.0)/fLength;
        fQ11 = fM01*fInvLength;
        fQ21 = fM02*fInvLength;
        fQ31 = fM03*fInvLength;

        m_afSubd[0] = fLength;

        // compute S*Q1
        Real fV0 = fM11*fQ11+fM12*fQ21+fM13*fQ31;
        Real fV1 = fM12*fQ11+fM22*fQ21+fM23*fQ31;
        Real fV2 = fM13*fQ11+fM23*fQ21+fM33*fQ31;

        m_afDiag[1] = fQ11*fV0+fQ21*fV1+fQ31*fV2;

        // build column Q3 = Q1x(S*Q1)
        fQ13 = fQ21*fV2-fQ31*fV1;
        fQ23 = fQ31*fV0-fQ11*fV2;
        fQ33 = fQ11*fV1-fQ21*fV0;
        fLength = Math<Real>::Sqrt(fQ13*fQ13+fQ23*fQ23+fQ33*fQ33);
        if ( fLength > (Real)0.0 )
        {
            fInvLength = ((Real)1.0)/fLength;
            fQ13 *= fInvLength;
            fQ23 *= fInvLength;
            fQ33 *= fInvLength;

            // build column Q2 = Q3xQ1
            fQ12 = fQ23*fQ31-fQ33*fQ21; 
            fQ22 = fQ33*fQ11-fQ13*fQ31;
            fQ32 = fQ13*fQ21-fQ23*fQ11;

            fV0 = fQ12*fM11+fQ22*fM12+fQ32*fM13;
            fV1 = fQ12*fM12+fQ22*fM22+fQ32*fM23;
            fV2 = fQ12*fM13+fQ22*fM23+fQ32*fM33;
            m_afSubd[1] = fQ11*fV0+fQ21*fV1+fQ31*fV2;
            m_afDiag[2] = fQ12*fV0+fQ22*fV1+fQ32*fV2;
            m_afSubd[2] = fQ13*fV0+fQ23*fV1+fQ33*fV2;

            fV0 = fQ13*fM11+fQ23*fM12+fQ33*fM13;
            fV1 = fQ13*fM12+fQ23*fM22+fQ33*fM23;
            fV2 = fQ13*fM13+fQ23*fM23+fQ33*fM33;
            m_afDiag[3] = fQ13*fV0+fQ23*fV1+fQ33*fV2;
        }
        else
        {
            // S*Q1 parallel to Q1, choose any valid Q2 and Q3
            m_afSubd[1] = (Real)0.0;

            fLength = fQ21*fQ21+fQ31*fQ31;
            if ( fLength > (Real)0.0 )
            {
                fInvLength = ((Real)1.0)/fLength;
                Real fTmp = fQ11-(Real)1.0;
                fQ12 = -fQ21;
                fQ22 = ((Real)1.0)+fTmp*fQ21*fQ21*fInvLength;
                fQ32 = fTmp*fQ21*fQ31*fInvLength;

                fQ13 = -fQ31;
                fQ23 = fQ32;
                fQ33 = ((Real)1.0)+fTmp*fQ31*fQ31*fInvLength;

                fV0 = fQ12*fM11+fQ22*fM12+fQ32*fM13;
                fV1 = fQ12*fM12+fQ22*fM22+fQ32*fM23;
                fV2 = fQ12*fM13+fQ22*fM23+fQ32*fM33;
                m_afDiag[2] = fQ12*fV0+fQ22*fV1+fQ32*fV2;
                m_afSubd[2] = fQ13*fV0+fQ23*fV1+fQ33*fV2;

                fV0 = fQ13*fM11+fQ23*fM12+fQ33*fM13;
                fV1 = fQ13*fM12+fQ23*fM22+fQ33*fM23;
                fV2 = fQ13*fM13+fQ23*fM23+fQ33*fM33;
                m_afDiag[3] = fQ13*fV0+fQ23*fV1+fQ33*fV2;
            }
            else
            {
                // Q1 = (+-1,0,0)
                fQ12 = (Real)0.0; fQ22 = (Real)1.0; fQ32 = (Real)0.0;
                fQ13 = (Real)0.0; fQ23 = (Real)0.0; fQ33 = (Real)1.0;

                m_afDiag[2] = fM22;
                m_afDiag[3] = fM33;
                m_afSubd[2] = fM23;
            }
        }

        m_kMat[1][1] = fQ11; m_kMat[1][2] = fQ12; m_kMat[1][3] = fQ13;
        m_kMat[2][1] = fQ21; m_kMat[2][2] = fQ22; m_kMat[2][3] = fQ23;
        m_kMat[3][1] = fQ31; m_kMat[3][2] = fQ32; m_kMat[3][3] = fQ33;
    }
    else
    {
        m_afDiag[1] = fM11;
        m_afSubd[0] = fM01;
        m_kMat[1][1] = (Real)1.0;
        m_kMat[2][1] = (Real)0.0;
        m_kMat[3][1] = (Real)0.0; 

        if ( fM13 != (Real)0.0 )
        {
            fLength = Math<Real>::Sqrt(fM12*fM12+fM13*fM13);
            fInvLength = ((Real)1.0)/fLength;
            fM12 *= fInvLength;
            fM13 *= fInvLength;
            Real fQ = ((Real)2.0)*fM12*fM23+fM13*(fM33-fM22);

            m_afDiag[2] = fM22+fM13*fQ;
            m_afDiag[3] = fM33-fM13*fQ;
            m_afSubd[1] = fLength;
            m_afSubd[2] = fM23-fM12*fQ;
            m_kMat[1][2] = (Real)0.0;
            m_kMat[1][3] = (Real)0.0;
            m_kMat[2][2] = fM12;
            m_kMat[2][3] = fM13;
            m_kMat[3][2] = fM13;
            m_kMat[3][3] = -fM12;
        }
        else
        {
            m_afDiag[2] = fM22;
            m_afDiag[3] = fM33;
            m_afSubd[1] = fM12;
            m_afSubd[2] = fM23;
            m_kMat[1][2] = (Real)0.0;
            m_kMat[1][3] = (Real)0.0;
            m_kMat[2][2] = (Real)1.0;
            m_kMat[2][3] = (Real)0.0;
            m_kMat[3][2] = (Real)0.0;
            m_kMat[3][3] = (Real)1.0;
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::TridiagonalN ()
{
    int i0, i1, i2, i3;

    for (i0 = m_iSize-1, i3 = m_iSize-2; i0 >= 1; i0--, i3--)
    {
        Real fH = (Real)0.0, fScale = (Real)0.0;

        if ( i3 > 0 )
        {
            for (i2 = 0; i2 <= i3; i2++)
                fScale += Math<Real>::FAbs(m_kMat[i0][i2]);
            if ( fScale == (Real)0.0 )
            {
                m_afSubd[i0] = m_kMat[i0][i3];
            }
            else
            {
                Real fInvScale = ((Real)1.0)/fScale;
                for (i2 = 0; i2 <= i3; i2++)
                {
                    m_kMat[i0][i2] *= fInvScale;
                    fH += m_kMat[i0][i2]*m_kMat[i0][i2];
                }
                Real fF = m_kMat[i0][i3];
                Real fG = Math<Real>::Sqrt(fH);
                if ( fF > (Real)0.0 )
                    fG = -fG;
                m_afSubd[i0] = fScale*fG;
                fH -= fF*fG;
                m_kMat[i0][i3] = fF-fG;
                fF = (Real)0.0;
                Real fInvH = ((Real)1.0)/fH;
                for (i1 = 0; i1 <= i3; i1++)
                {
                    m_kMat[i1][i0] = m_kMat[i0][i1]*fInvH;
                    fG = (Real)0.0;
                    for (i2 = 0; i2 <= i1; i2++)
                        fG += m_kMat[i1][i2]*m_kMat[i0][i2];
                    for (i2 = i1+1; i2 <= i3; i2++)
                        fG += m_kMat[i2][i1]*m_kMat[i0][i2];
                    m_afSubd[i1] = fG*fInvH;
                    fF += m_afSubd[i1]*m_kMat[i0][i1];
                }
                Real fHalfFdivH = ((Real)0.5)*fF*fInvH;
                for (i1 = 0; i1 <= i3; i1++)
                {
                    fF = m_kMat[i0][i1];
                    fG = m_afSubd[i1] - fHalfFdivH*fF;
                    m_afSubd[i1] = fG;
                    for (i2 = 0; i2 <= i1; i2++)
                    {
                        m_kMat[i1][i2] -= fF*m_afSubd[i2] +
                            fG*m_kMat[i0][i2];
                    }
                }
            }
        }
        else
        {
            m_afSubd[i0] = m_kMat[i0][i3];
        }

        m_afDiag[i0] = fH;
    }

    m_afDiag[0] = (Real)0.0;
    m_afSubd[0] = (Real)0.0;
    for (i0 = 0, i3 = -1; i0 <= m_iSize-1; i0++, i3++)
    {
        if ( m_afDiag[i0] != (Real)0.0 )
        {
            for (i1 = 0; i1 <= i3; i1++)
            {
                Real fSum = (Real)0.0;
                for (i2 = 0; i2 <= i3; i2++)
                    fSum += m_kMat[i0][i2]*m_kMat[i2][i1];
                for (i2 = 0; i2 <= i3; i2++)
                    m_kMat[i2][i1] -= fSum*m_kMat[i2][i0];
            }
        }
        m_afDiag[i0] = m_kMat[i0][i0];
        m_kMat[i0][i0] = (Real)1.0;
        for (i1 = 0; i1 <= i3; i1++)
        {
            m_kMat[i1][i0] = (Real)0.0;
            m_kMat[i0][i1] = (Real)0.0;
        }
    }

    // re-ordering if Eigen::QLAlgorithm is used subsequently
    for (i0 = 1, i3 = 0; i0 < m_iSize; i0++, i3++)
        m_afSubd[i3] = m_afSubd[i0];
    m_afSubd[m_iSize-1] = (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Eigen<Real>::QLAlgorithm ()
{
    const int iMaxIter = 32;

    for (int i0 = 0; i0 < m_iSize; i0++)
    {
        int i1;
        for (i1 = 0; i1 < iMaxIter; i1++)
        {
            int i2;
            for (i2 = i0; i2 <= m_iSize-2; i2++)
            {
                Real fTmp = Math<Real>::FAbs(m_afDiag[i2]) +
                    Math<Real>::FAbs(m_afDiag[i2+1]);
                if ( Math<Real>::FAbs(m_afSubd[i2]) + fTmp == fTmp )
                    break;
            }
            if ( i2 == i0 )
                break;

            Real fG = (m_afDiag[i0+1] - m_afDiag[i0])/(((Real)2.0) *
                m_afSubd[i0]);
            Real fR = Math<Real>::Sqrt(fG*fG+(Real)1.0);
            if ( fG < (Real)0.0 )
                fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG-fR);
            else
                fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG+fR);
            Real fSin = (Real)1.0, fCos = (Real)1.0, fP = (Real)0.0;
            for (int i3 = i2-1; i3 >= i0; i3--)
            {
                Real fF = fSin*m_afSubd[i3];
                Real fB = fCos*m_afSubd[i3];
                if ( Math<Real>::FAbs(fF) >= Math<Real>::FAbs(fG) )
                {
                    fCos = fG/fF;
                    fR = Math<Real>::Sqrt(fCos*fCos+(Real)1.0);
                    m_afSubd[i3+1] = fF*fR;
                    fSin = ((Real)1.0)/fR;
                    fCos *= fSin;
                }
                else
                {
                    fSin = fF/fG;
                    fR = Math<Real>::Sqrt(fSin*fSin+(Real)1.0);
                    m_afSubd[i3+1] = fG*fR;
                    fCos = ((Real)1.0)/fR;
                    fSin *= fCos;
                }
                fG = m_afDiag[i3+1]-fP;
                fR = (m_afDiag[i3]-fG)*fSin+((Real)2.0)*fB*fCos;
                fP = fSin*fR;
                m_afDiag[i3+1] = fG+fP;
                fG = fCos*fR-fB;

                for (int i4 = 0; i4 < m_iSize; i4++)
                {
                    fF = m_kMat[i4][i3+1];
                    m_kMat[i4][i3+1] = fSin*m_kMat[i4][i3]+fCos*fF;
                    m_kMat[i4][i3] = fCos*m_kMat[i4][i3]-fSin*fF;
                }
            }
            m_afDiag[i0] -= fP;
            m_afSubd[i0] = fG;
            m_afSubd[i2] = (Real)0.0;
        }
        if ( i1 == iMaxIter )
            return false;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecreasingSort ()
{
    // sort eigenvalues in decreasing order, e[0] >= ... >= e[iSize-1]
    for (int i0 = 0, i1; i0 <= m_iSize-2; i0++)
    {
        // locate maximum eigenvalue
        i1 = i0;
        Real fMax = m_afDiag[i1];
        int i2;
        for (i2 = i0+1; i2 < m_iSize; i2++)
        {
            if ( m_afDiag[i2] > fMax )
            {
                i1 = i2;
                fMax = m_afDiag[i1];
            }
        }

        if ( i1 != i0 )
        {
            // swap eigenvalues
            m_afDiag[i1] = m_afDiag[i0];
            m_afDiag[i0] = fMax;

            // swap eigenvectors
            for (i2 = 0; i2 < m_iSize; i2++)
            {
                Real fTmp = m_kMat[i2][i0];
                m_kMat[i2][i0] = m_kMat[i2][i1];
                m_kMat[i2][i1] = fTmp;
                m_bIsRotation = !m_bIsRotation;
            }
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncreasingSort ()
{
    // sort eigenvalues in increasing order, e[0] <= ... <= e[iSize-1]
    for (int i0 = 0, i1; i0 <= m_iSize-2; i0++)
    {
        // locate minimum eigenvalue
        i1 = i0;
        Real fMin = m_afDiag[i1];
        int i2;
        for (i2 = i0+1; i2 < m_iSize; i2++)
        {
            if ( m_afDiag[i2] < fMin )
            {
                i1 = i2;
                fMin = m_afDiag[i1];
            }
        }

        if ( i1 != i0 )
        {
            // swap eigenvalues
            m_afDiag[i1] = m_afDiag[i0];
            m_afDiag[i0] = fMin;

            // swap eigenvectors
            for (i2 = 0; i2 < m_iSize; i2++)
            {
                Real fTmp = m_kMat[i2][i0];
                m_kMat[i2][i0] = m_kMat[i2][i1];
                m_kMat[i2][i1] = fTmp;
                m_bIsRotation = !m_bIsRotation;
            }
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::GuaranteeRotation ()
{
    if ( !m_bIsRotation )
    {
        // change sign on the first column
        for (int iRow = 0; iRow < m_iSize; iRow++)
            m_kMat[iRow][0] = -m_kMat[iRow][0];
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::EigenStuff2 ()
{
    Tridiagonal2();
    QLAlgorithm();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::EigenStuff3 ()
{
    Tridiagonal3();
    QLAlgorithm();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::EigenStuff4 ()
{
    Tridiagonal4();
    QLAlgorithm();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::EigenStuffN ()
{
    TridiagonalN();
    QLAlgorithm();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::EigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:  Tridiagonal2();  break;
        case 3:  Tridiagonal3();  break;
        case 4:  Tridiagonal4();  break;
        default: TridiagonalN();  break;
    }
    QLAlgorithm();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecrSortEigenStuff2 ()
{
    Tridiagonal2();
    QLAlgorithm();
    DecreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecrSortEigenStuff3 ()
{
    Tridiagonal3();
    QLAlgorithm();
    DecreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecrSortEigenStuff4 ()
{
    Tridiagonal4();
    QLAlgorithm();
    DecreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecrSortEigenStuffN ()
{
    TridiagonalN();
    QLAlgorithm();
    DecreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::DecrSortEigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:  Tridiagonal2();  break;
        case 3:  Tridiagonal3();  break;
        case 4:  Tridiagonal4();  break;
        default: TridiagonalN();  break;
    }
    QLAlgorithm();
    DecreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncrSortEigenStuff2 ()
{
    Tridiagonal2();
    QLAlgorithm();
    IncreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncrSortEigenStuff3 ()
{
    Tridiagonal3();
    QLAlgorithm();
    IncreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncrSortEigenStuff4 ()
{
    Tridiagonal4();
    QLAlgorithm();
    IncreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncrSortEigenStuffN ()
{
    TridiagonalN();
    QLAlgorithm();
    IncreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------
template <class Real>
void Eigen<Real>::IncrSortEigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:  Tridiagonal2();  break;
        case 3:  Tridiagonal3();  break;
        case 4:  Tridiagonal4();  break;
        default: TridiagonalN();  break;
    }
    QLAlgorithm();
    IncreasingSort();
    GuaranteeRotation();
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template class WML_ITEM Eigen<float>;
template class WML_ITEM Eigen<double>;
}
//----------------------------------------------------------------------------
