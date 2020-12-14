///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *  OPCODE - Optimized Collision Detection
 *  Copyright (C) 2001 Pierre Terdiman
 *  Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Contains code for OPCODE models.
 *  \file       OPC_Model.cpp
 *  \author     Pierre Terdiman
 *  \date       March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  The main collision wrapper, for all trees. Supported trees are:
 *  - Normal trees (2*N-1 nodes, full size)
 *  - No-leaf trees (N-1 nodes, full size)
 *  - Quantized trees (2*N-1 nodes, half size)
 *  - Quantized no-leaf trees (N-1 nodes, half size)
 *
 *  Usage:
 *
 *  1) Build an OPCODE_Model using a creation structure:
 *
 *  \code
 *      OPCODE_Model Sample;
 *
 *      OPCODECREATE OPCC;
 *      OPCC.NbTris         = ...;
 *      OPCC.NbVerts        = ...;
 *      OPCC.Tris           = ...;
 *      OPCC.Verts          = ...;
 *      OPCC.Rules          = ...;
 *      OPCC.NoLeaf         = ...;
 *      OPCC.Quantized      = ...;
 *      OPCC.KeepOriginal   = ...;
 *      bool Status = Sample.Build(OPCC);
 *  \endcode
 *
 *  2) Create a tree collider and setup it:
 *
 *  \code
 *      AABBTreeCollider TC;
 *      TC.SetFirstContact(...);
 *      TC.SetFullBoxBoxTest(...);
 *      TC.SetFullPrimBoxTest(...);
 *      TC.SetTemporalCoherence(...);
 *  \endcode
 *
 *  3) Setup object callbacks. Geometry & topology are NOT stored in the collision system,
 *  in order to save some ram. So, when the system needs them to perform accurate intersection
 *  tests, you're requested to provide the triangle-vertices corresponding to a given face index.
 *
 *  Ex:
 *
 *  \code
 *      static void ColCallback(udword triangleindex, VertexPointers& triangle, udword user_data)
 *      {
 *          // Get back Mesh0 or Mesh1 (you also can use 2 different callbacks)
 *          Mesh* MyMesh = (Mesh*)user_data;
 *          // Get correct triangle in the app-controlled database
 *          const Triangle* Tri = MyMesh->GetTriangle(triangleindex);
 *          // Setup pointers to vertices for the collision system
 *          triangle.Vertex[0] = MyMesh->GetVertex(Tri->mVRef[0]);
 *          triangle.Vertex[1] = MyMesh->GetVertex(Tri->mVRef[1]);
 *          triangle.Vertex[2] = MyMesh->GetVertex(Tri->mVRef[2]);
 *      }
 *
 *      // Setup callbacks
 *      TC.SetCallback0(ColCallback, udword(Mesh0));
 *      TC.SetCallback1(ColCallback, udword(Mesh1));
 *  \endcode
 *
 *  Of course, you should make this callback as fast as possible. And you're also not supposed
 *  to modify the geometry *after* the collision trees have been built. The alternative was to
 *  store the geometry & topology in the collision system as well (as in RAPID) but we have found
 *  this approach to waste a lot of ram in many cases.
 *
 *  Since version 1.2 you can also use plain pointers. It's a tiny bit faster, but not as safe.
 *
 *  Ex:
 *
 *  \code
 *      TC.SetPointers0(Mesh0->GetFaces(), Mesh0->GetVerts());
 *      TC.SetPointers1(Mesh1->GetFaces(), Mesh1->GetVerts());
 *  \endcode
 *
 *  4) Perform a collision query
 *
 *  \code
 *      // Setup cache
 *      static BVTCache ColCache;
 *      ColCache.Model0 = &Model0;
 *      ColCache.Model1 = &Model1;
 *
 *      // Collision query
 *      bool IsOk = TC.Collide(ColCache, World0, World1);
 *
 *      // Get collision status => if true, objects overlap
 *      BOOL Status = TC.GetContactStatus();
 *
 *      // Number of colliding pairs and list of pairs
 *      udword NbPairs = TC.GetNbPairs();
 *      const Pair* p = TC.GetPairs()
 *  \endcode
 *
 *  5) Stats
 *
 *  \code
 *      Model0.GetUsedBytes()   = number of bytes used for this collision tree
 *      TC.GetNbBVBVTests()     = number of BV-BV overlap tests performed during last query
 *      TC.GetNbPrimPrimTests() = number of Triangle-Triangle overlap tests performed during last query
 *      TC.GetNbBVPrimTests()   = number of Triangle-BV overlap tests performed during last query
 *  \endcode
 *
 *  \class      OPCODE_Model
 *  \author     Pierre Terdiman
 *  \version    1.2
 *  \date       March, 20, 2001
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "pch.hpp"

namespace Opcode
{
#include "OPC_TreeBuilders.h"
} // namespace Opcode

using namespace Opcode;

OPCODECREATE::OPCODECREATE()
{
    NbTris = 0;
    NbVerts = 0;
    Tris = nullptr;
    Verts = nullptr;
    Rules = SPLIT_COMPLETE | SPLIT_LARGESTAXIS;
    NoLeaf = true;
    Quantized = true;
#ifdef __MESHMERIZER_H__
    CollisionHull = false;
#endif // __MESHMERIZER_H__
    KeepOriginal = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPCODE_Model::OPCODE_Model() : mSource(nullptr), mTree(nullptr), mNoLeaf(false), mQuantized(false)
{
#ifdef __MESHMERIZER_H__ // Collision hulls only supported within ICE !
    mHull = null;
#endif // __MESHMERIZER_H__
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPCODE_Model::~OPCODE_Model()
{
    xr_delete(mSource);
    xr_delete(mTree);
#ifdef __MESHMERIZER_H__ // Collision hulls only supported within ICE !
    xr_delete(mHull);
#endif // __MESHMERIZER_H__
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Builds a collision model.
 *  \param      create      [in] model creation structure
 *  \return     true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OPCODE_Model::Build(const OPCODECREATE& create)
{
    // 1) Checkings
    if (!create.NbTris || !create.Tris || !create.Verts)
        return false;

    // In this lib, we only support complete trees
    if (!(create.Rules & SPLIT_COMPLETE))
        return SetIceError; //("OPCODE WARNING: supports complete trees only! Use SPLIT_COMPLETE.\n");

    // Check topology. If the model contains degenerate faces, collision report can be wrong in some cases.
    // e.g. it happens with the standard MAX teapot. So clean your meshes first... If you don't have a mesh cleaner
    // you can try this: www.codercorner.com/Consolidation.zip
    const IndexedTriangle* Tris = (const IndexedTriangle*)create.Tris;
    udword NbDegenerate = 0;
    for (udword i = 0; i < create.NbTris; i++)
    {
        if (Tris[i].IsDegenerate())
            NbDegenerate++;
    }
    if (NbDegenerate)
    {
        Log("OPCODE WARNING: found %d degenerate faces in model! Collision might report wrong results!\n",
            NbDegenerate);
    }
    // We continue nonetheless....

    // 2) Build a generic AABB Tree.
    mSource = new AABBTree();
    CHECKALLOC(mSource);

    // 2-1) Setup a builder. Our primitives here are triangles from input mesh,
    // so we use an AABBTreeOfTrianglesBuilder.....
    AABBTreeOfTrianglesBuilder TB;
    TB.mTriList = Tris;
    TB.mVerts = create.Verts;
    TB.mRules = create.Rules;
    TB.mNbPrimitives = create.NbTris;
    if (!mSource->Build(&TB))
        return false;

    // 3) Create an optimized tree according to user-settings
    // 3-1) Create the correct class
    mNoLeaf = create.NoLeaf;
    mQuantized = create.Quantized;

    if (mNoLeaf)
    {
        if (mQuantized)
            mTree = new AABBQuantizedNoLeafTree();
        else
            mTree = new AABBNoLeafTree();
    }
    else
    {
        if (mQuantized)
            mTree = new AABBQuantizedTree();
        else
            mTree = new AABBCollisionTree();
    }

    // 3-2) Create optimized tree
    if (!mTree->Build(mSource))
        return false;

    // 3-3) Delete generic tree if needed
    if (!create.KeepOriginal)
    {
        mSource->destroy(&TB);
        xr_delete(mSource);
    }

#ifdef __MESHMERIZER_H__
    // 4) Convex hull
    if (create.CollisionHull)
    {
        // Create hull
        mHull = new CollisionHull();
        CHECKALLOC(mHull);

        CONVEXHULLCREATE CHC;
        CHC.NbVerts = create.NbVerts;
        CHC.Vertices = create.Verts;
        CHC.UnifyNormals = true;
        CHC.ReduceVertices = true;
        CHC.WordFaces = false;
        mHull->Compute(CHC);
    }
#endif // __MESHMERIZER_H__
    return true;
}

void OPCODE_Model::Load(IReader* stream)
{
    mNoLeaf    = stream->r_u32();
    mQuantized = stream->r_u32();

    const u32 nodesNum = stream->r_u32();
    if (mNoLeaf && !mQuantized)
    {
        mTree = xr_new<AABBNoLeafTree>();
        auto* ptr = xr_alloc<AABBNoLeafNode>(nodesNum);
        const u32 size = nodesNum * sizeof(AABBNoLeafNode);
        CopyMemory(ptr, stream->pointer(), size);
        for (int i = 0; i < nodesNum; ++i)
        {
            const auto* rootPtr = &ptr[0];
            if (!ptr[i].HasLeaf())
                ptr[i].mData  += (uintptr_t)rootPtr;
            if (!ptr[i].HasLeaf2())
                ptr[i].mData2 += (uintptr_t)rootPtr;
        }
        mTree->SetData(ptr, nodesNum);
    }
    else
    {
        NODEFAULT; // Not used by engine so not supported
    }
}

void OPCODE_Model::Save(IWriter* stream) const
{
    stream->w_u32(mNoLeaf);
    stream->w_u32(mQuantized);

    const u32 nodesNum = mTree->GetNbNodes();
    stream->w_u32(mTree->GetNbNodes());

    void* pData = nullptr;
    if (mNoLeaf && !mQuantized)
    {
        auto* ptr = xr_alloc<AABBNoLeafNode>(nodesNum);
        const u32 size = nodesNum * sizeof(AABBNoLeafNode);
        R_ASSERT(size == mTree->GetUsedBytes());
        CopyMemory(ptr, mTree->GetData(), size);
        pData = ptr;
        for (int i = 0; i < nodesNum; ++i, ++ptr)
        {
            const auto* rootPtr = mTree->GetData();
            if (!ptr->HasLeaf())
                ptr->mData  -= (uintptr_t)rootPtr;
            if (!ptr->HasLeaf2())
                ptr->mData2 -= (uintptr_t)rootPtr;
        }
    }
    else
    {
        NODEFAULT; // Not used by engine so not supported
    }
    stream->w(pData, mTree->GetUsedBytes());
    xr_free(pData);
}
