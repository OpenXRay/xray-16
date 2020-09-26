/* Copyright (C) Tom Forsyth, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Tom Forsyth, 2001"
 */
// Helper routines for manipulating large complex meshes.
// Pretty memory-hungry, but good for cross-referencing between
// edges, points and triangles.

//.#include "tomslib.h"
#include "dlink.h"
#include "arbitrarylist.h"

// Before including this file, #define the following:
//
// MESHPT_APP_DEFINED
// MESHTRI_APP_DEFINED
// MESHEDGE_APP_DEFINED
//
// These declarations are included in the declarations of the relevant
// classes. They may be defined to be nothing of course.
//
// For efficiency, also define some of the following according to use:
//
// MESHCTRL_EDGES_ALWAYS_ADDED_BEFORE_TRIS:
//		Set to 1 to enforce/assume that when adding a tri, all three edges
//		will exist. So all tris must have all three edge entries filled.
//		This is a speedup, and an error check, if that's what your app does.
//		The exception is if autocreation of edges is done for the MeshTri creator.
//		Similarly, before deleteing edges, make sure you have deleted all the
//		tris that use them
// MESHCTRL_PTS_ALWAYS_ADDED_BEFORE_TRIS:
//		Ditto, but for pts.
// MESHCTRL_PTS_ALWAYS_ADDED_BEFORE_EDGES:
//		Ditto, but pts must be added before edges are created.

class MeshPt;
class MeshEdge;
class MeshTri;

class MeshTri
{
    friend class MeshPt;
    friend class MeshEdge;

private:
    DlinkDefine(MeshTri, List);
    u32 dwListId; // For use when doing consistency checks.

    void InternalDelete(BOOL bBinUnusedEdges);

public:
    MeshPt* pPt1; // Points.
    MeshPt* pPt2;
    MeshPt* pPt3;
    MeshEdge* pEdge12; // Edges between point numbers.
    MeshEdge* pEdge23;
    MeshEdge* pEdge31;

    DlinkMethods(MeshTri, List);

    MESHTRI_APP_DEFINED // App-defined data.

    MeshTri(void);
    // Set pEdgeListRoot to non-NULL to autocreate edges.
    MeshTri(
    MeshPt* pNewPt1, MeshPt* pNewPt2, MeshPt* pNewPt3, MeshTri* pListRoot = NULL, MeshEdge* pEdgeListRoot = NULL);
    ~MeshTri(void);
    // Set bBinUnusedEdges to TRUE to autodestroy edges.
    void Delete(BOOL bBinUnusedEdges = FALSE);
    // Which list is this tri in?
    MeshTri* QueryList(void);
    // Move this tri to this list.
    void SetList(MeshTri* pListRoot);
    // Checks that all edges and pts refer back to this tri,
    // and that they're in the respective lists.
    // If the lists are NULL, the check is not made.
    bool ConsistencyCheck(MeshPt* pPtRoot = NULL, MeshEdge* pEdgeRoot = NULL, MeshTri* pTriRoot = NULL);

protected:
    // Add the edge to this tri.
    void AddEdge(MeshEdge* pEdge);
    // Remove the edge from this tri.
    void RemoveEdge(MeshEdge* pEdge);

protected:
    // Remove the point from this tri.
    // NOTE! This is probably not a good thing to do.
    void RemovePt(MeshPt* pPt);

public:
    IC bool Equal(MeshTri* F)
    {
        // Test for 6 variations
        if ((pPt1 == F->pPt1) && (pPt2 == F->pPt2) && (pPt3 == F->pPt3))
            return true;
        if ((pPt1 == F->pPt1) && (pPt3 == F->pPt2) && (pPt2 == F->pPt3))
            return true;
        if ((pPt3 == F->pPt1) && (pPt1 == F->pPt2) && (pPt2 == F->pPt3))
            return true;
        if ((pPt3 == F->pPt1) && (pPt2 == F->pPt2) && (pPt1 == F->pPt3))
            return true;
        if ((pPt2 == F->pPt1) && (pPt1 == F->pPt2) && (pPt3 == F->pPt3))
            return true;
        if ((pPt2 == F->pPt1) && (pPt3 == F->pPt2) && (pPt1 == F->pPt3))
            return true;
        return false;
    }
};

class MeshEdge
{
    friend class MeshPt;
    friend class MeshTri;

private:
    DlinkDefine(MeshEdge, List);
    u32 dwListId; // For use when doing consistency checks.

public:
    MeshPt* pPt1;
    MeshPt* pPt2;
    MeshTri* pTri12; // Tri that numbers pt1, pt2 in that order.
    MeshTri* pTri21; // Tri that numbers pt2, pt1 in that order.

    MeshEdge* pEdgeProx; // The edge that this is close to, if any.

    DlinkMethods(MeshEdge, List);

    MESHEDGE_APP_DEFINED // App-defined data.

    //	BINARY_HEAP_VARS();			// Helper stuff.

    MeshEdge(void);
    MeshEdge(MeshPt* pNewPt1, MeshPt* pNewPt2, MeshEdge* pListRoot = NULL);
    ~MeshEdge(void);
    // Find the other triangle that uses this edge.
    MeshTri* OtherTri(MeshTri* pTri);
    // Find the other point that uses this edge.
    MeshPt* OtherPt(MeshPt* pPt);
    // Try to merge these two edges. Result is TRUE if it succeeded - note that the other edge will be NOT deleted.
    bool bTryToMergeEdges(MeshEdge* pedge);
    // Which list is this Edge in?
    MeshEdge* QueryList(void);
    // Move this Edge to this list.
    void SetList(MeshEdge* pListRoot);
    // Makes these two edges prox.
    // The point prox data must agree.
    // Returns TRUE on success, or FALSE if it failed.
    bool AddProx(MeshEdge* pEdge);
    // Find the proximity edge, if any.
    // Relies on the point proximity values having been set up.
    // If one is found, it is returned.
    MeshEdge* DoProxMatch(void);
    // Removes any edge prox data.
    // Returns TRUE if there was some.
    // The pt prox data can still agree - it is not touched.
    bool RemoveProx(void);
    // Checks that all pts and tris refer back to this edge,
    // and that they're in the respective lists.
    // If the lists are NULL, the check is not made.
    bool ConsistencyCheck(MeshPt* pPtRoot = NULL, MeshEdge* pEdgeRoot = NULL, MeshTri* pTriRoot = NULL);

protected:
    // Remove the tri from this edge.
    void RemoveTri(MeshTri* pTri);
    // Add the tri to this edge.
    void AddTri(MeshTri* pTri);

protected:
    // Remove the pt from this edge.
    // NOTE! This is probably not a good thing to do.
    void RemovePt(MeshPt* pPt);
};

class MeshPt
{
    friend class MeshEdge;
    friend class MeshTri;

private:
    ArbitraryList<MeshEdge*> EdgeList; // The list of edges that use this point (in no order).
    ArbitraryList<MeshTri*> TriList; // The list of tris that use this point (in no order).
    ArbitraryList<MeshPt*> ProxPtList; // The list of prox pts (in no order).

    int iCurTriNum; // Used with First/NextTri.
    int iCurEdgeNum; // Used with First/NextEdge.
    int iCurProxNum; // Used with First/NextProx.

    DlinkDefine(MeshPt, List);
    u32 dwListId; // For use when doing consistency checks.

public:
    MESHPT_APP_DEFINED // App-defined data.

    DlinkMethods(MeshPt, List);

    MeshPt(MeshPt* pListRoot = NULL);
    ~MeshPt(void);
    // Find the edge that uses this pt and the other one given.
    // NOTE! You almost never want to use this - almost always,
    // use FindTriEdge, because edges are typically created on-demand
    // when creating tris, and using this rout means that no two
    // points can have more than two tris use them, which lots of models
    // violate.
    MeshEdge* FindEdge(MeshPt* pPt);
    // Find the first edge that uses this pt and the other given, and
    // also has a free triangle entry, assuming that the points are
    // used in that clockwise order. This allows two edges that share
    // the same points to exist, e.g. where multiple triangles share
    // the same edge (think of the diagonal of the tris of a back-to-back
    // quad - same edge, four tris.
    // The tri will use the points in the order *this,*pPt.
    MeshEdge* FindTriEdge(MeshPt* pPt);
    // Find the tri that uses this pt and the other two given.
    // They must be in the order this,pPt1,pPt2 - not the other way round.
    MeshTri* FindTri(MeshPt* pPt1, MeshPt* pPt2);

    // Retired - there may be several tris like this - call FirstTri/NextTri.
    // Return the first tri in the list. MUST be called before calling NextTri().
    // If a non-NULL pPt is supplied, only tris using this,pPt in that order
    // are returned, otherwise all tris are returned.
    MeshTri* FirstTri(MeshPt* pPt = NULL);
    // Return the next tri in the list.
    // If a non-NULL pPt is supplied, only tris using this,pPt in that order
    // are returned, otherwise all tris are returned.
    MeshTri* NextTri(MeshPt* pPt = NULL);
    // Terminate the current First/Next loop.
    // No need to call this if NULL was returned from NextTri().
    void EndTri(void);

    // Return the first Edge in the list. MUST be called before calling NextEdge().
    // If a non-NULL pPt is supplied, only edges using this and pPt
    // are returned, otherwise all edges are returned.
    MeshEdge* FirstEdge(MeshPt* pPt = NULL);
    // Return the next Edge in the list.
    // If a non-NULL pPt is supplied, only edges using this and pPt
    // are returned, otherwise all edges are returned.
    MeshEdge* NextEdge(MeshPt* pPt = NULL);
    // Terminate the current First/Next loop.
    // No need to call this if NULL was returned from NextEdge().
    void EndEdge(void);

    // Add the given pt to the prox list (and vice versa).
    // If the pt was not already there, returns TRUE;
    // If bProxEdges is set to TRUE (default is FALSE ),
    // the edges that these two pts use are made prox if possible.
    bool AddProx(MeshPt* pPt, bool bProxEdges = FALSE);
    // Remove the given pt from the prox list (and vice versa).
    // If the pt was there, returns TRUE.
    bool RemoveProx(MeshPt* pPt);
    // Returns TRUE if the two pts are marked as being in proximity.
    bool CheckProx(MeshPt* pPt);
    // Return the first prox pt. MUST be called before calling NextProx().
    MeshPt* FirstProx(void);
    // Return the next prox pt.
    MeshPt* NextProx(void);
    // Terminate the current First/Next loop.
    // No need to call this if NULL was returned from NextProx().
    void EndProx(void);

    // Which list is this Pt in?
    MeshPt* QueryList(void);
    // Move this Pt to this list.
    void SetList(MeshPt* pListRoot);
    // Checks that all edges and tris refer back to this pt,
    // and that they're in the respective lists.
    // If the lists are NULL, the check is not made.
    bool ConsistencyCheck(MeshPt* pPtRoot = NULL, MeshEdge* pEdgeRoot = NULL, MeshTri* pTriRoot = NULL);

protected:
    // Remove the edge from this point.
    void RemoveEdge(MeshEdge* pEdge);
    // Add the edge to this point.
    void AddEdge(MeshEdge* pEdge);

protected:
    // Remove the tri from this point.
    void RemoveTri(MeshTri* pTri);
    // Add the tri to this point.
    void AddTri(MeshTri* pTri);
};

inline MeshTri::MeshTri(void)
{
    // Should only be used for list roots.
    pPt1 = NULL;
    pPt2 = NULL;
    pPt3 = NULL;
    pEdge12 = NULL;
    pEdge23 = NULL;
    pEdge31 = NULL;
    ListInit();
}

inline MeshTri::MeshTri(MeshPt* pNewPt1, MeshPt* pNewPt2, MeshPt* pNewPt3, MeshTri* pListRoot, MeshEdge* pEdgeListRoot)
{
    VERIFY(pNewPt1 != NULL);
    VERIFY(pNewPt2 != NULL);
    VERIFY(pNewPt3 != NULL);
    pPt1 = pNewPt1;
    pPt2 = pNewPt2;
    pPt3 = pNewPt3;
    pEdge12 = NULL;
    pEdge23 = NULL;
    pEdge31 = NULL;

    pPt1->AddTri(this);
    pPt2->AddTri(this);
    pPt3->AddTri(this);

    pEdge12 = pPt1->FindTriEdge(pPt2);
    if ((pEdge12 == NULL) && (pEdgeListRoot != NULL))
    {
        // Autocreate the edge.
        pEdge12 = xr_new<MeshEdge>(pPt1, pPt2, pEdgeListRoot);
    }
    VERIFY(pEdge12 != NULL);
    {
        pEdge12->AddTri(this);
    }

    pEdge23 = pPt2->FindTriEdge(pPt3);
    if ((pEdge23 == NULL) && (pEdgeListRoot != NULL))
    {
        // Autocreate the edge.
        pEdge23 = xr_new<MeshEdge>(pPt2, pPt3, pEdgeListRoot);
    }
    VERIFY(pEdge23 != NULL);
    {
        pEdge23->AddTri(this);
    }

    pEdge31 = pPt3->FindTriEdge(pPt1);
    if ((pEdge31 == NULL) && (pEdgeListRoot != NULL))
    {
        // Autocreate the edge.
        pEdge31 = xr_new<MeshEdge>(pPt3, pPt1, pEdgeListRoot);
    }
    VERIFY(pEdge31 != NULL);
    {
        pEdge31->AddTri(this);
    }

    ListInit();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

inline void MeshTri::InternalDelete(BOOL bBinUnusedEdges)
{
    // Remove edge references.
    if (pEdge12 != NULL)
    {
        pEdge12->RemoveTri(this);
        if (bBinUnusedEdges && (pEdge12->pTri12 == NULL) && (pEdge12->pTri21 == NULL))
        {
            // This edge is no longer in use.
            xr_delete(pEdge12);
        }
        pEdge12 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    if (pEdge23 != NULL)
    {
        pEdge23->RemoveTri(this);
        if (bBinUnusedEdges && (pEdge23->pTri12 == NULL) && (pEdge23->pTri21 == NULL))
        {
            // This edge is no longer in use.
            xr_delete(pEdge23);
        }
        pEdge23 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    if (pEdge31 != NULL)
    {
        pEdge31->RemoveTri(this);
        if (bBinUnusedEdges && (pEdge31->pTri12 == NULL) && (pEdge31->pTri21 == NULL))
        {
            // This edge is no longer in use.
            xr_delete(pEdge31);
        }
        pEdge31 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    // Remove point references.
    if (pPt1 != NULL)
    {
        pPt1->RemoveTri(this);
        pPt1 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    if (pPt2 != NULL)
    {
        pPt2->RemoveTri(this);
        pPt2 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    if (pPt3 != NULL)
    {
        pPt3->RemoveTri(this);
        pPt3 = NULL;
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pEdge12 == NULL);
        VERIFY(pEdge23 == NULL);
        VERIFY(pEdge31 == NULL);
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pPt3 == NULL);
    }

    ListDel();
}

inline MeshTri::~MeshTri(void) { InternalDelete(FALSE); }
inline void MeshTri::Delete(BOOL bBinUnusedEdges /*= FALSE*/)
{
    InternalDelete(bBinUnusedEdges);
    MeshTri* tri = (MeshTri*)this;
    xr_delete(tri);
}

inline void MeshTri::AddEdge(MeshEdge* pEdge)
{
    // Can't mess with a tri's edges once created.
    VERIFY(FALSE);
}

inline void MeshTri::RemoveEdge(MeshEdge* pEdge)
{
    VERIFY(pEdge != NULL);
    if (pEdge12 == pEdge)
    {
        pEdge12 = NULL;
    }
    else if (pEdge23 == pEdge)
    {
        pEdge23 = NULL;
    }
    else
    {
        VERIFY(pEdge31 == pEdge);
        pEdge31 = NULL;
    }
}

inline void MeshTri::RemovePt(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    if (pPt1 == pPt)
    {
        pPt1 = NULL;
    }
    else if (pPt2 == pPt)
    {
        pPt2 = NULL;
    }
    else
    {
        VERIFY(pPt3 == pPt);
        pPt3 = NULL;
    }
}

inline MeshTri* MeshTri::QueryList(void)
{
    MeshTri* pListRoot = ListFindFirst();
    if (pListRoot == this)
    {
        VERIFY(ListFindLast() == this);
        pListRoot = NULL;
    }
    return (pListRoot);
}

inline void MeshTri::SetList(MeshTri* pListRoot)
{
    ListDel();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

#define FAIL_CHECK()\
    bRes = FALSE;\
    VERIFY(FALSE)
inline bool MeshTri::ConsistencyCheck(MeshPt* pPtRoot, MeshEdge* pEdgeRoot, MeshTri* pTriRoot)
{
    bool bRes = TRUE;
    if ((pTriRoot != NULL) && (QueryList() != pTriRoot))
    {
        FAIL_CHECK();
    }

    if (pEdge12 != NULL)
    {
        if ((pEdgeRoot != NULL) && (pEdge12->QueryList() != pEdgeRoot))
        {
            FAIL_CHECK();
        }

        if (pEdge12->pTri12 == this)
        {
            if ((pEdge12->pPt1 != pPt1) || (pEdge12->pPt2 != pPt2))
            {
                FAIL_CHECK();
            }
        }
        else if (pEdge12->pTri21 == this)
        {
            if ((pEdge12->pPt1 != pPt2) || (pEdge12->pPt2 != pPt1))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }
    else
    {
        FAIL_CHECK();
    }

    if (pEdge23 != NULL)
    {
        if ((pEdgeRoot != NULL) && (pEdge23->QueryList() != pEdgeRoot))
        {
            FAIL_CHECK();
        }

        if (pEdge23->pTri12 == this)
        {
            if ((pEdge23->pPt1 != pPt2) || (pEdge23->pPt2 != pPt3))
            {
                FAIL_CHECK();
            }
        }
        else if (pEdge23->pTri21 == this)
        {
            if ((pEdge23->pPt1 != pPt3) || (pEdge23->pPt2 != pPt2))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }
    else
    {
        FAIL_CHECK();
    }

    if (pEdge31 != NULL)
    {
        if ((pEdgeRoot != NULL) && (pEdge31->QueryList() != pEdgeRoot))
        {
            FAIL_CHECK();
        }

        if (pEdge31->pTri12 == this)
        {
            if ((pEdge31->pPt1 != pPt3) || (pEdge31->pPt2 != pPt1))
            {
                FAIL_CHECK();
            }
        }
        else if (pEdge31->pTri21 == this)
        {
            if ((pEdge31->pPt1 != pPt1) || (pEdge31->pPt2 != pPt3))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }
    else
    {
        FAIL_CHECK();
    }

    if ((pPt1 == NULL) || (pPt2 == NULL) || (pPt3 == NULL))
    {
        FAIL_CHECK();
    }
    else
    {
        if (pPtRoot != NULL)
        {
            if (pPt1->QueryList() != pPtRoot)
            {
                FAIL_CHECK();
            }
            if (pPt2->QueryList() != pPtRoot)
            {
                FAIL_CHECK();
            }
            if (pPt3->QueryList() != pPtRoot)
            {
                FAIL_CHECK();
            }
        }
    }

    return (bRes);
}

inline MeshEdge::MeshEdge(void)
{
    pPt1 = NULL;
    pPt2 = NULL;
    pTri12 = NULL;
    pTri21 = NULL;
    pEdgeProx = NULL;
    ListInit();
}

inline MeshEdge::MeshEdge(MeshPt* pNewPt1, MeshPt* pNewPt2, MeshEdge* pListRoot)
{
    VERIFY(pNewPt1 != NULL);
    VERIFY(pNewPt2 != NULL);
    pPt1 = pNewPt1;
    pPt2 = pNewPt2;
    pTri12 = NULL;
    pTri21 = NULL;
    pEdgeProx = NULL;

    pPt1->AddEdge(this);
    pPt2->AddEdge(this);

    ListInit();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

inline MeshEdge::~MeshEdge(void)
{
    RemoveProx();

    if (pPt1 != NULL)
    {
        MeshPt* pPt = pPt1;
        RemovePt(pPt);
        pPt->RemoveEdge(this);
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pTri12 == NULL);
        VERIFY(pTri21 == NULL);
    }

    if (pPt2 != NULL)
    {
        MeshPt* pPt = pPt2;
        RemovePt(pPt);
        pPt->RemoveEdge(this);
    }
    else
    {
        // The only good reason is if this is a local var,
        // in which case everything should be NULL.
        VERIFY(pPt1 == NULL);
        VERIFY(pPt2 == NULL);
        VERIFY(pTri12 == NULL);
        VERIFY(pTri21 == NULL);
    }

    // Any tri should have been binned already
    VERIFY(pTri12 == NULL);
    VERIFY(pTri21 == NULL);

    ListDel();
}

// Remove this edge from the tri.
inline void MeshEdge::RemoveTri(MeshTri* pTri)
{
    VERIFY(pTri != NULL);
    if (pTri12 == pTri)
    {
        pTri12 = NULL;
    }
    else
    {
        VERIFY(pTri21 == pTri);
        pTri21 = NULL;
    }
}

// Remove this edge from the pt.
inline void MeshEdge::RemovePt(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    if (pPt1 == pPt)
    {
        pPt1 = NULL;
    }
    else
    {
        VERIFY(pPt2 == pPt);
        pPt2 = NULL;
    }
}

inline void MeshEdge::AddTri(MeshTri* pTri)
{
    VERIFY(pTri != NULL);
    // Assumes the tri's pt pointers have already been set up.
    if (((pPt1 == pTri->pPt1) && (pPt2 == pTri->pPt2)) || ((pPt1 == pTri->pPt2) && (pPt2 == pTri->pPt3)) ||
    ((pPt1 == pTri->pPt3) && (pPt2 == pTri->pPt1)))
    {
        VERIFY(pTri12 == NULL);
        pTri12 = pTri;
    }
    else
    {
        VERIFY(((pPt1 == pTri->pPt2) && (pPt2 == pTri->pPt1)) || ((pPt1 == pTri->pPt3) && (pPt2 == pTri->pPt2)) ||
        ((pPt1 == pTri->pPt1) && (pPt2 == pTri->pPt3)));
        VERIFY(pTri21 == NULL);
        pTri21 = pTri;
    }
}

// Returns the other triangle that uses this edge.
inline MeshTri* MeshEdge::OtherTri(MeshTri* pTri)
{
    VERIFY(pTri != NULL);
    if (pTri == pTri12)
    {
        return (pTri21);
    }
    else
    {
        VERIFY(pTri == pTri21);
        return (pTri12);
    }
}

inline MeshPt* MeshEdge::OtherPt(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    if (pPt == pPt1)
    {
        return (pPt2);
    }
    else
    {
        VERIFY(pPt == pPt2);
        return (pPt1);
    }
}

// Try to merge these two edges. Result is TRUE if it succeeded - note that the other edge will be NOT deleted.
inline bool MeshEdge::bTryToMergeEdges(MeshEdge* pedge)
{
    VERIFY(pedge != this);
    if (pPt1 == pedge->pPt1)
    {
        VERIFY(pPt2 == pedge->pPt2);
        if ((pTri12 == NULL) && (pedge->pTri21 == NULL) && ((pEdgeProx == NULL) || (pedge->pEdgeProx == NULL)))
        {
            // Merge them.
            pTri12 = pedge->pTri12;
            if (pTri12 != NULL)
            {
                if (pTri12->pEdge12 == pedge)
                {
                    pTri12->pEdge12 = this;
                }
                else if (pTri12->pEdge23 == pedge)
                {
                    pTri12->pEdge23 = this;
                }
                else
                {
                    VERIFY(pTri12->pEdge31 == pedge);
                    pTri12->pEdge31 = this;
                }
            }
            pedge->pTri12 = NULL;
            if (pedge->pEdgeProx != NULL)
            {
                VERIFY(pEdgeProx == NULL);
                pEdgeProx = pedge->pEdgeProx;
                VERIFY(pEdgeProx->pEdgeProx == pedge);
                pEdgeProx->pEdgeProx = this;
                pedge->pEdgeProx = NULL;
            }
            return TRUE;
        }
        else if ((pTri21 == NULL) && (pedge->pTri12 == NULL) && ((pEdgeProx == NULL) || (pedge->pEdgeProx == NULL)))
        {
            // Merge them.
            pTri21 = pedge->pTri21;
            if (pTri21 != NULL)
            {
                if (pTri21->pEdge12 == pedge)
                {
                    pTri21->pEdge12 = this;
                }
                else if (pTri21->pEdge23 == pedge)
                {
                    pTri21->pEdge23 = this;
                }
                else
                {
                    VERIFY(pTri21->pEdge31 == pedge);
                    pTri21->pEdge31 = this;
                }
            }
            pedge->pTri21 = NULL;
            if (pedge->pEdgeProx != NULL)
            {
                VERIFY(pEdgeProx == NULL);
                pEdgeProx = pedge->pEdgeProx;
                VERIFY(pEdgeProx->pEdgeProx == pedge);
                pEdgeProx->pEdgeProx = this;
                pedge->pEdgeProx = NULL;
            }
            return TRUE;
        }
        else
        {
            // Nope - they don't match.
            return (FALSE);
        }
    }
    else
    {
        VERIFY(pPt1 == pedge->pPt2);
        VERIFY(pPt2 == pedge->pPt1);
        if ((pTri12 == NULL) && (pedge->pTri12 == NULL) && ((pEdgeProx == NULL) || (pedge->pEdgeProx == NULL)))
        {
            // Merge them.
            pTri12 = pedge->pTri21;
            if (pTri12 != NULL)
            {
                if (pTri12->pEdge12 == pedge)
                {
                    pTri12->pEdge12 = this;
                }
                else if (pTri12->pEdge23 == pedge)
                {
                    pTri12->pEdge23 = this;
                }
                else
                {
                    VERIFY(pTri12->pEdge31 == pedge);
                    pTri12->pEdge31 = this;
                }
            }
            pedge->pTri21 = NULL;
            if (pedge->pEdgeProx != NULL)
            {
                VERIFY(pEdgeProx == NULL);
                pEdgeProx = pedge->pEdgeProx;
                VERIFY(pEdgeProx->pEdgeProx == pedge);
                pEdgeProx->pEdgeProx = this;
                pedge->pEdgeProx = NULL;
            }
            return TRUE;
        }
        else if ((pTri21 == NULL) && (pedge->pTri21 == NULL) && ((pEdgeProx == NULL) || (pedge->pEdgeProx == NULL)))
        {
            // Merge them.
            pTri21 = pedge->pTri12;
            if (pTri21 != NULL)
            {
                if (pTri21->pEdge12 == pedge)
                {
                    pTri21->pEdge12 = this;
                }
                else if (pTri21->pEdge23 == pedge)
                {
                    pTri21->pEdge23 = this;
                }
                else
                {
                    VERIFY(pTri21->pEdge31 == pedge);
                    pTri21->pEdge31 = this;
                }
            }
            pedge->pTri12 = NULL;
            if (pedge->pEdgeProx != NULL)
            {
                VERIFY(pEdgeProx == NULL);
                pEdgeProx = pedge->pEdgeProx;
                VERIFY(pEdgeProx->pEdgeProx == pedge);
                pEdgeProx->pEdgeProx = this;
                pedge->pEdgeProx = NULL;
            }
            return TRUE;
        }
        else
        {
            // Nope - they don't match.
            return (FALSE);
        }
    }
}

inline MeshEdge* MeshEdge::QueryList(void)
{
    MeshEdge* pListRoot = ListFindFirst();
    if (pListRoot == this)
    {
        VERIFY(ListFindLast() == this);
        pListRoot = NULL;
    }
    return (pListRoot);
}

inline void MeshEdge::SetList(MeshEdge* pListRoot)
{
    ListDel();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

// Makes these two edges prox.
// The point prox data must agree.
// Returns TRUE on success, or FALSE if it failed.
inline bool MeshEdge::AddProx(MeshEdge* pEdge)
{
    VERIFY(pEdge != NULL);
    if (pEdgeProx != NULL)
    {
        // Already got prox.
        return (FALSE);
    }
    else if (pEdge->pEdgeProx != NULL)
    {
        // Already got prox.
        return (FALSE);
    }
    else
    {
        // Check that the pts agree.
        // Either pPt1<->pPt1 and pPt2<->pPt2, or the other way round.
        if (pEdge->pPt1->CheckProx(pPt1))
        {
            if (!pEdge->pPt2->CheckProx(pPt2))
            {
                return (FALSE);
            }
        }
        else if (pEdge->pPt1->CheckProx(pPt2))
        {
            if (!pEdge->pPt2->CheckProx(pPt1))
            {
                return (FALSE);
            }
        }
        else
        {
            return (FALSE);
        }

        // OK, must have passed.
        pEdgeProx = pEdge;
        pEdge->pEdgeProx = this;
        return (TRUE);
    }
}

// Find the proximity edge, if any.
// Relies on the point proximity values having been set up.
// If one is found, it is returned.
inline MeshEdge* MeshEdge::DoProxMatch(void)
{
    // Loop through all the prox pts to pPt1
    //		Loop through all their edges.
    //			If the other pt is prox to pPt2, then we found a prox edge.
    u32 i;
    MeshPt** ppPt = pPt1->ProxPtList.ptr();
    for (i = 0; i < pPt1->ProxPtList.size(); i++)
    {
        MeshPt* pPtProx = ppPt[i];
        VERIFY(pPtProx != NULL);

        MeshEdge* pEdgeOther = pPtProx->FirstEdge();
        while (pEdgeOther != NULL)
        {
            MeshPt* pPtOther = pEdgeOther->OtherPt(pPtProx);
            if (pPtOther->CheckProx(pPt2))
            {
                // Yes - this is prox.
                bool bRes = AddProx(pEdgeOther);
                if (bRes)
                {
                    pPtProx->EndEdge();
                    return pEdgeOther;
                }
                else
                {
                    // Too many edges trying to be prox!
                    pPtProx->EndEdge();
                    return NULL;
                }
            }

            pEdgeOther = pPtProx->NextEdge();
        }
    }
    return NULL;
}

// Removes any edge prox data.
// Returns TRUE if there was some.
// The pt prox data can still agree - it is not touched.
inline bool MeshEdge::RemoveProx(void)
{
    if (pEdgeProx == NULL)
    {
        return (FALSE);
    }
    else
    {
        VERIFY(pEdgeProx->pEdgeProx == this);
        pEdgeProx->pEdgeProx = NULL;
        pEdgeProx = NULL;
        return (TRUE);
    }
}

inline bool MeshEdge::ConsistencyCheck(MeshPt* pPtRoot, MeshEdge* pEdgeRoot, MeshTri* pTriRoot)
{
    bool bRes = TRUE;
    if ((pEdgeRoot != NULL) && (QueryList() != pEdgeRoot))
    {
        FAIL_CHECK();
    }

    if (pEdgeProx != NULL)
    {
        if (pEdgeProx->pEdgeProx != this)
        {
            FAIL_CHECK();
        }
        if ((pEdgeRoot != NULL) && (pEdgeProx->QueryList() != pEdgeRoot))
        {
            FAIL_CHECK();
        }
        // Either pPt1<->pPt1 and pPt2<->pPt2, or the other way round.
        if (pEdgeProx->pPt1->CheckProx(pPt1))
        {
            if (!pEdgeProx->pPt2->CheckProx(pPt2))
            {
                FAIL_CHECK();
            }
        }
        else if (pEdgeProx->pPt1->CheckProx(pPt2))
        {
            if (!pEdgeProx->pPt2->CheckProx(pPt1))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }

    if (pTri12 != NULL)
    {
        if ((pTriRoot != NULL) && (pTri12->QueryList() != pTriRoot))
        {
            FAIL_CHECK();
        }

        if (pTri12->pEdge12 == this)
        {
            if ((pTri12->pPt1 != pPt1) || (pTri12->pPt2 != pPt2))
            {
                FAIL_CHECK();
            }
        }
        else if (pTri12->pEdge23 == this)
        {
            if ((pTri12->pPt2 != pPt1) || (pTri12->pPt3 != pPt2))
            {
                FAIL_CHECK();
            }
        }
        else if (pTri12->pEdge31 == this)
        {
            if ((pTri12->pPt3 != pPt1) || (pTri12->pPt1 != pPt2))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }

    if (pTri21 != NULL)
    {
        if ((pTriRoot != NULL) && (pTri21->QueryList() != pTriRoot))
        {
            FAIL_CHECK();
        }

        if (pTri21->pEdge12 == this)
        {
            if ((pTri21->pPt1 != pPt2) || (pTri21->pPt2 != pPt1))
            {
                FAIL_CHECK();
            }
        }
        else if (pTri21->pEdge23 == this)
        {
            if ((pTri21->pPt2 != pPt2) || (pTri21->pPt3 != pPt1))
            {
                FAIL_CHECK();
            }
        }
        else if (pTri21->pEdge31 == this)
        {
            if ((pTri21->pPt3 != pPt2) || (pTri21->pPt1 != pPt1))
            {
                FAIL_CHECK();
            }
        }
        else
        {
            FAIL_CHECK();
        }
    }

    if ((pPt1 == NULL) || (pPt2 == NULL))
    {
        FAIL_CHECK();
    }
    else
    {
        if (pPtRoot != NULL)
        {
            if (pPt1->QueryList() != pPtRoot)
            {
                FAIL_CHECK();
            }
            if (pPt2->QueryList() != pPtRoot)
            {
                FAIL_CHECK();
            }
        }
    }

    return (bRes);
}

inline MeshPt::MeshPt(MeshPt* pListRoot)
{
    iCurEdgeNum = -1;
    iCurTriNum = -1;
    iCurProxNum = -1;

    ListInit();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

inline MeshPt::~MeshPt(void)
{
    // Can't just do a simple loop - RemoveProx modifies the list.
    while (ProxPtList.size() > 0)
    {
        bool bRes = RemoveProx(ProxPtList.ptr()[0]);
        VERIFY(bRes);
    }

    // Should not be any tris.
    VERIFY(TriList.size() == 0);

    // Should not be any edges.
    VERIFY(EdgeList.size() == 0);

    ListDel();
}

inline void MeshPt::RemoveEdge(MeshEdge* pEdge)
{
    VERIFY(pEdge != NULL);
    MeshEdge** ppEdgeList = EdgeList.ptr();
    u32 i;
    for (i = 0; i < EdgeList.size(); i++)
    {
        if (ppEdgeList[i] == pEdge)
        {
            break;
        }
    }
    VERIFY(i < EdgeList.size());
    // Bin this entry.
    EdgeList.erase_fast(i);
}

inline void MeshPt::RemoveTri(MeshTri* pTri)
{
    VERIFY(pTri != NULL);
    MeshTri** ppTriList = TriList.ptr();
    u32 i;
    for (i = 0; i < TriList.size(); i++)
    {
        if (ppTriList[i] == pTri)
        {
            break;
        }
    }
    VERIFY(i < TriList.size());
    // Bin this entry with the last entry.
    TriList.erase_fast(i);
}

inline void MeshPt::AddTri(MeshTri* pTri)
{
    VERIFY(pTri != NULL);
#ifdef DEBUG
    // Make sure this hasn't been added already.
    MeshTri** ppTriList = TriList.ptr();
    u32 i;
    for (i = 0; i < TriList.size(); i++)
    {
        VERIFY(ppTriList[i] != NULL);
        VERIFY(ppTriList[i] != pTri);
    }
#endif
    *(TriList.append()) = pTri;
}

inline void MeshPt::AddEdge(MeshEdge* pEdge)
{
    VERIFY(pEdge != NULL);
#ifdef DEBUG
    // Make sure this hasn't been added already.
    u32 i;
    MeshEdge** ppEdgeList = EdgeList.ptr();
    for (i = 0; i < EdgeList.size(); i++)
    {
        VERIFY(ppEdgeList[i] != NULL);
        VERIFY(ppEdgeList[i] != pEdge);
    }
#endif
    *(EdgeList.append()) = pEdge;
}

inline MeshEdge* MeshPt::FindEdge(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    VERIFY(pPt != this);
    MeshEdge** ppEdgeList = EdgeList.ptr();
    u32 i;
    for (i = 0; i < EdgeList.size(); i++)
    {
        MeshEdge* pEdge = ppEdgeList[i];
        VERIFY(pEdge != NULL);
        VERIFY((pEdge->pPt1 == this) || (pEdge->pPt2 == this));
        if (pEdge->pPt2 == pPt)
        {
            VERIFY(pEdge->pPt1 == this);
            return (pEdge);
        }
        if (pEdge->pPt1 == pPt)
        {
            VERIFY(pEdge->pPt2 == this);
            return (pEdge);
        }
    }
    return (NULL);
}

// Find the first edge that uses this pt and the other given, and
// also has a free triangle entry, assuming that the points are
// used in that clockwise order. This allows two edges that share
// the same points to exist, e.g. where multiple triangles share
// the same edge (think of the diagonal of the tris of a back-to-back
// quad - same edge, four tris.
// The tri will use the points in the order *this,*pPt.
inline MeshEdge* MeshPt::FindTriEdge(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    VERIFY(pPt != this);
    MeshEdge** ppEdgeList = EdgeList.ptr();
    u32 i;
    for (i = 0; i < EdgeList.size(); i++)
    {
        MeshEdge* pEdge = ppEdgeList[i];
        VERIFY(pEdge != NULL);
        VERIFY((pEdge->pPt1 == this) || (pEdge->pPt2 == this));
        if (pEdge->pPt2 == pPt)
        {
            VERIFY(pEdge->pPt1 == this);
            // Check that it would be possible to add a tri to this.
            // The tri will use this,pPt in that order, so must be in
            // pTri12
            if (pEdge->pTri12 == NULL)
            {
                return (pEdge);
            }
            //			else
            //			{
            //				int bogus = 0;
            //			}
        }
        if (pEdge->pPt1 == pPt)
        {
            VERIFY(pEdge->pPt2 == this);
            // Check that it would be possible to add a tri to this.
            // The tri will use this,pPt in that order, so must be in
            // pTri21
            if (pEdge->pTri21 == NULL)
            {
                return (pEdge);
            }
            //			else
            //			{
            //				int bogus = 0;
            //			}
        }
    }
    return (NULL);
}

inline MeshTri* MeshPt::FindTri(MeshPt* pPt1, MeshPt* pPt2)
{
    VERIFY(pPt1 != NULL);
    VERIFY(pPt2 != NULL);
    MeshTri** ppTriList = TriList.ptr();
    u32 i;
    for (i = 0; i < TriList.size(); i++)
    {
        MeshTri* pTri = ppTriList[i];
        VERIFY(pTri != NULL);
        VERIFY((pTri->pPt1 == this) || (pTri->pPt2 == this) || (pTri->pPt3 == this));
        if ((pTri->pPt1 == this) && (pTri->pPt2 == pPt1) && (pTri->pPt3 == pPt2))
        {
            return (pTri);
        }
        if ((pTri->pPt2 == this) && (pTri->pPt3 == pPt1) && (pTri->pPt1 == pPt2))
        {
            return (pTri);
        }
        if ((pTri->pPt3 == this) && (pTri->pPt1 == pPt1) && (pTri->pPt2 == pPt2))
        {
            return (pTri);
        }
    }
    return (NULL);
}

// Return the next tri in the list.
// If a non-NULL pPt is supplied, only tris using this,pPt in that order
// are returned, otherwise all tris are returned.
inline MeshTri* MeshPt::NextTri(MeshPt* pPt)
{
    VERIFY(this != pPt);
    VERIFY(iCurTriNum >= 0);
    while (TRUE)
    {
        if (iCurTriNum < (int)TriList.size())
        {
            MeshTri* pTri = (TriList.ptr())[iCurTriNum++];
            VERIFY(pTri != NULL);
            if (pPt == NULL)
            {
                // Return all tris.
                return (pTri);
            }

            // Return only tris that use this,pPt
            if (((pTri->pPt1 == this) && (pTri->pPt2 == pPt)) || ((pTri->pPt2 == this) && (pTri->pPt3 == pPt)) ||
            ((pTri->pPt3 == this) && (pTri->pPt1 == pPt)))
            {
                return (pTri);
            }
        }
        else
        {
            // End of the list.
            iCurTriNum = -1;
            return (NULL);
        }
    }
}

// Return the first tri in the list. MUST be called before calling NextTri().
// If a non-NULL pPt is supplied, only tris using this,pPt in that order
// are returned, otherwise all tris are returned.
inline MeshTri* MeshPt::FirstTri(MeshPt* pPt)
{
    VERIFY(iCurTriNum == -1);
    iCurTriNum = 0;
    return (NextTri(pPt));
}

// Terminate the current First/Next loop.
inline void MeshPt::EndTri(void) { iCurTriNum = -1; }
// Return the next Edge in the list.
// If a non-NULL pPt is supplied, only edges using this and pPt
// are returned, otherwise all edges are returned.
inline MeshEdge* MeshPt::NextEdge(MeshPt* pPt)
{
    VERIFY(this != pPt);
    VERIFY(iCurEdgeNum >= 0);
    while (TRUE)
    {
        if (iCurEdgeNum < (int)EdgeList.size())
        {
            MeshEdge* pEdge = (EdgeList.ptr())[iCurEdgeNum++];
            VERIFY(pEdge != NULL);
            if (pPt == NULL)
            {
                // Return all edges.
                return (pEdge);
            }

            // Return only the edges the use this & pPt.
            if ((pEdge->pPt1 == pPt) || (pEdge->pPt2 == pPt))
            {
                VERIFY((pEdge->pPt1 == this) || (pEdge->pPt2 == this));
                return (pEdge);
            }
        }
        else
        {
            // End of the list.
            iCurEdgeNum = -1;
            return (NULL);
        }
    }
}

// Return the first Edge in the list. MUST be called before calling NextEdge().
// If a non-NULL pPt is supplied, only edges using this and pPt
// are returned, otherwise all edges are returned.
inline MeshEdge* MeshPt::FirstEdge(MeshPt* pPt)
{
    iCurEdgeNum = 0;
    return (NextEdge(pPt));
}

// Terminate the current First/Next loop.
inline void MeshPt::EndEdge(void) { iCurEdgeNum = -1; }
// Returns TRUE if the two pts are marked as being in proximity.
inline bool MeshPt::CheckProx(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    MeshPt** ppPt = ProxPtList.ptr();
    for (u32 i = 0; i < ProxPtList.size(); i++)
    {
        VERIFY(ppPt[i] != NULL);
        if (ppPt[i] == pPt)
        {
            // Yes.
            return (TRUE);
        }
    }
    return (FALSE);
}

// Add the given pt to the prox list (and vice versa).
// If the pt was not already there, returns TRUE;
// If bProxEdges is set to TRUE (default is FALSE ),
// the edges that these two pts use are made prox if possible.
inline bool MeshPt::AddProx(MeshPt* pPt, bool bProxEdges)
{
    bool bRes;

    VERIFY(pPt != NULL);
    if (CheckProx(pPt))
    {
        // Already prox.
        VERIFY(pPt->CheckProx(this));
        bRes = FALSE;
    }
    else
    {
        VERIFY(!pPt->CheckProx(this));

        // Add to this pt.
        *(ProxPtList.append()) = pPt;

        // Add to the other pt.
        *(pPt->ProxPtList.append()) = this;

        bRes = TRUE;
    }

    // Now check all their edges for proximity.
    // For each edge of this.
    //		Find other pt and scan proxs of that.
    //			If those proxes form and edge with pPt, the edges are prox.
    MeshEdge* pedge = FirstEdge();
    while (pedge != NULL)
    {
        MeshPt* pptOther = pedge->OtherPt(this);
        MeshPt** ppPt = pptOther->ProxPtList.ptr();
        for (u32 i = 0; i < pptOther->ProxPtList.size(); i++)
        {
            VERIFY(ppPt[i] != NULL);
            MeshEdge* pedgeProx = pPt->FindEdge(ppPt[i]);
            if (pedgeProx != NULL)
            {
                bool bRes = pedgeProx->AddProx(pedge);
                VERIFY(bRes);
                break;
            }
        }

        pedge = NextEdge();
    }

    return (bRes);
}

// Remove the given pt from the prox list (and vice versa).
// If the pt was there, returns TRUE.
inline bool MeshPt::RemoveProx(MeshPt* pPt)
{
    VERIFY(pPt != NULL);
    if (CheckProx(pPt))
    {
        // Yep, they are prox.
        VERIFY(pPt->CheckProx(this));

        MeshPt** ppPtList;
        u32 i;

        // Remove pPt from this.
        ppPtList = ProxPtList.ptr();
        for (i = 0; i < ProxPtList.size(); i++)
        {
            if (ppPtList[i] == pPt)
            {
                break;
            }
        }
        VERIFY(i < ProxPtList.size());
        // Replace this entry with the last entry.
        ProxPtList.erase_fast(i);

        // Remove this from pPt.
        ppPtList = pPt->ProxPtList.ptr();
        for (i = 0; i < pPt->ProxPtList.size(); i++)
        {
            if (ppPtList[i] == this)
            {
                break;
            }
        }
        VERIFY(i < pPt->ProxPtList.size());
        // Replace this entry with the last entry.
        ProxPtList.erase_fast(i);

        return (TRUE);
    }
    else
    {
        // No, they're not prox.
        VERIFY(!pPt->CheckProx(this));
        return (FALSE);
    }
}

// Return the first prox pt. MUST be called before calling NextProx().
inline MeshPt* MeshPt::FirstProx(void)
{
    VERIFY(iCurProxNum == -1);
    iCurProxNum = 0;
    return (NextProx());
}

// Return the next prox pt.
inline MeshPt* MeshPt::NextProx(void)
{
    VERIFY(iCurProxNum >= 0);
    while (TRUE)
    {
        if (iCurProxNum < (int)ProxPtList.size())
        {
            MeshPt* pptProx = (ProxPtList.ptr())[iCurProxNum++];
            VERIFY(pptProx != NULL);
            return (pptProx);
        }
        else
        {
            // End of the list.
            iCurProxNum = -1;
            return (NULL);
        }
    }
}

// Terminate the current First/Next loop.
inline void MeshPt::EndProx(void) { iCurProxNum = -1; }
inline MeshPt* MeshPt::QueryList(void)
{
    MeshPt* pListRoot = ListFindFirst();
    if (pListRoot == this)
    {
        VERIFY(ListFindLast() == this);
        pListRoot = NULL;
    }
    return (pListRoot);
}

inline void MeshPt::SetList(MeshPt* pListRoot)
{
    ListDel();
    if (pListRoot != NULL)
    {
        ListAddAfter(pListRoot);
    }
}

inline bool MeshPt::ConsistencyCheck(MeshPt* pPtRoot, MeshEdge* pEdgeRoot, MeshTri* pTriRoot)
{
    bool bRes = TRUE;
    if ((pPtRoot != NULL) && (QueryList() != pPtRoot))
    {
        FAIL_CHECK();
    }

    // Check prox.
    MeshPt** ppPt = ProxPtList.ptr();
    for (u32 i = 0; i < ProxPtList.size(); i++)
    {
        VERIFY(ppPt[i] != NULL);
        if (!ppPt[i]->CheckProx(this))
        {
            FAIL_CHECK();
        }
        if ((pPtRoot != NULL) && (ppPt[i]->QueryList() != pPtRoot))
        {
            FAIL_CHECK();
        }
    }

    // Just check the consistency of all tris and edges that use this.
    MeshEdge* pEdge = FirstEdge();
    while (pEdge != NULL)
    {
        if (!pEdge->ConsistencyCheck(pPtRoot, pEdgeRoot, pTriRoot))
        {
            // Will have already VERIFYed.
            bRes = FALSE;
        }
        pEdge = NextEdge();
    }
    MeshTri* pTri = FirstTri();
    while (pTri != NULL)
    {
        if (!pTri->ConsistencyCheck(pPtRoot, pEdgeRoot, pTriRoot))
        {
            // Will have already VERIFYed.
            bRes = FALSE;
        }
        pTri = NextTri();
    }
    return (bRes);
}
