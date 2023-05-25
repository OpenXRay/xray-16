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

#ifndef MGCAPPR3DLINEFIT_H
#define MGCAPPR3DLINEFIT_H

#include "MgcVector3.h"

namespace Mgc
{

    // Least-squares fit of a line to (x,y,z) data by using distance measurements
    // orthogonal to the proposed line.  The resulting line is represented by
    // Offset + t*Direction where the returned direction is a unit-length vector.

    MAGICFM void OrthogonalLineFit(int iQuantity, const Vector3 *akPoint,
                                   Vector3 &rkOffset, Vector3 &rkDirection);

    // This function allows for selection of vertices from a pool.  The return
    // value is 'true' if and only if at least one vertex is valid.

    MAGICFM bool OrthogonalLineFit(int iQuantity, const Vector3 *akPoint,
                                   const bool *abValid, Vector3 &rkOffset, Vector3 &rkDirection);

} // namespace Mgc

#endif
