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

#ifndef MGCSMALLSET_H
#define MGCSMALLSET_H

// This template class is used by Mgc::TriangleMesh to store the edges and
// triangles that share a vertex and to store the triangles that share an
// edge.  Before this the 'set' class was used from STL, but the memory
// overhead for the class is enormous.  As an example, I used the code to
// build the adjacency information for a level surface that was a collection
// of 825K vertices and 1.6M triangles (mesh had 2.5M edges).  The memory
// used by the program that built the adjacency information was 980M.  The
// program based on Mgc::SmallSet used 503M.
//
// The idea is that for a typical mesh, the average number of edges sharing
// a vertex is 6 and the average number of triangles sharing a vertex is 6.
// The average number of triangles sharing an edge is 2.  Since the sets of
// adjacent objects are small, the Insert method does a linear search to
// check if the element already exists.  Reallocation occurs if the current
// set is full to make room for the new element.  The Remove method does a
// linear search and, if the requested element is found, removes it and
// shifts the higher-indexed array elements to fill in the gap.  Because of
// the linear searches, this class should not be used for large sets.  Large
// sets are better handled by a hashed data structure.

#include "MgcRTLib.h"

namespace Mgc
{

    template <class T>
    class SmallSet
    {
    public:
        SmallSet();
        SmallSet(int iCapacity, int iGrowBy);
        SmallSet(const SmallSet &rkSet);
        ~SmallSet();

        SmallSet &operator=(const SmallSet &rkSet);

        int GetCapacity() const;
        int GetGrowBy() const;
        int GetSize() const;
        const T *GetElements() const;
        const T &operator[](int i) const;

        bool Insert(const T &rkElement);
        void InsertNoCheck(const T &rkElement);
        bool Remove(const T &rkElement);
        bool Exists(const T &rkElement);
        void Clear(int iCapacity, int iGrowBy);

    protected:
        int m_iCapacity, m_iGrowBy, m_iSize;
        T *m_atElement;
    };

#include "MgcSmallSet.inl"

} // namespace Mgc

#endif
