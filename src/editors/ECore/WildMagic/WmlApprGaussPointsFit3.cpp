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

#include "WmlEigen.h"
#include "WmlApprGaussPointsFit3.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
void Wml::GaussPointsFit (int iQuantity, const Vector3<Real>* akPoint,
    Vector3<Real>& rkCenter, Vector3<Real> akAxis[3], Real afExtent[3])
{
    // compute mean of points
    rkCenter = akPoint[0];
    int i;
    for (i = 1; i < iQuantity; i++)
        rkCenter += akPoint[i];
    Real fInvQuantity = ((Real)1.0)/iQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    Real fSumXX = (Real)0.0, fSumXY = (Real)0.0, fSumXZ = (Real)0.0;
    Real fSumYY = (Real)0.0, fSumYZ = (Real)0.0, fSumZZ = (Real)0.0;
    for (i = 0; i < iQuantity; i++)
    {
        Vector3<Real> kDiff = akPoint[i] - rkCenter;
        fSumXX += kDiff.X()*kDiff.X();
        fSumXY += kDiff.X()*kDiff.Y();
        fSumXZ += kDiff.X()*kDiff.Z();
        fSumYY += kDiff.Y()*kDiff.Y();
        fSumYZ += kDiff.Y()*kDiff.Z();
        fSumZZ += kDiff.Z()*kDiff.Z();
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    Eigen<Real> kES(3);
    kES(0,0) = fSumXX;
    kES(0,1) = fSumXY;
    kES(0,2) = fSumXZ;
    kES(1,0) = fSumXY;
    kES(1,1) = fSumYY;
    kES(1,2) = fSumYZ;
    kES(2,0) = fSumXZ;
    kES(2,1) = fSumYZ;
    kES(2,2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    for (i = 0; i < 3; i++)
    {
        afExtent[i] = kES.GetEigenvalue(i);
        kES.GetEigenvector(i,akAxis[i]);
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Wml::GaussPointsFit (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid, Vector3<Real>& rkCenter, Vector3<Real> akAxis[3],
    Real afExtent[3])
{
    // compute mean of points
    rkCenter = Vector3<Real>::ZERO;
    int i, iValidQuantity = 0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            rkCenter += akPoint[i];
            iValidQuantity++;
        }
    }
    if ( iValidQuantity == 0 )
        return false;

    Real fInvQuantity = ((Real)1.0)/iValidQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    Real fSumXX = (Real)0.0, fSumXY = (Real)0.0, fSumXZ = (Real)0.0;
    Real fSumYY = (Real)0.0, fSumYZ = (Real)0.0, fSumZZ = (Real)0.0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            Vector3<Real> kDiff = akPoint[i] - rkCenter;
            fSumXX += kDiff.X()*kDiff.X();
            fSumXY += kDiff.X()*kDiff.Y();
            fSumXZ += kDiff.X()*kDiff.Z();
            fSumYY += kDiff.Y()*kDiff.Y();
            fSumYZ += kDiff.Y()*kDiff.Z();
            fSumZZ += kDiff.Z()*kDiff.Z();
        }
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    Eigen<Real> kES(3);
    kES(0,0) = fSumXX;
    kES(0,1) = fSumXY;
    kES(0,2) = fSumXZ;
    kES(1,0) = fSumXY;
    kES(1,1) = fSumYY;
    kES(1,2) = fSumYZ;
    kES(2,0) = fSumXZ;
    kES(2,1) = fSumYZ;
    kES(2,2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    for (i = 0; i < 3; i++)
    {
        afExtent[i] = kES.GetEigenvalue(i);
        kES.GetEigenvector(i,akAxis[i]);
    }

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template WML_ITEM void GaussPointsFit<float> (int,
    const Vector3<float>*, Vector3<float>&, Vector3<float>[3], float[3]);
template WML_ITEM bool GaussPointsFit<float> (int,
    const Vector3<float>*, const bool*, Vector3<float>&, Vector3<float>[3],
    float[3]);

template WML_ITEM void GaussPointsFit<double> (int,
    const Vector3<double>*, Vector3<double>&, Vector3<double>[3], double[3]);
template WML_ITEM bool GaussPointsFit<double> (int,
    const Vector3<double>*, const bool*, Vector3<double>&, Vector3<double>[3],
    double[3]);
}
//----------------------------------------------------------------------------
