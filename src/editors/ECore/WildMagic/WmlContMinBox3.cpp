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

#include "WmlContMinBox3.h"
#include "WmlMatrix3.h"
#include "WmlMinimizeN.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
MinBox3<Real>::MinBox3 (int iQuantity, const Vector3<Real>* akPoint,
    Box3<Real>& rkBox)
{
    m_iQuantity = iQuantity;
    m_akPoint = akPoint;

    MinimizeN<Real> kMinimizer(3,Volume,8,8,32,this);

    Real afA0[3] =
    {
        (Real)0.0,
        (Real)0.0,
        (Real)0.0
    };

    Real afA1[3] =
    {
        Math<Real>::_PI,
        Math<Real>::HALF_PI,
        Math<Real>::_PI
    };

    // compute some samples to narrow down the search region
    Real fMinVolume = Math<Real>::MAX_REAL;
    Real afAngle[3], afAInitial[3];
    const int iMax = 3;
    const Real fInvMax = ((Real)1.0)/(Real)iMax;
    for (int i0 = 0; i0 <= iMax; i0++)
    {
        afAngle[0] = afA0[0] + i0*(afA1[0] - afA0[0])*fInvMax;
        for (int i1 = 0; i1 <= iMax; i1++)
        {
            afAngle[1] = afA0[1] + i1*(afA1[1] - afA0[1])*fInvMax;
            for (int i2 = 0; i2 <= iMax; i2++)
            {
                afAngle[2] = afA0[2] + i2*(afA1[2] - afA0[2])*fInvMax;
                Real fVolume = Volume(afAngle,this);
                if ( fVolume < fMinVolume )
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
    kMinimizer.GetMinimum(afA0,afA1,afAInitial,afAMin,fVMin);

    MinimalBoxForAngles(iQuantity,akPoint,afAMin,rkBox);
}
//----------------------------------------------------------------------------
template <class Real>
Real MinBox3<Real>::Volume (const Real* afAngle, void* pvData)
{
    MinBox3& rkSelf = *(MinBox3*)pvData;

    Real fCos0 = Math<Real>::Cos(afAngle[0]);
    Real fSin0 = Math<Real>::Sin(afAngle[0]);
    Real fCos1 = Math<Real>::Cos(afAngle[1]);
    Real fSin1 = Math<Real>::Sin(afAngle[1]);
    Vector3<Real> kAxis(fCos0*fSin1,fSin0*fSin1,fCos1);
    Matrix3<Real> kRot(kAxis,afAngle[2]);

    Vector3<Real> kMin = rkSelf.m_akPoint[0]*kRot, kMax = kMin;
    for (int i = 1; i < rkSelf.m_iQuantity; i++)
    {
        Vector3<Real> kTest = rkSelf.m_akPoint[i]*kRot;

        if ( kTest.X() < kMin.X() )
            kMin.X() = kTest.X();
        else if ( kTest.X() > kMax.X() )
            kMax.X() = kTest.X();

        if ( kTest.Y() < kMin.Y() )
            kMin.Y() = kTest.Y();
        else if ( kTest.Y() > kMax.Y() )
            kMax.Y() = kTest.Y();

        if ( kTest.Z() < kMin.Z() )
            kMin.Z() = kTest.Z();
        else if ( kTest.Z() > kMax.Z() )
            kMax.Z() = kTest.Z();
    }

    Real fVolume = (kMax.X()-kMin.X()) * (kMax.Y()-kMin.Y()) *
        (kMax.Z()-kMin.Z());
    return fVolume;
}
//----------------------------------------------------------------------------
template <class Real>
void MinBox3<Real>::MinimalBoxForAngles (int iQuantity,
    const Vector3<Real>* akPoint, Real afAngle[3], Box3<Real>& rkBox)
{
    Real fCos0 = Math<Real>::Cos(afAngle[0]);
    Real fSin0 = Math<Real>::Sin(afAngle[0]);
    Real fCos1 = Math<Real>::Cos(afAngle[1]);
    Real fSin1 = Math<Real>::Sin(afAngle[1]);
    Vector3<Real> kAxis(fCos0*fSin1,fSin0*fSin1,fCos1);
    Matrix3<Real> kRot(kAxis,afAngle[2]);

    Vector3<Real> kMin = akPoint[0]*kRot, kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        Vector3<Real> kTest = akPoint[i]*kRot;

        if ( kTest.X() < kMin.X() )
            kMin.X() = kTest.X();
        else if ( kTest.X() > kMax.X() )
            kMax.X() = kTest.X();

        if ( kTest.Y() < kMin.Y() )
            kMin.Y() = kTest.Y();
        else if ( kTest.Y() > kMax.Y() )
            kMax.Y() = kTest.Y();

        if ( kTest.Z() < kMin.Z() )
            kMin.Z() = kTest.Z();
        else if ( kTest.Z() > kMax.Z() )
            kMax.Z() = kTest.Z();
    }

    Vector3<Real> kMid = ((Real)0.5)*(kMax + kMin);
    Vector3<Real> kRng = ((Real)0.5)*(kMax - kMin);

    rkBox.Center() = kRot*kMid;
    rkBox.Axis(0) = kRot.GetColumn(0);
    rkBox.Axis(1) = kRot.GetColumn(1);
    rkBox.Axis(2) = kRot.GetColumn(2);
    rkBox.Extent(0) = kRng.X();
    rkBox.Extent(1) = kRng.Y();
    rkBox.Extent(2) = kRng.Z();
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template WML_ITEM class MinBox3<float>;
template WML_ITEM class MinBox3<double>;
}
//----------------------------------------------------------------------------
