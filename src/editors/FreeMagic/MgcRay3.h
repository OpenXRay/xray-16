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

#ifndef MGCRAY3_H
#define MGCRAY3_H

#include "MgcVector3.h"

namespace Mgc
{

    class MAGICFM Ray3
    {
    public:
        // Ray is R(t) = P+t*D for t >= 0.  D is not necessarily unit length.
        Ray3();

        Vector3 &Origin();
        const Vector3 &Origin() const;

        Vector3 &Direction();
        const Vector3 &Direction() const;

    protected:
        Vector3 m_kOrigin;    // P
        Vector3 m_kDirection; // D
    };

#include "MgcRay3.inl"

} // namespace Mgc

#endif
