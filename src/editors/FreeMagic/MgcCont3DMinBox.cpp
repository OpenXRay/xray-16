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
#include "MgcCont3DMinBox.h"
#include "MgcMatrix3.h"
#include "MgcMinimizeND.h"
using namespace Mgc;

class PointArray
{
public:
    PointArray(int iQuantity, const Vector3 *akPoint)
        : m_akPoint(akPoint)
    {
        m_iQuantity = iQuantity;
    }

    int m_iQuantity;
    const Vector3 *m_akPoint;
};

//----------------------------------------------------------------------------
static Real Volume(const Real *afAngle, void *pvUserData)
{
    int iQuantity = ((PointArray *)pvUserData)->m_iQuantity;
    const Vector3 *akPoint = ((PointArray *)pvUserData)->m_akPoint;

    Real fCos0 = Math::Cos(afAngle[0]);
    Real fSin0 = Math::Sin(afAngle[0]);
    Real fCos1 = Math::Cos(afAngle[1]);
    Real fSin1 = Math::Sin(afAngle[1]);
    Vector3 kAxis(fCos0 * fSin1, fSin0 * fSin1, fCos1);
    Matrix3 kRot;
    kRot.FromAxisAngle(kAxis, afAngle[2]);

    Vector3 kMin = akPoint[0] * kRot, kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        Vector3 kTest = akPoint[i] * kRot;

        if (kTest.x < kMin.x)
            kMin.x = kTest.x;
        else if (kTest.x > kMax.x)
            kMax.x = kTest.x;

        if (kTest.y < kMin.y)
            kMin.y = kTest.y;
        else if (kTest.y > kMax.y)
            kMax.y = kTest.y;

        if (kTest.z < kMin.z)
            kMin.z = kTest.z;
        else if (kTest.z > kMax.z)
            kMax.z = kTest.z;
    }

    Real fVolume = (kMax.x - kMin.x) * (kMax.y - kMin.y) * (kMax.z - kMin.z);
    return fVolume;
}
//----------------------------------------------------------------------------
static void MinimalBoxForAngles(int iQuantity, const Vector3 *akPoint,
                                Real afAngle[3], Box3 &rkBox)
{
    Real fCos0 = Math::Cos(afAngle[0]);
    Real fSin0 = Math::Sin(afAngle[0]);
    Real fCos1 = Math::Cos(afAngle[1]);
    Real fSin1 = Math::Sin(afAngle[1]);
    Vector3 kAxis(fCos0 * fSin1, fSin0 * fSin1, fCos1);
    Matrix3 kRot;
    kRot.FromAxisAngle(kAxis, afAngle[2]);

    Vector3 kMin = akPoint[0] * kRot, kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        Vector3 kTest = akPoint[i] * kRot;

        if (kTest.x < kMin.x)
            kMin.x = kTest.x;
        else if (kTest.x > kMax.x)
            kMax.x = kTest.x;

        if (kTest.y < kMin.y)
            kMin.y = kTest.y;
        else if (kTest.y > kMax.y)
            kMax.y = kTest.y;

        if (kTest.z < kMin.z)
            kMin.z = kTest.z;
        else if (kTest.z > kMax.z)
            kMax.z = kTest.z;
    }

    Vector3 kMid = 0.5f * (kMax + kMin);
    Vector3 kRng = 0.5f * (kMax - kMin);

    rkBox.Center() = kRot * kMid;
    rkBox.Axis(0) = kRot.GetColumn(0);
    rkBox.Axis(1) = kRot.GetColumn(1);
    rkBox.Axis(2) = kRot.GetColumn(2);
    rkBox.Extent(0) = kRng.x;
    rkBox.Extent(1) = kRng.y;
    rkBox.Extent(2) = kRng.z;
}
//----------------------------------------------------------------------------
Box3 Mgc::MinBox(int iQuantity, const Vector3 *akPoint)
{
    int iMaxLevel = 8;
    int iMaxBracket = 8;
    int iMaxIterations = 32;
    PointArray kPA(iQuantity, akPoint);
    MinimizeND kMinimizer(3, Volume, iMaxLevel, iMaxBracket, iMaxIterations, &kPA);

    Real afA0[3] =
        {
            0.0f,
            0.0f,
            0.0f};

    Real afA1[3] =
        {
            Math::_PI,
            Math::HALF_PI,
            Math::_PI};

    // compute some samples to narrow down the search region
    Real fMinVolume = Math::MAX_REAL;
    Real afAngle[3], afAInitial[3];
    const int iMax = 3;
    for (int i0 = 0; i0 <= iMax; i0++)
    {
        afAngle[0] = afA0[0] + i0 * (afA1[0] - afA0[0]) / iMax;
        for (int i1 = 0; i1 <= iMax; i1++)
        {
            afAngle[1] = afA0[1] + i1 * (afA1[1] - afA0[1]) / iMax;
            for (int i2 = 0; i2 <= iMax; i2++)
            {
                afAngle[2] = afA0[2] + i2 * (afA1[2] - afA0[2]) / iMax;
                Real fVolume = Volume(afAngle, &kPA);
                if (fVolume < fMinVolume)
                {
                    fMinVolume = fVolume;
                    afAInitial[0] = afAngle[0];
                    afAInitial[1] = afAngle[1];
                    afAInitial[2] = afAngle[2];
                }
            }
        }
    }

    Real afAMin[3], fVMin;
    kMinimizer.GetMinimum(afA0, afA1, afAInitial, afAMin, fVMin);

    Box3 kBox;
    MinimalBoxForAngles(iQuantity, akPoint, afAMin, kBox);
    return kBox;
}
//----------------------------------------------------------------------------
