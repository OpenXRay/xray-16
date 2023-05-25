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
Matrix3<Real>::Matrix3()
{
    // the matrix is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Matrix3 &rkM)
{
    memcpy(Matrix3<Real>::m_afEntry, rkM.Matrix3<Real>::m_afEntry, 9 * sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Matrix<3, Real> &rkM)
{
    memcpy(Matrix3<Real>::m_afEntry, (const Real *)rkM, 9 * sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real> &Matrix3<Real>::operator=(const Matrix3 &rkM)
{
    memcpy(Matrix3<Real>::m_afEntry, rkM.Matrix3<Real>::m_afEntry, 9 * sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real> &Matrix3<Real>::operator=(const Matrix<3, Real> &rkM)
{
    memcpy(Matrix3<Real>::m_afEntry, (const Real *)rkM, 9 * sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(Real fM00, Real fM01, Real fM02, Real fM10,
                       Real fM11, Real fM12, Real fM20, Real fM21, Real fM22)
{
    Matrix3<Real>::m_afEntry[0] = fM00;
    Matrix3<Real>::m_afEntry[1] = fM01;
    Matrix3<Real>::m_afEntry[2] = fM02;
    Matrix3<Real>::m_afEntry[3] = fM10;
    Matrix3<Real>::m_afEntry[4] = fM11;
    Matrix3<Real>::m_afEntry[5] = fM12;
    Matrix3<Real>::m_afEntry[6] = fM20;
    Matrix3<Real>::m_afEntry[7] = fM21;
    Matrix3<Real>::m_afEntry[8] = fM22;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Real afEntry[9], bool bRowMajor)
{
    if (bRowMajor)
    {
        memcpy(Matrix3<Real>::m_afEntry, afEntry, 9 * sizeof(Real));
    }
    else
    {
        Matrix3<Real>::m_afEntry[0] = afEntry[0];
        Matrix3<Real>::m_afEntry[1] = afEntry[3];
        Matrix3<Real>::m_afEntry[2] = afEntry[6];
        Matrix3<Real>::m_afEntry[3] = afEntry[1];
        Matrix3<Real>::m_afEntry[4] = afEntry[4];
        Matrix3<Real>::m_afEntry[5] = afEntry[7];
        Matrix3<Real>::m_afEntry[6] = afEntry[2];
        Matrix3<Real>::m_afEntry[7] = afEntry[5];
        Matrix3<Real>::m_afEntry[8] = afEntry[8];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Vector3<Real> &rkU,
                       const Vector3<Real> &rkV, const Vector3<Real> &rkW, bool bColumns)
{
    if (bColumns)
    {
        Matrix3<Real>::m_afEntry[0] = rkU[0];
        Matrix3<Real>::m_afEntry[1] = rkV[0];
        Matrix3<Real>::m_afEntry[2] = rkW[0];
        Matrix3<Real>::m_afEntry[3] = rkU[1];
        Matrix3<Real>::m_afEntry[4] = rkV[1];
        Matrix3<Real>::m_afEntry[5] = rkW[1];
        Matrix3<Real>::m_afEntry[6] = rkU[2];
        Matrix3<Real>::m_afEntry[7] = rkV[2];
        Matrix3<Real>::m_afEntry[8] = rkW[2];
    }
    else
    {
        Matrix3<Real>::m_afEntry[0] = rkU[0];
        Matrix3<Real>::m_afEntry[1] = rkU[1];
        Matrix3<Real>::m_afEntry[2] = rkU[2];
        Matrix3<Real>::m_afEntry[3] = rkV[0];
        Matrix3<Real>::m_afEntry[4] = rkV[1];
        Matrix3<Real>::m_afEntry[5] = rkV[2];
        Matrix3<Real>::m_afEntry[6] = rkW[0];
        Matrix3<Real>::m_afEntry[7] = rkW[1];
        Matrix3<Real>::m_afEntry[8] = rkW[2];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Vector3<Real> *akV, bool bColumns)
{
    if (bColumns)
    {
        Matrix3<Real>::m_afEntry[0] = akV[0][0];
        Matrix3<Real>::m_afEntry[1] = akV[1][0];
        Matrix3<Real>::m_afEntry[2] = akV[2][0];
        Matrix3<Real>::m_afEntry[3] = akV[0][1];
        Matrix3<Real>::m_afEntry[4] = akV[1][1];
        Matrix3<Real>::m_afEntry[5] = akV[2][1];
        Matrix3<Real>::m_afEntry[6] = akV[0][2];
        Matrix3<Real>::m_afEntry[7] = akV[1][2];
        Matrix3<Real>::m_afEntry[8] = akV[2][2];
    }
    else
    {
        Matrix3<Real>::m_afEntry[0] = akV[0][0];
        Matrix3<Real>::m_afEntry[1] = akV[0][1];
        Matrix3<Real>::m_afEntry[2] = akV[0][2];
        Matrix3<Real>::m_afEntry[3] = akV[1][0];
        Matrix3<Real>::m_afEntry[4] = akV[1][1];
        Matrix3<Real>::m_afEntry[5] = akV[1][2];
        Matrix3<Real>::m_afEntry[6] = akV[2][0];
        Matrix3<Real>::m_afEntry[7] = akV[2][1];
        Matrix3<Real>::m_afEntry[8] = akV[2][2];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Vector3<Real> &rkU,
                       const Vector3<Real> &rkV)
{
    MakeTensorProduct(rkU, rkV);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::MakeTensorProduct(const Vector3<Real> &rkU,
                                      const Vector3<Real> &rkV)
{
    Matrix3<Real>::m_afEntry[0] = rkU[0] * rkV[0];
    Matrix3<Real>::m_afEntry[1] = rkU[0] * rkV[1];
    Matrix3<Real>::m_afEntry[2] = rkU[0] * rkV[2];
    Matrix3<Real>::m_afEntry[3] = rkU[1] * rkV[0];
    Matrix3<Real>::m_afEntry[4] = rkU[1] * rkV[1];
    Matrix3<Real>::m_afEntry[5] = rkU[1] * rkV[2];
    Matrix3<Real>::m_afEntry[6] = rkU[2] * rkV[0];
    Matrix3<Real>::m_afEntry[7] = rkU[2] * rkV[1];
    Matrix3<Real>::m_afEntry[8] = rkU[2] * rkV[2];
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(Real fM00, Real fM11, Real fM22)
{
    MakeDiagonal(fM00, fM11, fM22);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::MakeDiagonal(Real fM00, Real fM11, Real fM22)
{
    Matrix3<Real>::m_afEntry[0] = fM00;
    Matrix3<Real>::m_afEntry[1] = (Real)0.0;
    Matrix3<Real>::m_afEntry[2] = (Real)0.0;
    Matrix3<Real>::m_afEntry[3] = (Real)0.0;
    Matrix3<Real>::m_afEntry[4] = fM11;
    Matrix3<Real>::m_afEntry[5] = (Real)0.0;
    Matrix3<Real>::m_afEntry[6] = (Real)0.0;
    Matrix3<Real>::m_afEntry[7] = (Real)0.0;
    Matrix3<Real>::m_afEntry[8] = fM22;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real>::Matrix3(const Vector3<Real> &rkAxis, Real fAngle)
{
    FromAxisAngle(rkAxis, fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromAxisAngle(const Vector3<Real> &rkAxis, Real fAngle)
{
    Real fCos = Math<Real>::Cos(fAngle);
    Real fSin = Math<Real>::Sin(fAngle);
    Real fOneMinusCos = ((Real)1.0) - fCos;
    Real fX2 = rkAxis[0] * rkAxis[0];
    Real fY2 = rkAxis[1] * rkAxis[1];
    Real fZ2 = rkAxis[2] * rkAxis[2];
    Real fXYM = rkAxis[0] * rkAxis[1] * fOneMinusCos;
    Real fXZM = rkAxis[0] * rkAxis[2] * fOneMinusCos;
    Real fYZM = rkAxis[1] * rkAxis[2] * fOneMinusCos;
    Real fXSin = rkAxis[0] * fSin;
    Real fYSin = rkAxis[1] * fSin;
    Real fZSin = rkAxis[2] * fSin;

    Matrix3<Real>::m_afEntry[0] = fX2 * fOneMinusCos + fCos;
    Matrix3<Real>::m_afEntry[1] = fXYM - fZSin;
    Matrix3<Real>::m_afEntry[2] = fXZM + fYSin;
    Matrix3<Real>::m_afEntry[3] = fXYM + fZSin;
    Matrix3<Real>::m_afEntry[4] = fY2 * fOneMinusCos + fCos;
    Matrix3<Real>::m_afEntry[5] = fYZM - fXSin;
    Matrix3<Real>::m_afEntry[6] = fXZM - fYSin;
    Matrix3<Real>::m_afEntry[7] = fYZM + fXSin;
    Matrix3<Real>::m_afEntry[8] = fZ2 * fOneMinusCos + fCos;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real> Matrix3<Real>::Inverse() const
{
    // Invert a 3x3 using cofactors.  This is faster than using a generic
    // Gaussian elimination because of the loop overhead of such a method.

    Matrix3<Real> kInverse;

    kInverse[0][0] = Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[8] - Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[7];
    kInverse[0][1] = Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[8];
    kInverse[0][2] = Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[5] - Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[4];
    kInverse[1][0] = Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[6] - Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[8];
    kInverse[1][1] = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[8] - Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[6];
    kInverse[1][2] = Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[3] - Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[5];
    kInverse[2][0] = Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[6];
    kInverse[2][1] = Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[6] - Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[7];
    kInverse[2][2] = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[4] - Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[3];

    Real fDet = Matrix3<Real>::m_afEntry[0] * kInverse[0][0] + Matrix3<Real>::m_afEntry[1] * kInverse[1][0] +
                Matrix3<Real>::m_afEntry[2] * kInverse[2][0];

    if (Math<Real>::FAbs(fDet) <= Math<Real>::EPSILON)
        return Matrix3::ZERO;

    kInverse /= fDet;
    return kInverse;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real> Matrix3<Real>::Adjoint() const
{
    Matrix3<Real> kAdjoint;

    kAdjoint[0][0] = Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[8] - Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[7];
    kAdjoint[0][1] = Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[8];
    kAdjoint[0][2] = Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[5] - Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[4];
    kAdjoint[1][0] = Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[6] - Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[8];
    kAdjoint[1][1] = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[8] - Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[6];
    kAdjoint[1][2] = Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[3] - Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[5];
    kAdjoint[2][0] = Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[6];
    kAdjoint[2][1] = Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[6] - Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[7];
    kAdjoint[2][2] = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[4] - Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[3];

    return kAdjoint;
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix3<Real>::Determinant() const
{
    Real fCo00 = Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[8] - Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[7];
    Real fCo10 = Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[6] - Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[8];
    Real fCo20 = Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[6];
    Real fDet = Matrix3<Real>::m_afEntry[0] * fCo00 + Matrix3<Real>::m_afEntry[1] * fCo10 + Matrix3<Real>::m_afEntry[2] * fCo20;
    return fDet;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::ToAxisAngle(Vector3<Real> &rkAxis, Real &rfAngle) const
{
    // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
    // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
    // I is the identity and
    //
    //       +-        -+
    //   P = |  0 -z +y |
    //       | +z  0 -x |
    //       | -y +x  0 |
    //       +-        -+
    //
    // If A > 0, R represents a counterclockwise rotation about the axis in
    // the sense of looking from the tip of the axis vector towards the
    // origin.  Some algebra will show that
    //
    //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P
    //
    // In the event that A = pi, R-R^t = 0 which prevents us from extracting
    // the axis through P.  Instead note that R = I+2*P^2 when A = pi, so
    // P^2 = (R-I)/2.  The diagonal entries of P^2 are x^2-1, y^2-1, and
    // z^2-1.  We can solve these for axis (x,y,z).  Because the angle is pi,
    // it does not matter which sign you choose on the square roots.

    Real fTrace = Matrix3<Real>::m_afEntry[0] + Matrix3<Real>::m_afEntry[4] + Matrix3<Real>::m_afEntry[8];
    Real fCos = ((Real)0.5) * (fTrace - ((Real)1.0));
    rfAngle = Math<Real>::ACos(fCos); // in [0,PI]

    if (rfAngle > (Real)0.0)
    {
        if (rfAngle < Math<Real>::_PI)
        {
            rkAxis[0] = Matrix3<Real>::m_afEntry[7] - Matrix3<Real>::m_afEntry[5];
            rkAxis[1] = Matrix3<Real>::m_afEntry[2] - Matrix3<Real>::m_afEntry[6];
            rkAxis[2] = Matrix3<Real>::m_afEntry[3] - Matrix3<Real>::m_afEntry[1];
            rkAxis.Normalize();
        }
        else
        {
            // angle is PI
            Real fHalfInverse;
            if (Matrix3<Real>::m_afEntry[0] >= Matrix3<Real>::m_afEntry[4])
            {
                // r00 >= r11
                if (Matrix3<Real>::m_afEntry[0] >= Matrix3<Real>::m_afEntry[8])
                {
                    // r00 is maximum diagonal term
                    rkAxis[0] = ((Real)0.5) * Math<Real>::Sqrt(Matrix3<Real>::m_afEntry[0] -
                                                               Matrix3<Real>::m_afEntry[4] - Matrix3<Real>::m_afEntry[8] + (Real)1.0);
                    fHalfInverse = ((Real)0.5) / rkAxis[0];
                    rkAxis[1] = fHalfInverse * Matrix3<Real>::m_afEntry[1];
                    rkAxis[2] = fHalfInverse * Matrix3<Real>::m_afEntry[2];
                }
                else
                {
                    // r22 is maximum diagonal term
                    rkAxis[2] = ((Real)0.5) * Math<Real>::Sqrt(Matrix3<Real>::m_afEntry[8] -
                                                               Matrix3<Real>::m_afEntry[0] - Matrix3<Real>::m_afEntry[4] + (Real)1.0);
                    fHalfInverse = ((Real)0.5) / rkAxis[2];
                    rkAxis[0] = fHalfInverse * Matrix3<Real>::m_afEntry[2];
                    rkAxis[1] = fHalfInverse * Matrix3<Real>::m_afEntry[5];
                }
            }
            else
            {
                // r11 > r00
                if (Matrix3<Real>::m_afEntry[4] >= Matrix3<Real>::m_afEntry[8])
                {
                    // r11 is maximum diagonal term
                    rkAxis[1] = ((Real)0.5) * Math<Real>::Sqrt(Matrix3<Real>::m_afEntry[4] -
                                                               Matrix3<Real>::m_afEntry[0] - Matrix3<Real>::m_afEntry[8] + (Real)1.0);
                    fHalfInverse = ((Real)0.5) / rkAxis[1];
                    rkAxis[0] = fHalfInverse * Matrix3<Real>::m_afEntry[1];
                    rkAxis[2] = fHalfInverse * Matrix3<Real>::m_afEntry[5];
                }
                else
                {
                    // r22 is maximum diagonal term
                    rkAxis[2] = ((Real)0.5) * Math<Real>::Sqrt(Matrix3<Real>::m_afEntry[8] -
                                                               Matrix3<Real>::m_afEntry[0] - Matrix3<Real>::m_afEntry[4] + (Real)1.0);
                    fHalfInverse = ((Real)0.5) / rkAxis[2];
                    rkAxis[0] = fHalfInverse * Matrix3<Real>::m_afEntry[2];
                    rkAxis[1] = fHalfInverse * Matrix3<Real>::m_afEntry[5];
                }
            }
        }
    }
    else
    {
        // The angle is 0 and the matrix is the identity.  Any axis will
        // work, so just use the x-axis.
        rkAxis[0] = (Real)1.0;
        rkAxis[1] = (Real)0.0;
        rkAxis[2] = (Real)0.0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::Orthonormalize()
{
    // Algorithm uses Gram-Schmidt orthogonalization.  If 'this' matrix is
    // M = [m0|m1|m2], then orthonormal output matrix is Q = [q0|q1|q2],
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.

    // compute q0
    Real fInvLength = Math<Real>::InvSqrt(Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[0] +
                                          Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[3] + Matrix3<Real>::m_afEntry[6] * Matrix3<Real>::m_afEntry[6]);

    Matrix3<Real>::m_afEntry[0] *= fInvLength;
    Matrix3<Real>::m_afEntry[3] *= fInvLength;
    Matrix3<Real>::m_afEntry[6] *= fInvLength;

    // compute q1
    Real fDot0 = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[1] + Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[4] +
                 Matrix3<Real>::m_afEntry[6] * Matrix3<Real>::m_afEntry[7];

    Matrix3<Real>::m_afEntry[1] -= fDot0 * Matrix3<Real>::m_afEntry[0];
    Matrix3<Real>::m_afEntry[4] -= fDot0 * Matrix3<Real>::m_afEntry[3];
    Matrix3<Real>::m_afEntry[7] -= fDot0 * Matrix3<Real>::m_afEntry[6];

    fInvLength = Math<Real>::InvSqrt(Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[1] +
                                     Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[4] + Matrix3<Real>::m_afEntry[7] * Matrix3<Real>::m_afEntry[7]);

    Matrix3<Real>::m_afEntry[1] *= fInvLength;
    Matrix3<Real>::m_afEntry[4] *= fInvLength;
    Matrix3<Real>::m_afEntry[7] *= fInvLength;

    // compute q2
    Real fDot1 = Matrix3<Real>::m_afEntry[1] * Matrix3<Real>::m_afEntry[2] + Matrix3<Real>::m_afEntry[4] * Matrix3<Real>::m_afEntry[5] +
                 Matrix3<Real>::m_afEntry[7] * Matrix3<Real>::m_afEntry[8];

    fDot0 = Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[2] + Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[5] +
            Matrix3<Real>::m_afEntry[6] * Matrix3<Real>::m_afEntry[8];

    Matrix3<Real>::m_afEntry[2] -= fDot0 * Matrix3<Real>::m_afEntry[0] + fDot1 * Matrix3<Real>::m_afEntry[1];
    Matrix3<Real>::m_afEntry[5] -= fDot0 * Matrix3<Real>::m_afEntry[3] + fDot1 * Matrix3<Real>::m_afEntry[4];
    Matrix3<Real>::m_afEntry[8] -= fDot0 * Matrix3<Real>::m_afEntry[6] + fDot1 * Matrix3<Real>::m_afEntry[7];

    fInvLength = Math<Real>::InvSqrt(Matrix3<Real>::m_afEntry[2] * Matrix3<Real>::m_afEntry[2] +
                                     Matrix3<Real>::m_afEntry[5] * Matrix3<Real>::m_afEntry[5] + Matrix3<Real>::m_afEntry[8] * Matrix3<Real>::m_afEntry[8]);

    Matrix3<Real>::m_afEntry[2] *= fInvLength;
    Matrix3<Real>::m_afEntry[5] *= fInvLength;
    Matrix3<Real>::m_afEntry[8] *= fInvLength;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesXYZ(Real &rfXAngle, Real &rfYAngle,
                                     Real &rfZAngle) const
{
    // rot =  cy*cz          -cy*sz           sy
    //        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
    //       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

    if (Matrix3<Real>::m_afEntry[2] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[2] > -(Real)1.0)
        {
            rfXAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[5], Matrix3<Real>::m_afEntry[8]);
            rfYAngle = (Real)asin((double)Matrix3<Real>::m_afEntry[2]);
            rfZAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[1], Matrix3<Real>::m_afEntry[0]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  XA - ZA = -atan2(r10,r11)
            rfXAngle = -Math<Real>::ATan2(Matrix3<Real>::m_afEntry[3], Matrix3<Real>::m_afEntry[4]);
            rfYAngle = -Math<Real>::HALF_PI;
            rfZAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  XAngle + ZAngle = atan2(r10,r11)
        rfXAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[3], Matrix3<Real>::m_afEntry[4]);
        rfYAngle = Math<Real>::HALF_PI;
        rfZAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesXZY(Real &rfXAngle, Real &rfZAngle,
                                     Real &rfYAngle) const
{
    // rot =  cy*cz          -sz              cz*sy
    //        sx*sy+cx*cy*sz  cx*cz          -cy*sx+cx*sy*sz
    //       -cx*sy+cy*sx*sz  cz*sx           cx*cy+sx*sy*sz

    if (Matrix3<Real>::m_afEntry[1] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[1] > -(Real)1.0)
        {
            rfXAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[7], Matrix3<Real>::m_afEntry[4]);
            rfZAngle = (Real)asin(-(double)Matrix3<Real>::m_afEntry[1]);
            rfYAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[2], Matrix3<Real>::m_afEntry[0]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  XA - YA = atan2(r20,r22)
            rfXAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[6], Matrix3<Real>::m_afEntry[8]);
            rfZAngle = Math<Real>::HALF_PI;
            rfYAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  XA + YA = atan2(-r20,r22)
        rfXAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[6], Matrix3<Real>::m_afEntry[8]);
        rfZAngle = -Math<Real>::HALF_PI;
        rfYAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesYXZ(Real &rfYAngle, Real &rfXAngle,
                                     Real &rfZAngle) const
{
    // rot =  cy*cz+sx*sy*sz  cz*sx*sy-cy*sz  cx*sy
    //        cx*sz           cx*cz          -sx
    //       -cz*sy+cy*sx*sz  cy*cz*sx+sy*sz  cx*cy

    if (Matrix3<Real>::m_afEntry[5] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[5] > -(Real)1.0)
        {
            rfYAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[2], Matrix3<Real>::m_afEntry[8]);
            rfXAngle = (Real)asin(-(double)Matrix3<Real>::m_afEntry[5]);
            rfZAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[3], Matrix3<Real>::m_afEntry[4]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  YA - ZA = atan2(r01,r00)
            rfYAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[1], Matrix3<Real>::m_afEntry[0]);
            rfXAngle = Math<Real>::HALF_PI;
            rfZAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  YA + ZA = atan2(-r01,r00)
        rfYAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[1], Matrix3<Real>::m_afEntry[0]);
        rfXAngle = -Math<Real>::HALF_PI;
        rfZAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesYZX(Real &rfYAngle, Real &rfZAngle,
                                     Real &rfXAngle) const
{
    // rot =  cy*cz           sx*sy-cx*cy*sz  cx*sy+cy*sx*sz
    //        sz              cx*cz          -cz*sx
    //       -cz*sy           cy*sx+cx*sy*sz  cx*cy-sx*sy*sz

    if (Matrix3<Real>::m_afEntry[3] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[3] > -(Real)1.0)
        {
            rfYAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[6], Matrix3<Real>::m_afEntry[0]);
            rfZAngle = (Real)asin((double)Matrix3<Real>::m_afEntry[3]);
            rfXAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[5], Matrix3<Real>::m_afEntry[4]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  YA - XA = -atan2(r21,r22);
            rfYAngle = -Math<Real>::ATan2(Matrix3<Real>::m_afEntry[7], Matrix3<Real>::m_afEntry[8]);
            rfZAngle = -Math<Real>::HALF_PI;
            rfXAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  YA + XA = atan2(r21,r22)
        rfYAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[7], Matrix3<Real>::m_afEntry[8]);
        rfZAngle = Math<Real>::HALF_PI;
        rfXAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesZXY(Real &rfZAngle, Real &rfXAngle,
                                     Real &rfYAngle) const
{
    // rot =  cy*cz-sx*sy*sz -cx*sz           cz*sy+cy*sx*sz
    //        cz*sx*sy+cy*sz  cx*cz          -cy*cz*sx+sy*sz
    //       -cx*sy           sx              cx*cy

    if (Matrix3<Real>::m_afEntry[7] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[7] > -(Real)1.0)
        {
            rfZAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[1], Matrix3<Real>::m_afEntry[4]);
            rfXAngle = (Real)asin((double)Matrix3<Real>::m_afEntry[7]);
            rfYAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[6], Matrix3<Real>::m_afEntry[8]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  ZA - YA = -atan(r02,r00)
            rfZAngle = -Math<Real>::ATan2(Matrix3<Real>::m_afEntry[2], Matrix3<Real>::m_afEntry[0]);
            rfXAngle = -Math<Real>::HALF_PI;
            rfYAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  ZA + YA = atan2(r02,r00)
        rfZAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[2], Matrix3<Real>::m_afEntry[0]);
        rfXAngle = Math<Real>::HALF_PI;
        rfYAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::ToEulerAnglesZYX(Real &rfZAngle, Real &rfYAngle,
                                     Real &rfXAngle) const
{
    // rot =  cy*cz           cz*sx*sy-cx*sz  cx*cz*sy+sx*sz
    //        cy*sz           cx*cz+sx*sy*sz -cz*sx+cx*sy*sz
    //       -sy              cy*sx           cx*cy

    if (Matrix3<Real>::m_afEntry[6] < (Real)1.0)
    {
        if (Matrix3<Real>::m_afEntry[6] > -(Real)1.0)
        {
            rfZAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[3], Matrix3<Real>::m_afEntry[0]);
            rfYAngle = (Real)asin(-(double)Matrix3<Real>::m_afEntry[6]);
            rfXAngle = Math<Real>::ATan2(Matrix3<Real>::m_afEntry[7], Matrix3<Real>::m_afEntry[8]);
            return true;
        }
        else
        {
            // WARNING.  Not unique.  ZA - XA = -atan2(r01,r02)
            rfZAngle = -Math<Real>::ATan2(Matrix3<Real>::m_afEntry[1], Matrix3<Real>::m_afEntry[2]);
            rfYAngle = Math<Real>::HALF_PI;
            rfXAngle = (Real)0.0;
            return false;
        }
    }
    else
    {
        // WARNING.  Not unique.  ZA + XA = atan2(-r01,-r02)
        rfZAngle = Math<Real>::ATan2(-Matrix3<Real>::m_afEntry[1], -Matrix3<Real>::m_afEntry[2]);
        rfYAngle = -Math<Real>::HALF_PI;
        rfXAngle = (Real)0.0;
        return false;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesXYZ(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    *this = kXMat * (kYMat * kZMat);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesXZY(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    *this = kXMat * (kZMat * kYMat);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesYXZ(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    *this = kYMat * (kXMat * kZMat);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesYZX(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    *this = kYMat * (kZMat * kXMat);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesZXY(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    *this = kZMat * (kXMat * kYMat);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::FromEulerAnglesZYX(Real fYAngle, Real fPAngle,
                                       Real fRAngle)
{
    Real fCos, fSin;

    fCos = Math<Real>::Cos(fYAngle);
    fSin = Math<Real>::Sin(fYAngle);
    Matrix3 kZMat(
        fCos, -fSin, (Real)0.0,
        fSin, fCos, (Real)0.0,
        (Real)0.0, (Real)0.0, (Real)1.0);

    fCos = Math<Real>::Cos(fPAngle);
    fSin = Math<Real>::Sin(fPAngle);
    Matrix3 kYMat(
        fCos, (Real)0.0, fSin,
        (Real)0.0, (Real)1.0, (Real)0.0,
        -fSin, (Real)0.0, fCos);

    fCos = Math<Real>::Cos(fRAngle);
    fSin = Math<Real>::Sin(fRAngle);
    Matrix3 kXMat(
        (Real)1.0, (Real)0.0, (Real)0.0,
        (Real)0.0, fCos, -fSin,
        (Real)0.0, fSin, fCos);

    *this = kZMat * (kYMat * kXMat);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix3<Real> Matrix3<Real>::Slerp(Real fT, const Matrix3 &rkR0,
                                   const Matrix3 &rkR1)
{
    Vector3<Real> kAxis;
    Real fAngle;
    Matrix3 kProd = rkR0.TransposeTimes(rkR1);
    kProd.ToAxisAngle(kAxis, fAngle);
    return rkR0 * Matrix3<Real>(kAxis, fT * fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::Tridiagonalize(Real afDiag[3], Real afSubDiag[3])
{
    // Householder reduction T = Q^t M Q
    //   Input:
    //     mat, symmetric 3x3 matrix M
    //   Output:
    //     mat, orthogonal matrix Q (a reflection)
    //     diag, diagonal entries of T
    //     subd, subdiagonal entries of T (T is symmetric)

    Real fA = Matrix3<Real>::m_afEntry[0];
    Real fB = Matrix3<Real>::m_afEntry[1];
    Real fC = Matrix3<Real>::m_afEntry[2];
    Real fD = Matrix3<Real>::m_afEntry[4];
    Real fE = Matrix3<Real>::m_afEntry[5];
    Real fF = Matrix3<Real>::m_afEntry[8];

    afDiag[0] = fA;
    afSubDiag[2] = (Real)0.0;
    if (Math<Real>::FAbs(fC) >= Math<Real>::EPSILON)
    {
        Real fLength = Math<Real>::Sqrt(fB * fB + fC * fC);
        Real fInvLength = ((Real)1.0) / fLength;
        fB *= fInvLength;
        fC *= fInvLength;
        Real fQ = ((Real)2.0) * fB * fE + fC * (fF - fD);
        afDiag[1] = fD + fC * fQ;
        afDiag[2] = fF - fC * fQ;
        afSubDiag[0] = fLength;
        afSubDiag[1] = fE - fB * fQ;

        Matrix3<Real>::m_afEntry[0] = (Real)1.0;
        Matrix3<Real>::m_afEntry[1] = (Real)0.0;
        Matrix3<Real>::m_afEntry[2] = (Real)0.0;
        Matrix3<Real>::m_afEntry[3] = (Real)0.0;
        Matrix3<Real>::m_afEntry[4] = fB;
        Matrix3<Real>::m_afEntry[5] = fC;
        Matrix3<Real>::m_afEntry[6] = (Real)0.0;
        Matrix3<Real>::m_afEntry[7] = fC;
        Matrix3<Real>::m_afEntry[8] = -fB;
    }
    else
    {
        afDiag[1] = fD;
        afDiag[2] = fF;
        afSubDiag[0] = fB;
        afSubDiag[1] = fE;

        Matrix3<Real>::m_afEntry[0] = (Real)1.0;
        Matrix3<Real>::m_afEntry[1] = (Real)0.0;
        Matrix3<Real>::m_afEntry[2] = (Real)0.0;
        Matrix3<Real>::m_afEntry[3] = (Real)0.0;
        Matrix3<Real>::m_afEntry[4] = (Real)1.0;
        Matrix3<Real>::m_afEntry[5] = (Real)0.0;
        Matrix3<Real>::m_afEntry[6] = (Real)0.0;
        Matrix3<Real>::m_afEntry[7] = (Real)0.0;
        Matrix3<Real>::m_afEntry[8] = -(Real)1.0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix3<Real>::QLAlgorithm(Real afDiag[3], Real afSubDiag[3])
{
    // QL iteration with implicit shifting to reduce matrix from tridiagonal
    // to diagonal

    for (int i0 = 0; i0 < 3; i0++)
    {
        const int iMaxIter = 32;
        int iIter;
        for (iIter = 0; iIter < iMaxIter; iIter++)
        {
            int i1;
            for (i1 = i0; i1 <= 1; i1++)
            {
                Real fSum = Math<Real>::FAbs(afDiag[i1]) +
                            Math<Real>::FAbs(afDiag[i1 + 1]);
                if (Math<Real>::FAbs(afSubDiag[i1]) + fSum == fSum)
                    break;
            }
            if (i1 == i0)
                break;

            Real fTmp0 = (afDiag[i0 + 1] - afDiag[i0]) /
                         (((Real)2.0) * afSubDiag[i0]);
            Real fTmp1 = Math<Real>::Sqrt(fTmp0 * fTmp0 + (Real)1.0);
            if (fTmp0 < (Real)0.0)
                fTmp0 = afDiag[i1] - afDiag[i0] + afSubDiag[i0] / (fTmp0 - fTmp1);
            else
                fTmp0 = afDiag[i1] - afDiag[i0] + afSubDiag[i0] / (fTmp0 + fTmp1);
            Real fSin = (Real)1.0;
            Real fCos = (Real)1.0;
            Real fTmp2 = (Real)0.0;
            for (int i2 = i1 - 1; i2 >= i0; i2--)
            {
                Real fTmp3 = fSin * afSubDiag[i2];
                Real fTmp4 = fCos * afSubDiag[i2];
                if (Math<Real>::FAbs(fTmp3) >= Math<Real>::FAbs(fTmp0))
                {
                    fCos = fTmp0 / fTmp3;
                    fTmp1 = Math<Real>::Sqrt(fCos * fCos + (Real)1.0);
                    afSubDiag[i2 + 1] = fTmp3 * fTmp1;
                    fSin = ((Real)1.0) / fTmp1;
                    fCos *= fSin;
                }
                else
                {
                    fSin = fTmp3 / fTmp0;
                    fTmp1 = Math<Real>::Sqrt(fSin * fSin + (Real)1.0);
                    afSubDiag[i2 + 1] = fTmp0 * fTmp1;
                    fCos = ((Real)1.0) / fTmp1;
                    fSin *= fCos;
                }
                fTmp0 = afDiag[i2 + 1] - fTmp2;
                fTmp1 = (afDiag[i2] - fTmp0) * fSin + ((Real)2.0) * fTmp4 * fCos;
                fTmp2 = fSin * fTmp1;
                afDiag[i2 + 1] = fTmp0 + fTmp2;
                fTmp0 = fCos * fTmp1 - fTmp4;

                for (int iRow = 0; iRow < 3; iRow++)
                {
                    fTmp3 = Matrix3<Real>::m_afEntry[Matrix<3, Real>::I(iRow, i2 + 1)];
                    Matrix3<Real>::m_afEntry[Matrix<3, Real>::I(iRow, i2 + 1)] = fSin * Matrix3<Real>::m_afEntry[Matrix<3, Real>::I(iRow, i2)] +
                                                                                 fCos * fTmp3;
                    Matrix3<Real>::m_afEntry[Matrix<3, Real>::I(iRow, i2)] = fCos * Matrix3<Real>::m_afEntry[Matrix<3, Real>::I(iRow, i2)] -
                                                                             fSin * fTmp3;
                }
            }
            afDiag[i0] -= fTmp2;
            afSubDiag[i0] = fTmp0;
            afSubDiag[i1] = (Real)0.0;
        }

        if (iIter == iMaxIter)
        {
            // should not get here under normal circumstances
            assert(false);
            return false;
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::EigenDecomposition(Matrix3 &rkRot, Matrix3 &rkDiag)
    const
{
    // Factor M = R*D*R^T.  The columns of R are the eigenvectors.  The
    // diagonal entries of D are the corresponding eigenvalues.
    Real afDiag[3], afSubDiag[3];
    rkRot = *this;
    rkRot.Tridiagonalize(afDiag, afSubDiag);
    rkRot.QLAlgorithm(afDiag, afSubDiag);

    // The Householder transformation is a reflection.  Make the eigenvectors
    // a right--handed system by changing sign on the last column.
    rkRot[2][0] = -rkRot[2][0];
    rkRot[2][1] = -rkRot[2][1];
    rkRot[2][2] = -rkRot[2][2];

    // (insertion) sort eigenvalues in increasing order, d0 <= d1 <= d2
    int i;
    Real fSave;

    if (afDiag[1] < afDiag[0])
    {
        // swap d0 and d1
        fSave = afDiag[0];
        afDiag[0] = afDiag[1];
        afDiag[1] = fSave;

        // swap V0 and V1
        for (i = 0; i < 3; i++)
        {
            fSave = rkRot[i][0];
            rkRot[i][0] = rkRot[i][1];
            rkRot[i][1] = fSave;
        }
    }

    if (afDiag[2] < afDiag[1])
    {
        // swap d1 and d2
        fSave = afDiag[1];
        afDiag[1] = afDiag[2];
        afDiag[2] = fSave;

        // swap V1 and V2
        for (i = 0; i < 3; i++)
        {
            fSave = rkRot[i][1];
            rkRot[i][1] = rkRot[i][2];
            rkRot[i][2] = fSave;
        }
    }

    if (afDiag[1] < afDiag[0])
    {
        // swap d0 and d1
        fSave = afDiag[0];
        afDiag[0] = afDiag[1];
        afDiag[1] = fSave;

        // swap V0 and V1
        for (i = 0; i < 3; i++)
        {
            fSave = rkRot[i][0];
            rkRot[i][0] = rkRot[i][1];
            rkRot[i][1] = fSave;
        }
    }

    rkDiag.MakeDiagonal(afDiag[0], afDiag[1], afDiag[2]);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::Bidiagonalize(Matrix3 &rkA, Matrix3 &rkL, Matrix3 &rkR)
{
    Real afV[3], afW[3];
    Real fLength, fSign, fT1, fInvT1, fT2;
    bool bIdentity;

    // map first column to (*,0,0)
    fLength = Math<Real>::Sqrt(rkA[0][0] * rkA[0][0] + rkA[1][0] * rkA[1][0] +
                               rkA[2][0] * rkA[2][0]);
    if (fLength > (Real)0.0)
    {
        fSign = (rkA[0][0] > (Real)0.0 ? (Real)1.0 : -(Real)1.0);
        fT1 = rkA[0][0] + fSign * fLength;
        fInvT1 = ((Real)1.0) / fT1;
        afV[1] = rkA[1][0] * fInvT1;
        afV[2] = rkA[2][0] * fInvT1;

        fT2 = -((Real)2.0) / (((Real)1.0) + afV[1] * afV[1] + afV[2] * afV[2]);
        afW[0] = fT2 * (rkA[0][0] + rkA[1][0] * afV[1] + rkA[2][0] * afV[2]);
        afW[1] = fT2 * (rkA[0][1] + rkA[1][1] * afV[1] + rkA[2][1] * afV[2]);
        afW[2] = fT2 * (rkA[0][2] + rkA[1][2] * afV[1] + rkA[2][2] * afV[2]);
        rkA[0][0] += afW[0];
        rkA[0][1] += afW[1];
        rkA[0][2] += afW[2];
        rkA[1][1] += afV[1] * afW[1];
        rkA[1][2] += afV[1] * afW[2];
        rkA[2][1] += afV[2] * afW[1];
        rkA[2][2] += afV[2] * afW[2];

        rkL[0][0] = ((Real)1.0) + fT2;
        rkL[0][1] = fT2 * afV[1];
        rkL[1][0] = rkL[0][1];
        rkL[0][2] = fT2 * afV[2];
        rkL[2][0] = rkL[0][2];
        rkL[1][1] = ((Real)1.0) + fT2 * afV[1] * afV[1];
        rkL[1][2] = fT2 * afV[1] * afV[2];
        rkL[2][1] = rkL[1][2];
        rkL[2][2] = ((Real)1.0) + fT2 * afV[2] * afV[2];
        bIdentity = false;
    }
    else
    {
        rkL = Matrix3::IDENTITY;
        bIdentity = true;
    }

    // map first row to (*,*,0)
    fLength = Math<Real>::Sqrt(rkA[0][1] * rkA[0][1] + rkA[0][2] * rkA[0][2]);
    if (fLength > (Real)0.0)
    {
        fSign = (rkA[0][1] > (Real)0.0 ? (Real)1.0 : -(Real)1.0);
        fT1 = rkA[0][1] + fSign * fLength;
        afV[2] = rkA[0][2] / fT1;

        fT2 = -((Real)2.0) / (((Real)1.0) + afV[2] * afV[2]);
        afW[0] = fT2 * (rkA[0][1] + rkA[0][2] * afV[2]);
        afW[1] = fT2 * (rkA[1][1] + rkA[1][2] * afV[2]);
        afW[2] = fT2 * (rkA[2][1] + rkA[2][2] * afV[2]);
        rkA[0][1] += afW[0];
        rkA[1][1] += afW[1];
        rkA[1][2] += afW[1] * afV[2];
        rkA[2][1] += afW[2];
        rkA[2][2] += afW[2] * afV[2];

        rkR[0][0] = (Real)1.0;
        rkR[0][1] = (Real)0.0;
        rkR[1][0] = (Real)0.0;
        rkR[0][2] = (Real)0.0;
        rkR[2][0] = (Real)0.0;
        rkR[1][1] = ((Real)1.0) + fT2;
        rkR[1][2] = fT2 * afV[2];
        rkR[2][1] = rkR[1][2];
        rkR[2][2] = ((Real)1.0) + fT2 * afV[2] * afV[2];
    }
    else
    {
        rkR = Matrix3::IDENTITY;
    }

    // map second column to (*,*,0)
    fLength = Math<Real>::Sqrt(rkA[1][1] * rkA[1][1] + rkA[2][1] * rkA[2][1]);
    if (fLength > (Real)0.0)
    {
        fSign = (rkA[1][1] > (Real)0.0 ? (Real)1.0 : -(Real)1.0);
        fT1 = rkA[1][1] + fSign * fLength;
        afV[2] = rkA[2][1] / fT1;

        fT2 = -((Real)2.0) / (((Real)1.0) + afV[2] * afV[2]);
        afW[1] = fT2 * (rkA[1][1] + rkA[2][1] * afV[2]);
        afW[2] = fT2 * (rkA[1][2] + rkA[2][2] * afV[2]);
        rkA[1][1] += afW[1];
        rkA[1][2] += afW[2];
        rkA[2][2] += afV[2] * afW[2];

        Real fA = ((Real)1.0) + fT2;
        Real fB = fT2 * afV[2];
        Real fC = ((Real)1.0) + fB * afV[2];

        if (bIdentity)
        {
            rkL[0][0] = (Real)1.0;
            rkL[0][1] = (Real)0.0;
            rkL[1][0] = (Real)0.0;
            rkL[0][2] = (Real)0.0;
            rkL[2][0] = (Real)0.0;
            rkL[1][1] = fA;
            rkL[1][2] = fB;
            rkL[2][1] = fB;
            rkL[2][2] = fC;
        }
        else
        {
            for (int iRow = 0; iRow < 3; iRow++)
            {
                Real fTmp0 = rkL[iRow][1];
                Real fTmp1 = rkL[iRow][2];
                rkL[iRow][1] = fA * fTmp0 + fB * fTmp1;
                rkL[iRow][2] = fB * fTmp0 + fC * fTmp1;
            }
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::GolubKahanStep(Matrix3 &rkA, Matrix3 &rkL, Matrix3 &rkR)
{
    Real fT11 = rkA[0][1] * rkA[0][1] + rkA[1][1] * rkA[1][1];
    Real fT22 = rkA[1][2] * rkA[1][2] + rkA[2][2] * rkA[2][2];
    Real fT12 = rkA[1][1] * rkA[1][2];
    Real fTrace = fT11 + fT22;
    Real fDiff = fT11 - fT22;
    Real fDiscr = Math<Real>::Sqrt(fDiff * fDiff + ((Real)4.0) * fT12 * fT12);
    Real fRoot1 = ((Real)0.5) * (fTrace + fDiscr);
    Real fRoot2 = ((Real)0.5) * (fTrace - fDiscr);

    // adjust right
    Real fY = rkA[0][0] - (Math<Real>::FAbs(fRoot1 - fT22) <=
                                   Math<Real>::FAbs(fRoot2 - fT22)
                               ? fRoot1
                               : fRoot2);
    Real fZ = rkA[0][1];
    Real fInvLength = Math<Real>::InvSqrt(fY * fY + fZ * fZ);
    Real fSin = fZ * fInvLength;
    Real fCos = -fY * fInvLength;

    Real fTmp0 = rkA[0][0];
    Real fTmp1 = rkA[0][1];
    rkA[0][0] = fCos * fTmp0 - fSin * fTmp1;
    rkA[0][1] = fSin * fTmp0 + fCos * fTmp1;
    rkA[1][0] = -fSin * rkA[1][1];
    rkA[1][1] *= fCos;

    int iRow;
    for (iRow = 0; iRow < 3; iRow++)
    {
        fTmp0 = rkR[0][iRow];
        fTmp1 = rkR[1][iRow];
        rkR[0][iRow] = fCos * fTmp0 - fSin * fTmp1;
        rkR[1][iRow] = fSin * fTmp0 + fCos * fTmp1;
    }

    // adjust left
    fY = rkA[0][0];
    fZ = rkA[1][0];
    fInvLength = Math<Real>::InvSqrt(fY * fY + fZ * fZ);
    fSin = fZ * fInvLength;
    fCos = -fY * fInvLength;

    rkA[0][0] = fCos * rkA[0][0] - fSin * rkA[1][0];
    fTmp0 = rkA[0][1];
    fTmp1 = rkA[1][1];
    rkA[0][1] = fCos * fTmp0 - fSin * fTmp1;
    rkA[1][1] = fSin * fTmp0 + fCos * fTmp1;
    rkA[0][2] = -fSin * rkA[1][2];
    rkA[1][2] *= fCos;

    int iCol;
    for (iCol = 0; iCol < 3; iCol++)
    {
        fTmp0 = rkL[iCol][0];
        fTmp1 = rkL[iCol][1];
        rkL[iCol][0] = fCos * fTmp0 - fSin * fTmp1;
        rkL[iCol][1] = fSin * fTmp0 + fCos * fTmp1;
    }

    // adjust right
    fY = rkA[0][1];
    fZ = rkA[0][2];
    fInvLength = Math<Real>::InvSqrt(fY * fY + fZ * fZ);
    fSin = fZ * fInvLength;
    fCos = -fY * fInvLength;

    rkA[0][1] = fCos * rkA[0][1] - fSin * rkA[0][2];
    fTmp0 = rkA[1][1];
    fTmp1 = rkA[1][2];
    rkA[1][1] = fCos * fTmp0 - fSin * fTmp1;
    rkA[1][2] = fSin * fTmp0 + fCos * fTmp1;
    rkA[2][1] = -fSin * rkA[2][2];
    rkA[2][2] *= fCos;

    for (iRow = 0; iRow < 3; iRow++)
    {
        fTmp0 = rkR[1][iRow];
        fTmp1 = rkR[2][iRow];
        rkR[1][iRow] = fCos * fTmp0 - fSin * fTmp1;
        rkR[2][iRow] = fSin * fTmp0 + fCos * fTmp1;
    }

    // adjust left
    fY = rkA[1][1];
    fZ = rkA[2][1];
    fInvLength = Math<Real>::InvSqrt(fY * fY + fZ * fZ);
    fSin = fZ * fInvLength;
    fCos = -fY * fInvLength;

    rkA[1][1] = fCos * rkA[1][1] - fSin * rkA[2][1];
    fTmp0 = rkA[1][2];
    fTmp1 = rkA[2][2];
    rkA[1][2] = fCos * fTmp0 - fSin * fTmp1;
    rkA[2][2] = fSin * fTmp0 + fCos * fTmp1;

    for (iCol = 0; iCol < 3; iCol++)
    {
        fTmp0 = rkL[iCol][1];
        fTmp1 = rkL[iCol][2];
        rkL[iCol][1] = fCos * fTmp0 - fSin * fTmp1;
        rkL[iCol][2] = fSin * fTmp0 + fCos * fTmp1;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::SingularValueDecomposition(Matrix3 &rkL, Matrix3 &rkS,
                                               Matrix3 &rkR) const
{
    int iRow, iCol;

    Matrix3 kA = *this;
    Bidiagonalize(kA, rkL, rkR);
    rkS.MakeZero();

    const int iMax = 32;
    const Real fEpsilon = (Real)1e-04;
    for (int i = 0; i < iMax; i++)
    {
        Real fTmp, fTmp0, fTmp1;
        Real fSin0, fCos0, fTan0;
        Real fSin1, fCos1, fTan1;

        bool bTest1 = (Math<Real>::FAbs(kA[0][1]) <=
                       fEpsilon * (Math<Real>::FAbs(kA[0][0]) +
                                   Math<Real>::FAbs(kA[1][1])));
        bool bTest2 = (Math<Real>::FAbs(kA[1][2]) <=
                       fEpsilon * (Math<Real>::FAbs(kA[1][1]) +
                                   Math<Real>::FAbs(kA[2][2])));
        if (bTest1)
        {
            if (bTest2)
            {
                rkS[0][0] = kA[0][0];
                rkS[1][1] = kA[1][1];
                rkS[2][2] = kA[2][2];
                break;
            }
            else
            {
                // 2x2 closed form factorization
                fTmp = (kA[1][1] * kA[1][1] - kA[2][2] * kA[2][2] +
                        kA[1][2] * kA[1][2]) /
                       (kA[1][2] * kA[2][2]);
                fTan0 = ((Real)0.5) * (fTmp + Math<Real>::Sqrt(fTmp * fTmp +
                                                               ((Real)4.0)));
                fCos0 = Math<Real>::InvSqrt(((Real)1.0) + fTan0 * fTan0);
                fSin0 = fTan0 * fCos0;

                for (iCol = 0; iCol < 3; iCol++)
                {
                    fTmp0 = rkL[iCol][1];
                    fTmp1 = rkL[iCol][2];
                    rkL[iCol][1] = fCos0 * fTmp0 - fSin0 * fTmp1;
                    rkL[iCol][2] = fSin0 * fTmp0 + fCos0 * fTmp1;
                }

                fTan1 = (kA[1][2] - kA[2][2] * fTan0) / kA[1][1];
                fCos1 = Math<Real>::InvSqrt(((Real)1.0) + fTan1 * fTan1);
                fSin1 = -fTan1 * fCos1;

                for (iRow = 0; iRow < 3; iRow++)
                {
                    fTmp0 = rkR[1][iRow];
                    fTmp1 = rkR[2][iRow];
                    rkR[1][iRow] = fCos1 * fTmp0 - fSin1 * fTmp1;
                    rkR[2][iRow] = fSin1 * fTmp0 + fCos1 * fTmp1;
                }

                rkS[0][0] = kA[0][0];
                rkS[1][1] = fCos0 * fCos1 * kA[1][1] -
                            fSin1 * (fCos0 * kA[1][2] - fSin0 * kA[2][2]);
                rkS[2][2] = fSin0 * fSin1 * kA[1][1] +
                            fCos1 * (fSin0 * kA[1][2] + fCos0 * kA[2][2]);
                break;
            }
        }
        else
        {
            if (bTest2)
            {
                // 2x2 closed form factorization
                fTmp = (kA[0][0] * kA[0][0] + kA[1][1] * kA[1][1] -
                        kA[0][1] * kA[0][1]) /
                       (kA[0][1] * kA[1][1]);
                fTan0 = ((Real)0.5) * (-fTmp + Math<Real>::Sqrt(fTmp * fTmp +
                                                                ((Real)4.0)));
                fCos0 = Math<Real>::InvSqrt(((Real)1.0) + fTan0 * fTan0);
                fSin0 = fTan0 * fCos0;

                for (iCol = 0; iCol < 3; iCol++)
                {
                    fTmp0 = rkL[iCol][0];
                    fTmp1 = rkL[iCol][1];
                    rkL[iCol][0] = fCos0 * fTmp0 - fSin0 * fTmp1;
                    rkL[iCol][1] = fSin0 * fTmp0 + fCos0 * fTmp1;
                }

                fTan1 = (kA[0][1] - kA[1][1] * fTan0) / kA[0][0];
                fCos1 = Math<Real>::InvSqrt(((Real)1.0) + fTan1 * fTan1);
                fSin1 = -fTan1 * fCos1;

                for (iRow = 0; iRow < 3; iRow++)
                {
                    fTmp0 = rkR[0][iRow];
                    fTmp1 = rkR[1][iRow];
                    rkR[0][iRow] = fCos1 * fTmp0 - fSin1 * fTmp1;
                    rkR[1][iRow] = fSin1 * fTmp0 + fCos1 * fTmp1;
                }

                rkS[0][0] = fCos0 * fCos1 * kA[0][0] -
                            fSin1 * (fCos0 * kA[0][1] - fSin0 * kA[1][1]);
                rkS[1][1] = fSin0 * fSin1 * kA[0][0] +
                            fCos1 * (fSin0 * kA[0][1] + fCos0 * kA[1][1]);
                rkS[2][2] = kA[2][2];
                break;
            }
            else
            {
                GolubKahanStep(kA, rkL, rkR);
            }
        }
    }

    // positize diagonal
    for (iRow = 0; iRow < 3; iRow++)
    {
        if (rkS[iRow][iRow] < (Real)0.0)
        {
            rkS[iRow][iRow] = -rkS[iRow][iRow];
            for (iCol = 0; iCol < 3; iCol++)
                rkR[iRow][iCol] = -rkR[iRow][iCol];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::SingularValueComposition(const Matrix3 &rkL,
                                             const Matrix3 &rkS, const Matrix3 &rkR)
{
    *this = rkL * (rkS * rkR);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix3<Real>::QDUDecomposition(Matrix3 &rkQ, Matrix3 &rkD,
                                     Matrix3 &rkU) const
{
    // Factor M = QR = QDU where Q is orthogonal (rotation), D is diagonal
    // (scaling),  and U is upper triangular with ones on its diagonal
    // (shear).  Algorithm uses Gram-Schmidt orthogonalization (the QR
    // algorithm).
    //
    // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.  The matrix R has entries
    //
    //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
    //   r10 = 0      r11 = q1*m1  r12 = q1*m2
    //   r20 = 0      r21 = 0      r22 = q2*m2
    //
    // so D = diag(r00,r11,r22) and U has entries u01 = r01/r00,
    // u02 = r02/r00, and u12 = r12/r11.

    // build orthogonal matrix Q
    Real fInvLength = Math<Real>::InvSqrt(Matrix3<Real>::m_afEntry[0] * Matrix3<Real>::m_afEntry[0] +
                                          Matrix3<Real>::m_afEntry[3] * Matrix3<Real>::m_afEntry[3] + Matrix3<Real>::m_afEntry[6] * Matrix3<Real>::m_afEntry[6]);
    rkQ[0][0] = Matrix3<Real>::m_afEntry[0] * fInvLength;
    rkQ[1][0] = Matrix3<Real>::m_afEntry[3] * fInvLength;
    rkQ[2][0] = Matrix3<Real>::m_afEntry[6] * fInvLength;

    Real fDot = rkQ[0][0] * Matrix3<Real>::m_afEntry[1] + rkQ[1][0] * Matrix3<Real>::m_afEntry[4] +
                rkQ[2][0] * Matrix3<Real>::m_afEntry[7];
    rkQ[0][1] = Matrix3<Real>::m_afEntry[1] - fDot * rkQ[0][0];
    rkQ[1][1] = Matrix3<Real>::m_afEntry[4] - fDot * rkQ[1][0];
    rkQ[2][1] = Matrix3<Real>::m_afEntry[7] - fDot * rkQ[2][0];
    fInvLength = Math<Real>::InvSqrt(rkQ[0][1] * rkQ[0][1] +
                                     rkQ[1][1] * rkQ[1][1] + rkQ[2][1] * rkQ[2][1]);
    rkQ[0][1] *= fInvLength;
    rkQ[1][1] *= fInvLength;
    rkQ[2][1] *= fInvLength;

    fDot = rkQ[0][0] * Matrix3<Real>::m_afEntry[2] + rkQ[1][0] * Matrix3<Real>::m_afEntry[5] +
           rkQ[2][0] * Matrix3<Real>::m_afEntry[8];
    rkQ[0][2] = Matrix3<Real>::m_afEntry[2] - fDot * rkQ[0][0];
    rkQ[1][2] = Matrix3<Real>::m_afEntry[5] - fDot * rkQ[1][0];
    rkQ[2][2] = Matrix3<Real>::m_afEntry[8] - fDot * rkQ[2][0];
    fDot = rkQ[0][1] * Matrix3<Real>::m_afEntry[2] + rkQ[1][1] * Matrix3<Real>::m_afEntry[5] +
           rkQ[2][1] * Matrix3<Real>::m_afEntry[8];
    rkQ[0][2] -= fDot * rkQ[0][1];
    rkQ[1][2] -= fDot * rkQ[1][1];
    rkQ[2][2] -= fDot * rkQ[2][1];
    fInvLength = Math<Real>::InvSqrt(rkQ[0][2] * rkQ[0][2] +
                                     rkQ[1][2] * rkQ[1][2] + rkQ[2][2] * rkQ[2][2]);
    rkQ[0][2] *= fInvLength;
    rkQ[1][2] *= fInvLength;
    rkQ[2][2] *= fInvLength;

    // guarantee that orthogonal matrix has determinant 1 (no reflections)
    Real fDet = rkQ[0][0] * rkQ[1][1] * rkQ[2][2] + rkQ[0][1] * rkQ[1][2] * rkQ[2][0] + rkQ[0][2] * rkQ[1][0] * rkQ[2][1] - rkQ[0][2] * rkQ[1][1] * rkQ[2][0] - rkQ[0][1] * rkQ[1][0] * rkQ[2][2] - rkQ[0][0] * rkQ[1][2] * rkQ[2][1];

    if (fDet < (Real)0.0)
    {
        for (int iRow = 0; iRow < 3; iRow++)
        {
            for (int iCol = 0; iCol < 3; iCol++)
                rkQ[iRow][iCol] = -rkQ[iRow][iCol];
        }
    }

    // build "right" matrix R
    Matrix3 kR;
    kR[0][0] = rkQ[0][0] * Matrix3<Real>::m_afEntry[0] + rkQ[1][0] * Matrix3<Real>::m_afEntry[3] +
               rkQ[2][0] * Matrix3<Real>::m_afEntry[6];
    kR[0][1] = rkQ[0][0] * Matrix3<Real>::m_afEntry[1] + rkQ[1][0] * Matrix3<Real>::m_afEntry[4] +
               rkQ[2][0] * Matrix3<Real>::m_afEntry[7];
    kR[1][1] = rkQ[0][1] * Matrix3<Real>::m_afEntry[1] + rkQ[1][1] * Matrix3<Real>::m_afEntry[4] +
               rkQ[2][1] * Matrix3<Real>::m_afEntry[7];
    kR[0][2] = rkQ[0][0] * Matrix3<Real>::m_afEntry[2] + rkQ[1][0] * Matrix3<Real>::m_afEntry[5] +
               rkQ[2][0] * Matrix3<Real>::m_afEntry[8];
    kR[1][2] = rkQ[0][1] * Matrix3<Real>::m_afEntry[2] + rkQ[1][1] * Matrix3<Real>::m_afEntry[5] +
               rkQ[2][1] * Matrix3<Real>::m_afEntry[8];
    kR[2][2] = rkQ[0][2] * Matrix3<Real>::m_afEntry[2] + rkQ[1][2] * Matrix3<Real>::m_afEntry[5] +
               rkQ[2][2] * Matrix3<Real>::m_afEntry[8];

    // the scaling component
    rkD.MakeDiagonal(kR[0][0], kR[1][1], kR[2][2]);

    // the shear component
    Real fInvD0 = ((Real)1.0) / rkD[0][0];
    rkU[0][0] = (Real)1.0;
    rkU[0][1] = kR[0][1] * fInvD0;
    rkU[0][2] = kR[0][2] * fInvD0;
    rkU[1][0] = (Real)0.0;
    rkU[1][1] = (Real)1.0;
    rkU[1][2] = kR[1][2] / rkD[1][1];
    rkU[2][0] = (Real)0.0;
    rkU[2][1] = (Real)0.0;
    rkU[2][2] = (Real)1.0;
}
//----------------------------------------------------------------------------
