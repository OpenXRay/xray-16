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

#ifndef MGCBOX3_H
#define MGCBOX3_H

#include "MgcVector3.h"

namespace Mgc
{

    class MAGICFM Box3
    {
    public:
        Box3();

        Vector3 &Center();
        const Vector3 &Center() const;

        Vector3 &Axis(int i);
        const Vector3 &Axis(int i) const;
        Vector3 *Axes();
        const Vector3 *Axes() const;

        Real &Extent(int i);
        const Real &Extent(int i) const;
        Real *Extents();
        const Real *Extents() const;

        void ComputeVertices(Vector3 akVertex[8]) const;

    protected:
        Vector3 m_kCenter;
        Vector3 m_akAxis[3];
        Real m_afExtent[3];
    };

#include "MgcBox3.inl"

} // namespace Mgc

#endif
