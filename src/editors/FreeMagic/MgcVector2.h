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

#ifndef MGCVECTOR2_H
#define MGCVECTOR2_H

#include "MgcMath.h"

namespace Mgc
{

    class MAGICFM Vector2
    {
    public:
        // construction
        Vector2();
        Vector2(Real fX, Real fY);
        Vector2(Real afCoordinate[2]);
        Vector2(const Vector2 &rkVector);

        // coordinates
        Real x, y;

        // access vector V as V[0] = V.x, V[1] = V.y
        //
        // WARNING.  These member functions rely on
        // (1) Vector2 not having virtual functions
        // (2) the data packed in a 2*sizeof(Real) memory block
        Real &operator[](int i) const;
        operator Real *();

        // assignment
        Vector2 &operator=(const Vector2 &rkVector);

        // comparison (supports fuzzy arithmetic when FUZZ > 0)
        bool operator==(const Vector2 &rkVector) const;
        bool operator!=(const Vector2 &rkVector) const;
        bool operator<(const Vector2 &rkVector) const;
        bool operator<=(const Vector2 &rkVector) const;
        bool operator>(const Vector2 &rkVector) const;
        bool operator>=(const Vector2 &rkVector) const;

        // arithmetic operations
        Vector2 operator+(const Vector2 &rkVector) const;
        Vector2 operator-(const Vector2 &rkVector) const;
        Vector2 operator*(Real fScalar) const;
        Vector2 operator/(Real fScalar) const;
        Vector2 operator-() const;
        MAGICFM friend Vector2 operator*(Real fScalar, const Vector2 &rkVector);

        // arithmetic updates
        Vector2 &operator+=(const Vector2 &rkVector);
        Vector2 &operator-=(const Vector2 &rkVector);
        Vector2 &operator*=(Real fScalar);
        Vector2 &operator/=(Real fScalar);

        // vector operations
        Real Length() const;
        Real SquaredLength() const;
        Real Dot(const Vector2 &rkVector) const;
        Real Unitize(Real fTolerance = 1e-06f);
        Vector2 Cross() const;     // returns (y,-x)
        Vector2 UnitCross() const; // returns (y,-x)/sqrt(x*x+y*y)

        // Gram-Schmidt orthonormalization.
        static void Orthonormalize(Vector2 akVector[/*2*/]);

        // special points
        static const Vector2 ZERO;
        static const Vector2 UNIT_X;
        static const Vector2 UNIT_Y;

        // fuzzy arithmetic (set FUZZ > 0 to enable)
        static Real FUZZ;
    };

#include "MgcVector2.inl"

} // namespace Mgc

#endif
