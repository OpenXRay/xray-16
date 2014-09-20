// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLAPPR3DGAUSSPOINTSFITH
#define WMLAPPR3DGAUSSPOINTSFITH

// Fit points with a Gaussian distribution.  The center is the mean of the
// points, the axes are the eigenvectors of the covariance matrix, and the
// extents are the eigenvalues of the covariance matrix and are returned in
// increasing order.  The last function allows selection of valid vertices
// from a pool.  The return value is 'true' if and only if at least one
// vertex was valid.

#include "WmlVector3.h"

namespace Wml
{

template <class Real>
WML_ITEM void GaussPointsFit (int iQuantity, const Vector3<Real>* akPoint,
    Vector3<Real>& rkCenter, Vector3<Real> akAxis[3], Real afExtent[3]);

template <class Real>
WML_ITEM bool GaussPointsFit (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid, Vector3<Real>& rkCenter, Vector3<Real> akAxis[3],
    Real afExtent[3]);

}

#endif
