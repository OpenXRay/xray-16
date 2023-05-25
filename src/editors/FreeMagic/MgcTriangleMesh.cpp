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

#include "MgcTriangleMesh.h"
using namespace Mgc;

#include <fstream>
#include <set>
#include <stack>
using namespace std;

#ifdef SGI_MIPSPRO
// Instantiations to support dynamic link libraries.  This appears to be
// necessary on the SGI running IRIX and using the MIPSPRO CC compiler.
template class Mgc::SmallSet<TriangleMesh::Edge>;
template class Mgc::SmallSet<TriangleMesh::Triangle>;
#endif

//----------------------------------------------------------------------------
TriangleMesh::TriangleMesh()
{
}
//----------------------------------------------------------------------------
TriangleMesh::~TriangleMesh()
{
}
//----------------------------------------------------------------------------
void TriangleMesh::InsertTriangle(int iV0, int iV1, int iV2)
{
    Triangle kT(iV0, iV1, iV2);
    Edge kE0(iV0, iV1), kE1(iV1, iV2), kE2(iV2, iV0);

    // insert triangle
    pair<MTIter, bool> kRT = m_kTMap.insert(make_pair(kT, TriangleAttribute()));

    // insert vertices
    pair<MVIter, bool> kRV0 = m_kVMap.insert(make_pair(iV0, VertexAttribute()));
    kRV0.first->second.m_kESet.Insert(kE0);
    kRV0.first->second.m_kESet.Insert(kE2);
    kRV0.first->second.m_kTSet.Insert(kT);

    pair<MVIter, bool> kRV1 = m_kVMap.insert(make_pair(iV1, VertexAttribute()));
    kRV1.first->second.m_kESet.Insert(kE0);
    kRV1.first->second.m_kESet.Insert(kE1);
    kRV1.first->second.m_kTSet.Insert(kT);

    pair<MVIter, bool> kRV2 = m_kVMap.insert(make_pair(iV2, VertexAttribute()));
    kRV2.first->second.m_kESet.Insert(kE1);
    kRV2.first->second.m_kESet.Insert(kE2);
    kRV2.first->second.m_kTSet.Insert(kT);

    // insert edges
    pair<MEIter, bool> kRE0 = m_kEMap.insert(make_pair(kE0, EdgeAttribute()));
    kRE0.first->second.m_kTSet.Insert(kT);

    pair<MEIter, bool> kRE1 = m_kEMap.insert(make_pair(kE1, EdgeAttribute()));
    kRE1.first->second.m_kTSet.Insert(kT);

    pair<MEIter, bool> kRE2 = m_kEMap.insert(make_pair(kE2, EdgeAttribute()));
    kRE2.first->second.m_kTSet.Insert(kT);

    // Notify derived classes that mesh components have been inserted.  The
    // notification occurs here to make sure the derived classes have access
    // to the current state of the mesh after the triangle insertion.
    OnVertexInsert(iV0, kRV0.second, kRV0.first->second.m_pvData);
    OnVertexInsert(iV1, kRV1.second, kRV1.first->second.m_pvData);
    OnVertexInsert(iV2, kRV2.second, kRV2.first->second.m_pvData);
    OnEdgeInsert(kE0, kRE0.second, kRE0.first->second.m_pvData);
    OnEdgeInsert(kE1, kRE1.second, kRE1.first->second.m_pvData);
    OnEdgeInsert(kE2, kRE2.second, kRE2.first->second.m_pvData);
    OnTriangleInsert(kT, kRT.second, kRT.first->second.m_pvData);
}
//----------------------------------------------------------------------------
void TriangleMesh::InsertTriangle(const Triangle &rkT)
{
    InsertTriangle(rkT.m_aiV[0], rkT.m_aiV[1], rkT.m_aiV[2]);
}
//----------------------------------------------------------------------------
void TriangleMesh::RemoveTriangle(int iV0, int iV1, int iV2)
{
    // remove triangle
    Triangle kT(iV0, iV1, iV2);
    MTIter pkT = m_kTMap.find(kT);
    if (pkT == m_kTMap.end())
    {
        // triangle does not exist, nothing to do
        return;
    }

    // update edges
    Edge kE0(iV0, iV1), kE1(iV1, iV2), kE2(iV2, iV0);

    MEIter pkE0 = m_kEMap.find(kE0);
    assert(pkE0 != m_kEMap.end());
    pkE0->second.m_kTSet.Remove(kT);

    MEIter pkE1 = m_kEMap.find(kE1);
    assert(pkE1 != m_kEMap.end());
    pkE1->second.m_kTSet.Remove(kT);

    MEIter pkE2 = m_kEMap.find(kE2);
    assert(pkE2 != m_kEMap.end());
    pkE2->second.m_kTSet.Remove(kT);

    // update vertices
    MVIter pkV0 = m_kVMap.find(iV0);
    assert(pkV0 != m_kVMap.end());
    pkV0->second.m_kTSet.Remove(kT);

    MVIter pkV1 = m_kVMap.find(iV1);
    assert(pkV1 != m_kVMap.end());
    pkV1->second.m_kTSet.Remove(kT);

    MVIter pkV2 = m_kVMap.find(iV2);
    assert(pkV2 != m_kVMap.end());
    pkV2->second.m_kTSet.Remove(kT);

    if (pkE0->second.m_kTSet.GetSize() == 0)
    {
        pkV0->second.m_kESet.Remove(kE0);
        pkV1->second.m_kESet.Remove(kE0);
    }

    if (pkE1->second.m_kTSet.GetSize() == 0)
    {
        pkV1->second.m_kESet.Remove(kE1);
        pkV2->second.m_kESet.Remove(kE1);
    }

    if (pkE2->second.m_kTSet.GetSize() == 0)
    {
        pkV0->second.m_kESet.Remove(kE2);
        pkV2->second.m_kESet.Remove(kE2);
    }

    // Notify derived classes that mesh components are about to be destroyed.
    // The notification occurs here to make sure the derived classes have
    // access to the current state of the mesh before the triangle removal.

    bool bDestroy = pkV0->second.m_kESet.GetSize() == 0 &&
                    pkV0->second.m_kTSet.GetSize() == 0;
    OnVertexRemove(iV0, bDestroy, pkV0->second.m_pvData);
    if (bDestroy)
        m_kVMap.erase(iV0);

    bDestroy = pkV1->second.m_kESet.GetSize() == 0 &&
               pkV1->second.m_kTSet.GetSize() == 0;
    OnVertexRemove(iV1, bDestroy, pkV1->second.m_pvData);
    if (bDestroy)
        m_kVMap.erase(iV1);

    bDestroy = pkV2->second.m_kESet.GetSize() == 0 &&
               pkV2->second.m_kTSet.GetSize() == 0;
    OnVertexRemove(iV2, bDestroy, pkV2->second.m_pvData);
    if (bDestroy)
        m_kVMap.erase(iV2);

    bDestroy = pkE0->second.m_kTSet.GetSize() == 0;
    OnEdgeRemove(kE0, bDestroy, pkE0->second.m_pvData);
    if (bDestroy)
        m_kEMap.erase(kE0);

    bDestroy = pkE1->second.m_kTSet.GetSize() == 0;
    OnEdgeRemove(kE1, bDestroy, pkE1->second.m_pvData);
    if (bDestroy)
        m_kEMap.erase(kE1);

    bDestroy = pkE2->second.m_kTSet.GetSize() == 0;
    OnEdgeRemove(kE2, bDestroy, pkE2->second.m_pvData);
    if (bDestroy)
        m_kEMap.erase(kE2);

    OnTriangleRemove(kT, true, pkT->second.m_pvData);
    m_kTMap.erase(kT);
}
//----------------------------------------------------------------------------
void TriangleMesh::RemoveTriangle(const Triangle &rkT)
{
    RemoveTriangle(rkT.m_aiV[0], rkT.m_aiV[1], rkT.m_aiV[2]);
}
//----------------------------------------------------------------------------
void TriangleMesh::RemoveAllTriangles()
{
    MTIter pkT = m_kTMap.begin();
    while (pkT != m_kTMap.end())
    {
        int iV0 = pkT->first.m_aiV[0];
        int iV1 = pkT->first.m_aiV[1];
        int iV2 = pkT->first.m_aiV[2];
        pkT++;

        RemoveTriangle(iV0, iV1, iV2);
    }
}
//----------------------------------------------------------------------------
void TriangleMesh::Print(const char *acFilename) const
{
    ofstream kOStr(acFilename);
    int i;

    // print vertices
    kOStr << "vertex quantity = " << m_kVMap.size() << endl;
    for (MVCIter pkVM = m_kVMap.begin(); pkVM != m_kVMap.end(); pkVM++)
    {
        kOStr << "v<" << pkVM->first << "> : e ";

        const SmallSet<Edge> &rkESet = pkVM->second.m_kESet;
        for (i = 0; i < rkESet.GetSize(); i++)
        {
            kOStr << '<' << rkESet[i].m_aiV[0]
                  << ',' << rkESet[i].m_aiV[1]
                  << "> ";
        }

        kOStr << ": t ";
        const SmallSet<Triangle> &rkTSet = pkVM->second.m_kTSet;
        for (i = 0; i < rkTSet.GetSize(); i++)
        {
            kOStr << '<' << rkTSet[i].m_aiV[0]
                  << ',' << rkTSet[i].m_aiV[1]
                  << ',' << rkTSet[i].m_aiV[2]
                  << "> ";
        }
        kOStr << endl;
    }
    kOStr << endl;

    // print edges
    kOStr << "edge quantity = " << m_kEMap.size() << endl;
    for (MECIter pkEM = m_kEMap.begin(); pkEM != m_kEMap.end(); pkEM++)
    {
        kOStr << "e<" << pkEM->first.m_aiV[0] << ',' << pkEM->first.m_aiV[1];
        kOStr << "> : t ";
        const SmallSet<Triangle> &rkTSet = pkEM->second.m_kTSet;
        for (i = 0; i < rkTSet.GetSize(); i++)
        {
            kOStr << '<' << rkTSet[i].m_aiV[0]
                  << ',' << rkTSet[i].m_aiV[1]
                  << ',' << rkTSet[i].m_aiV[2]
                  << "> ";
        }
        kOStr << endl;
    }
    kOStr << endl;

    // print triangles
    kOStr << "triangle quantity = " << m_kTMap.size() << endl;
    for (MTCIter pkTM = m_kTMap.begin(); pkTM != m_kTMap.end(); pkTM++)
    {
        kOStr << "t<" << pkTM->first.m_aiV[0] << ',' << pkTM->first.m_aiV[1];
        kOStr << ',' << pkTM->first.m_aiV[2] << ">" << endl;
    }
    kOStr << endl;
}
//----------------------------------------------------------------------------
void TriangleMesh::GetVertices(set<int> &rkVSet) const
{
    rkVSet.clear();
    for (MVCIter pkV = m_kVMap.begin(); pkV != m_kVMap.end(); pkV++)
        rkVSet.insert(pkV->first);
}
//----------------------------------------------------------------------------
void *TriangleMesh::GetData(int iV)
{
    MVIter pkV = m_kVMap.find(iV);
    return (pkV != m_kVMap.end() ? pkV->second.m_pvData : NULL);
}
//----------------------------------------------------------------------------
const SmallSet<TriangleMesh::Edge> *TriangleMesh::GetEdges(int iV) const
{
    MVCIter pkV = m_kVMap.find(iV);
    return (pkV != m_kVMap.end() ? &pkV->second.m_kESet : NULL);
}
//----------------------------------------------------------------------------
const SmallSet<TriangleMesh::Triangle> *TriangleMesh::GetTriangles(int iV)
    const
{
    MVCIter pkV = m_kVMap.find(iV);
    return (pkV != m_kVMap.end() ? &pkV->second.m_kTSet : NULL);
}
//----------------------------------------------------------------------------
void TriangleMesh::GetEdges(set<Edge> &rkESet) const
{
    rkESet.clear();
    for (MECIter pkE = m_kEMap.begin(); pkE != m_kEMap.end(); pkE++)
        rkESet.insert(pkE->first);
}
//----------------------------------------------------------------------------
void *TriangleMesh::GetData(int iV0, int iV1)
{
    MEIter pkE = m_kEMap.find(Edge(iV0, iV1));
    return (pkE != m_kEMap.end() ? pkE->second.m_pvData : NULL);
}
//----------------------------------------------------------------------------
void *TriangleMesh::GetData(const Edge &rkE)
{
    return GetData(rkE.m_aiV[0], rkE.m_aiV[1]);
}
//----------------------------------------------------------------------------
const SmallSet<TriangleMesh::Triangle> *TriangleMesh::GetTriangles(int iV0,
                                                                   int iV1) const
{
    MECIter pkE = m_kEMap.find(Edge(iV0, iV1));
    return (pkE != m_kEMap.end() ? &pkE->second.m_kTSet : NULL);
}
//----------------------------------------------------------------------------
void TriangleMesh::GetTriangles(set<Triangle> &rkTSet) const
{
    rkTSet.clear();
    for (MTCIter pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
        rkTSet.insert(pkT->first);
}
//----------------------------------------------------------------------------
void *TriangleMesh::GetData(int iV0, int iV1, int iV2)
{
    MTIter pkT = m_kTMap.find(Triangle(iV0, iV1, iV2));
    return (pkT != m_kTMap.end() ? pkT->second.m_pvData : NULL);
}
//----------------------------------------------------------------------------
void *TriangleMesh::GetData(const Triangle &rkT)
{
    return GetData(rkT.m_aiV[0], rkT.m_aiV[1], rkT.m_aiV[2]);
}
//----------------------------------------------------------------------------
bool TriangleMesh::IsManifold() const
{
    for (MECIter pkE = m_kEMap.begin(); pkE != m_kEMap.end(); pkE++)
    {
        if (pkE->second.m_kTSet.GetSize() > 2)
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------
bool TriangleMesh::IsClosed() const
{
    for (MECIter pkE = m_kEMap.begin(); pkE != m_kEMap.end(); pkE++)
    {
        if (pkE->second.m_kTSet.GetSize() != 2)
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------
bool TriangleMesh::IsConnected() const
{
    // Do a depth-first search of the mesh.  It is connected if and only if
    // all of the triangles are visited on a single search.

    int iTSize = m_kTMap.size();
    if (iTSize == 0)
        return true;

    // for marking visited triangles during the traversal
    map<Triangle, bool> kVisitedMap;
    MTCIter pkT;
    for (pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
        kVisitedMap.insert(make_pair(pkT->first, false));

    // start the traversal at any triangle in the mesh
    stack<Triangle> kStack;
    kStack.push(m_kTMap.begin()->first);
    map<Triangle, bool>::iterator pkVI =
        kVisitedMap.find(m_kTMap.begin()->first);
    assert(pkVI != kVisitedMap.end());
    pkVI->second = true;
    iTSize--;

    while (!kStack.empty())
    {
        // start at the current triangle
        Triangle kT = kStack.top();
        kStack.pop();

        for (int i = 0; i < 3; i++)
        {
            // get an edge of the current triangle
            MECIter pkE = m_kEMap.find(Edge(kT.m_aiV[i], kT.m_aiV[(i + 1) % 3]));

            // visit each adjacent triangle
            const SmallSet<Triangle> &rkTSet = pkE->second.m_kTSet;
            for (int j = 0; j < rkTSet.GetSize(); j++)
            {
                const Triangle &rkTAdj = rkTSet[j];
                pkVI = kVisitedMap.find(rkTAdj);
                assert(pkVI != kVisitedMap.end());
                if (pkVI->second == false)
                {
                    // this adjacent triangle not yet visited
                    kStack.push(rkTAdj);
                    pkVI->second = true;
                    iTSize--;
                }
            }
        }
    }

    return iTSize == 0;
}
//----------------------------------------------------------------------------
void TriangleMesh::GetComponents(vector<TriangleMesh *> &rkComponents)
{
    // Do a depth-first search of the mesh to find connected components.
    int iTSize = m_kTMap.size();
    if (iTSize == 0)
        return;

    // for marking visited triangles during the traversal
    map<Triangle, bool> kVisitedMap;
    MTCIter pkT;
    for (pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
        kVisitedMap.insert(make_pair(pkT->first, false));

    while (iTSize > 0)
    {
        // find an unvisited triangle in the mesh
        stack<Triangle> kStack;
        map<Triangle, bool>::iterator pkVI = kVisitedMap.begin();
        while (pkVI != kVisitedMap.end())
        {
            if (pkVI->second == false)
            {
                kStack.push(pkVI->first);
                pkVI->second = true;
                iTSize--;
                break;
            }
            pkVI++;
        }

        // traverse the connected component of the starting triangle
        TriangleMesh *pkComponent = Create();
        while (!kStack.empty())
        {
            // start at the current triangle
            Triangle kT = kStack.top();
            kStack.pop();
            pkComponent->InsertTriangle(kT);

            for (int i = 0; i < 3; i++)
            {
                // get an edge of the current triangle
                Edge kE(kT.m_aiV[i], kT.m_aiV[(i + 1) % 3]);
                MECIter pkE = m_kEMap.find(kE);

                // visit each adjacent triangle
                const SmallSet<Triangle> &rkTSet = pkE->second.m_kTSet;
                for (int j = 0; j < rkTSet.GetSize(); j++)
                {
                    const Triangle &rkTAdj = rkTSet[i];
                    pkVI = kVisitedMap.find(rkTAdj);
                    assert(pkVI != kVisitedMap.end());
                    if (pkVI->second == false)
                    {
                        // this adjacent triangle not yet visited
                        kStack.push(rkTAdj);
                        pkVI->second = true;
                        iTSize--;
                    }
                }
            }
        }
        rkComponents.push_back(pkComponent);
    }
}
//----------------------------------------------------------------------------
void TriangleMesh::GetComponents(vector<int> &rkIndex, int *&raiConnect)
{
    rkIndex.clear();

    // Do a depth-first search of the mesh to find connected components.
    int iTSize = m_kTMap.size();
    if (iTSize == 0)
    {
        raiConnect = NULL;
        return;
    }

    int iIQuantity = 3 * iTSize;
    int iIndex = 0;
    raiConnect = new int[iIQuantity];

    // for marking visited triangles during the traversal
    map<Triangle, bool> kVisitedMap;
    MTCIter pkT;
    for (pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
        kVisitedMap.insert(make_pair(pkT->first, false));

    while (iTSize > 0)
    {
        // find an unvisited triangle in the mesh
        stack<Triangle> kStack;
        map<Triangle, bool>::iterator pkVI = kVisitedMap.begin();
        while (pkVI != kVisitedMap.end())
        {
            if (pkVI->second == false)
            {
                kStack.push(pkVI->first);
                pkVI->second = true;
                iTSize--;
                break;
            }
            pkVI++;
        }

        // traverse the connected component of the starting triangle
        TriangleMesh *pkComponent = Create();
        while (!kStack.empty())
        {
            // start at the current triangle
            Triangle kT = kStack.top();
            kStack.pop();
            pkComponent->InsertTriangle(kT);

            for (int i = 0; i < 3; i++)
            {
                // get an edge of the current triangle
                Edge kE(kT.m_aiV[i], kT.m_aiV[(i + 1) % 3]);
                MECIter pkE = m_kEMap.find(kE);

                // visit each adjacent triangle
                const SmallSet<Triangle> &rkTSet = pkE->second.m_kTSet;
                for (int j = 0; j < rkTSet.GetSize(); j++)
                {
                    const Triangle &rkTAdj = rkTSet[i];
                    pkVI = kVisitedMap.find(rkTAdj);
                    assert(pkVI != kVisitedMap.end());
                    if (pkVI->second == false)
                    {
                        // this adjacent triangle not yet visited
                        kStack.push(rkTAdj);
                        pkVI->second = true;
                        iTSize--;
                    }
                }
            }
        }

        // store the connectivity information for this component
        set<Triangle> kTSet;
        pkComponent->GetTriangles(kTSet);
        delete pkComponent;

        rkIndex.push_back(iIndex);
        set<Triangle>::iterator pkTIter;
        for (pkTIter = kTSet.begin(); pkTIter != kTSet.end(); pkTIter++)
        {
            assert(iIndex + 3 <= iIQuantity);
            const Triangle &rkT = *pkTIter;
            raiConnect[iIndex++] = rkT.m_aiV[0];
            raiConnect[iIndex++] = rkT.m_aiV[1];
            raiConnect[iIndex++] = rkT.m_aiV[2];
        }
    }

    rkIndex.push_back(iIQuantity);
}
//----------------------------------------------------------------------------
void TriangleMesh::RemoveComponent(int &riIQuantity, int *aiConnect)
{
    // Do a depth-first search of the mesh to find connected components.  The
    // input array is assumed to be large enough to hold the component (see
    // the comments in MgcTriangleMesh.h for RemoveComponent).
    riIQuantity = 0;

    int iTSize = m_kTMap.size();
    if (iTSize == 0)
        return;

    // Find the connected component containing the first triangle in the mesh.
    // A set is used instead of a stack to avoid having a large-memory
    // 'visited' map.
    set<Triangle> kVisited;
    kVisited.insert(m_kTMap.begin()->first);

    // traverse the connected component
    while (!kVisited.empty())
    {
        // start at the current triangle
        Triangle kT = *kVisited.begin();

        // add adjacent triangles to the set for recursive processing
        for (int i = 0; i < 3; i++)
        {
            // get an edge of the current triangle
            Edge kE(kT.m_aiV[i], kT.m_aiV[(i + 1) % 3]);
            MECIter pkE = m_kEMap.find(kE);
            assert(pkE != m_kEMap.end());

            // visit each adjacent triangle
            const SmallSet<Triangle> &rkTSet = pkE->second.m_kTSet;
            for (int j = 0; j < rkTSet.GetSize(); j++)
            {
                Triangle kTAdj = rkTSet[j];
                if (kTAdj != kT)
                    kVisited.insert(kTAdj);
            }
        }

        // add triangle to connectivity array
        aiConnect[riIQuantity++] = kT.m_aiV[0];
        aiConnect[riIQuantity++] = kT.m_aiV[1];
        aiConnect[riIQuantity++] = kT.m_aiV[2];

        // remove the current triangle (visited, no longer needed)
        kVisited.erase(kT);
        RemoveTriangle(kT);
    }
}
//----------------------------------------------------------------------------
bool TriangleMesh::GetConsistentComponents(
    vector<TriangleMesh *> &rkComponents)
{
    if (!IsManifold())
        return false;

    // Do a depth-first search of the mesh to find connected components.
    int iTSize = m_kTMap.size();
    if (iTSize == 0)
        return true;

    // for marking visited triangles during the traversal
    map<Triangle, bool> kVisitedMap;
    MTCIter pkT;
    for (pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
        kVisitedMap.insert(make_pair(pkT->first, false));

    while (iTSize > 0)
    {
        // Find an unvisited triangle in the mesh.  Any triangle pushed onto
        // the stack is considered to have a consistent ordering.
        stack<Triangle> kStack;
        map<Triangle, bool>::iterator pkVI = kVisitedMap.begin();
        while (pkVI != kVisitedMap.end())
        {
            if (pkVI->second == false)
            {
                kStack.push(pkVI->first);
                pkVI->second = true;
                iTSize--;
                break;
            }
            pkVI++;
        }

        // traverse the connected component of the starting triangle
        TriangleMesh *pkComponent = Create();
        while (!kStack.empty())
        {
            // start at the current triangle
            Triangle kT = kStack.top();
            kStack.pop();
            pkComponent->InsertTriangle(kT);

            for (int i = 0; i < 3; i++)
            {
                // get an edge of the current triangle
                int iV0 = kT.m_aiV[i], iV1 = kT.m_aiV[(i + 1) % 3], iV2;
                Edge kE(iV0, iV1);
                MECIter pkE = m_kEMap.find(kE);

                int iSize = pkE->second.m_kTSet.GetSize();
                assert(iSize == 1 || iSize == 2); // mesh is manifold
                const Triangle *pkTAdj = &pkE->second.m_kTSet[0];
                if (iSize == 2)
                {
                    // get the adjacent triangle to the current one
                    if (*pkTAdj == kT)
                        pkTAdj = &pkE->second.m_kTSet[1];

                    pkVI = kVisitedMap.find(*pkTAdj);
                    assert(pkVI != kVisitedMap.end());
                    if (pkVI->second == false)
                    {
                        // adjacent triangle not yet visited
                        if ((pkTAdj->m_aiV[0] == iV0 && pkTAdj->m_aiV[1] == iV1) || (pkTAdj->m_aiV[1] == iV0 && pkTAdj->m_aiV[2] == iV1) || (pkTAdj->m_aiV[2] == iV0 && pkTAdj->m_aiV[0] == iV1))
                        {
                            // adjacent triangle must be reordered
                            iV0 = pkTAdj->m_aiV[0];
                            iV1 = pkTAdj->m_aiV[1];
                            iV2 = pkTAdj->m_aiV[2];
                            kVisitedMap.erase(*pkTAdj);
                            RemoveTriangle(iV0, iV1, iV2);
                            InsertTriangle(iV1, iV0, iV2);
                            kVisitedMap.insert(make_pair(Triangle(iV1, iV0,
                                                                  iV2),
                                                         false));

                            // refresh the iterators since maps changed
                            pkE = m_kEMap.find(kE);
                            pkTAdj = &pkE->second.m_kTSet[0];
                            if (*pkTAdj == kT)
                                pkTAdj = &pkE->second.m_kTSet[1];
                            pkVI = kVisitedMap.find(*pkTAdj);
                            assert(pkVI != kVisitedMap.end());
                        }

                        kStack.push(*pkTAdj);
                        pkVI->second = true;
                        iTSize--;
                    }
                }
            }
        }
        rkComponents.push_back(pkComponent);
    }

    return true;
}
//----------------------------------------------------------------------------
TriangleMesh *TriangleMesh::GetReversedOrderMesh() const
{
    TriangleMesh *pkReversed = Create();

    for (MTCIter pkT = m_kTMap.begin(); pkT != m_kTMap.end(); pkT++)
    {
        pkReversed->InsertTriangle(pkT->first.m_aiV[0], pkT->first.m_aiV[2],
                                   pkT->first.m_aiV[1]);
    }

    return pkReversed;
}
//----------------------------------------------------------------------------
void TriangleMesh::GetStatistics(int &riVQuantity, int &riEQuantity,
                                 int &riTQuantity, Real &rfAverageEdgesPerVertex,
                                 Real &rfAverageTrianglesPerVertex, Real &rfAverageTrianglesPerEdge,
                                 int &riMaximumEdgesPerVertex, int &riMaximumTrianglesPerVertex,
                                 int &riMaximumTrianglesPerEdge)
{
    riVQuantity = m_kVMap.size();
    riEQuantity = m_kEMap.size();
    riTQuantity = m_kTMap.size();

    int iESumForV = 0;
    int iTSumForV = 0;
    riMaximumEdgesPerVertex = 0;
    riMaximumTrianglesPerVertex = 0;

    int iESize, iTSize;

    for (MVCIter pkV = m_kVMap.begin(); pkV != m_kVMap.end(); pkV++)
    {
        iESize = pkV->second.m_kESet.GetSize();
        iTSize = pkV->second.m_kTSet.GetSize();
        iESumForV += iESize;
        iTSumForV += iTSize;
        if (iESize > riMaximumEdgesPerVertex)
            riMaximumEdgesPerVertex = iESize;
        if (iTSize > riMaximumTrianglesPerVertex)
            riMaximumTrianglesPerVertex = iTSize;
    }

    int iTSumForE = 0;
    riMaximumTrianglesPerEdge = 0;
    for (MECIter pkE = m_kEMap.begin(); pkE != m_kEMap.end(); pkE++)
    {
        iTSize = pkE->second.m_kTSet.GetSize();
        iTSumForE += iTSize;
        if (iTSize > riMaximumTrianglesPerEdge)
            riMaximumTrianglesPerEdge = iTSize;
    }

    rfAverageEdgesPerVertex = ((Real)iESumForV) / riVQuantity;
    rfAverageTrianglesPerVertex = ((Real)iTSumForV) / riVQuantity;
    rfAverageTrianglesPerEdge = ((Real)iTSumForE) / riEQuantity;
}
//----------------------------------------------------------------------------
