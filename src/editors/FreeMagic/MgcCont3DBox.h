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

#ifndef MGCCONT3DBOX_H
#define MGCCONT3DBOX_H

#include "MgcBox3.h"

namespace Mgc
{

    MAGICFM void ContAlignedBox(int iQuantity, const Vector3 *akPoint,
                                Vector3 &rkMin, Vector3 &rkMax);

    MAGICFM Box3 ContOrientedBox(int iQuantity, const Vector3 *akPoint);

    // This function allows for selection of vertices from a pool.  The return
    //  value is 'true' if and only if at least one vertex is valid.
    MAGICFM bool ContOrientedBox(int iQuantity, const Vector3 *akPoint,
                                 const bool *abValid, Box3 &rkBox);

    // Test for containment.  Let X = C + y0*U0 + y1*U1 + y2*U2 where C is the
    // box center and U0, U1, U2 are the orthonormal axes of the box.  X is in
    // the box if |y_i| <= E_i for all i where E_i are the extents of the box.
    // If an epsilon e is supplied, the tests are |y_i| <= E_i + e.
    MAGICFM bool InBox(const Vector3 &rkPoint, const Box3 &rkBox,
                       Real fEpsilon = 0.0f);

    MAGICFM Box3 MergeBoxes(const Box3 &rkBox0, const Box3 &rkBox1);

} // namespace Mgc

#endif
