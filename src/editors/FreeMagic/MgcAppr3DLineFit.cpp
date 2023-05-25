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

#include "MgcEigen.h"
#include "MgcAppr3DLineFit.h"
using namespace Mgc;

//----------------------------------------------------------------------------
void Mgc::OrthogonalLineFit(int iQuantity, const Vector3 *akPoint,
                            Vector3 &rkOffset, Vector3 &rkDirection)
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
    kES.Matrix(0, 0) = fSumYY + fSumZZ;
    kES.Matrix(0, 1) = -fSumXY;
    kES.Matrix(0, 2) = -fSumXZ;
    kES.Matrix(1, 0) = kES.Matrix(0, 1);
    kES.Matrix(1, 1) = fSumXX + fSumZZ;
    kES.Matrix(1, 2) = -fSumYZ;
    kES.Matrix(2, 0) = kES.Matrix(0, 2);
    kES.Matrix(2, 1) = kES.Matrix(1, 2);
    kES.Matrix(2, 2) = fSumXX + fSumYY;

    // compute eigenstuff, smallest eigenvalue is in last position
    kES.DecrSortEigenStuff3();

    // unit-length direction for best-fit line
    rkDirection.x = kES.GetEigenvector(0, 2);
    rkDirection.y = kES.GetEigenvector(1, 2);
    rkDirection.z = kES.GetEigenvector(2, 2);
}
//----------------------------------------------------------------------------
bool Mgc::OrthogonalLineFit(int iQuantity, const Vector3 *akPoint,
                            const bool *abValid, Vector3 &rkOffset, Vector3 &rkDirection)
{
    // compute average of points
    rkOffset = Vector3::ZERO;
    int i, iValidQuantity = 0;
    for (i = 0; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            rkOffset += akPoint[i];
            iValidQuantity++;
        }
    }
    if (iValidQuantity == 0)
        return false;

    Real fInvQuantity = 1.0f / iQuantity;
    rkOffset *= fInvQuantity;

    // compute sums of products
    Real fSumXX = 0.0f, fSumXY = 0.0f, fSumXZ = 0.0f;
    Real fSumYY = 0.0f, fSumYZ = 0.0f, fSumZZ = 0.0f;
    for (i = 0; i < iQuantity; i++)
    {
        if (abValid[i])
        {
            Vector3 kDiff = akPoint[i] - rkOffset;
            fSumXX += kDiff.x * kDiff.x;
            fSumXY += kDiff.x * kDiff.y;
            fSumXZ += kDiff.x * kDiff.z;
            fSumYY += kDiff.y * kDiff.y;
            fSumYZ += kDiff.y * kDiff.z;
            fSumZZ += kDiff.z * kDiff.z;
        }
    }

    // setup the eigensolver
    Eigen kES(3);
    kES.Matrix(0, 0) = fSumYY + fSumZZ;
    kES.Matrix(0, 1) = -fSumXY;
    kES.Matrix(0, 2) = -fSumXZ;
    kES.Matrix(1, 0) = kES.Matrix(0, 1);
    kES.Matrix(1, 1) = fSumXX + fSumZZ;
    kES.Matrix(1, 2) = -fSumYZ;
    kES.Matrix(2, 0) = kES.Matrix(0, 2);
    kES.Matrix(2, 1) = kES.Matrix(1, 2);
    kES.Matrix(2, 2) = fSumXX + fSumYY;

    // compute eigenstuff, smallest eigenvalue is in last position
    kES.DecrSortEigenStuff3();

    // unit-length direction for best-fit line
    rkDirection.x = kES.GetEigenvector(0, 2);
    rkDirection.y = kES.GetEigenvector(1, 2);
    rkDirection.z = kES.GetEigenvector(2, 2);

    return true;
}
//----------------------------------------------------------------------------
