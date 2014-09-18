// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLCONTBOX3H
#define WMLCONTBOX3H

#include "WmlBox3.h"

namespace Wml
{

template <class Real>
WML_ITEM void ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint,
    Vector3<Real>& rkMin, Vector3<Real>& rkMax);

template <class Real>
WML_ITEM Box3<Real> ContOrientedBox (int iQuantity,
    const Vector3<Real>* akPoint);


// This function allows for selection of vertices from a pool.  The return
//  value is 'true' if and only if at least one vertex is valid.
template <class Real>
WML_ITEM bool ContOrientedBox (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid, Box3<Real>& rkBox);


// Test for containment.  Let X = C + y0*U0 + y1*U1 + y2*U2 where C is the
// box center and U0, U1, U2 are the orthonormal axes of the box.  X is in
// the box if |y_i| <= E_i for all i where E_i are the extents of the box.
// If an epsilon e is supplied, the tests are |y_i| <= E_i + e.
template <class Real>
WML_ITEM bool InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox,
    Real fEpsilon = (Real)0.0);

template <class Real>
WML_ITEM Box3<Real> MergeBoxes (const Box3<Real>& rkBox0,
    const Box3<Real>& rkBox1);

}

#endif
