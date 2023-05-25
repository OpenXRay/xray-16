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

#ifndef MGCCONVEXHULL2D_H
#define MGCCONVEXHULL2D_H

#include "MgcVector2.h"
#include <vector>

namespace Mgc
{

    class MAGICFM ConvexHull2D
    {
    public:
        // Construction and destruction.  ConvexHull2D does not take ownership
        // of the input array.  The application is responsible for deleting it.
        ConvexHull2D(int iVQuantity, const Vector2 *akVertex);
        ~ConvexHull2D();

        // two different methods to compute convex hull
        void ByDivideAndConquer();
        void ByIncremental();

        // hull stored in counterclockwise order
        int GetQuantity() const;
        const int *GetIndices() const;

        // remove collinear points on hull
        void RemoveCollinear();

        // test if point is contained by hull
        bool ContainsPoint(const Vector2 &rkP) const;

        // The 'vertex equality' epsilon is used to set Vector2::FUZZ for fuzzy
        // equality of vector components.  Two vectors (x0,y0) and (x1,y1) are
        // considered equal if |x1-x0| <= FUZZ and |y1-y0| <= FUZZ.  Observe
        // that FUZZ = 0 yields an exact equality test.  The default value is 0.0.
        static Real &VertexEqualityEpsilon();

        // The 'collinear epsilon' is used to test if three points P0, P1, and P2
        // are collinear.  If A = P1-P0 and B = P2-P0, the points are collinear
        // in theory if d = A.x*B.y-A.y*B.x = 0.  For numerical robustness, the
        // test is implemented as |d|^2 <= e*|A|^2*|B|^2 where e is the collinear
        // epsilon.  The idea is that d = |Cross((A,0),(B,0))| = |A|*|B|*|sin(t)|
        // where t is the angle between A and B.  Therefore, the comparison is
        // really |sin(t)|^2 <= e, a relative error test.  The default e = 1e-06.
        static Real &CollinearEpsilon();

    protected:
        // support for O(log N) point-in-hull test
        bool SubContainsPoint(const Vector2 &rkP, int i0, int i1) const;

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

            Vector2 m_kV;
            int m_iIndex;
        };

        typedef std::vector<SortedVertex> SVArray;

        // hull dimensions
        enum
        {
            HULL_POINT,
            HULL_LINEAR,
            HULL_PLANAR
        };

        // for collinearity tests
        enum
        {
            ORDER_POSITIVE,
            ORDER_NEGATIVE,
            ORDER_COLLINEAR_LEFT,
            ORDER_COLLINEAR_RIGHT,
            ORDER_COLLINEAR_CONTAIN
        };

        int CollinearTest(const Vector2 &rkP, const Vector2 &rkQ0,
                          const Vector2 &rkQ1) const;

        // construct convex hull using divide-and-conquer
        void GetHull(int i0, int i1, const SVArray &rkSVArray, SVArray &rkHull);
        void Merge(SVArray &rkLHull, SVArray &rkRHull, SVArray &rkHull);
        void MergeLinear(const SortedVertex &rkP, SVArray &rkHull);
        void GetTangent(const SVArray &rkLHull, const SVArray &rkRHull,
                        int &riL, int &riR);

        // construct convex hull incrementally
        void MergeLinear(const SortedVertex &rkP);
        void MergePlanar(const SortedVertex &rkP);

        // vertex information
        int m_iVQuantity;
        const Vector2 *m_akVertex;

        // hull information
        int m_iHullType;
        SVArray m_kHull;

        // indices for ordered vertices of hull
        int m_iHQuantity;
        int *m_aiHIndex;

        static Real ms_fVeqEpsilon;
        static Real ms_fColEpsilon;
    };

#include "MgcConvexHull2D.inl"

} // namespace Mgc

#endif
