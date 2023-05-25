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

//----------------------------------------------------------------------------
inline int TriangleMesh::GetVertexQuantity() const
{
    return m_kVMap.size();
}
//----------------------------------------------------------------------------
inline int TriangleMesh::GetEdgeQuantity() const
{
    return m_kEMap.size();
}
//----------------------------------------------------------------------------
inline int TriangleMesh::GetTriangleQuantity() const
{
    return m_kTMap.size();
}
//----------------------------------------------------------------------------
inline TriangleMesh *TriangleMesh::Create() const
{
    return new TriangleMesh;
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnVertexInsert(int, bool, void *&)
{
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnVertexRemove(int, bool, void *)
{
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnEdgeInsert(const Edge &, bool, void *&)
{
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnEdgeRemove(const Edge &, bool, void *)
{
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnTriangleInsert(const Triangle &, bool, void *&)
{
}
//----------------------------------------------------------------------------
inline void TriangleMesh::OnTriangleRemove(const Triangle &, bool, void *)
{
}
//----------------------------------------------------------------------------
inline const TriangleMesh::VMap &TriangleMesh::GetVertexMap() const
{
    return m_kVMap;
}
//----------------------------------------------------------------------------
inline const TriangleMesh::EMap &TriangleMesh::GetEdgeMap() const
{
    return m_kEMap;
}
//----------------------------------------------------------------------------
inline const TriangleMesh::TMap &TriangleMesh::GetTriangleMap() const
{
    return m_kTMap;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TriangleMesh::Edge
//----------------------------------------------------------------------------
inline TriangleMesh::Edge::Edge()
{
}
//----------------------------------------------------------------------------
inline TriangleMesh::Edge::Edge(int iV0, int iV1)
{
    if (iV0 < iV1)
    {
        // v0 is minimum
        m_aiV[0] = iV0;
        m_aiV[1] = iV1;
    }
    else
    {
        // v1 is minimum
        m_aiV[0] = iV1;
        m_aiV[1] = iV0;
    }
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Edge::operator<(const Edge &rkE) const
{
    if (m_aiV[1] < rkE.m_aiV[1])
        return true;

    if (m_aiV[1] == rkE.m_aiV[1])
        return m_aiV[0] < rkE.m_aiV[0];

    return false;
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Edge::operator==(const Edge &rkE) const
{
    return m_aiV[0] == rkE.m_aiV[0] && m_aiV[1] == rkE.m_aiV[1];
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Edge::operator!=(const Edge &rkE) const
{
    return !operator==(rkE);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TriangleMesh::Triangle
//----------------------------------------------------------------------------
inline TriangleMesh::Triangle::Triangle()
{
}
//----------------------------------------------------------------------------
inline TriangleMesh::Triangle::Triangle(int iV0, int iV1, int iV2)
{
    if (iV0 < iV1)
    {
        if (iV0 < iV2)
        {
            // v0 is minimum
            m_aiV[0] = iV0;
            m_aiV[1] = iV1;
            m_aiV[2] = iV2;
        }
        else
        {
            // v2 is minimum
            m_aiV[0] = iV2;
            m_aiV[1] = iV0;
            m_aiV[2] = iV1;
        }
    }
    else
    {
        if (iV1 < iV2)
        {
            // v1 is minimum
            m_aiV[0] = iV1;
            m_aiV[1] = iV2;
            m_aiV[2] = iV0;
        }
        else
        {
            // v2 is minimum
            m_aiV[0] = iV2;
            m_aiV[1] = iV0;
            m_aiV[2] = iV1;
        }
    }
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Triangle::operator<(const Triangle &rkT) const
{
    if (m_aiV[2] < rkT.m_aiV[2])
        return true;

    if (m_aiV[2] == rkT.m_aiV[2])
    {
        if (m_aiV[1] < rkT.m_aiV[1])
            return true;

        if (m_aiV[1] == rkT.m_aiV[1])
            return m_aiV[0] < rkT.m_aiV[0];
    }

    return false;
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Triangle::operator==(const Triangle &rkT) const
{
    return (m_aiV[0] == rkT.m_aiV[0]) &&
           ((m_aiV[1] == rkT.m_aiV[1] && m_aiV[2] == rkT.m_aiV[2]) ||
            (m_aiV[1] == rkT.m_aiV[2] && m_aiV[2] == rkT.m_aiV[1]));
}
//----------------------------------------------------------------------------
inline bool TriangleMesh::Triangle::operator!=(const Triangle &rkT) const
{
    return !operator==(rkT);
}
//----------------------------------------------------------------------------
