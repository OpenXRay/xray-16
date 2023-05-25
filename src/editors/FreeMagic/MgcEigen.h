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

#ifndef MGCEIGEN_H
#define MGCEIGEN_H

#include "MgcMath.h"

namespace Mgc
{

    class MAGICFM Eigen
    {
    public:
        Eigen(int iSize);
        ~Eigen();

        // set the matrix for eigensolving
        Real &Matrix(int iRow, int iCol);
        void SetMatrix(Real **aafMat);

        // get the results of eigensolving (eigenvectors are columns of matrix)
        Real GetEigenvalue(int i) const;
        Real GetEigenvector(int iRow, int iCol) const;
        Real *GetEigenvalue();
        Real **GetEigenvector();

        // solve eigensystem
        void EigenStuff2();
        void EigenStuff3();
        void EigenStuff4();
        void EigenStuffN();
        void EigenStuff();

        // solve eigensystem, use decreasing sort on eigenvalues
        void DecrSortEigenStuff2();
        void DecrSortEigenStuff3();
        void DecrSortEigenStuff4();
        void DecrSortEigenStuffN();
        void DecrSortEigenStuff();

        // solve eigensystem, use increasing sort on eigenvalues
        void IncrSortEigenStuff2();
        void IncrSortEigenStuff3();
        void IncrSortEigenStuff4();
        void IncrSortEigenStuffN();
        void IncrSortEigenStuff();

    protected:
        int m_iSize;
        Real **m_aafMat;
        Real *m_afDiag;
        Real *m_afSubd;

        // Householder reduction to tridiagonal form
        static void Tridiagonal2(Real **aafMat, Real *afDiag, Real *afSubd);
        static void Tridiagonal3(Real **aafMat, Real *afDiag, Real *afSubd);
        static void Tridiagonal4(Real **aafMat, Real *afDiag, Real *afSubd);
        static void TridiagonalN(int iSize, Real **aafMat, Real *afDiag,
                                 Real *afSubd);

        // QL algorithm with implicit shifting, applies to tridiagonal matrices
        static bool QLAlgorithm(int iSize, Real *afDiag, Real *afSubd,
                                Real **aafMat);

        // sort eigenvalues from largest to smallest
        static void DecreasingSort(int iSize, Real *afEigval,
                                   Real **aafEigvec);

        // sort eigenvalues from smallest to largest
        static void IncreasingSort(int iSize, Real *afEigval,
                                   Real **aafEigvec);
    };

#include "MgcEigen.inl"

} // namespace Mgc

#endif
