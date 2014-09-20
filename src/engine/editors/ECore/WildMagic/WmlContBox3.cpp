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

#include "WmlApprGaussPointsFit3.h"
#include "WmlContBox3.h"
#include "WmlQuaternion.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
void Wml::ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint,
    Vector3<Real>& rkMin, Vector3<Real>& rkMax)
{
    rkMin = akPoint[0];
    rkMax = rkMin;

    for (int i = 1; i < iQuantity; i++)
    {
        if ( akPoint[i].X() < rkMin.X() )
            rkMin.X() = akPoint[i].X();
        else if ( akPoint[i].X() > rkMax.X() )
            rkMax.X() = akPoint[i].X();

        if ( akPoint[i].Y() < rkMin.Y() )
            rkMin.Y() = akPoint[i].Y();
        else if ( akPoint[i].Y() > rkMax.Y() )
            rkMax.Y() = akPoint[i].Y();

        if ( akPoint[i].Z() < rkMin.Z() )
            rkMin.Z() = akPoint[i].Z();
        else if ( akPoint[i].Z() > rkMax.Z() )
            rkMax.Z() = akPoint[i].Z();
    }
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> Wml::ContOrientedBox (int iQuantity, const Vector3<Real>* akPoint)
{
    Box3<Real> kBox;

    GaussPointsFit(iQuantity,akPoint,kBox.Center(),kBox.Axes(),
        kBox.Extents());

    // Let C be the box center and let U0, U1, and U2 be the box axes.  Each
    // input point is of the form X = C + y0*U0 + y1*U1 + y2*U2.  The
    // following code computes min(y0), max(y0), min(y1), max(y1), min(y2),
    // and max(y2).  The box center is then adjusted to be
    //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1 +
    //        0.5*(min(y2)+max(y2))*U2

    Vector3<Real> kDiff = akPoint[0] - kBox.Center();
    Real fY0Min = kDiff.Dot(kBox.Axis(0)), fY0Max = fY0Min;
    Real fY1Min = kDiff.Dot(kBox.Axis(1)), fY1Max = fY1Min;
    Real fY2Min = kDiff.Dot(kBox.Axis(2)), fY2Max = fY2Min;

    for (int i = 1; i < iQuantity; i++)
    {
        kDiff = akPoint[i] - kBox.Center();

        Real fY0 = kDiff.Dot(kBox.Axis(0));
        if ( fY0 < fY0Min )
            fY0Min = fY0;
        else if ( fY0 > fY0Max )
            fY0Max = fY0;

        Real fY1 = kDiff.Dot(kBox.Axis(1));
        if ( fY1 < fY1Min )
            fY1Min = fY1;
        else if ( fY1 > fY1Max )
            fY1Max = fY1;

        Real fY2 = kDiff.Dot(kBox.Axis(2));
        if ( fY2 < fY2Min )
            fY2Min = fY2;
        else if ( fY2 > fY2Max )
            fY2Max = fY2;
    }

    kBox.Center() += (((Real)0.5)*(fY0Min+fY0Max))*kBox.Axis(0) +
        (((Real)0.5)*(fY1Min+fY1Max))*kBox.Axis(1) +
        (((Real)0.5)*(fY2Min+fY2Max))*kBox.Axis(2);

    kBox.Extent(0) = ((Real)0.5)*(fY0Max - fY0Min);
    kBox.Extent(1) = ((Real)0.5)*(fY1Max - fY1Min);
    kBox.Extent(2) = ((Real)0.5)*(fY2Max - fY2Min);

    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
bool Wml::ContOrientedBox (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid, Box3<Real>& rkBox)
{
    if ( !GaussPointsFit(iQuantity,akPoint,abValid,rkBox.Center(),
         rkBox.Axes(),rkBox.Extents()) )
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
    Vector3<Real> kDiff;
    Real fY0Min = (Real)0.0, fY0Max = (Real)0.0;
    Real fY1Min = (Real)0.0, fY1Max = (Real)0.0;
    Real fY2Min = (Real)0.0, fY2Max = (Real)0.0;
    int i;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
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
        if ( abValid[i] )
        {
            kDiff = akPoint[i] - rkBox.Center();

            Real fY0 = kDiff.Dot(rkBox.Axis(0));
            if ( fY0 < fY0Min )
                fY0Min = fY0;
            else if ( fY0 > fY0Max )
                fY0Max = fY0;

            Real fY1 = kDiff.Dot(rkBox.Axis(1));
            if ( fY1 < fY1Min )
                fY1Min = fY1;
            else if ( fY1 > fY1Max )
                fY1Max = fY1;

            Real fY2 = kDiff.Dot(rkBox.Axis(2));
            if ( fY2 < fY2Min )
                fY2Min = fY2;
            else if ( fY2 > fY2Max )
                fY2Max = fY2;
        }
    }

    rkBox.Center() += (0.5f*(fY0Min+fY0Max))*rkBox.Axis(0)
        + (0.5f*(fY1Min+fY1Max))*rkBox.Axis(1) +
        (0.5f*(fY2Min+fY2Max))*rkBox.Axis(2);

    rkBox.Extent(0) = 0.5f*(fY0Max - fY0Min);
    rkBox.Extent(1) = 0.5f*(fY1Max - fY1Min);
    rkBox.Extent(2) = 0.5f*(fY2Max - fY2Min);

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> Wml::MergeBoxes (const Box3<Real>& rkBox0,
    const Box3<Real>& rkBox1)
{
    // construct a box that contains the input boxes
    Box3<Real> kBox;

    // The first guess at the box center.  This value will be updated later
    // after the input box vertices are projected onto axes determined by an
    // average of box axes.
    kBox.Center() = ((Real)0.5)*(rkBox0.Center() + rkBox1.Center());

    // A box's axes, when viewed as the columns of a matrix, form a rotation
    // matrix.  The input box axes are converted to quaternions.  The average
    // quaternion is computed, then normalized to unit length.  The result is
    // the slerp of the two input quaternions with t-value of 1/2.  The result
    // is converted back to a rotation matrix and its columns are selected as
    // the merged box axes.
    Quaternion<Real> kQ0, kQ1;
    kQ0.FromRotationMatrix(rkBox0.Axes());
    kQ1.FromRotationMatrix(rkBox1.Axes());
    if ( kQ0.Dot(kQ1) < 0.0f )
        kQ1 = -kQ1;

    Quaternion<Real> kQ = kQ0 + kQ1;
    Real fInvLength = Math<Real>::InvSqrt(kQ.Dot(kQ));
    kQ = fInvLength*kQ;
    kQ.ToRotationMatrix(kBox.Axes());

    // Project the input box vertices onto the merged-box axes.  Each axis
    // D[i] containing the current center C has a minimum projected value
    // pmin[i] and a maximum projected value pmax[i].  The corresponding end
    // points on the axes are C+pmin[i]*D[i] and C+pmax[i]*D[i].  The point C
    // is not necessarily the midpoint for any of the intervals.  The actual
    // box center will be adjusted from C to a point C' that is the midpoint
    // of each interval,
    //   C' = C + sum_{i=0}^2 0.5*(pmin[i]+pmax[i])*D[i]
    // The box extents are
    //   e[i] = 0.5*(pmax[i]-pmin[i])

    int i, j;
    Real fDot;
    Vector3<Real> akVertex[8], kDiff;
    Vector3<Real> kMin = Vector3<Real>::ZERO;
    Vector3<Real> kMax = Vector3<Real>::ZERO;

    rkBox0.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center();
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis(j));
            if ( fDot > kMax[j] )
                kMax[j] = fDot;
            else if ( fDot < kMin[j] )
                kMin[j] = fDot;
        }
    }

    rkBox1.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center();
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis(j));
            if ( fDot > kMax[j] )
                kMax[j] = fDot;
            else if ( fDot < kMin[j] )
                kMin[j] = fDot;
        }
    }

    // [kMin,kMax] is the axis-aligned box in the coordinate system of the
    // merged box axes.  Update the current box center to be the center of
    // the new box.  Compute the extens based on the new center.
    for (j = 0; j < 3; j++)
    {
        kBox.Center() += (((Real)0.5)*(kMax[j]+kMin[j]))*kBox.Axis(j);
        kBox.Extent(j) = ((Real)0.5)*(kMax[j]-kMin[j]);
    }

    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
bool Wml::InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox,
    Real fEpsilon)
{
    Vector3<Real> kDiff = rkPoint - rkBox.Center();
    for (int i = 0; i < 3; i++)
    {
        Real fCoeff = kDiff.Dot(rkBox.Axis(i));
        if ( Math<Real>::FAbs(fCoeff) > rkBox.Extent(i) + fEpsilon )
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template WML_ITEM void ContAlignedBox<float> (int,
    const Vector3<float>*, Vector3<float>&, Vector3<float>&);
template WML_ITEM Box3<float> ContOrientedBox<float> (int,
    const Vector3<float>*);
template WML_ITEM bool ContOrientedBox<float> (int,
    const Vector3<float>*, const bool*, Box3<float>&);
template WML_ITEM bool InBox<float> (const Vector3<float>&,
    const Box3<float>&, float);
template WML_ITEM Box3<float> MergeBoxes<float> (const Box3<float>&,
    const Box3<float>&);

template WML_ITEM void ContAlignedBox<double> (int,
    const Vector3<double>*, Vector3<double>&, Vector3<double>&);
template WML_ITEM Box3<double> ContOrientedBox<double> (int,
    const Vector3<double>*);
template WML_ITEM bool ContOrientedBox<double> (int,
    const Vector3<double>*, const bool*, Box3<double>&);
template WML_ITEM bool InBox<double> (const Vector3<double>&,
    const Box3<double>&, double);
template WML_ITEM Box3<double> MergeBoxes<double> (const Box3<double>&,
    const Box3<double>&);
}
//----------------------------------------------------------------------------
