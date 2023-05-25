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

#ifndef MGCAPPR3DGAUSSPOINTSFIT_H
#define MGCAPPR3DGAUSSPOINTSFIT_H

// Fit points with a Gaussian distribution.  The center is the mean of the
// points, the axes are the eigenvectors of the covariance matrix, and the
// extents are the eigenvalues of the covariance matrix and are returned in
// increasing order.  The last function allows selection of valid vertices
// from a pool.  The return value is 'true' if and only if at least one
// vertex was valid.

#include "MgcVector3.h"

namespace Mgc
{

    MAGICFM void GaussPointsFit(int iQuantity, const Vector3 *akPoint,
                                Vector3 &rkCenter, Vector3 akAxis[3], Real afExtent[3]);

    MAGICFM bool GaussPointsFit(int iQuantity, const Vector3 *akPoint,
                                const bool *abValid, Vector3 &rkCenter, Vector3 akAxis[3],
                                Real afExtent[3]);

} // namespace Mgc

#endif
