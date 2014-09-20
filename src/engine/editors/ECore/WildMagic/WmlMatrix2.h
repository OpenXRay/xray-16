// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLMATRIX2_H
#define WMLMATRIX2_H

// Rotation matrices are of the form
//   R = cos(t) -sin(t)
//       sin(t)  cos(t)
// where t > 0 indicates a counterclockwise rotation in the xy-plane.

#include "WmlMatrix.h"
#include "WmlVector2.h"

namespace Wml
{

template <class Real>
class Matrix2 : public Matrix<2,Real>
{
public:
    // construction
    Matrix2 ();
    Matrix2 (const Matrix2& rkM);
    Matrix2 (const Matrix<2,Real>& rkM);

    // input Mrc is in row r, column c.
    Matrix2 (Real fM00, Real fM01, Real fM10, Real fM11);

    // Create a matrix from an array of numbers.  The input array is
    // interpreted based on the Boolean input as
    //   true:  entry[0..3] = {m00,m01,m10,m11}  [row major]
    //   false: entry[0..3] = {m00,m10,m01,m11}  [column major]
    Matrix2 (const Real afEntry[4], bool bRowMajor);

    // Create matrices based on vector input.  The Boolean is interpreted as
    //   true: vectors are columns of the matrix
    //   false: vectors are rows of the matrix
    Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV,
        bool bColumns);
    Matrix2 (const Vector2<Real>* akV, bool bColumns);

    // create a tensor product U*V^T
    Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV);
    void MakeTensorProduct (const Vector2<Real>& rkU,
        const Vector2<Real>& rkV);

    // create a diagonal matrix
    Matrix2 (Real fM00, Real fM11);
    void MakeDiagonal (Real fM00, Real fM11);

    // create a rotation matrix (positive angle - counterclockwise)
    Matrix2 (Real fAngle);
    void FromAngle (Real fAngle);

    // assignment
    Matrix2& operator= (const Matrix2& rkM);
    Matrix2& operator= (const Matrix<2,Real>& rkM);

    // matrix operations
    Matrix2 Inverse () const;
    Matrix2 Adjoint () const;
    Real Determinant () const;

    // The matrix must be a rotation for these functions to be valid.  The
    // last function uses Gram-Schmidt orthonormalization applied to the
    // columns of the rotation matrix.  The angle must be in radians, not
    // degrees.
    void ToAngle (Real& rfAngle) const;
    void Orthonormalize ();

    // The matrix must be symmetric.  Factor M = R * D * R^T where
    // R = [u0|u1] is a rotation matrix with columns u0 and u1 and
    // D = diag(d0,d1) is a diagonal matrix whose diagonal entries are d0 and
    // d1.  The eigenvector u[i] corresponds to eigenvector d[i].  The
    // eigenvalues are ordered as d0 <= d1.
    void EigenDecomposition (Matrix2& rkRot, Matrix2& rkDiag) const;

    WML_ITEM static const Matrix2 ZERO;
    WML_ITEM static const Matrix2 IDENTITY;
};

#include "WmlMatrix2.inl"

typedef Matrix2<float> Matrix2f;
typedef Matrix2<double> Matrix2d;

}

#endif
