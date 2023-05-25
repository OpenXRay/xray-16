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
#include "MgcAppr3DLineFit.h"
#include "MgcCont3DCylinder.h"
#include "MgcDist3DVecLin.h"
using namespace Mgc;

//----------------------------------------------------------------------------
Cylinder Mgc::ContCylinder(int iQuantity, const Vector3 *akPoint)
{
    Cylinder kCylinder;

    Line3 kLine;
    OrthogonalLineFit(iQuantity, akPoint, kLine.Origin(), kLine.Direction());

    Real fMaxRadiusSqr = 0.0f;
    int i;
    for (i = 0; i < iQuantity; i++)
    {
        Real fRadiusSqr = SqrDistance(akPoint[i], kLine);
        if (fRadiusSqr > fMaxRadiusSqr)
            fMaxRadiusSqr = fRadiusSqr;
    }

    Vector3 kDiff = akPoint[0] - kLine.Origin();
    Real fWMin = kLine.Direction().Dot(kDiff), fWMax = fWMin;
    for (i = 1; i < iQuantity; i++)
    {
        kDiff = akPoint[i] - kLine.Origin();
        Real fW = kLine.Direction().Dot(kDiff);
        if (fW < fWMin)
            fWMin = fW;
        else if (fW > fWMax)
            fWMax = fW;
    }

    kCylinder.Center() = kLine.Origin() +
                         (0.5f * (fWMax + fWMin)) * kLine.Direction();
    kCylinder.Direction() = kLine.Direction();
    kCylinder.Radius() = Math::Sqrt(fMaxRadiusSqr);
    kCylinder.Height() = fWMax - fWMin;

    return kCylinder;
}
//----------------------------------------------------------------------------
