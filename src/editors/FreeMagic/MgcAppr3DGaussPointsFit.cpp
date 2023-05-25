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
#include "MgcEigen.h"
#include "MgcAppr3DGaussPointsFit.h"
using namespace Mgc;

//----------------------------------------------------------------------------
void Mgc::GaussPointsFit(int iQuantity, const Vector3 *akPoint,
                         Vector3 &rkCenter, Vector3 akAxis[3], Real afExtent[3])
{
    // compute mean of points
    rkCenter = akPoint[0];
    int i;
    for (i = 1; i < iQuantity; i++)
        rkCenter += akPoint[i];
    Real fInvQuantity = 1.0f / iQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    Real fSumXX = 0.0f, fSumXY = 0.0f, fSumXZ = 0.0f;
    Real fSumYY = 0.0f, fSumYZ = 0.0f, fSumZZ = 0.0f;
    for (i = 0; i < iQuantity; i++)
    {
        Vector3 kDiff = akPoint[i] - rkCenter;
        fSumXX += kDiff.x * kDiff.x;
        fSumXY += kDiff.x * kDiff.y;
        fSumXZ += kDiff.x * kDiff.z;
        fSumYY += kDiff.y * kDiff.y;
        fSumYZ += kDiff.y * kDiff.z;
        fSumZZ += kDiff.z * kDiff.z;
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    Eigen kES(3);
    kES.Matrix(0, 0) = fSumXX;
    kES.Matrix(0, 1) = fSumXY;
    kES.Matrix(0, 2) = fSumXZ;
    kES.Matrix(1, 0) = fSumXY;
    kES.Matrix(1, 1) = fSumYY;
    kES.Matrix(1, 2) = fSumYZ;
    kES.Matrix(2, 0) = fSumXZ;
    kES.Matrix(2, 1) = fSumYZ;
    kES.Matrix(2, 2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    akAxis[0].x = kES.GetEigenvector(0, 0);
    akAxis[0].y = kES.GetEigenvector(1, 0);
    akAxis[0].z = kES.GetEigenvector(2, 0);
    akAxis[1].x = kES.GetEigenvector(0, 1);
    akAxis[1].y = kES.GetEigenvector(1, 1);
    akAxis[1].z = kES.GetEigenvector(2, 1);
    akAxis[2].x = kES.GetEigenvector(0, 2);
    akAxis[2].y = kES.GetEigenvector(1, 2);
    akAxis[2].z = kES.GetEigenvector(2, 2);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);
    afExtent[2] = kES.GetEigenvalue(2);
}
//----------------------------------------------------------------------------
bool Mgc::GaussPointsFit(int iQuantity, const Vector3 *akPoint,
                         const bool *abValid, Vector3 &rkCenter, Vector3 akAxis[3],
                         Real afExtent[3])
{
    // compute mean of points
    rkCenter = Vector3::ZERO;
    int i, iValidQuantity = 0;
    for (i = 0; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            rkCenter += akPoint[i];
            iValidQuantity++;
        }
    }
    if (iValidQuantity == 0)
        return false;

    Real fInvQuantity = 1.0f / iValidQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    Real fSumXX = 0.0f, fSumXY = 0.0f, fSumXZ = 0.0f;
    Real fSumYY = 0.0f, fSumYZ = 0.0f, fSumZZ = 0.0f;
    for (i = 0; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            Vector3 kDiff = akPoint[i] - rkCenter;
            fSumXX += kDiff.x * kDiff.x;
            fSumXY += kDiff.x * kDiff.y;
            fSumXZ += kDiff.x * kDiff.z;
            fSumYY += kDiff.y * kDiff.y;
            fSumYZ += kDiff.y * kDiff.z;
            fSumZZ += kDiff.z * kDiff.z;
        }
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    Eigen kES(3);
    kES.Matrix(0, 0) = fSumXX;
    kES.Matrix(0, 1) = fSumXY;
    kES.Matrix(0, 2) = fSumXZ;
    kES.Matrix(1, 0) = fSumXY;
    kES.Matrix(1, 1) = fSumYY;
    kES.Matrix(1, 2) = fSumYZ;
    kES.Matrix(2, 0) = fSumXZ;
    kES.Matrix(2, 1) = fSumYZ;
    kES.Matrix(2, 2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    akAxis[0].x = kES.GetEigenvector(0, 0);
    akAxis[0].y = kES.GetEigenvector(1, 0);
    akAxis[0].z = kES.GetEigenvector(2, 0);
    akAxis[1].x = kES.GetEigenvector(0, 1);
    akAxis[1].y = kES.GetEigenvector(1, 1);
    akAxis[1].z = kES.GetEigenvector(2, 1);
    akAxis[2].x = kES.GetEigenvector(0, 2);
    akAxis[2].y = kES.GetEigenvector(1, 2);
    akAxis[2].z = kES.GetEigenvector(2, 2);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);
    afExtent[2] = kES.GetEigenvalue(2);

    return true;
}
//----------------------------------------------------------------------------
