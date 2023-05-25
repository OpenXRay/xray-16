// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf
#pragma hdrstop
#include "MgcQuaternion.h"
using namespace Mgc;

static Real gs_fEpsilon = 1e-03f;
Quaternion Quaternion::ZERO(0.0f, 0.0f, 0.0f, 0.0f);
Quaternion Quaternion::IDENTITY(1.0f, 0.0f, 0.0f, 0.0f);

//----------------------------------------------------------------------------
Quaternion::Quaternion(Real fW, Real fX, Real fY, Real fZ)
{
    w = fW;
    x = fX;
    y = fY;
    z = fZ;
}
//----------------------------------------------------------------------------
Quaternion::Quaternion(const Quaternion &rkQ)
{
    w = rkQ.w;
    x = rkQ.x;
    y = rkQ.y;
    z = rkQ.z;
}
//----------------------------------------------------------------------------
void Quaternion::FromRotationMatrix(const Matrix3 &kRot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    Real fTrace = kRot[0][0] + kRot[1][1] + kRot[2][2];
    Real fRoot;

    if (fTrace > 0.0f)
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = Math::Sqrt(fTrace + 1.0f); // 2w
        w = 0.5f * fRoot;
        fRoot = 0.5f / fRoot; // 1/(4w)
        x = (kRot[2][1] - kRot[1][2]) * fRoot;
        y = (kRot[0][2] - kRot[2][0]) * fRoot;
        z = (kRot[1][0] - kRot[0][1]) * fRoot;
    }
    else
    {
        // |w| <= 1/2
        static int s_iNext[3] = {1, 2, 0};
        int i = 0;
        if (kRot[1][1] > kRot[0][0])
            i = 1;
        if (kRot[2][2] > kRot[i][i])
            i = 2;
        int j = s_iNext[i];
        int k = s_iNext[j];

        fRoot = Math::Sqrt(kRot[i][i] - kRot[j][j] - kRot[k][k] + 1.0f);
        Real *apkQuat[3] = {&x, &y, &z};
        *apkQuat[i] = 0.5f * fRoot;
        fRoot = 0.5f / fRoot;
        w = (kRot[k][j] - kRot[j][k]) * fRoot;
        *apkQuat[j] = (kRot[j][i] + kRot[i][j]) * fRoot;
        *apkQuat[k] = (kRot[k][i] + kRot[i][k]) * fRoot;
    }
}
//----------------------------------------------------------------------------
void Quaternion::ToRotationMatrix(Matrix3 &kRot) const
{
    Real fTx = 2.0f * x;
    Real fTy = 2.0f * y;
    Real fTz = 2.0f * z;
    Real fTwx = fTx * w;
    Real fTwy = fTy * w;
    Real fTwz = fTz * w;
    Real fTxx = fTx * x;
    Real fTxy = fTy * x;
    Real fTxz = fTz * x;
    Real fTyy = fTy * y;
    Real fTyz = fTz * y;
    Real fTzz = fTz * z;

    kRot[0][0] = 1.0f - (fTyy + fTzz);
    kRot[0][1] = fTxy - fTwz;
    kRot[0][2] = fTxz + fTwy;
    kRot[1][0] = fTxy + fTwz;
    kRot[1][1] = 1.0f - (fTxx + fTzz);
    kRot[1][2] = fTyz - fTwx;
    kRot[2][0] = fTxz - fTwy;
    kRot[2][1] = fTyz + fTwx;
    kRot[2][2] = 1.0f - (fTxx + fTyy);
}
//----------------------------------------------------------------------------
void Quaternion::FromAngleAxis(const Real &rfAngle, const Vector3 &rkAxis)
{
    // assert:  axis[] is unit length
    //
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fHalfAngle = 0.5f * rfAngle;
    Real fSin = Math::Sin(fHalfAngle);
    w = Math::Cos(fHalfAngle);
    x = fSin * rkAxis.x;
    y = fSin * rkAxis.y;
    z = fSin * rkAxis.z;
}
//----------------------------------------------------------------------------
void Quaternion::ToAngleAxis(Real &rfAngle, Vector3 &rkAxis) const
{
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fSqrLength = x * x + y * y + z * z;
    if (fSqrLength > 0.0f)
    {
        rfAngle = 2.0f * Math::ACos(w);
        Real fInvLength = Math::InvSqrt(fSqrLength);
        rkAxis.x = x * fInvLength;
        rkAxis.y = y * fInvLength;
        rkAxis.z = z * fInvLength;
    }
    else
    {
        // angle is 0 (mod 2*pi), so any axis will do
        rfAngle = 0.0f;
        rkAxis.x = 1.0f;
        rkAxis.y = 0.0f;
        rkAxis.z = 0.0f;
    }
}
//----------------------------------------------------------------------------
void Quaternion::FromAxes(const Vector3 *akAxis)
{
    Matrix3 kRot;

    for (int iCol = 0; iCol < 3; iCol++)
    {
        kRot[0][iCol] = akAxis[iCol].x;
        kRot[1][iCol] = akAxis[iCol].y;
        kRot[2][iCol] = akAxis[iCol].z;
    }

    FromRotationMatrix(kRot);
}
//----------------------------------------------------------------------------
void Quaternion::ToAxes(Vector3 *akAxis) const
{
    Matrix3 kRot;

    ToRotationMatrix(kRot);

    for (int iCol = 0; iCol < 3; iCol++)
    {
        akAxis[iCol].x = kRot[0][iCol];
        akAxis[iCol].y = kRot[1][iCol];
        akAxis[iCol].z = kRot[2][iCol];
    }
}
//----------------------------------------------------------------------------
Quaternion &Quaternion::operator=(const Quaternion &rkQ)
{
    w = rkQ.w;
    x = rkQ.x;
    y = rkQ.y;
    z = rkQ.z;
    return *this;
}
//----------------------------------------------------------------------------
Quaternion Quaternion::operator+(const Quaternion &rkQ) const
{
    return Quaternion(w + rkQ.w, x + rkQ.x, y + rkQ.y, z + rkQ.z);
}
//----------------------------------------------------------------------------
Quaternion Quaternion::operator-(const Quaternion &rkQ) const
{
    return Quaternion(w - rkQ.w, x - rkQ.x, y - rkQ.y, z - rkQ.z);
}
//----------------------------------------------------------------------------
Quaternion Quaternion::operator*(const Quaternion &rkQ) const
{
    // NOTE:  Multiplication is not generally commutative, so in most
    // cases p*q != q*p.

    return Quaternion(
        w * rkQ.w - x * rkQ.x - y * rkQ.y - z * rkQ.z,
        w * rkQ.x + x * rkQ.w + y * rkQ.z - z * rkQ.y,
        w * rkQ.y + y * rkQ.w + z * rkQ.x - x * rkQ.z,
        w * rkQ.z + z * rkQ.w + x * rkQ.y - y * rkQ.x);
}
//----------------------------------------------------------------------------
Quaternion Quaternion::operator*(Real fScalar) const
{
    return Quaternion(fScalar * w, fScalar * x, fScalar * y, fScalar * z);
}
//----------------------------------------------------------------------------
Quaternion Mgc::operator*(Real fScalar, const Quaternion &rkQ)
{
    return Quaternion(fScalar * rkQ.w, fScalar * rkQ.x, fScalar * rkQ.y,
                      fScalar * rkQ.z);
}
//----------------------------------------------------------------------------
Quaternion Quaternion::operator-() const
{
    return Quaternion(-w, -x, -y, -z);
}
//----------------------------------------------------------------------------
Real Quaternion::Dot(const Quaternion &rkQ) const
{
    return w * rkQ.w + x * rkQ.x + y * rkQ.y + z * rkQ.z;
}
//----------------------------------------------------------------------------
Real Quaternion::Norm() const
{
    return w * w + x * x + y * y + z * z;
}
//----------------------------------------------------------------------------
Quaternion Quaternion::Inverse() const
{
    Real fNorm = w * w + x * x + y * y + z * z;
    if (fNorm > 0.0f)
    {
        Real fInvNorm = 1.0f / fNorm;
        return Quaternion(w * fInvNorm, -x * fInvNorm, -y * fInvNorm, -z * fInvNorm);
    }
    else
    {
        // return an invalid result to flag the error
        return ZERO;
    }
}
//----------------------------------------------------------------------------
Quaternion Quaternion::UnitInverse() const
{
    // assert:  'this' is unit length
    return Quaternion(w, -x, -y, -z);
}
//----------------------------------------------------------------------------
Quaternion Quaternion::Exp() const
{
    // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
    // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

    Real fAngle = Math::Sqrt(x * x + y * y + z * z);
    Real fSin = Math::Sin(fAngle);

    Quaternion kResult;
    kResult.w = Math::Cos(fAngle);

    if (Math::FAbs(fSin) >= gs_fEpsilon)
    {
        Real fCoeff = fSin / fAngle;
        kResult.x = fCoeff * x;
        kResult.y = fCoeff * y;
        kResult.z = fCoeff * z;
    }
    else
    {
        kResult.x = x;
        kResult.y = y;
        kResult.z = z;
    }

    return kResult;
}
//----------------------------------------------------------------------------
Quaternion Quaternion::Log() const
{
    // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // log(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use log(q) =
    // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

    Quaternion kResult;
    kResult.w = 0.0f;

    if (Math::FAbs(w) < 1.0f)
    {
        Real fAngle = Math::ACos(w);
        Real fSin = Math::Sin(fAngle);
        if (Math::FAbs(fSin) >= gs_fEpsilon)
        {
            Real fCoeff = fAngle / fSin;
            kResult.x = fCoeff * x;
            kResult.y = fCoeff * y;
            kResult.z = fCoeff * z;
            return kResult;
        }
    }

    kResult.x = x;
    kResult.y = y;
    kResult.z = z;

    return kResult;
}
//----------------------------------------------------------------------------
Vector3 Quaternion::operator*(const Vector3 &rkVector) const
{
    // Given a vector u = (x0,y0,z0) and a unit length quaternion
    // q = <w,x,y,z>, the vector v = (x1,y1,z1) which represents the
    // rotation of u by q is v = q*u*q^{-1} where * indicates quaternion
    // multiplication and where u is treated as the quaternion <0,x0,y0,z0>.
    // Note that q^{-1} = <w,-x,-y,-z>, so no real work is required to
    // invert q.  Now
    //
    //   q*u*q^{-1} = q*<0,x0,y0,z0>*q^{-1}
    //     = q*(x0*i+y0*j+z0*k)*q^{-1}
    //     = x0*(q*i*q^{-1})+y0*(q*j*q^{-1})+z0*(q*k*q^{-1})
    //
    // As 3-vectors, q*i*q^{-1}, q*j*q^{-1}, and 2*k*q^{-1} are the columns
    // of the rotation matrix computed in Quaternion::ToRotationMatrix.
    // The vector v is obtained as the product of that rotation matrix with
    // vector u.  As such, the quaternion representation of a rotation
    // matrix requires less space than the matrix and more time to compute
    // the rotated vector.  Typical space-time tradeoff...

    Matrix3 kRot;
    ToRotationMatrix(kRot);
    return kRot * rkVector;
}
//----------------------------------------------------------------------------
Quaternion Quaternion::Slerp(Real fT, const Quaternion &rkP,
                             const Quaternion &rkQ)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math::ACos(fCos);

    if (Math::FAbs(fAngle) < gs_fEpsilon)
        return rkP;

    Real fSin = Math::Sin(fAngle);
    Real fInvSin = 1.0f / fSin;
    Real fCoeff0 = Math::Sin((1.0f - fT) * fAngle) * fInvSin;
    Real fCoeff1 = Math::Sin(fT * fAngle) * fInvSin;
    return fCoeff0 * rkP + fCoeff1 * rkQ;
}
//----------------------------------------------------------------------------
Quaternion Quaternion::SlerpExtraSpins(Real fT,
                                       const Quaternion &rkP, const Quaternion &rkQ, int iExtraSpins)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math::ACos(fCos);

    if (Math::FAbs(fAngle) < gs_fEpsilon)
        return rkP;

    Real fSin = Math::Sin(fAngle);
    Real fPhase = Math::_PI * iExtraSpins * fT;
    Real fInvSin = 1.0f / fSin;
    Real fCoeff0 = Math::Sin((1.0f - fT) * fAngle - fPhase) * fInvSin;
    Real fCoeff1 = Math::Sin(fT * fAngle + fPhase) * fInvSin;
    return fCoeff0 * rkP + fCoeff1 * rkQ;
}
//----------------------------------------------------------------------------
void Quaternion::Intermediate(const Quaternion &rkQ0,
                              const Quaternion &rkQ1, const Quaternion &rkQ2, Quaternion &rkA,
                              Quaternion &rkB)
{
    // assert:  q0, q1, q2 are unit quaternions

    Quaternion kQ0inv = rkQ0.UnitInverse();
    Quaternion kQ1inv = rkQ1.UnitInverse();
    Quaternion rkP0 = kQ0inv * rkQ1;
    Quaternion rkP1 = kQ1inv * rkQ2;
    Quaternion kArg = 0.25f * (rkP0.Log() - rkP1.Log());
    Quaternion kMinusArg = -kArg;

    rkA = rkQ1 * kArg.Exp();
    rkB = rkQ1 * kMinusArg.Exp();
}
//----------------------------------------------------------------------------
Quaternion Quaternion::Squad(Real fT, const Quaternion &rkP,
                             const Quaternion &rkA, const Quaternion &rkB, const Quaternion &rkQ)
{
    Real fSlerpT = 2.0f * fT * (1.0f - fT);
    Quaternion kSlerpP = Slerp(fT, rkP, rkQ);
    Quaternion kSlerpQ = Slerp(fT, rkA, rkB);
    return Slerp(fSlerpT, kSlerpP, kSlerpQ);
}
//----------------------------------------------------------------------------
