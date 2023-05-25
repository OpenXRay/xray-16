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

#ifndef MGCDIST3DVECLIN_H
#define MGCDIST3DVECLIN_H

#include "MgcLine3.h"
#include "MgcRay3.h"
#include "MgcSegment3.h"

namespace Mgc
{

    // squared distance measurements

    MAGICFM Real SqrDistance(const Vector3 &rkPoint, const Line3 &rkLine,
                             Real *pfParam = NULL);

    MAGICFM Real SqrDistance(const Vector3 &rkPoint, const Ray3 &rkRay,
                             Real *pfParam = NULL);

    MAGICFM Real SqrDistance(const Vector3 &rkPoint, const Segment3 &rkSegment,
                             Real *pfParam = NULL);

    // distance measurements

    MAGICFM Real Distance(const Vector3 &rkPoint, const Line3 &rkLine,
                          Real *pfParam = NULL);

    MAGICFM Real Distance(const Vector3 &rkPoint, const Ray3 &rkRay,
                          Real *pfParam = NULL);

    MAGICFM Real Distance(const Vector3 &rkPoint, const Segment3 &rkSegment,
                          Real *pfParam = NULL);

} // namespace Mgc

#endif
