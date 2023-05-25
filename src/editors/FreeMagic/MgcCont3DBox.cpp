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
#include "MgcAppr3DGaussPointsFit.h"
#include "MgcCont3DBox.h"
#include "MgcQuaternion.h"
using namespace Mgc;

//----------------------------------------------------------------------------
void Mgc::ContAlignedBox(int iQuantity, const Vector3 *akPoint,
                         Vector3 &rkMin, Vector3 &rkMax)
{
    rkMin = akPoint[0];
    rkMax = rkMin;

    for (int i = 1; i < iQuantity; i++)
    {
        if (akPoint[i].x < rkMin.x)
            rkMin.x = akPoint[i].x;
        else if (akPoint[i].x > rkMax.x)
            rkMax.x = akPoint[i].x;

        if (akPoint[i].y < rkMin.y)
            rkMin.y = akPoint[i].y;
        else if (akPoint[i].y > rkMax.y)
            rkMax.y = akPoint[i].y;

        if (akPoint[i].z < rkMin.z)
            rkMin.z = akPoint[i].z;
        else if (akPoint[i].z > rkMax.z)
            rkMax.z = akPoint[i].z;
    }
}
//----------------------------------------------------------------------------
Box3 Mgc::ContOrientedBox(int iQuantity, const Vector3 *akPoint)
{
    Box3 kBox;

    GaussPointsFit(iQuantity, akPoint, kBox.Center(), kBox.Axes(),
                   kBox.Extents());

    // Let C be the box center and let U0, U1, and U2 be the box axes.  Each
    // input point is of the form X = C + y0*U0 + y1*U1 + y2*U2.  The
    // following code computes min(y0), max(y0), min(y1), max(y1), min(y2),
    // and max(y2).  The box center is then adjusted to be
    //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1 +
    //        0.5*(min(y2)+max(y2))*U2

    Vector3 kDiff = akPoint[0] - kBox.Center();
    Real fY0Min = kDiff.Dot(kBox.Axis(0)), fY0Max = fY0Min;
    Real fY1Min = kDiff.Dot(kBox.Axis(1)), fY1Max = fY1Min;
    Real fY2Min = kDiff.Dot(kBox.Axis(2)), fY2Max = fY2Min;

    for (int i = 1; i < iQuantity; i++)
    {
        kDiff = akPoint[i] - kBox.Center();

        Real fY0 = kDiff.Dot(kBox.Axis(0));
        if (fY0 < fY0Min)
            fY0Min = fY0;
        else if (fY0 > fY0Max)
            fY0Max = fY0;

        Real fY1 = kDiff.Dot(kBox.Axis(1));
        if (fY1 < fY1Min)
            fY1Min = fY1;
        else if (fY1 > fY1Max)
            fY1Max = fY1;

        Real fY2 = kDiff.Dot(kBox.Axis(2));
        if (fY2 < fY2Min)
            fY2Min = fY2;
        else if (fY2 > fY2Max)
            fY2Max = fY2;
    }

    kBox.Center() += (0.5f * (fY0Min + fY0Max)) * kBox.Axis(0) +
                     (0.5f * (fY1Min + fY1Max)) * kBox.Axis(1) +
                     (0.5f * (fY2Min + fY2Max)) * kBox.Axis(2);

    kBox.Extent(0) = 0.5f * (fY0Max - fY0Min);
    kBox.Extent(1) = 0.5f * (fY1Max - fY1Min);
    kBox.Extent(2) = 0.5f * (fY2Max - fY2Min);

    return kBox;
}
//----------------------------------------------------------------------------
bool Mgc::ContOrientedBox(int iQuantity, const Vector3 *akPoint,
                          const bool *abValid, Box3 &rkBox)
{
    if (!GaussPointsFit(iQuantity, akPoint, abValid, rkBox.Center(),
                        rkBox.Axes(), rkBox.Extents()))
    {
        return false;
    }

    // Let C be the box center and let U0, U1, and U2 be the box axes.  Each
    // input point is of the form X = C + y0*U0 + y1*U1 + y2*U2.  The
    // following code computes min(y0), max(y0), min(y1), max(y1), min(y2),
    // and max(y2).  The box center is then adjusted to be
    //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1 +
    //        0.5*(min(y2)+max(y2))*U2

    // get first valid vertex
    Vector3 kDiff;
    Real fY0Min, fY0Max, fY1Min, fY1Max, fY2Min, fY2Max;
    int i;
    for (i = 0; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            kDiff = akPoint[i] - rkBox.Center();
            fY0Min = kDiff.Dot(rkBox.Axis(0));
            fY0Max = fY0Min;
            fY1Min = kDiff.Dot(rkBox.Axis(1));
            fY1Max = fY1Min;
            fY2Min = kDiff.Dot(rkBox.Axis(2));
            fY2Max = fY2Min;
            break;
        }
    }

    for (i++; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            kDiff = akPoint[i] - rkBox.Center();

            Real fY0 = kDiff.Dot(rkBox.Axis(0));
            if (fY0 < fY0Min)
                fY0Min = fY0;
            else if (fY0 > fY0Max)
                fY0Max = fY0;

            Real fY1 = kDiff.Dot(rkBox.Axis(1));
            if (fY1 < fY1Min)
                fY1Min = fY1;
            else if (fY1 > fY1Max)
                fY1Max = fY1;

            Real fY2 = kDiff.Dot(rkBox.Axis(2));
            if (fY2 < fY2Min)
                fY2Min = fY2;
            else if (fY2 > fY2Max)
                fY2Max = fY2;
        }
    }

    rkBox.Center() += (0.5f * (fY0Min + fY0Max)) * rkBox.Axis(0) + (0.5f * (fY1Min + fY1Max)) * rkBox.Axis(1) +
                      (0.5f * (fY2Min + fY2Max)) * rkBox.Axis(2);

    rkBox.Extent(0) = 0.5f * (fY0Max - fY0Min);
    rkBox.Extent(1) = 0.5f * (fY1Max - fY1Min);
    rkBox.Extent(2) = 0.5f * (fY2Max - fY2Min);

    return true;
}
//----------------------------------------------------------------------------
Box3 Mgc::MergeBoxes(const Box3 &rkBox0, const Box3 &rkBox1)
{
    Box3 kBox;
    kBox.Center() = 0.5f * (rkBox0.Center() + rkBox1.Center());

    Quaternion kQ0, kQ1;
    kQ0.FromAxes(rkBox0.Axes());
    kQ1.FromAxes(rkBox1.Axes());
    if (kQ0.Dot(kQ1) < 0.0f)
        kQ1 = -kQ1;

    Quaternion kQ = kQ0 + kQ1;
    Real fInvLength = Math::InvSqrt(kQ.Norm());
    kQ = fInvLength * kQ;
    kQ.ToAxes(kBox.Axes());

    int i, j;
    Vector3 akVertex[8], kDiff;
    Real fADot;

    kBox.Extent(0) = 0.0f;
    kBox.Extent(1) = 0.0f;
    kBox.Extent(2) = 0.0f;

    rkBox0.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center();
        for (j = 0; j < 3; j++)
        {
            fADot = Math::FAbs(kDiff.Dot(kBox.Axis(j)));
            if (fADot > kBox.Extent(j))
                kBox.Extent(j) = fADot;
        }
    }

    rkBox1.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center();
        for (j = 0; j < 3; j++)
        {
            fADot = Math::FAbs(kDiff.Dot(kBox.Axis(j)));
            if (fADot > kBox.Extent(j))
                kBox.Extent(j) = fADot;
        }
    }

    return kBox;
}
//----------------------------------------------------------------------------
bool Mgc::InBox(const Vector3 &rkPoint, const Box3 &rkBox, Real fEpsilon)
{
    Vector3 kDiff = rkPoint - rkBox.Center();
    for (int i = 0; i < 3; i++)
    {
        Real fCoeff = kDiff.Dot(rkBox.Axis(i));
        if (Math::FAbs(fCoeff) > rkBox.Extent(i) + fEpsilon)
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------
