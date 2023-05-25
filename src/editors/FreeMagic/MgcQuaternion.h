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

#ifndef MGCQUATERNION_H
#define MGCQUATERNION_H

#include "MgcMatrix3.h"

namespace Mgc
{

    class MAGICFM Quaternion
    {
    public:
        Real w, x, y, z;

        // construction and destruction
        Quaternion(Real fW = 1.0f, Real fX = 0.0f, Real fY = 0.0f,
                   Real fZ = 0.0f);
        Quaternion(const Quaternion &rkQ);

        // conversion between quaternions, matrices, and angle-axes
        void FromRotationMatrix(const Matrix3 &kRot);
        void ToRotationMatrix(Matrix3 &kRot) const;
        void FromAngleAxis(const Real &rfAngle, const Vector3 &rkAxis);
        void ToAngleAxis(Real &rfAngle, Vector3 &rkAxis) const;
        void FromAxes(const Vector3 *akAxis);
        void ToAxes(Vector3 *akAxis) const;

        // arithmetic operations
        Quaternion &operator=(const Quaternion &rkQ);
        Quaternion operator+(const Quaternion &rkQ) const;
        Quaternion operator-(const Quaternion &rkQ) const;
        Quaternion operator*(const Quaternion &rkQ) const;
        Quaternion operator*(Real fScalar) const;
        MAGICFM friend Quaternion operator*(Real fScalar,
                                            const Quaternion &rkQ);
        Quaternion operator-() const;

        // functions of a quaternion
        Real Dot(const Quaternion &rkQ) const; // dot product
        Real Norm() const;                     // squared-length
        Quaternion Inverse() const;            // apply to non-zero quaternion
        Quaternion UnitInverse() const;        // apply to unit-length quaternion
        Quaternion Exp() const;
        Quaternion Log() const;

        // rotation of a vector by a quaternion
        Vector3 operator*(const Vector3 &rkVector) const;

        // spherical linear interpolation
        static Quaternion Slerp(Real fT, const Quaternion &rkP,
                                const Quaternion &rkQ);

        static Quaternion SlerpExtraSpins(Real fT,
                                          const Quaternion &rkP, const Quaternion &rkQ,
                                          int iExtraSpins);

        // setup for spherical quadratic interpolation
        static void Intermediate(const Quaternion &rkQ0,
                                 const Quaternion &rkQ1, const Quaternion &rkQ2,
                                 Quaternion &rka, Quaternion &rkB);

        // spherical quadratic interpolation
        static Quaternion Squad(Real fT, const Quaternion &rkP,
                                const Quaternion &rkA, const Quaternion &rkB,
                                const Quaternion &rkQ);

        // special values
        static Quaternion ZERO;
        static Quaternion IDENTITY;
    };

} // namespace Mgc

#endif
