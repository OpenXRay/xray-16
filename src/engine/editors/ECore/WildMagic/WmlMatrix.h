// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLMATRIX_H
#define WMLMATRIX_H

// Matrix operations are applied on the left.  For example, given a matrix M
// and a vector V, matrix-times-vector is M*V.  That is, V is treated as a
// column vector.  Some graphics APIs use V*M where V is treated as a row
// vector.  In this context the "M" matrix is really a transpose of the M as
// represented in Wild Magic.  Similarly, to apply two matrix operations M0
// and M1, in that order, you compute M1*M0 so that the transform of a vector
// is (M1*M0)*V = M1*(M0*V).  Some graphics APIs use M0*M1, but again these
// matrices are the transpose of those as represented in Wild Magic.  You
// must therefore be careful about how you interface the transformation code
// with graphics APIS.
//
// For memory organization it might seem natural to chose Real[N][N] for the
// matrix storage, but this can be a problem on a platform/console that
// chooses to store the data in column-major rather than row-major format.
// To avoid potential portability problems, the matrix is stored as Real[N*N]
// and organized in row-major order.  That is, the entry of the matrix in row
// r (0 <= r < N) and column c (0 <= c < N) is stored at index i = c+N*r
// (0 <= i < N*N).

#include "WmlVector.h"

namespace Wml
{

template <int N, class Real>
class Matrix
{
public:
    // Construction.  In the second constructor, if bZero is 'true', the
    // matrix is set to the zero matrix.  If bZero is 'false', the matrix
    // is set to the identity matrix.
    Matrix ();
    Matrix (bool bZero);
    Matrix (const Matrix& rkM);

    void MakeZero ();
    void MakeIdentity ();
    void MakeDiagonal (const Real* afDiag);

    // member access
    operator const Real* () const;
    operator Real* ();
    const Real* operator[] (int iRow) const;
    Real* operator[] (int iRow);
    Real operator() (int iRow, int iCol) const;
    Real& operator() (int iRow, int iCol);
    void SetRow (int iRow, const Vector<N,Real>& rkV);
    Vector<N,Real> GetRow (int iRow) const;
    void SetColumn (int iCol, const Vector<N,Real>& rkV);
    Vector<N,Real> GetColumn (int iCol) const;
    void GetColumnMajor (Real* afCMajor) const;

    // assignment
    Matrix& operator= (const Matrix& rkM);

    // comparison
    bool operator== (const Matrix& rkM) const;
    bool operator!= (const Matrix& rkM) const;
    bool operator<  (const Matrix& rkM) const;
    bool operator<= (const Matrix& rkM) const;
    bool operator>  (const Matrix& rkM) const;
    bool operator>= (const Matrix& rkM) const;

    // arithmetic operations
    Matrix operator+ (const Matrix& rkM) const;
    Matrix operator- (const Matrix& rkM) const;
    Matrix operator* (const Matrix& rkM) const;
    Matrix operator* (Real fScalar) const;
    Matrix operator/ (Real fScalar) const;
    Matrix operator- () const;

    // arithmetic updates
    Matrix& operator+= (const Matrix& rkM);
    Matrix& operator-= (const Matrix& rkM);
    Matrix& operator*= (Real fScalar);
    Matrix& operator/= (Real fScalar);

    // matrix products
    Matrix Transpose () const;  // M^T
    Matrix TransposeTimes (const Matrix& rkM) const;  // this^T * M
    Matrix TimesTranspose (const Matrix& rkM) const;  // this * M^T

    // matrix-vector operations
    Vector<N,Real> operator* (const Vector<N,Real>& rkV) const;  // M * v
    Real QForm (const Vector<N,Real>& rkU, const Vector<N,Real>& rkV)
        const;  // u^T*M*v

protected:
    // for indexing into the 1D array of the matrix, iCol+N*iRow
    static int I (int iRow, int iCol);

    // support for comparisons
    int CompareArrays (const Matrix& rkM) const;

    Real m_afEntry[N*N];
};

// c * M
template <int N, class Real>
Matrix<N,Real> operator* (Real fScalar, const Matrix<N,Real>& rkM);

// v^T * M
template <int N, class Real>
Vector<N,Real> operator* (const Vector<N,Real>& rkV,
    const Matrix<N,Real>& rkM);

#include "WmlMatrix.inl"

}

#endif
