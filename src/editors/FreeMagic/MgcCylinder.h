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

#ifndef MGCCYLINDER_H
#define MGCCYLINDER_H

#include "MgcSegment3.h"

namespace Mgc
{

    class MAGICFM Cylinder
    {
    public:
        // Cylinder line segment has end points C-(H/2)*D and C+(H/2)*D where
        // D is a unit-length vector.  H is infinity for infinite cylinder.

        Cylinder();

        Vector3 &Center();
        const Vector3 &Center() const;

        Vector3 &Direction();
        const Vector3 &Direction() const;

        Real &Height();
        Real Height() const;

        Real &Radius();
        Real Radius() const;

        // A value of 'true' means the cylinder caps (the end disks) are included
        // as part of the cylinder.  A value of 'false' means treat the cylinder
        // as hollow--the end disks are not part of the object.
        bool &Capped();
        bool Capped() const;

        Segment3 GetSegment() const;

        // Call this function to generate a coordinate system for the cylinder,
        // {U,V,W}, an orthonormal set where W is the unit-length direction of
        // the cylinder axis.  This is necessary for cylinder-cylinder
        // intersection testing to avoid creating U and V for every test.
        void GenerateCoordinateSystem();
        Vector3 &U();
        const Vector3 &U() const;
        Vector3 &V();
        const Vector3 &V() const;
        Vector3 &W();
        const Vector3 &W() const;

    protected:
        Vector3 m_kCenter;
        Vector3 m_kDirection; // W
        Vector3 m_kU, m_kV;   // U, V
        Real m_fHeight;
        Real m_fRadius;
        bool m_bCapped;
    };

#include "MgcCylinder.inl"

} // namespace Mgc

#endif
