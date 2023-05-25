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

#ifndef MGCCONT3DMINBOX3_H
#define MGCCONT3DMINBOX3_H

// Compute minimum volume oriented box containing the specified points.

#include "MgcBox3.h"

namespace Mgc
{

    MAGICFM Box3 MinBox(int iQuantity, const Vector3 *akPoint);

} // namespace Mgc

#endif
