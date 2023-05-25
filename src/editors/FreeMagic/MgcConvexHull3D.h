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

#ifndef MGCCONVEXHULL3D_H
#define MGCCONVEXHULL3D_H

#include "MgcTriangleMesh.h"
#include "MgcVector3.h"
#include <vector>

namespace Mgc
{

    class MAGICFM ConvexHull3D
    {
    public:
        // Construction and destruction.  ConvexHull3D does not take ownership
        // of the input array.  The application is responsible for deleting it.
        ConvexHull3D(int iVQuantity, const Vector3 *akVertex);
        ~ConvexHull3D();

        // Construct the hull a point at a time.  This is an O(N^2) algorithm.
        void ByIncremental();

        // TO DO.  Implement 'void ByDivideAndConquer'.  This is an O(N log N)
        // algorithm.

        // Hull types.  The quantity and indices are interpreted as follows.
        enum
        {
            // Hull is a single point.  Quantity is 1, index array has one
            // element (index 0).
            HULL_POINT,

            // Hull is a line segment.  Quantity is 2, index array has two
            // elements that are indices to the end points of the segment.
            HULL_LINEAR,

            // Hull is a planar convex polygon.  Quantity is number of vertices,
            // index array has the indices to that number of vertices.  The
            // indices represent an ordered polygon, but since there is no
            // associated normal vector, you need to supply your own and determine
            // if the ordering is clockwise or counterclockwise relative to that
            // normal.  If you want a triangle connectivity array, you will have
            // to triangulate the polygon yourself.
            HULL_PLANAR,

            // The hull is a convex polyhedron (positive volume).  Quantity is
            // number of triangles, index array has 3 times that many elements.
            // Each triple of indices represents a triangle in the hull.  All the
            // triangles are counterclockwise ordered as you view the polyhedron
            // from the outside.
            HULL_SPATIAL
        };

        int GetType() const;
        int GetQuantity() const;
        const int *GetIndices() const;

        // Remove collinear points in edges of planar hull.  Has no effect on
        // hulls with positive volume.
        void RemoveCollinear();

        // TO DO.  Implement 'void RemoveCoplanar ()'.  The result is a convex
        // polyhedron whose faces are non-coplanar convex polygons, not
        // necessarily triangles.  After such a call, GetQuantity() should return
        // the number of faces.  GetIndices() will return an integer array that
        // consists of subarrays, each subarray corresponding to a face.  The
        // first index of the subarray is number of indices for that face, those
        // indices stored in the remainder of the subblock.

        // The 'vertex equality' epsilon is used to set Vector3::FUZZ for fuzzy
        // equality of vector components.  Two vectors (x0,y0,z0) and (x1,y1,z1)
        // are considered equal if |x1-x0| <= FUZZ, |y1-y0| <= FUZZ, and
        // |z1-z0| <= FUZZ.  Observe that FUZZ = 0 yields an exact equality test.
        // The default value is 0.0.
        static Real &VertexEqualityEpsilon();

        // The 'collinear epsilon' is used to test if three points P0, P1, and P2
        // are collinear.  This is a relative error test on the sine of the angle
        // between P1-P0 and P2-P0.  The default value is 1e-06.
        static Real &CollinearEpsilon();

        // The 'coplanar epsilon' is used to test if four points P0, P1, P2, and
        // P3 are coplanar.  This is a relative error test on the volume of the
        // tetrahedron formed by the four points.  The default value is 1e-06.
        static Real &CoplanarEpsilon();

    protected:
        // for sorting
        class SortedVertex
        {
        public:
            bool operator==(const SortedVertex &rkSV) const;
            bool operator<(const SortedVertex &rkSV) const;

            // Added to satisfy the SGI Mips Pro CC compiler that appears to be
            // instantiating this, but never using it.
            bool operator!=(const SortedVertex &rkSV) const
            {
                return !operator==(rkSV);
            }

            Vector3 m_kV;
            int m_iIndex;
        };

        typedef std::vector<SortedVertex> SVArray;

        // for collinearity and coplanaritytests
        enum
        {
            ORDER_TRIANGLE,
            ORDER_COLLINEAR_LEFT,
            ORDER_COLLINEAR_RIGHT,
            ORDER_COLLINEAR_CONTAIN,
            ORDER_POSITIVE,
            ORDER_NEGATIVE,
            ORDER_COPLANAR,
            ORDER_COPLANAR_INSIDE,
            ORDER_COPLANAR_OUTSIDE
        };

        int CollinearTest(const Vector3 &rkP, const Vector3 &rkQ0,
                          const Vector3 &rkQ1) const;

        int CoplanarTest(const Vector3 &rkP, const Vector3 &rkQ0,
                         const Vector3 &rkQ1, const Vector3 &rkQ2) const;

        // test uses the plane defined by m_kPlaneOrigin and m_kPlaneNormal
        int CoplanarTest(const Vector3 &rkP) const;

        // construct convex hull incrementally
        void MergeLinear(const SortedVertex &rkP);
        void MergePlanar(const SortedVertex &rkP);
        void MergeSpatial(const SortedVertex &rkP);

        // vertex information
        int m_iVQuantity;
        const Vector3 *m_akVertex;

        // hull information
        int m_iHQuantity;
        int *m_aiHIndex;
        int m_iHullType;

        // linear or planar hull
        SVArray m_kHullP;
        Vector3 m_kPlaneOrigin, m_kPlaneNormal;

        // spatial hull
        TriangleMesh m_kHullS;
        int m_iLastIndex;

        // tweaking parameters
        static Real ms_fVertexEqualityEpsilon;
        static Real ms_fCollinearEpsilon;
        static Real ms_fCoplanarEpsilon;
    };

#include "MgcConvexHull3D.inl"

} // namespace Mgc

#endif
