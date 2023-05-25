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
Matrix2<Real>::Matrix2()
{
    // the matrix is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Matrix2 &rkM)
{
    memcpy(Matrix2<Real>::m_afEntry, rkM.Matrix2<Real>::m_afEntry, 4 * sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Matrix<2, Real> &rkM)
{
    memcpy(Matrix2<Real>::m_afEntry, (const Real *)rkM, 4 * sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(Real fM00, Real fM01, Real fM10, Real fM11)
{
    Matrix2<Real>::m_afEntry[0] = fM00;
    Matrix2<Real>::m_afEntry[1] = fM01;
    Matrix2<Real>::m_afEntry[2] = fM10;
    Matrix2<Real>::m_afEntry[3] = fM11;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Real afEntry[4], bool bRowMajor)
{
    if (bRowMajor)
    {
        memcpy(Matrix2<Real>::m_afEntry, afEntry, 4 * sizeof(Real));
    }
    else
    {
        Matrix2<Real>::m_afEntry[0] = afEntry[0];
        Matrix2<Real>::m_afEntry[1] = afEntry[2];
        Matrix2<Real>::m_afEntry[2] = afEntry[1];
        Matrix2<Real>::m_afEntry[3] = afEntry[3];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Vector2<Real> &rkU,
                       const Vector2<Real> &rkV, bool bColumns)
{
    if (bColumns)
    {
        Matrix2<Real>::m_afEntry[0] = rkU[0];
        Matrix2<Real>::m_afEntry[1] = rkV[0];
        Matrix2<Real>::m_afEntry[2] = rkU[1];
        Matrix2<Real>::m_afEntry[3] = rkV[1];
    }
    else
    {
        Matrix2<Real>::m_afEntry[0] = rkU[0];
        Matrix2<Real>::m_afEntry[1] = rkU[1];
        Matrix2<Real>::m_afEntry[2] = rkV[0];
        Matrix2<Real>::m_afEntry[3] = rkV[1];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Vector2<Real> *akV, bool bColumns)
{
    if (bColumns)
    {
        Matrix2<Real>::m_afEntry[0] = akV[0][0];
        Matrix2<Real>::m_afEntry[1] = akV[1][0];
        Matrix2<Real>::m_afEntry[2] = akV[0][1];
        Matrix2<Real>::m_afEntry[3] = akV[1][1];
    }
    else
    {
        Matrix2<Real>::m_afEntry[0] = akV[0][0];
        Matrix2<Real>::m_afEntry[1] = akV[0][1];
        Matrix2<Real>::m_afEntry[2] = akV[1][0];
        Matrix2<Real>::m_afEntry[3] = akV[1][1];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(const Vector2<Real> &rkU,
                       const Vector2<Real> &rkV)
{
    MakeTensorProduct(rkU, rkV);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeTensorProduct(const Vector2<Real> &rkU,
                                      const Vector2<Real> &rkV)
{
    Matrix2<Real>::m_afEntry[0] = rkU[0] * rkV[0];
    Matrix2<Real>::m_afEntry[1] = rkU[0] * rkV[1];
    Matrix2<Real>::m_afEntry[2] = rkU[1] * rkV[0];
    Matrix2<Real>::m_afEntry[3] = rkU[1] * rkV[1];
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(Real fM00, Real fM11)
{
    MakeDiagonal(fM00, fM11);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeDiagonal(Real fM00, Real fM11)
{
    Matrix2<Real>::m_afEntry[0] = fM00;
    Matrix2<Real>::m_afEntry[1] = (Real)0.0;
    Matrix2<Real>::m_afEntry[2] = (Real)0.0;
    Matrix2<Real>::m_afEntry[3] = fM11;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2(Real fAngle)
{
    FromAngle(fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::FromAngle(Real fAngle)
{
    Matrix2<Real>::m_afEntry[0] = Math<Real>::Cos(fAngle);
    Matrix2<Real>::m_afEntry[1] = -Math<Real>::Sin(fAngle);
    Matrix2<Real>::m_afEntry[2] = Matrix2<Real>::m_afEntry[1];
    Matrix2<Real>::m_afEntry[3] = Matrix2<Real>::m_afEntry[0];
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> &Matrix2<Real>::operator=(const Matrix2 &rkM)
{
    memcpy(Matrix2<Real>::m_afEntry, rkM.Matrix2<Real>::m_afEntry, 4 * sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> &Matrix2<Real>::operator=(const Matrix<2, Real> &rkM)
{
    memcpy(Matrix2<Real>::m_afEntry, (const Real *)rkM, 4 * sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::Inverse() const
{
    Matrix2<Real> kInverse;

    Real fDet = Matrix2<Real>::m_afEntry[0] * Matrix2<Real>::m_afEntry[3] - Matrix2<Real>::m_afEntry[1] * Matrix2<Real>::m_afEntry[2];
    if (Math<Real>::FAbs(fDet) > Math<Real>::EPSILON)
    {
        Real fInvDet = ((Real)1.0) / fDet;
        kInverse[0][0] = Matrix2<Real>::m_afEntry[3] * fInvDet;
        kInverse[0][1] = -Matrix2<Real>::m_afEntry[1] * fInvDet;
        kInverse[1][0] = -Matrix2<Real>::m_afEntry[2] * fInvDet;
        kInverse[1][1] = Matrix2<Real>::m_afEntry[0] * fInvDet;
    }
    else
    {
        kInverse.MakeZero();
    }

    return kInverse;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::Adjoint() const
{
    return Matrix2<Real>(
        Matrix2<Real>::m_afEntry[3], -Matrix2<Real>::m_afEntry[1],
        -Matrix2<Real>::m_afEntry[2], Matrix2<Real>::m_afEntry[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix2<Real>::Determinant() const
{
    return Matrix2<Real>::m_afEntry[0] * Matrix2<Real>::m_afEntry[3] - Matrix2<Real>::m_afEntry[1] * Matrix2<Real>::m_afEntry[2];
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::ToAngle(Real &rfAngle) const
{
    // assert:  matrix is a rotation
    rfAngle = Math<Real>::ATan2(Matrix2<Real>::m_afEntry[2], Matrix2<Real>::m_afEntry[0]);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::Orthonormalize()
{
    // Algorithm uses Gram-Schmidt orthogonalization.  If 'this' matrix is
    // M = [m0|m1], then orthonormal output matrix is Q = [q0|q1],
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.

    // compute q0
    Real fInvLength = Math<Real>::InvSqrt(Matrix2<Real>::m_afEntry[0] * Matrix2<Real>::m_afEntry[0] +
                                          Matrix2<Real>::m_afEntry[2] * Matrix2<Real>::m_afEntry[2]);

    Matrix2<Real>::m_afEntry[0] *= fInvLength;
    Matrix2<Real>::m_afEntry[2] *= fInvLength;

    // compute q1
    Real fDot0 = Matrix2<Real>::m_afEntry[0] * Matrix2<Real>::m_afEntry[1] + Matrix2<Real>::m_afEntry[2] * Matrix2<Real>::m_afEntry[3];
    Matrix2<Real>::m_afEntry[1] -= fDot0 * Matrix2<Real>::m_afEntry[0];
    Matrix2<Real>::m_afEntry[3] -= fDot0 * Matrix2<Real>::m_afEntry[2];

    fInvLength = Math<Real>::InvSqrt(Matrix2<Real>::m_afEntry[1] * Matrix2<Real>::m_afEntry[1] +
                                     Matrix2<Real>::m_afEntry[3] * Matrix2<Real>::m_afEntry[3]);

    Matrix2<Real>::m_afEntry[1] *= fInvLength;
    Matrix2<Real>::m_afEntry[3] *= fInvLength;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::EigenDecomposition(Matrix2 &rkRot, Matrix2 &rkDiag) const
{
    Real fTrace = Matrix2<Real>::m_afEntry[0] + Matrix2<Real>::m_afEntry[3];
    Real fDiff = Matrix2<Real>::m_afEntry[0] - Matrix2<Real>::m_afEntry[3];
    Real fDiscr = Math<Real>::Sqrt(fDiff * fDiff +
                                   ((Real)4.0) * Matrix2<Real>::m_afEntry[1] * Matrix2<Real>::m_afEntry[1]);
    Real fEVal0 = ((Real)0.5) * (fTrace - fDiscr);
    Real fEVal1 = ((Real)0.5) * (fTrace + fDiscr);
    rkDiag.MakeDiagonal(fEVal0, fEVal1);

    Vector2<Real> kRow0(Matrix2<Real>::m_afEntry[0] - fEVal0, Matrix2<Real>::m_afEntry[1]);
    Vector2<Real> kRow1(Matrix2<Real>::m_afEntry[1], Matrix2<Real>::m_afEntry[3] - fEVal0);
    Real fLength0 = kRow0.Length();
    Real fLength1 = kRow1.Length();

    if (fLength0 > fLength1)
    {
        if (fLength0 > Math<Real>::EPSILON)
        {
            rkRot.Matrix2<Real>::m_afEntry[0] = kRow0[1];
            rkRot.Matrix2<Real>::m_afEntry[1] = kRow0[0];
            rkRot.Matrix2<Real>::m_afEntry[2] = -kRow0[0];
            rkRot.Matrix2<Real>::m_afEntry[3] = kRow0[1];
        }
        else
        {
            rkRot.MakeIdentity();
        }
    }
    else
    {
        if (fLength1 > Math<Real>::EPSILON)
        {
            rkRot.Matrix2<Real>::m_afEntry[0] = kRow1[1];
            rkRot.Matrix2<Real>::m_afEntry[1] = kRow1[0];
            rkRot.Matrix2<Real>::m_afEntry[2] = -kRow1[0];
            rkRot.Matrix2<Real>::m_afEntry[3] = kRow1[1];
        }
        else
        {
            rkRot.MakeIdentity();
        }
    }
}
//----------------------------------------------------------------------------
