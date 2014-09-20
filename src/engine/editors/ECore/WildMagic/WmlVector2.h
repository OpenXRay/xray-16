// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLVECTOR2_H
#define WMLVECTOR2_H

#include "WmlVector.h"

namespace Wml
{

template <class Real>
class Vector2 : public Vector<2,Real>
{
public:
    // construction
    Vector2 ();
    Vector2 (Real fX, Real fY);
    Vector2 (const Vector2& rkV);
    Vector2 (const Vector<2,Real>& rkV);

    // member access
    Real X () const;
    Real& X ();
    Real Y () const;
    Real& Y ();

    // assignment
    Vector2& operator= (const Vector2& rkV);
    Vector2& operator= (const Vector<2,Real>& rkV);

    // returns (y,-x)
    Vector2 Perp () const;

    // returns (y,-x)/sqrt(x*x+y*y)
    Vector2 UnitPerp () const;

    // returns Cross((x,y,0),(V.x,V.y,0)) = x*V.y - y*V.x
    Real Kross (const Vector2& rkV) const;

    // NOTE.  These exist to support template code that takes vectors of
    // dimensions 2 or 3.  In 2D, the input vector is ignored, but allows
    // the correct signature matching in the template code.
    Vector2 Cross (const Vector2& rkV) const;
    Vector2 UnitCross (const Vector2& rkV) const;

    // Gram-Schmidt orthonormalization.  Take linearly independent vectors U
    // and V and compute an orthonormal set (unit length, mutually
    // perpendicular).
    static void Orthonormalize (Vector2& rkU, Vector2& rkV);

    // Input V must be initialized to a nonzero vector, output is {U,V}, an
    // orthonormal basis.  A hint is provided about whether or not V is
    // already unit length.
    static void GenerateOrthonormalBasis (Vector2& rkU, Vector2& rkV,
        bool bUnitLengthV);

    // special vectors
    WML_ITEM static const Vector2 ZERO;
    WML_ITEM static const Vector2 UNIT_X;
    WML_ITEM static const Vector2 UNIT_Y;
};

#include "WmlVector2.inl"

typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

}

#endif
