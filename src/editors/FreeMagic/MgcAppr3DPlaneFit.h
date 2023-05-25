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

#ifndef MGCAPPR3DPLANEFIT_H
#define MGCAPPR3DPLANEFIT_H

#include "MgcVector3.h"

namespace Mgc
{

    // Least-squares fit of a plane to (x,y,f(x,y)) data by using distance
    // measurements in the z-direction.  The resulting plane is represented by
    // z = A*x + B*y + C.  The return value is 'false' if the 3x3 coefficient
    // matrix in the linear system that defines A, B, and C is nearly singular.

    MAGICFM bool HeightPlaneFit(int iQuantity, Vector3 *akPoint,
                                Real &rfA, Real &rfB, Real &rfC);

    // Least-squares fit of a plane to (x,y,z) data by using distance measurements
    // orthogonal to the proposed plane.  The resulting plane is represented by
    // Normal.Dot(X - Offset) = 0.  The return value of the function is the
    // associated minimum in the function that was minimized.

    MAGICFM Real OrthogonalPlaneFit(int iQuantity, Vector3 *akPoint,
                                    Vector3 &rkOffset, Vector3 &rkNormal);

} // namespace Mgc

#endif
