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

#ifndef MGCVECTOR3_H
#define MGCVECTOR3_H

#include "MgcMath.h"

namespace Mgc
{

    class MAGICFM Vector3
    {
    public:
        // construction
        Vector3();
        Vector3(Real fX, Real fY, Real fZ);
        Vector3(Real afCoordinate[3]);
        Vector3(const Vector3 &rkVector);

        // coordinates
        Real x, y, z;

        // access vector V as V[0] = V.x, V[1] = V.y, V[2] = V.z
        //
        // WARNING.  These member functions rely on
        // (1) Vector3 not having virtual functions
        // (2) the data packed in a 3*sizeof(Real) memory block
        Real &operator[](int i) const;
        operator Real *();

        // assignment
        Vector3 &operator=(const Vector3 &rkVector);

        // comparison (supports fuzzy arithmetic when FUZZ > 0)
        bool operator==(const Vector3 &rkVector) const;
        bool operator!=(const Vector3 &rkVector) const;
        bool operator<(const Vector3 &rkVector) const;
        bool operator<=(const Vector3 &rkVector) const;
        bool operator>(const Vector3 &rkVector) const;
        bool operator>=(const Vector3 &rkVector) const;

        // arithmetic operations
        Vector3 operator+(const Vector3 &rkVector) const;
        Vector3 operator-(const Vector3 &rkVector) const;
        Vector3 operator*(Real fScalar) const;
        Vector3 operator/(Real fScalar) const;
        Vector3 operator-() const;
        MAGICFM friend Vector3 operator*(Real fScalar, const Vector3 &rkVector);

        // arithmetic updates
        Vector3 &operator+=(const Vector3 &rkVector);
        Vector3 &operator-=(const Vector3 &rkVector);
        Vector3 &operator*=(Real fScalar);
        Vector3 &operator/=(Real fScalar);

        // vector operations
        Real Length() const;
        Real SquaredLength() const;
        Real Dot(const Vector3 &rkVector) const;
        Real Unitize(Real fTolerance = 1e-06f);
        Vector3 Cross(const Vector3 &rkVector) const;
        Vector3 UnitCross(const Vector3 &rkVector) const;

        // Gram-Schmidt orthonormalization.
        static void Orthonormalize(Vector3 akVector[/*3*/]);

        // Input W must be initialize to a nonzero vector, output is {U,V,W}
        // an orthonormal basis.  A hint is provided about whether or not W
        // is already unit length.
        static void GenerateOrthonormalBasis(Vector3 &rkU, Vector3 &rkV,
                                             Vector3 &rkW, bool bUnitLengthW = true);

        // special points
        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;

        // fuzzy arithmetic (set FUZZ > 0 to enable)
        static Real FUZZ;
    };

#include "MgcVector3.inl"

} // namespace Mgc

#endif
