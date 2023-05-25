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

#ifndef MGCLINEARSYSTEM_H
#define MGCLINEARSYSTEM_H

#include "MgcMath.h"

namespace Mgc
{

    class MAGICFM LinearSystem
    {
    public:
        // 2x2 and 3x3 systems (avoids overhead of Gaussian elimination)
        static Real &Tolerance();
        static bool Solve2(const Real aafA[2][2], const Real afB[2],
                           Real afX[2]);
        static bool Solve3(const Real aafA[3][3], const Real afB[3],
                           Real afX[3]);

        // convenience for allocation, memory is zeroed out
        static Real **NewMatrix(int iSize);
        static void DeleteMatrix(int iSize, Real **aafA);
        static Real *NewVector(int iSize);
        static void DeleteVector(Real *afB);

        // Input:
        //     A[iSize][iSize], entries are A[row][col]
        // Output:
        //     return value is TRUE if successful, FALSE if pivoting failed
        //     A[iSize][iSize], inverse matrix
        static bool Inverse(int iSize, Real **aafA);

        // Input:
        //     A[iSize][iSize] coefficient matrix, entries are A[row][col]
        //     B[iSize] vector, entries are B[row]
        // Output:
        //     return value is TRUE if successful, FALSE if pivoting failed
        //     A[iSize][iSize] is inverse matrix
        //     B[iSize] is solution x to Ax = B
        static bool Solve(int iSize, Real **aafA, Real *afB);

        // Input:
        //     Matrix is tridiagonal.
        //     Lower diagonal A[iSize-1]
        //     Main  diagonal B[iSize]
        //     Upper diagonal C[iSize-1]
        //     Right-hand side R[iSize]
        // Output:
        //     return value is TRUE if successful, FALSE if pivoting failed
        //     U[iSize] is solution
        static bool SolveTri(int iSize, Real *afA, Real *afB, Real *afC,
                             Real *afR, Real *afU);

        // Input:
        //     Matrix is tridiagonal.
        //     Lower diagonal is constant, A
        //     Main  diagonal is constant, B
        //     Upper diagonal is constant, C
        //     Right-hand side Rr[iSize]
        // Output:
        //     return value is TRUE if successful, FALSE if pivoting failed
        //     U[iSize] is solution
        static bool SolveConstTri(int iSize, Real fA, Real fB, Real fC,
                                  Real *afR, Real *afU);

        // Input:
        //     A[iSize][iSize] symmetric matrix, entries are A[row][col]
        //     B[iSize] vector, entries are B[row]
        // Output:
        //     return value is TRUE if successful, FALSE if (nearly) singular
        //     decomposition A = L D L^t (diagonal terms of L are all 1)
        //         A[i][i] = entries of diagonal D
        //         A[i][j] for i > j = lower triangular part of L
        //     B[iSize] is solution to x to Ax = B
        static bool SolveSymmetric(int iSize, Real **aafA, Real *afB);

        // Input:
        //     A[iSize][iSize], entries are A[row][col]
        // Output:
        //     return value is TRUE if successful, FALSE if algorithm failed
        //     AInv[iSize][iSize], inverse matrix
        static bool SymmetricInverse(int iSize, Real **aafA, Real **aafAInv);

    protected:
        // tolerance for 2x2 and 3x3 system solving
        static Real ms_fTolerance;
    };

#include "MgcLinearSystem.inl"

} // namespace Mgc

#endif
