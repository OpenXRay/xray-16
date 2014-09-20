// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLCONTMINBOX3H
#define WMLCONTMINBOX3H

// Compute minimum volume oriented box containing the specified points.

#include "WmlBox3.h"

namespace Wml
{

template <class Real>
class WML_ITEM MinBox3
{
public:
    MinBox3 (int iQuantity, const Vector3<Real>* akPoint, Box3<Real> &kBox);

private:
    static Real Volume (const Real* afAngle, void* pvData);

    static void MinimalBoxForAngles (int iQuantity,
        const Vector3<Real>* akPoint, Real afAngle[3], Box3<Real>& rkBox);

    int m_iQuantity;
    const Vector3<Real>* m_akPoint;
};

typedef MinBox3<float> MinBox3f;
typedef MinBox3<double> MinBox3d;

}

#endif
