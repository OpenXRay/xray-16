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

#include "MgcAppr3DPlaneFit.h"
#include "MgcEigen.h"
#include "MgcLinearSystem.h"
using namespace Mgc;

//----------------------------------------------------------------------------
bool Mgc::HeightPlaneFit(int iQuantity, Vector3 *akPoint, Real &rfA,
                         Real &rfB, Real &rfC)
{
    // You need at least three points to determine the plane.  Even so, if
    // the points are on a vertical plane, there is no least-squares fit in
    // the 'height' sense.  This will be trapped by the determinant of the
    // coefficient matrix.
    if (iQuantity < 3)
        return false;

    // compute sums for linear system
    Real fSumX = 0.0f, fSumY = 0.0f, fSumZ = 0.0f;
    Real fSumXX = 0.0f, fSumXY = 0.0f, fSumXZ = 0.0f;
    Real fSumYY = 0.0f, fSumYZ = 0.0f;
    for (int i = 0; i < iQuantity; i++)
    {
        fSumX += akPoint[i].x;
        fSumY += akPoint[i].y;
        fSumZ += akPoint[i].z;
        fSumXX += akPoint[i].x * akPoint[i].x;
        fSumXY += akPoint[i].x * akPoint[i].y;
        fSumXZ += akPoint[i].x * akPoint[i].z;
        fSumYY += akPoint[i].y * akPoint[i].y;
        fSumYZ += akPoint[i].y * akPoint[i].z;
    }

    Real aafA[3][3] =
        {
            fSumXX, fSumXY, fSumX,
            fSumXY, fSumYY, fSumY,
            fSumX, fSumY, Real(iQuantity)};

    Real afB[3] =
        {
            fSumXZ,
            fSumYZ,
            fSumZ};

    Real afX[3];

    bool bNonsingular = LinearSystem::Solve3(aafA, afB, afX);
    if (bNonsingular)
    {
        rfA = afX[0];
        rfB = afX[1];
        rfC = afX[2];
    }
    else
    {
        rfA = Math::MAX_REAL;
        rfB = Math::MAX_REAL;
        rfC = Math::MAX_REAL;
    }

    return bNonsingular;
}
//----------------------------------------------------------------------------
Real Mgc::OrthogonalPlaneFit(int iQuantity, Vector3 *akPoint,
                             Vector3 &rkOffset, Vector3 &rkNormal)
{
    // compute average of points
    rkOffset = akPoint[0];
    int i;
    for (i = 1; i < iQuantity; i++)
        rkOffset += akPoint[i];
    Real fInvQuantity = 1.0f / iQuantity;
    rkOffset *= fInvQuantity;

    // compute sums of products
    Real fSumXX = 0.0f, fSumXY = 0.0f, fSumXZ = 0.0f;
    Real fSumYY = 0.0f, fSumYZ = 0.0f, fSumZZ = 0.0f;
    for (i = 0; i < iQuantity; i++)
    {
        Vector3 kDiff = akPoint[i] - rkOffset;
        fSumXX += kDiff.x * kDiff.x;
        fSumXY += kDiff.x * kDiff.y;
        fSumXZ += kDiff.x * kDiff.z;
        fSumYY += kDiff.y * kDiff.y;
        fSumYZ += kDiff.y * kDiff.z;
        fSumZZ += kDiff.z * kDiff.z;
    }

    // setup the eigensolver
    Eigen kES(3);
    kES.Matrix(0, 0) = fSumXX;
    kES.Matrix(0, 1) = fSumXY;
    kES.Matrix(0, 2) = fSumXZ;
    kES.Matrix(1, 0) = kES.Matrix(0, 1);
    kES.Matrix(1, 1) = fSumYY;
    kES.Matrix(1, 2) = fSumYZ;
    kES.Matrix(2, 0) = kES.Matrix(0, 2);
    kES.Matrix(2, 1) = kES.Matrix(1, 2);
    kES.Matrix(2, 2) = fSumZZ;

    // compute eigenstuff, smallest eigenvalue is in last position
    kES.DecrSortEigenStuff3();

    // unit-length direction for best-fit line
    rkNormal.x = kES.GetEigenvector(0, 2);
    rkNormal.y = kES.GetEigenvector(1, 2);
    rkNormal.z = kES.GetEigenvector(2, 2);

    // the minimum energy
    return kES.GetEigenvalue(2);
}
//----------------------------------------------------------------------------
