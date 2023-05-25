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

#ifndef MGCMATRIX3_H
#define MGCMATRIX3_H

#include "MgcVector3.h"

namespace Mgc
{

    // NOTE.  The (x,y,z) coordinate system is assumed to be right-handed.
    // Coordinate axis rotation matrices are of the form
    //   RX =    1       0       0
    //           0     cos(t) -sin(t)
    //           0     sin(t)  cos(t)
    // where t > 0 indicates a counterclockwise rotation in the yz-plane
    //   RY =  cos(t)    0     sin(t)
    //           0       1       0
    //        -sin(t)    0     cos(t)
    // where t > 0 indicates a counterclockwise rotation in the zx-plane
    //   RZ =  cos(t) -sin(t)    0
    //         sin(t)  cos(t)    0
    //           0       0       1
    // where t > 0 indicates a counterclockwise rotation in the xy-plane.

    class MAGICFM Matrix3
    {
    public:
        // construction
        Matrix3();
        Matrix3(const Real aafEntry[3][3]);
        Matrix3(const Matrix3 &rkMatrix);
        Matrix3(Real fEntry00, Real fEntry01, Real fEntry02,
                Real fEntry10, Real fEntry11, Real fEntry12,
                Real fEntry20, Real fEntry21, Real fEntry22);

        // member access, allows use of construct mat[r][c]
        Real *operator[](int iRow) const;
        operator Real *();
        Vector3 GetColumn(int iCol) const;

        // assignment and comparison
        Matrix3 &operator=(const Matrix3 &rkMatrix);
        bool operator==(const Matrix3 &rkMatrix) const;
        bool operator!=(const Matrix3 &rkMatrix) const;

        // arithmetic operations
        Matrix3 operator+(const Matrix3 &rkMatrix) const;
        Matrix3 operator-(const Matrix3 &rkMatrix) const;
        Matrix3 operator*(const Matrix3 &rkMatrix) const;
        Matrix3 operator-() const;

        // matrix * vector [3x3 * 3x1 = 3x1]
        Vector3 operator*(const Vector3 &rkVector) const;

        // vector * matrix [1x3 * 3x3 = 1x3]
        MAGICFM friend Vector3 operator*(const Vector3 &rkVector,
                                         const Matrix3 &rkMatrix);

        // matrix * scalar
        Matrix3 operator*(Real fScalar) const;

        // scalar * matrix
        MAGICFM friend Matrix3 operator*(Real fScalar, const Matrix3 &rkMatrix);

        // M0.TransposeTimes(M1) = M0^t*M1 where M0^t is the transpose of M0
        Matrix3 TransposeTimes(const Matrix3 &rkM) const;

        // M0.TimesTranspose(M1) = M0*M1^t where M1^t is the transpose of M1
        Matrix3 TimesTranspose(const Matrix3 &rkM) const;

        // utilities
        Matrix3 Transpose() const;
        bool Inverse(Matrix3 &rkInverse, Real fTolerance = 1e-06f) const;
        Matrix3 Inverse(Real fTolerance = 1e-06f) const;
        Real Determinant() const;

        // SLERP (spherical linear interpolation) without quaternions.  Computes
        // R(t) = R0*(Transpose(R0)*R1)^t.  If Q is a rotation matrix with
        // unit-length axis U and angle A, then Q^t is a rotation matrix with
        // unit-length axis U and rotation angle t*A.
        static Matrix3 Slerp(Real fT, const Matrix3 &rkR0, const Matrix3 &rkR1);

        // singular value decomposition
        void SingularValueDecomposition(Matrix3 &rkL, Vector3 &rkS,
                                        Matrix3 &rkR) const;
        void SingularValueComposition(const Matrix3 &rkL,
                                      const Vector3 &rkS, const Matrix3 &rkR);

        // Gram-Schmidt orthonormalization (applied to columns of rotation matrix)
        void Orthonormalize();

        // orthogonal Q, diagonal D, upper triangular U stored as (u01,u02,u12)
        void QDUDecomposition(Matrix3 &rkQ, Vector3 &rkD, Vector3 &rkU) const;

        Real SpectralNorm() const;

        // matrix must be orthonormal
        void ToAxisAngle(Vector3 &rkAxis, Real &rfRadians) const;
        void FromAxisAngle(const Vector3 &rkAxis, Real fRadians);

        // The matrix must be orthonormal.  The decomposition is yaw*pitch*roll
        // where yaw is rotation about the Up vector, pitch is rotation about the
        // Right axis, and roll is rotation about the Direction axis.
        bool ToEulerAnglesXYZ(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        bool ToEulerAnglesXZY(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        bool ToEulerAnglesYXZ(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        bool ToEulerAnglesYZX(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        bool ToEulerAnglesZXY(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        bool ToEulerAnglesZYX(Real &rfYAngle, Real &rfPAngle,
                              Real &rfRAngle) const;
        void FromEulerAnglesXYZ(Real fYAngle, Real fPAngle, Real fRAngle);
        void FromEulerAnglesXZY(Real fYAngle, Real fPAngle, Real fRAngle);
        void FromEulerAnglesYXZ(Real fYAngle, Real fPAngle, Real fRAngle);
        void FromEulerAnglesYZX(Real fYAngle, Real fPAngle, Real fRAngle);
        void FromEulerAnglesZXY(Real fYAngle, Real fPAngle, Real fRAngle);
        void FromEulerAnglesZYX(Real fYAngle, Real fPAngle, Real fRAngle);

        // eigensolver, matrix must be symmetric
        void EigenSolveSymmetric(Real afEigenvalue[3],
                                 Vector3 akEigenvector[3]) const;

        static void TensorProduct(const Vector3 &rkU, const Vector3 &rkV,
                                  Matrix3 &rkProduct);

        static const Real EPSILON;
        static const Matrix3 ZERO;
        static const Matrix3 IDENTITY;

    protected:
        // support for eigensolver
        void Tridiagonal(Real afDiag[3], Real afSubDiag[3]);
        bool QLAlgorithm(Real afDiag[3], Real afSubDiag[3]);

        // support for singular value decomposition
        static const Real ms_fSvdEpsilon;
        static const int ms_iSvdMaxIterations;
        static void Bidiagonalize(Matrix3 &kA, Matrix3 &kL, Matrix3 &kR);
        static void GolubKahanStep(Matrix3 &kA, Matrix3 &kL, Matrix3 &kR);

        // support for spectral norm
        static Real MaxCubicRoot(Real afCoeff[3]);

        Real m_aafEntry[3][3];
    };

#include "MgcMatrix3.inl"

} // namespace Mgc

#endif
