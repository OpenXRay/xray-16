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

#include "MgcLinearSystem.h"
using namespace Mgc;

Real LinearSystem::ms_fTolerance = 1e-06f;

//----------------------------------------------------------------------------
Real **LinearSystem::NewMatrix(int iSize)
{
    Real **aafA = new Real *[iSize];
    assert(aafA);

    for (int iRow = 0; iRow < iSize; iRow++)
    {
        aafA[iRow] = new Real[iSize];
        memset(aafA[iRow], 0, iSize * sizeof(Real));
    }

    return aafA;
}
//----------------------------------------------------------------------------
void LinearSystem::DeleteMatrix(int iSize, Real **aafA)
{
    for (int iRow = 0; iRow < iSize; iRow++)
        delete[] aafA[iRow];
    delete[] aafA;
}
//----------------------------------------------------------------------------
Real *LinearSystem::NewVector(int iSize)
{
    Real *afB = new Real[iSize];
    assert(afB);
    memset(afB, 0, iSize * sizeof(Real));
    return afB;
}
//----------------------------------------------------------------------------
void LinearSystem::DeleteVector(Real *afB)
{
    delete[] afB;
}
//----------------------------------------------------------------------------
bool LinearSystem::Solve2(const Real aafA[2][2], const Real afB[2],
                          Real afX[2])
{
    Real fDet = aafA[0][0] * aafA[1][1] - aafA[0][1] * aafA[1][0];
    if (Math::FAbs(fDet) < ms_fTolerance)
        return false;

    Real fInvDet = 1.0f / fDet;
    afX[0] = (aafA[1][1] * afB[0] - aafA[0][1] * afB[1]) * fInvDet;
    afX[1] = (aafA[0][0] * afB[1] - aafA[1][0] * afB[0]) * fInvDet;
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::Solve3(const Real aafA[3][3], const Real afB[3],
                          Real afX[3])
{
    Real aafAInv[3][3];
    aafAInv[0][0] = aafA[1][1] * aafA[2][2] - aafA[1][2] * aafA[2][1];
    aafAInv[0][1] = aafA[0][2] * aafA[2][1] - aafA[0][1] * aafA[2][2];
    aafAInv[0][2] = aafA[0][1] * aafA[1][2] - aafA[0][2] * aafA[1][1];
    aafAInv[1][0] = aafA[1][2] * aafA[2][0] - aafA[1][0] * aafA[2][2];
    aafAInv[1][1] = aafA[0][0] * aafA[2][2] - aafA[0][2] * aafA[2][0];
    aafAInv[1][2] = aafA[0][2] * aafA[1][0] - aafA[0][0] * aafA[1][2];
    aafAInv[2][0] = aafA[1][0] * aafA[2][1] - aafA[1][1] * aafA[2][0];
    aafAInv[2][1] = aafA[0][1] * aafA[2][0] - aafA[0][0] * aafA[2][1];
    aafAInv[2][2] = aafA[0][0] * aafA[1][1] - aafA[0][1] * aafA[1][0];
    Real fDet = aafA[0][0] * aafAInv[0][0] + aafA[0][1] * aafAInv[1][0] +
                aafA[0][2] * aafAInv[2][0];
    if (Math::FAbs(fDet) < ms_fTolerance)
        return false;

    Real fInvDet = 1.0f / fDet;
    for (int iRow = 0; iRow < 3; iRow++)
    {
        for (int iCol = 0; iCol < 3; iCol++)
            aafAInv[iRow][iCol] *= fInvDet;
    }

    afX[0] = aafAInv[0][0] * afB[0] + aafAInv[0][1] * afB[1] + aafAInv[0][2] * afB[2];
    afX[1] = aafAInv[1][0] * afB[0] + aafAInv[1][1] * afB[1] + aafAInv[1][2] * afB[2];
    afX[2] = aafAInv[2][0] * afB[0] + aafAInv[2][1] * afB[1] + aafAInv[2][2] * afB[2];
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::Inverse(int iSize, Real **aafA)
{
    int *aiColIndex = new int[iSize];
    assert(aiColIndex);

    int *aiRowIndex = new int[iSize];
    assert(aiRowIndex);

    bool *abPivoted = new bool[iSize];
    assert(abPivoted);
    memset(abPivoted, 0, iSize * sizeof(bool));

    int i1, i2, iRow, iCol;
    Real fSave;

    // elimination by full pivoting
    for (int i0 = 0; i0 < iSize; i0++)
    {
        // search matrix (excluding pivoted rows) for maximum absolute entry
        Real fMax = 0.0f;
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (!abPivoted[i1])
            {
                for (i2 = 0; i2 < iSize; i2++)
                {
                    if (!abPivoted[i2])
                    {
                        Real fAbs = Math::FAbs(aafA[i1][i2]);
                        if (fAbs > fMax)
                        {
                            fMax = fAbs;
                            iRow = i1;
                            iCol = i2;
                        }
                    }
                }
            }
        }

        if (fMax == 0.0f)
        {
            // matrix is not invertible
            delete[] aiColIndex;
            delete[] aiRowIndex;
            delete[] abPivoted;
            return false;
        }

        abPivoted[iCol] = true;

        // swap rows so that A[iCol][iCol] contains the pivot entry
        if (iRow != iCol)
        {
            Real *afRowPtr = aafA[iRow];
            aafA[iRow] = aafA[iCol];
            aafA[iCol] = afRowPtr;
        }

        // keep track of the permutations of the rows
        aiRowIndex[i0] = iRow;
        aiColIndex[i0] = iCol;

        // scale the row so that the pivot entry is 1
        Real fInv = 1.0f / aafA[iCol][iCol];
        aafA[iCol][iCol] = 1.0f;
        for (i2 = 0; i2 < iSize; i2++)
            aafA[iCol][i2] *= fInv;

        // zero out the pivot column locations in the other rows
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (i1 != iCol)
            {
                fSave = aafA[i1][iCol];
                aafA[i1][iCol] = 0.0f;
                for (i2 = 0; i2 < iSize; i2++)
                    aafA[i1][i2] -= aafA[iCol][i2] * fSave;
            }
        }
    }

    // reorder rows so that A[][] stores the inverse of the original matrix
    for (i1 = iSize - 1; i1 >= 0; i1--)
    {
        if (aiRowIndex[i1] != aiColIndex[i1])
        {
            for (i2 = 0; i2 < iSize; i2++)
            {
                fSave = aafA[i2][aiRowIndex[i1]];
                aafA[i2][aiRowIndex[i1]] = aafA[i2][aiColIndex[i1]];
                aafA[i2][aiColIndex[i1]] = fSave;
            }
        }
    }

    delete[] aiColIndex;
    delete[] aiRowIndex;
    delete[] abPivoted;
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::Solve(int iSize, Real **aafA, Real *afB)
{
    int *aiColIndex = new int[iSize];
    assert(aiColIndex);

    int *aiRowIndex = new int[iSize];
    assert(aiRowIndex);

    bool *abPivoted = new bool[iSize];
    assert(abPivoted);
    memset(abPivoted, 0, iSize * sizeof(bool));

    int i1, i2, iRow, iCol;
    Real fSave;

    // elimination by full pivoting
    for (int i0 = 0; i0 < iSize; i0++)
    {
        // search matrix (excluding pivoted rows) for maximum absolute entry
        Real fMax = 0.0f;
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (!abPivoted[i1])
            {
                for (i2 = 0; i2 < iSize; i2++)
                {
                    if (!abPivoted[i2])
                    {
                        Real fAbs = Math::FAbs(aafA[i1][i2]);
                        if (fAbs > fMax)
                        {
                            fMax = fAbs;
                            iRow = i1;
                            iCol = i2;
                        }
                    }
                }
            }
        }

        if (fMax == 0.0f)
        {
            // matrix is not invertible
            delete[] aiColIndex;
            delete[] aiRowIndex;
            delete[] abPivoted;
            return false;
        }

        abPivoted[iCol] = true;

        // swap rows so that A[iCol][iCol] contains the pivot entry
        if (iRow != iCol)
        {
            Real *afRowPtr = aafA[iRow];
            aafA[iRow] = aafA[iCol];
            aafA[iCol] = afRowPtr;

            fSave = afB[iRow];
            afB[iRow] = afB[iCol];
            afB[iCol] = fSave;
        }

        // keep track of the permutations of the rows
        aiRowIndex[i0] = iRow;
        aiColIndex[i0] = iCol;

        // scale the row so that the pivot entry is 1
        Real fInv = 1.0f / aafA[iCol][iCol];
        aafA[iCol][iCol] = 1.0f;
        for (i2 = 0; i2 < iSize; i2++)
            aafA[iCol][i2] *= fInv;
        afB[iCol] *= fInv;

        // zero out the pivot column locations in the other rows
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (i1 != iCol)
            {
                fSave = aafA[i1][iCol];
                aafA[i1][iCol] = 0.0f;
                for (i2 = 0; i2 < iSize; i2++)
                    aafA[i1][i2] -= aafA[iCol][i2] * fSave;
                afB[i1] -= afB[iCol] * fSave;
            }
        }
    }

    // reorder rows so that A[][] stores the inverse of the original matrix
    for (i1 = iSize - 1; i1 >= 0; i1--)
    {
        if (aiRowIndex[i1] != aiColIndex[i1])
        {
            for (i2 = 0; i2 < iSize; i2++)
            {
                fSave = aafA[i2][aiRowIndex[i1]];
                aafA[i2][aiRowIndex[i1]] = aafA[i2][aiColIndex[i1]];
                aafA[i2][aiColIndex[i1]] = fSave;
            }
        }
    }

    delete[] aiColIndex;
    delete[] aiRowIndex;
    delete[] abPivoted;
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::SolveTri(int iSize, Real *afA, Real *afB,
                            Real *afC, Real *afR, Real *afU)
{
    if (afB[0] == 0.0f)
        return false;

    Real *afD = new Real[iSize - 1];
    assert(afD);

    Real fE = afB[0];
    Real fInvE = 1.0f / fE;
    afU[0] = afR[0] * fInvE;

    int i0, i1;
    for (i0 = 0, i1 = 1; i1 < iSize; i0++, i1++)
    {
        afD[i0] = afC[i0] * fInvE;
        fE = afB[i1] - afA[i0] * afD[i0];
        if (fE == 0.0f)
        {
            delete[] afD;
            return false;
        }
        fInvE = 1.0f / fE;
        afU[i1] = (afR[i1] - afA[i0] * afU[i0]) * fInvE;
    }

    for (i0 = iSize - 1, i1 = iSize - 2; i1 >= 0; i0--, i1--)
        afU[i1] -= afD[i1] * afU[i0];

    delete[] afD;
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::SolveConstTri(int iSize, Real fA, Real fB,
                                 Real fC, Real *afR, Real *afU)
{
    if (fB == 0.0f)
        return false;

    Real *afD = new Real[iSize - 1];
    assert(afD);

    Real fE = fB;
    Real fInvE = 1.0f / fE;
    afU[0] = afR[0] * fInvE;

    int i0, i1;
    for (i0 = 0, i1 = 1; i1 < iSize; i0++, i1++)
    {
        afD[i0] = fC * fInvE;
        fE = fB - fA * afD[i0];
        if (fE == 0.0f)
        {
            delete[] afD;
            return false;
        }
        fInvE = 1.0f / fE;
        afU[i1] = (afR[i1] - fA * afU[i0]) * fInvE;
    }
    for (i0 = iSize - 1, i1 = iSize - 2; i1 >= 0; i0--, i1--)
        afU[i1] -= afD[i1] * afU[i0];

    delete[] afD;
    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::SolveSymmetric(int iSize, Real **aafA, Real *afB)
{
    // A = L D L^t decomposition with diagonal terms of L equal to 1.  The
    // algorithm stores D terms in A[i][i] and off-diagonal L terms in
    // A[i][j] for i > j.  (G. Golub and C. Van Loan, Matrix Computations)

    const Real fTolerance = 1e-06f;

    int i0, i1;
    Real *afV = new Real[iSize];
    assert(afV);

    for (i1 = 0; i1 < iSize; i1++)
    {
        for (i0 = 0; i0 < i1; i0++)
            afV[i0] = aafA[i1][i0] * aafA[i0][i0];

        afV[i1] = aafA[i1][i1];
        for (i0 = 0; i0 < i1; i0++)
            afV[i1] -= aafA[i1][i0] * afV[i0];

        aafA[i1][i1] = afV[i1];
        if (Math::FAbs(afV[i1]) <= fTolerance)
        {
            delete[] afV;
            return false;
        }

        Real fInv = 1.0f / afV[i1];
        for (i0 = i1 + 1; i0 < iSize; i0++)
        {
            for (int i2 = 0; i2 < i1; i2++)
                aafA[i0][i1] -= aafA[i0][i2] * afV[i2];
            aafA[i0][i1] *= fInv;
        }
    }
    delete[] afV;

    // Solve Ax = B.

    // Forward substitution:  Let z = DL^t x, then Lz = B.  Algorithm
    // stores z terms in B vector.
    for (i0 = 0; i0 < iSize; i0++)
    {
        for (i1 = 0; i1 < i0; i1++)
            afB[i0] -= aafA[i0][i1] * afB[i1];
    }

    // Diagonal division:  Let y = L^t x, then Dy = z.  Algorithm stores
    // y terms in B vector.
    for (i0 = 0; i0 < iSize; i0++)
    {
        if (Math::FAbs(aafA[i0][i0]) <= fTolerance)
            return false;
        afB[i0] /= aafA[i0][i0];
    }

    // Back substitution:  Solve L^t x = y.  Algorithm stores x terms in
    // B vector.
    for (i0 = iSize - 2; i0 >= 0; i0--)
    {
        for (i1 = i0 + 1; i1 < iSize; i1++)
            afB[i0] -= aafA[i1][i0] * afB[i1];
    }

    return true;
}
//----------------------------------------------------------------------------
bool LinearSystem::SymmetricInverse(int iSize, Real **aafA,
                                    Real **aafAInv)
{
    // Same algorithm as SolveSymmetric, but applied simultaneously to
    // columns of identity matrix.

    Real *afV = new Real[iSize];
    assert(afV);

    int i0, i1;
    for (i0 = 0; i0 < iSize; i0++)
    {
        for (i1 = 0; i1 < iSize; i1++)
            aafAInv[i0][i1] = (i0 != i1 ? 0.0f : 1.0f);
    }

    for (i1 = 0; i1 < iSize; i1++)
    {
        for (i0 = 0; i0 < i1; i0++)
            afV[i0] = aafA[i1][i0] * aafA[i0][i0];

        afV[i1] = aafA[i1][i1];
        for (i0 = 0; i0 < i1; i0++)
            afV[i1] -= aafA[i1][i0] * afV[i0];

        aafA[i1][i1] = afV[i1];
        for (i0 = i1 + 1; i0 < iSize; i0++)
        {
            for (int i2 = 0; i2 < i1; i2++)
                aafA[i0][i1] -= aafA[i0][i2] * afV[i2];
            aafA[i0][i1] /= afV[i1];
        }
    }
    delete[] afV;

    for (int iCol = 0; iCol < iSize; iCol++)
    {
        // forward substitution
        for (i0 = 0; i0 < iSize; i0++)
        {
            for (i1 = 0; i1 < i0; i1++)
                aafAInv[i0][iCol] -= aafA[i0][i1] * aafAInv[i1][iCol];
        }

        // diagonal division
        const Real fTolerance = 1e-06f;
        for (i0 = 0; i0 < iSize; i0++)
        {
            if (fabs(aafA[i0][i0]) <= fTolerance)
                return false;
            aafAInv[i0][iCol] /= aafA[i0][i0];
        }

        // back substitution
        for (i0 = iSize - 2; i0 >= 0; i0--)
        {
            for (i1 = i0 + 1; i1 < iSize; i1++)
                aafAInv[i0][iCol] -= aafA[i1][i0] * aafAInv[i1][iCol];
        }
    }

    return true;
}
//----------------------------------------------------------------------------
