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

#ifndef MGCCONT3DMINSPHERE_H
#define MGCCONT3DMINSPHERE_H

// Compute the minimum volume sphere containing the input set of points.  The
// algorithm randomly permutes the input points so that the construction
// occurs in 'expected' O(N) time.

#include "MgcSphere.h"

namespace Mgc
{

    MAGICFM Sphere MinSphere(int iQuantity, const Vector3 *akPoint);

} // namespace Mgc

#endif
