// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLVECTOR4_H
#define WMLVECTOR4_H

#include "WmlVector.h"

namespace Wml
{

template <class Real>
class Vector4 : public Vector<4,Real>
{
public:
    // construction
    Vector4 ();
    Vector4 (Real fX, Real fY, Real fZ, Real fW);
    Vector4 (const Vector4& rkV);
    Vector4 (const Vector<4,Real>& rkV);

    // member access
    Real X () const;
    Real& X ();
    Real Y () const;
    Real& Y ();
    Real Z () const;
    Real& Z ();
    Real W () const;
    Real& W ();

    // assignment
    Vector4& operator= (const Vector4& rkV);
    Vector4& operator= (const Vector<4,Real>& rkV);

    // special vectors
    WML_ITEM static const Vector4 ZERO;
    WML_ITEM static const Vector4 UNIT_X;
    WML_ITEM static const Vector4 UNIT_Y;
    WML_ITEM static const Vector4 UNIT_Z;
    WML_ITEM static const Vector4 UNIT_W;
};

#include "WmlVector4.inl"

typedef Vector4<float> Vector4f;
typedef Vector4<double> Vector4d;

}

#endif
