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

#include "MgcDist3DVecLin.h"
using namespace Mgc;

//----------------------------------------------------------------------------
Real Mgc::SqrDistance(const Vector3 &rkPoint, const Line3 &rkLine,
                      Real *pfParam)
{
    Vector3 kDiff = rkPoint - rkLine.Origin();
    Real fSqrLen = rkLine.Direction().SquaredLength();
    Real fT = kDiff.Dot(rkLine.Direction()) / fSqrLen;
    kDiff -= fT * rkLine.Direction();

    if (pfParam)
        *pfParam = fT;

    return kDiff.SquaredLength();
}
//----------------------------------------------------------------------------
Real Mgc::SqrDistance(const Vector3 &rkPoint, const Ray3 &rkRay,
                      Real *pfParam)
{
    Vector3 kDiff = rkPoint - rkRay.Origin();
    Real fT = kDiff.Dot(rkRay.Direction());

    if (fT <= 0.0f)
    {
        fT = 0.0f;
    }
    else
    {
        fT /= rkRay.Direction().SquaredLength();
        kDiff -= fT * rkRay.Direction();
    }

    if (pfParam)
        *pfParam = fT;

    return kDiff.SquaredLength();
}
//----------------------------------------------------------------------------
Real Mgc::SqrDistance(const Vector3 &rkPoint, const Segment3 &rkSegment,
                      Real *pfParam)
{
    Vector3 kDiff = rkPoint - rkSegment.Origin();
    Real fT = kDiff.Dot(rkSegment.Direction());

    if (fT <= 0.0f)
    {
        fT = 0.0f;
    }
    else
    {
        Real fSqrLen = rkSegment.Direction().SquaredLength();
        if (fT >= fSqrLen)
        {
            fT = 1.0f;
            kDiff -= rkSegment.Direction();
        }
        else
        {
            fT /= fSqrLen;
            kDiff -= fT * rkSegment.Direction();
        }
    }

    if (pfParam)
        *pfParam = fT;

    return kDiff.SquaredLength();
}
//----------------------------------------------------------------------------
Real Mgc::Distance(const Vector3 &rkPoint, const Line3 &rkLine,
                   Real *pfParam)
{
    return Math::Sqrt(SqrDistance(rkPoint, rkLine, pfParam));
}
//----------------------------------------------------------------------------
Real Mgc::Distance(const Vector3 &rkPoint, const Ray3 &rkRay,
                   Real *pfParam)
{
    return Math::Sqrt(SqrDistance(rkPoint, rkRay, pfParam));
}
//----------------------------------------------------------------------------
Real Mgc::Distance(const Vector3 &rkPoint, const Segment3 &rkSegment,
                   Real *pfParam)
{
    return Math::Sqrt(SqrDistance(rkPoint, rkSegment, pfParam));
}
//----------------------------------------------------------------------------
