// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLVECTOR3H
#define WMLVECTOR3H

#include "WmlVector.h"

namespace Wml
{

template <class Real>
class Vector3 : public Vector<3,Real>
{
public:
    // construction
    Vector3 ();
    Vector3 (Real fX, Real fY, Real fZ);
    Vector3 (const Vector3& rkV);
    Vector3 (const Vector<3,Real>& rkV);

    // member access
    Real X () const;
    Real& X ();
    Real Y () const;
    Real& Y ();
    Real Z () const;
    Real& Z ();

    // assignment
    Vector3& operator= (const Vector3& rkV);
    Vector3& operator= (const Vector<3,Real>& rkV);

    // The cross products are computed using the right-handed rule.  Be aware
    // that some graphics APIs use a left-handed rule.  If you have to compute
    // a cross product with these functions and send the result to the API
    // that expects left-handed, you will need to change sign on the vector
    // (replace each component value c by -c).
    Vector3 Cross (const Vector3& rkV) const;
    Vector3 UnitCross (const Vector3& rkV) const;

    // Gram-Schmidt orthonormalization.  Take linearly independent vectors
    // U, V, and W and compute an orthonormal set (unit length, mutually
    // perpendicular).
    static void Orthonormalize (Vector3& rkU, Vector3& rkV, Vector3& rkW);
    static void Orthonormalize (Vector3 akV[/*3*/]);

    // Input W must be initialized to a nonzero vector, output is {U,V,W},
    // an orthonormal basis.  A hint is provided about whether or not W
    // is already unit length.
    static void GenerateOrthonormalBasis (Vector3& rkU, Vector3& rkV,
        Vector3& rkW, bool bUnitLengthW);

    // special vectors
    WML_ITEM static const Vector3 ZERO;
    WML_ITEM static const Vector3 UNIT_X;
    WML_ITEM static const Vector3 UNIT_Y;
    WML_ITEM static const Vector3 UNIT_Z;
};

#include "WmlVector3.inl"

typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

}

#endif
