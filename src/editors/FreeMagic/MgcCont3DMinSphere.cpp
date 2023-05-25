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
//#include "stdafx.h"
#pragma hdrstop

#include "MgcCont3DMinSphere.h"
using namespace Mgc;

// error checking
static const Real gs_fEpsilon = 1e-03f;
static const Real gs_fOnePlusEpsilon = 1.0f + gs_fEpsilon;

// indices of points that support current minimum volume sphere
class Support
{
public:
    int m_iQuantity;
    int m_aiIndex[4];

    bool Contains(int iIndex, Vector3 **apkPoint)
    {
        for (int i = 0; i < m_iQuantity; i++)
        {
            Vector3 kDiff = *apkPoint[iIndex] - *apkPoint[m_aiIndex[i]];
            if (kDiff.SquaredLength() < gs_fEpsilon)
                return true;
        }
        return false;
    }
};

// All internal minimal sphere calculations store the squared radius in the
// radius member of Sphere.  Only at the end is a sqrt computed.

//----------------------------------------------------------------------------
static bool PointInsideSphere(const Vector3 &rkP, const Sphere &rkS)
{
    Vector3 kDiff = rkP - rkS.Center();
    Real fTest = kDiff.SquaredLength();
    return fTest <= gs_fOnePlusEpsilon * rkS.Radius(); // theory:  test <= R^2
}
//----------------------------------------------------------------------------
static Sphere ExactSphere1(const Vector3 &rkP)
{
    Sphere kMinimal;
    kMinimal.Center() = rkP;
    kMinimal.Radius() = 0.0f;
    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere ExactSphere2(const Vector3 &rkP0, const Vector3 &rkP1)
{
    Sphere kMinimal;
    kMinimal.Center() = 0.5f * (rkP0 + rkP1);
    Vector3 kDiff = rkP1 - rkP0;
    kMinimal.Radius() = 0.25f * kDiff.SquaredLength();
    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere ExactSphere3(const Vector3 &rkP0, const Vector3 &rkP1,
                           const Vector3 &rkP2)
{
    // Compute the circle (in 3D) containing p0, p1, and p2.  The Center() in
    // barycentric coordinates is K = u0*p0+u1*p1+u2*p2 where u0+u1+u2=1.
    // The Center() is equidistant from the three points, so |K-p0| = |K-p1| =
    // |K-p2| = R where R is the radius of the circle.
    //
    // From these conditions,
    //   K-p0 = u0*A + u1*B - A
    //   K-p1 = u0*A + u1*B - B
    //   K-p2 = u0*A + u1*B
    // where A = p0-p2 and B = p1-p2, which leads to
    //   r^2 = |u0*A+u1*B|^2 - 2*Dot(A,u0*A+u1*B) + |A|^2
    //   r^2 = |u0*A+u1*B|^2 - 2*Dot(B,u0*A+u1*B) + |B|^2
    //   r^2 = |u0*A+u1*B|^2
    // Subtracting the last equation from the first two and writing
    // the equations as a linear system,
    //
    // +-                 -++   -+       +-        -+
    // | Dot(A,A) Dot(A,B) || u0 | = 0.5 | Dot(A,A) |
    // | Dot(B,A) Dot(B,B) || u1 |       | Dot(B,B) |
    // +-                 -++   -+       +-        -+
    //
    // The following code solves this system for u0 and u1, then
    // evaluates the third equation in r^2 to obtain r.

    Vector3 kA = rkP0 - rkP2;
    Vector3 kB = rkP1 - rkP2;
    Real fAdA = kA.Dot(kA);
    Real fAdB = kA.Dot(kB);
    Real fBdB = kB.Dot(kB);
    Real fDet = fAdA * fBdB - fAdB * fAdB;

    Sphere kMinimal;

    if (::Math::FAbs(fDet) > gs_fEpsilon)
    {
        Real fHalfInvDet = 0.5f / fDet;
        Real fU0 = fHalfInvDet * fBdB * (fAdA - fAdB);
        Real fU1 = fHalfInvDet * fAdA * (fBdB - fAdB);
        Real fU2 = 1.0f - fU0 - fU1;
        kMinimal.Center() = fU0 * rkP0 + fU1 * rkP1 + fU2 * rkP2;
        Vector3 kTmp = fU0 * kA + fU1 * kB;
        kMinimal.Radius() = kTmp.SquaredLength();
    }
    else
    {
        kMinimal.Center() = Vector3::ZERO;
        kMinimal.Radius() = ::Math::MAX_REAL;
    }

    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere ExactSphere4(const Vector3 &rkP0, const Vector3 &rkP1,
                           const Vector3 &rkP2, const Vector3 &rkP3)
{
    // Compute the sphere containing p0, p1, p2, and p3.  The Center() in
    // barycentric coordinates is K = u0*p0+u1*p1+u2*p2+u3*p3 where
    // u0+u1+u2+u3=1.  The Center() is equidistant from the three points, so
    // |K-p0| = |K-p1| = |K-p2| = |K-p3| = R where R is the radius of the
    // sphere.
    //
    // From these conditions,
    //   K-p0 = u0*A + u1*B + u2*C - A
    //   K-p1 = u0*A + u1*B + u2*C - B
    //   K-p2 = u0*A + u1*B + u2*C - C
    //   K-p3 = u0*A + u1*B + u2*C
    // where A = p0-p3, B = p1-p3, and C = p2-p3 which leads to
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(A,u0*A+u1*B+u2*C) + |A|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(B,u0*A+u1*B+u2*C) + |B|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(C,u0*A+u1*B+u2*C) + |C|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2
    // Subtracting the last equation from the first three and writing
    // the equations as a linear system,
    //
    // +-                          -++   -+       +-        -+
    // | Dot(A,A) Dot(A,B) Dot(A,C) || u0 | = 0.5 | Dot(A,A) |
    // | Dot(B,A) Dot(B,B) Dot(B,C) || u1 |       | Dot(B,B) |
    // | Dot(C,A) Dot(C,B) Dot(C,C) || u2 |       | Dot(C,C) |
    // +-                          -++   -+       +-        -+
    //
    // The following code solves this system for u0, u1, and u2, then
    // evaluates the fourth equation in r^2 to obtain r.

    Vector3 kE10 = rkP0 - rkP3;
    Vector3 kE20 = rkP1 - rkP3;
    Vector3 kE30 = rkP2 - rkP3;

    Real aafA[3][3];
    aafA[0][0] = kE10.Dot(kE10);
    aafA[0][1] = kE10.Dot(kE20);
    aafA[0][2] = kE10.Dot(kE30);
    aafA[1][0] = aafA[0][1];
    aafA[1][1] = kE20.Dot(kE20);
    aafA[1][2] = kE20.Dot(kE30);
    aafA[2][0] = aafA[0][2];
    aafA[2][1] = aafA[1][2];
    aafA[2][2] = kE30.Dot(kE30);

    Real afB[3];
    afB[0] = 0.5f * aafA[0][0];
    afB[1] = 0.5f * aafA[1][1];
    afB[2] = 0.5f * aafA[2][2];

    Real aafAInv[3][3];
    aafAInv[0][0] = aafA[1][1] * aafA[2][2] - aafA[1][2] * aafA[2][1];
    aafAInv[0][1] = aafA[0][2] * aafA[2][1] - aafA[0][1] * aafA[2][2];
    aafAInv[0][2] = aafA[0][1] * aafA[1][2] - aafA[0][2] * aafA[1][1];
    aafAInv[1][0] = aafA[1][2] * aafA[2][0] - aafA[1][0] * aafA[2][2];
    aafAInv[1][1] = aafA[0][0] * aafA[2][2] - aafA[0][2] * aafA[2][0];
    aafAInv[1][2] = aafA[0][2] * aafA[1][0] - aafA[0][0] * aafA[1][2];
    aafAInv[2][0] = aafA[1][0] * aafA[2][1] - aafA[1][1] * aafA[2][0];
    aafAInv[2][1] = aafA[0][1] * aafA[2][0] - aafA[0][0] * aafA[2][1];
    aafAInv[2][2] = aafA[0][0] * aafA[1][1] - aafA[0][1] * aafA[1][0];
    Real fDet = aafA[0][0] * aafAInv[0][0] + aafA[0][1] * aafAInv[1][0] +
                aafA[0][2] * aafAInv[2][0];

    Sphere kMinimal;

    if (::Math::FAbs(fDet) > gs_fEpsilon)
    {
        Real fInvDet = 1.0f / fDet;
        int iRow, iCol;
        for (iRow = 0; iRow < 3; iRow++)
        {
            for (iCol = 0; iCol < 3; iCol++)
                aafAInv[iRow][iCol] *= fInvDet;
        }

        Real afU[4];
        for (iRow = 0; iRow < 3; iRow++)
        {
            afU[iRow] = 0.0f;
            for (iCol = 0; iCol < 3; iCol++)
                afU[iRow] += aafAInv[iRow][iCol] * afB[iCol];
        }
        afU[3] = 1.0f - afU[0] - afU[1] - afU[2];

        kMinimal.Center() = afU[0] * rkP0 + afU[1] * rkP1 + afU[2] * rkP2 +
                            afU[3] * rkP3;
        Vector3 kTmp = afU[0] * kE10 + afU[1] * kE20 + afU[2] * kE30;
        kMinimal.Radius() = kTmp.SquaredLength();
    }
    else
    {
        kMinimal.Center() = Vector3::ZERO;
        kMinimal.Radius() = ::Math::MAX_REAL;
    }

    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere UpdateSupport1(int i, Vector3 **apkPerm, Support &rkSupp)
{
    const Vector3 &rkP0 = *apkPerm[rkSupp.m_aiIndex[0]];
    const Vector3 &rkP1 = *apkPerm[i];

    Sphere kMinimal = ExactSphere2(rkP0, rkP1);
    rkSupp.m_iQuantity = 2;
    rkSupp.m_aiIndex[1] = i;

    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere UpdateSupport2(int i, Vector3 **apkPerm, Support &rkSupp)
{
    const Vector3 &rkP0 = *apkPerm[rkSupp.m_aiIndex[0]];
    const Vector3 &rkP1 = *apkPerm[rkSupp.m_aiIndex[1]];
    const Vector3 &rkP2 = *apkPerm[i];

    Sphere akS[3];
    Real fMinRSqr = ::Math::MAX_REAL;
    int iIndex = -1;

    akS[0] = ExactSphere2(rkP0, rkP2);
    if (PointInsideSphere(rkP1, akS[0]))
    {
        fMinRSqr = akS[0].Radius();
        iIndex = 0;
    }

    akS[1] = ExactSphere2(rkP1, rkP2);
    if (akS[1].Radius() < fMinRSqr)
    {
        if (PointInsideSphere(rkP0, akS[1]))
        {
            fMinRSqr = akS[1].Radius();
            iIndex = 1;
        }
    }

    Sphere kMinimal;

    if (iIndex != -1)
    {
        kMinimal = akS[iIndex];
        rkSupp.m_aiIndex[1 - iIndex] = i;
    }
    else
    {
        kMinimal = ExactSphere3(rkP0, rkP1, rkP2);
        assert(kMinimal.Radius() <= fMinRSqr);
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[2] = i;
    }

    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere UpdateSupport3(int i, Vector3 **apkPerm, Support &rkSupp)
{
    const Vector3 &rkP0 = *apkPerm[rkSupp.m_aiIndex[0]];
    const Vector3 &rkP1 = *apkPerm[rkSupp.m_aiIndex[1]];
    const Vector3 &rkP2 = *apkPerm[rkSupp.m_aiIndex[2]];
    const Vector3 &rkP3 = *apkPerm[i];

    Sphere akS[6];
    Real fMinRSqr = ::Math::MAX_REAL;
    int iIndex = -1;

    akS[0] = ExactSphere2(rkP0, rkP3);
    if (PointInsideSphere(rkP1, akS[0]) && PointInsideSphere(rkP2, akS[0]))
    {
        fMinRSqr = akS[0].Radius();
        iIndex = 0;
    }

    akS[1] = ExactSphere2(rkP1, rkP3);
    if (akS[1].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[1]) && PointInsideSphere(rkP2, akS[1]))
    {
        fMinRSqr = akS[1].Radius();
        iIndex = 1;
    }

    akS[2] = ExactSphere2(rkP2, rkP3);
    if (akS[2].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[2]) && PointInsideSphere(rkP1, akS[2]))
    {
        fMinRSqr = akS[2].Radius();
        iIndex = 2;
    }

    akS[3] = ExactSphere3(rkP0, rkP1, rkP3);
    if (akS[3].Radius() < fMinRSqr && PointInsideSphere(rkP2, akS[3]))
    {
        fMinRSqr = akS[3].Radius();
        iIndex = 3;
    }

    akS[4] = ExactSphere3(rkP0, rkP2, rkP3);
    if (akS[4].Radius() < fMinRSqr && PointInsideSphere(rkP1, akS[4]))
    {
        fMinRSqr = akS[4].Radius();
        iIndex = 4;
    }

    akS[5] = ExactSphere3(rkP1, rkP2, rkP3);
    if (akS[5].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[5]))
    {
        fMinRSqr = akS[5].Radius();
        iIndex = 5;
    }

    Sphere kMinimal;

    switch (iIndex)
    {
    case 0:
        kMinimal = akS[0];
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[1] = i;
        break;
    case 1:
        kMinimal = akS[1];
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[0] = i;
        break;
    case 2:
        kMinimal = akS[2];
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[0] = rkSupp.m_aiIndex[2];
        rkSupp.m_aiIndex[1] = i;
        break;
    case 3:
        kMinimal = akS[3];
        rkSupp.m_aiIndex[2] = i;
        break;
    case 4:
        kMinimal = akS[4];
        rkSupp.m_aiIndex[1] = i;
        break;
    case 5:
        kMinimal = akS[5];
        rkSupp.m_aiIndex[0] = i;
        break;
    default:
        kMinimal = ExactSphere4(rkP0, rkP1, rkP2, rkP3);
        assert(kMinimal.Radius() <= fMinRSqr);
        rkSupp.m_iQuantity = 4;
        rkSupp.m_aiIndex[3] = i;
        break;
    }

    return kMinimal;
}
//----------------------------------------------------------------------------
static Sphere UpdateSupport4(int i, Vector3 **apkPerm, Support &rkSupp)
{
    const Vector3 &rkP0 = *apkPerm[rkSupp.m_aiIndex[0]];
    const Vector3 &rkP1 = *apkPerm[rkSupp.m_aiIndex[1]];
    const Vector3 &rkP2 = *apkPerm[rkSupp.m_aiIndex[2]];
    const Vector3 &rkP3 = *apkPerm[rkSupp.m_aiIndex[3]];
    const Vector3 &rkP4 = *apkPerm[i];

    Sphere akS[14];
    Real fMinRSqr = ::Math::MAX_REAL;
    int iIndex = -1;

    akS[0] = ExactSphere2(rkP0, rkP4);
    if (PointInsideSphere(rkP1, akS[0]) && PointInsideSphere(rkP2, akS[0]) && PointInsideSphere(rkP3, akS[0]))
    {
        fMinRSqr = akS[0].Radius();
        iIndex = 0;
    }

    akS[1] = ExactSphere2(rkP1, rkP4);
    if (akS[1].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[1]) && PointInsideSphere(rkP2, akS[1]) && PointInsideSphere(rkP3, akS[1]))
    {
        fMinRSqr = akS[1].Radius();
        iIndex = 1;
    }

    akS[2] = ExactSphere2(rkP2, rkP4);
    if (akS[2].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[2]) && PointInsideSphere(rkP1, akS[2]) && PointInsideSphere(rkP3, akS[2]))
    {
        fMinRSqr = akS[2].Radius();
        iIndex = 2;
    }

    akS[3] = ExactSphere2(rkP3, rkP4);
    if (akS[3].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[3]) && PointInsideSphere(rkP1, akS[3]) && PointInsideSphere(rkP2, akS[3]))
    {
        fMinRSqr = akS[3].Radius();
        iIndex = 3;
    }

    akS[4] = ExactSphere3(rkP0, rkP1, rkP4);
    if (akS[4].Radius() < fMinRSqr && PointInsideSphere(rkP2, akS[4]) && PointInsideSphere(rkP3, akS[4]))
    {
        fMinRSqr = akS[4].Radius();
        iIndex = 4;
    }

    akS[5] = ExactSphere3(rkP0, rkP2, rkP4);
    if (akS[5].Radius() < fMinRSqr && PointInsideSphere(rkP1, akS[5]) && PointInsideSphere(rkP3, akS[5]))
    {
        fMinRSqr = akS[5].Radius();
        iIndex = 5;
    }

    akS[6] = ExactSphere3(rkP0, rkP3, rkP4);
    if (akS[6].Radius() < fMinRSqr && PointInsideSphere(rkP1, akS[6]) && PointInsideSphere(rkP2, akS[6]))
    {
        fMinRSqr = akS[6].Radius();
        iIndex = 6;
    }

    akS[7] = ExactSphere3(rkP1, rkP2, rkP4);
    if (akS[7].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[7]) && PointInsideSphere(rkP3, akS[7]))
    {
        fMinRSqr = akS[7].Radius();
        iIndex = 7;
    }

    akS[8] = ExactSphere3(rkP1, rkP3, rkP4);
    if (akS[8].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[8]) && PointInsideSphere(rkP2, akS[8]))
    {
        fMinRSqr = akS[8].Radius();
        iIndex = 8;
    }

    akS[9] = ExactSphere3(rkP2, rkP3, rkP4);
    if (akS[9].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[9]) && PointInsideSphere(rkP1, akS[9]))
    {
        fMinRSqr = akS[9].Radius();
        iIndex = 9;
    }

    akS[10] = ExactSphere4(rkP0, rkP1, rkP2, rkP4);
    if (akS[10].Radius() < fMinRSqr && PointInsideSphere(rkP3, akS[10]))
    {
        fMinRSqr = akS[10].Radius();
        iIndex = 10;
    }

    akS[11] = ExactSphere4(rkP0, rkP1, rkP3, rkP4);
    if (akS[11].Radius() < fMinRSqr && PointInsideSphere(rkP2, akS[11]))
    {
        fMinRSqr = akS[11].Radius();
        iIndex = 11;
    }

    akS[12] = ExactSphere4(rkP0, rkP2, rkP3, rkP4);
    if (akS[12].Radius() < fMinRSqr && PointInsideSphere(rkP1, akS[12]))
    {
        fMinRSqr = akS[12].Radius();
        iIndex = 12;
    }

    akS[13] = ExactSphere4(rkP1, rkP2, rkP3, rkP4);
    if (akS[13].Radius() < fMinRSqr && PointInsideSphere(rkP0, akS[13]))
    {
        fMinRSqr = akS[13].Radius();
        iIndex = 13;
    }

    Sphere kMinimal = akS[iIndex];

    switch (iIndex)
    {
    case 0:
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[1] = i;
        break;
    case 1:
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[0] = i;
        break;
    case 2:
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[0] = rkSupp.m_aiIndex[2];
        rkSupp.m_aiIndex[1] = i;
        break;
    case 3:
        rkSupp.m_iQuantity = 2;
        rkSupp.m_aiIndex[0] = rkSupp.m_aiIndex[3];
        rkSupp.m_aiIndex[1] = i;
        break;
    case 4:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[2] = i;
        break;
    case 5:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[1] = i;
        break;
    case 6:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[1] = rkSupp.m_aiIndex[3];
        rkSupp.m_aiIndex[2] = i;
        break;
    case 7:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[0] = i;
        break;
    case 8:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[0] = rkSupp.m_aiIndex[3];
        rkSupp.m_aiIndex[2] = i;
        break;
    case 9:
        rkSupp.m_iQuantity = 3;
        rkSupp.m_aiIndex[0] = rkSupp.m_aiIndex[3];
        rkSupp.m_aiIndex[1] = i;
        break;
    case 10:
        rkSupp.m_aiIndex[3] = i;
        break;
    case 11:
        rkSupp.m_aiIndex[2] = i;
        break;
    case 12:
        rkSupp.m_aiIndex[1] = i;
        break;
    case 13:
        rkSupp.m_aiIndex[0] = i;
        break;
    default:
        rkSupp.m_iQuantity = 5;
    }

    return kMinimal;
}
//----------------------------------------------------------------------------
typedef Sphere (*UpdateFunction)(int, Vector3 **, Support &);
static UpdateFunction gs_aoUpdate[5] =
    {
        NULL,
        UpdateSupport1,
        UpdateSupport2,
        UpdateSupport3,
        UpdateSupport4};
//----------------------------------------------------------------------------
Sphere Mgc::MinSphere(int iQuantity, const Vector3 *akPoint)
{
    // initialize random number generator
    size_t CountRestart = 0;
restart:
    static bool s_bFirstTime = true;
    if (s_bFirstTime)
    {
        srand(367);
        s_bFirstTime = false;
    }

    Sphere kMinimal;
    Support kSupp;

    if (iQuantity >= 1)
    {
        // create identity permutation (0,1,...,iQuantity-1)
        Vector3 **apkPerm = new Vector3 *[iQuantity];
        int i;
        for (i = 0; i < iQuantity; i++)
            apkPerm[i] = (Vector3 *)&akPoint[i];

        // generate random permutation
        for (i = iQuantity - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            if (j != i)
            {
                Vector3 *pSave = apkPerm[i];
                apkPerm[i] = apkPerm[j];
                apkPerm[j] = pSave;
            }
        }

        kMinimal = ExactSphere1(*apkPerm[0]);
        kSupp.m_iQuantity = 1;
        kSupp.m_aiIndex[0] = 0;
        i = 1;
        while (i < iQuantity)
        {
            if (!kSupp.Contains(i, apkPerm))
            {
                if (!PointInsideSphere(*apkPerm[i], kMinimal))
                {
                    Sphere kSph = gs_aoUpdate[kSupp.m_iQuantity](i, apkPerm, kSupp);
                    if (kSupp.m_iQuantity == 5)
                    {
                        assert(CountRestart < 1000);
                        delete[] apkPerm;
                        CountRestart++;
                        goto restart;
                    }
                    if (kSph.Radius() > kMinimal.Radius())
                    {
                        kMinimal = kSph;
                        i = 0;
                        continue;
                    }
                }
            }
            i++;
        }

        delete[] apkPerm;
    }
    else
    {
        assert(false);
    }

    kMinimal.Radius() = Math::Sqrt(kMinimal.Radius());
    return kMinimal;
}
//----------------------------------------------------------------------------
