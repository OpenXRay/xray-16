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

#ifndef MGCCONT3DCYLINDER_H
#define MGCCONT3DCYLINDER_H

#include "MgcCylinder.h"

namespace Mgc
{

    // Compute axis of capsule segment using least-squares fit.  Radius is
    // maximum distance from points to axis.  Height is determined by projection
    // of points onto axis and determining the containing interval.

    MAGICFM Cylinder ContCylinder(int iQuantity, const Vector3 *akPoint);

} // namespace Mgc

#endif
