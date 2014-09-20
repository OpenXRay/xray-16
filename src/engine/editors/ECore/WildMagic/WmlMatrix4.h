// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLMATRIX4_H
#define WMLMATRIX4_H

#include "WmlMatrix.h"
#include "WmlVector4.h"

namespace Wml
{

template <class Real>
class Matrix4 : public Matrix<4,Real>
{
public:
    // construction
    Matrix4 ();
    Matrix4 (const Matrix4& rkM);
    Matrix4 (const Matrix<4,Real>& rkM);

    // input Mrc is in row r, column c.
    Matrix4 (Real fM00, Real fM01, Real fM02, Real fM03,
             Real fM10, Real fM11, Real fM12, Real fM13,
             Real fM20, Real fM21, Real fM22, Real fM23,
             Real fM30, Real fM31, Real fM32, Real fM33);

    // Create a matrix from an array of numbers.  The input array is
    // interpreted based on the Boolean input as
    //   true:  entry[0..15]={m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,
    //                        m23,m30,m31,m32,m33} [row major]
    //   false: entry[0..15]={m00,m10,m20,m30,m01,m11,m21,m31,m02,m12,m22,
    //                        m32,m03,m13,m23,m33} [col major]
    Matrix4 (const Real afEntry[16], bool bRowMajor);

    // assignment
    Matrix4& operator= (const Matrix4& rkM);
    Matrix4& operator= (const Matrix<4,Real>& rkM);

    // matrix operations
    Matrix4 Inverse () const;
    Matrix4 Adjoint () const;
    Real Determinant () const;

    // special matrices
    WML_ITEM static const Matrix4 ZERO;
    WML_ITEM static const Matrix4 IDENTITY;
};

#include "WmlMatrix4.inl"

typedef Matrix4<float> Matrix4f;
typedef Matrix4<double> Matrix4d;

}

#endif
