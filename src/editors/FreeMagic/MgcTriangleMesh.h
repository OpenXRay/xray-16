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

#ifndef MGCTRIANGLEMESH_H
#define MGCTRIANGLEMESH_H

#include "MagicFMLibType.h"
#include "MgcMath.h"
#include "MgcSmallSet.h"
#include <map>
#include <set>
#include <vector>

namespace Mgc
{

    class MAGICFM TriangleMesh
    {
    public:
        // vertex is <v>
        // edge is <v0,v1> where v0 = min(v0,v1)
        // triangle is <v0,v1,v2> where v0 = min(v0,v1,v2)

        class Edge
        {
        public:
            Edge();
            Edge(int iV0, int iV1);
            bool operator<(const Edge &rkE) const;
            bool operator==(const Edge &rkE) const;
            bool operator!=(const Edge &rkE) const;

            int m_aiV[2];
        };

        class Triangle
        {
        public:
            Triangle();
            Triangle(int iV0, int iV1, int iV2);
            bool operator<(const Triangle &rkT) const;
            bool operator==(const Triangle &rkT) const;
            bool operator!=(const Triangle &rkT) const;

            int m_aiV[3];
        };

        class VertexAttribute
        {
        public:
            VertexAttribute()
                : m_kESet(8, 8),
                  m_kTSet(8, 8)
            {
                m_pvData = NULL;
            }

            void *m_pvData;
            SmallSet<Edge> m_kESet;
            SmallSet<Triangle> m_kTSet;
        };

        class EdgeAttribute
        {
        public:
            EdgeAttribute()
                : m_kTSet(2, 2)
            {
                m_pvData = NULL;
            }

            void *m_pvData;
            SmallSet<Triangle> m_kTSet;
        };

        class TriangleAttribute
        {
        public:
            TriangleAttribute()
            {
                m_pvData = NULL;
            }

            void *m_pvData;
        };

        // for readability of the code
        typedef std::map<int, VertexAttribute> VMap;
        typedef VMap::iterator MVIter;
        typedef VMap::const_iterator MVCIter;
        typedef std::map<Edge, EdgeAttribute> EMap;
        typedef EMap::iterator MEIter;
        typedef EMap::const_iterator MECIter;
        typedef std::map<Triangle, TriangleAttribute> TMap;
        typedef TMap::iterator MTIter;
        typedef TMap::const_iterator MTCIter;

        // construction and destruction
        TriangleMesh();
        virtual ~TriangleMesh();

        // accessors for sizes
        int GetVertexQuantity() const;
        int GetEdgeQuantity() const;
        int GetTriangleQuantity() const;

        // Used for operations that create new meshes from the current one.  This
        // allows derived class construction within the base class operations.
        virtual TriangleMesh *Create() const;

        // Callbacks that are executed when vertices, edges, or triangles are
        // inserted or removed during triangle insertion, triangle removal, or
        // edge collapsing.  The default behavior for the creation is to return
        // NULL pointers.  A derived class may override the creation and return
        // data to be attached to the specific mesh component.  The default
        // behavior for the destruction is to do nothing.  A derived class may
        // override the destruction and handle the data that was detached from
        // the specific mesh component before its destruction.
        virtual void OnVertexInsert(int iV, bool bCreate, void *&rpvData);
        virtual void OnVertexRemove(int iV, bool bDestroy, void *pvData);
        virtual void OnEdgeInsert(const Edge &rkE, bool bCreate, void *&rpvData);
        virtual void OnEdgeRemove(const Edge &rkE, bool bDestroy, void *pvData);
        virtual void OnTriangleInsert(const Triangle &rkT, bool bCreate,
                                      void *&rpvData);
        virtual void OnTriangleRemove(const Triangle &rkT, bool bDestroy,
                                      void *pvData);

        // Insert and remove triangles.  The functions are virtual in case a
        // derived class wants to implement pre- and post-operation semantics.
        virtual void InsertTriangle(int iV0, int iV1, int iV2);
        virtual void InsertTriangle(const Triangle &rkT);
        virtual void RemoveTriangle(int iV0, int iV1, int iV2);
        virtual void RemoveTriangle(const Triangle &rkT);

        // This should be called before Mesh destruction if a derived class has
        // allocated vertex, edge, or triangle data and attached it to the mesh
        // components.  Since the creation and destruction callbacks are virtual,
        // any insert/remove operations in the base Mesh destructor will only
        // call the base virtual callbacks, not any derived-class ones.  An
        // alternative to calling this is that the derived class maintain enough
        // information to know which data objects to destroy during its own
        // destructor call.
        virtual void RemoveAllTriangles();

        // write the mesh to an ASCII file
        void Print(const char *acFilename) const;

        // vertex attributes
        const VMap &GetVertexMap() const;
        void GetVertices(std::set<int> &rkVSet) const;
        void *GetData(int iV);
        const SmallSet<Edge> *GetEdges(int iV) const;
        const SmallSet<Triangle> *GetTriangles(int iV) const;

        // edge attributes
        const EMap &GetEdgeMap() const;
        void GetEdges(std::set<Edge> &rkESet) const;
        void *GetData(int iV0, int iV1);
        void *GetData(const Edge &rkE);
        const SmallSet<Triangle> *GetTriangles(int iV0, int iV1) const;

        // triangle attributes
        const TMap &GetTriangleMap() const;
        void GetTriangles(std::set<Triangle> &rkTSet) const;
        void *GetData(int iV0, int iV1, int iV2);
        void *GetData(const Triangle &rkT);

        // The mesh is manifold if each edge has at most two adjacent triangles.
        // It is possible that the mesh has multiple connected components.
        bool IsManifold() const;

        // The mesh is closed if each edge has exactly two adjacent triangles.
        // It is possible that the mesh has multiple connected components.
        bool IsClosed() const;

        // The mesh is connected if each triangle can be reached from any other
        // triangle by a traversal.
        bool IsConnected() const;

        // Extract the connected components from the mesh.  For large data sets,
        // the array of TriangleMesh can use a lot of memory.  Instead use the
        // second form that just stores a sorted connectivity array.  Let N be
        // the number of components.  The value Index[i] indicates the starting
        // index for component i with 0 <= i < N, so it is always the case that
        // Index[0] = 0.  The value Index[N] is the total number of indices in
        // the raiConnect array.  The quantity of indices for component i is
        // Q(i) = Index[i+1]-Index[i] for 0 <= i < N.  The application is
        // responsible for deleting raiConnect.
        void GetComponents(std::vector<TriangleMesh *> &rkComponents);
        void GetComponents(std::vector<int> &rkIndex, int *&raiConnect);

        // Extract a connected component from the mesh and remove all the
        // triangles of the component from the mesh.  This is useful for computing
        // the components in a very large mesh that uses a lot of memory.  The
        // intention is that the function is called until all components are
        // found.  The typical code is
        //
        //     TriangleMesh kMesh = <some mesh>;
        //     int iITotalQuantity = 3*kMesh.GetTriangleQuantity();
        //     int* aiConnect = new int[iITotalQuantity];
        //     for (int iIQuantity = 0; iIQuantity < iITotalQuantity; /**/ )
        //     {
        //         int iCurrentIQuantity;
        //         int* aiCurrentConnect = aiConnect + iIQuantity;
        //         kMesh.RemoveComponent(iCurrentIQuantity,aiCurrentConnect);
        //         iIQuantity += iCurrentIQuantity;
        //     }
        void RemoveComponent(int &riIQuantity, int *aiConnect);

        // Extract the connected components from the mesh, but each component has
        // a consistent ordering across all triangles of that component.  The
        // mesh must be manifold.  The return value is 'true' if and only if the
        // mesh is manifold.  If the mesh has multiple components, each component
        // will have a consistent ordering.  However, the mesh knows nothing about
        // the mesh geometry, so it is possible that ordering across components is
        // not consistent.  For example, if the mesh has two disjoint closed
        // manifold components, one of them could have an ordering that implies
        // outward pointing normals and the other inward pointing normals.
        //
        // NOTE.  It is possible to create a nonorientable mesh such as a Moebius
        // strip.  In this case, GetConsistentComponents will return connected
        // components, but in fact the triangles will not (and can not) be
        // consistently ordered.
        bool GetConsistentComponents(std::vector<TriangleMesh *> &rkComponents);

        // Reverse the ordering of all triangles in the mesh.
        TriangleMesh *GetReversedOrderMesh() const;

        // statistics
        void GetStatistics(int &riVQuantity, int &riEQuantity, int &riTQuantity,
                           Real &rfAverageEdgesPerVertex, Real &rfAverageTrianglesPerVertex,
                           Real &rfAverageTrianglesPerEdge, int &riMaximumEdgesPerVertex,
                           int &riMaximumTrianglesPerVertex, int &riMaximumTrianglesPerEdge);

    protected:
        std::map<int, VertexAttribute> m_kVMap;
        std::map<Edge, EdgeAttribute> m_kEMap;
        std::map<Triangle, TriangleAttribute> m_kTMap;
    };

#include "MgcTriangleMesh.inl"

} // namespace Mgc

#endif
