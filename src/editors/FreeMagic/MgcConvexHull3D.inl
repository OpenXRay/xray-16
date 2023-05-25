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
inline int ConvexHull3D::GetType() const
{
    return m_iHullType;
}
//----------------------------------------------------------------------------
inline int ConvexHull3D::GetQuantity() const
{
    return m_iHQuantity;
}
//----------------------------------------------------------------------------
inline const int *ConvexHull3D::GetIndices() const
{
    return m_aiHIndex;
}
//----------------------------------------------------------------------------
inline Real &ConvexHull3D::VertexEqualityEpsilon()
{
    return ms_fVertexEqualityEpsilon;
}
//----------------------------------------------------------------------------
inline Real &ConvexHull3D::CollinearEpsilon()
{
    return ms_fCollinearEpsilon;
}
//----------------------------------------------------------------------------
inline Real &ConvexHull3D::CoplanarEpsilon()
{
    return ms_fCoplanarEpsilon;
}
//----------------------------------------------------------------------------
