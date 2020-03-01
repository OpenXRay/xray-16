///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for an AABB collider.
 *	\file		OPC_AABBCollider.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 1st, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains an AABB-vs-tree collider.
 *
 *	\class		AABBCollider
 *	\author		Pierre Terdiman
 *	\version	1.2
 *	\date		January, 1st, 2002
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "pch.hpp"

using namespace Opcode;

#include "OPC_BoxBoxOverlap.h"
#include "OPC_TriBoxOverlap.h"

//! AABB-triangle test
#ifdef OPC_USE_CALLBACKS
#define AABB_PRIM(primindex, flag)\
    /* Request vertices from the app */\
    VertexPointers VP;\
    (mObjCallback)(primindex, VP, mUserData);\
    mLeafVerts[0] = *VP.Vertex[0];\
    mLeafVerts[1] = *VP.Vertex[1];\
    mLeafVerts[2] = *VP.Vertex[2];\
    /* Perform triangle-box overlap test */\
    if (TriBoxOverlap())\
    {\
        /* Set contact status */\
        mFlags |= flag;\
        mTouchedPrimitives->Add(primindex);\
    }
#else
#define AABB_PRIM(primindex, flag)\
    /* Direct access to vertices */\
    const IndexedTriangle* T = &mFaces[primindex];\
    mLeafVerts[0] = mVerts[T->mVRef[0]];\
    mLeafVerts[1] = mVerts[T->mVRef[1]];\
    mLeafVerts[2] = mVerts[T->mVRef[2]];\
    /* Perform triangle-box overlap test */\
    if (TriBoxOverlap())\
    {\
        /* Set contact status */\
        mFlags |= flag;\
        mTouchedPrimitives->Add(primindex);\
    }
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBCollider::AABBCollider() {}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBCollider::~AABBCollider() {}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Validates current settings. You should call this method after all the settings and callbacks have been defined.
 *	\return		null if everything is ok, else a string describing the problem
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* AABBCollider::ValidateSettings() { return VolumeCollider::ValidateSettings(); }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Generic collision query for generic OPCODE models. After the call, access the results:
 *	- with GetContactStatus()
 *	- with GetNbTouchedFaces()
 *	- with GetTouchedFaces()
 *
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		model		[in] Opcode model to collide with
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, OPCODE_Model* model)
{
    // Checkings
    if (!model)
        return false;

    // Simple double-dispatch
    if (!model->HasLeafNodes())
    {
        if (model->IsQuantized())
            return Collide(cache, box, (const AABBQuantizedNoLeafTree*)model->GetTree());
        else
            return Collide(cache, box, (const AABBNoLeafTree*)model->GetTree());
    }
    else
    {
        if (model->IsQuantized())
            return Collide(cache, box, (const AABBQuantizedTree*)model->GetTree());
        else
            return Collide(cache, box, (const AABBCollisionTree*)model->GetTree());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes a collision query :
 *	- reset stats & contact status
 *	- check temporal coherence
 *
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] AABB in world space
 *	\return		contact status
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AABBCollider::InitQuery(AABBCache& cache, const CollisionAABB& box)
{
    // 1) Call the base method
    VolumeCollider::InitQueryEx();

    // 2) Keep track of the query box
    mBox = box;

    // 3) Setup destination pointer
    mTouchedPrimitives = &cache.TouchedPrimitives;

    // 4) Check temporal coherence :
    if (TemporalCoherenceEnabled())
    {
        // Here we use temporal coherence
        // => check results from previous frame before performing the collision query
        if (FirstContactEnabled())
        {
            // We're only interested in the first contact found => test the unique previously touched face
            if (mTouchedPrimitives->GetNbEntries())
            {
                // Get index of previously touched face = the first entry in the array
                udword PreviouslyTouchedFace = mTouchedPrimitives->GetEntry(0);

                // Then reset the array:
                // - if the overlap test below is successful, the index we'll get added back anyway
                // - if it isn't, then the array should be reset anyway for the normal query
                mTouchedPrimitives->Reset();

                // Perform overlap test between the cached triangle and the box (and set contact status if needed)
                AABB_PRIM(PreviouslyTouchedFace, OPC_TEMPORAL_CONTACT)
            }
            // else no face has been touched during previous query
            // => we'll have to perform a normal query
        }
        else
        {
            // We're interested in all contacts =>test the _new_ real box N(ew) against the previous fat box P(revious):
            if (mBox.IsInside(cache.FatBox))
            {
                // - if N is included in P, return previous list
                // => we simply leave the list (mTouchedFaces) unchanged

                // Set contact status if needed
                if (mTouchedPrimitives->GetNbEntries())
                    mFlags |= OPC_TEMPORAL_CONTACT;
            }
            else
            {
                // - else do the query using a fat N

                // Reset cache since we'll about to perform a real query
                mTouchedPrimitives->Reset();

                // Make a fat box so that coherence will work for subsequent frames
                mBox.mExtents *= cache.FatCoeff;

                // Update cache with query data (signature for cached faces)
                cache.FatBox = mBox;
            }
        }
    }
    else
    {
        // Here we don't use temporal coherence => do a normal query
        mTouchedPrimitives->Reset();
    }

    // 5) Precompute min & max bounds if needed
    if (!GetContactStatus())
    {
        mMin = box.mCenter - box.mExtents;
        mMax = box.mCenter + box.mExtents;
    }

    return GetContactStatus();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for normal trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		tree		[in] model's AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, const AABBCollisionTree* tree)
{
    // Checkings
    if (!tree)
        return false;
#ifdef OPC_USE_CALLBACKS
    if (!mObjCallback)
        return false;
#else
    if (!mFaces || !mVerts)
        return false;
#endif

    // Init collision query
    if (InitQuery(cache, box))
        return true;

    // Perform collision query
    _Collide(tree->GetNodes());

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for no-leaf trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		tree		[in] model's AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, const AABBNoLeafTree* tree)
{
    // Checkings
    if (!tree)
        return false;
#ifdef OPC_USE_CALLBACKS
    if (!mObjCallback)
        return false;
#else
    if (!mFaces || !mVerts)
        return false;
#endif

    // Init collision query
    if (InitQuery(cache, box))
        return true;

    // Perform collision query
    _Collide(tree->GetNodes());

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		tree		[in] model's AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, const AABBQuantizedTree* tree)
{
    // Checkings
    if (!tree)
        return false;
#ifdef OPC_USE_CALLBACKS
    if (!mObjCallback)
        return false;
#else
    if (!mFaces || !mVerts)
        return false;
#endif

    // Init collision query
    if (InitQuery(cache, box))
        return true;

    // Setup dequantization coeffs
    mCenterCoeff = tree->mCenterCoeff;
    mExtentsCoeff = tree->mExtentsCoeff;

    // Perform collision query
    _Collide(tree->GetNodes());

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized no-leaf trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		tree		[in] model's AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, const AABBQuantizedNoLeafTree* tree)
{
    // Checkings
    if (!tree)
        return false;
#ifdef OPC_USE_CALLBACKS
    if (!mObjCallback)
        return false;
#else
    if (!mFaces || !mVerts)
        return false;
#endif

    // Init collision query
    if (InitQuery(cache, box))
        return true;

    // Setup dequantization coeffs
    mCenterCoeff = tree->mCenterCoeff;
    mExtentsCoeff = tree->mExtentsCoeff;

    // Perform collision query
    _Collide(tree->GetNodes());

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for vanilla AABB trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision AABB in world space
 *	\param		tree		[in] AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBCollider::Collide(AABBCache& cache, const CollisionAABB& box, const AABBTree* tree)
{
    // This is typically called for a scene tree, full of -AABBs-, not full of triangles.
    // So we don't really have "primitives" to deal with. Hence it doesn't work with
    // "FirstContact" + "TemporalCoherence".
    ASSERT(!(FirstContactEnabled() && TemporalCoherenceEnabled()));

    // Checkings
    if (!tree)
        return false;

    // Init collision query
    if (InitQuery(cache, box))
        return true;

    // Perform collision query
    _Collide(tree);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks the AABB completely contains the box. In which case we can end the query sooner.
 *	\param		bc	[in] box center
 *	\param		be	[in] box extents
 *	\return		true if the AABB contains the whole box
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL AABBCollider::AABBContainsBox(const Point& bc, const Point& be)
{
    if (mMin.x > bc.x - be.x)
        return FALSE;
    if (mMin.y > bc.y - be.y)
        return FALSE;
    if (mMin.z > bc.z - be.z)
        return FALSE;

    if (mMax.x < bc.x + be.x)
        return FALSE;
    if (mMax.y < bc.y + be.y)
        return FALSE;
    if (mMax.z < bc.z + be.z)
        return FALSE;

    return TRUE;
}

#define TEST_AABB_IN_BOX(center, extents)\
    if (AABBContainsBox(center, extents))\
    {\
        /* Set contact status */\
        mFlags |= OPC_CONTACT;\
        _Dump(node);\
        return;\
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for normal AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBCollider::_Collide(const AABBCollisionNode* node)
{
    // Perform AABB-AABB overlap test
    if (!AABBAABBOverlap(node->mAABB.mExtents, node->mAABB.mCenter))
        return;

    TEST_AABB_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

    if (node->IsLeaf())
    {
        AABB_PRIM(node->GetPrimitive(), OPC_CONTACT)
    }
    else
    {
        _Collide(node->GetPos());

        if (ContactFound())
            return;

        _Collide(node->GetNeg());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBCollider::_Collide(const AABBQuantizedNode* node)
{
    // Dequantize box
    const QuantizedAABB* Box = &node->mAABB;
    const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y,
    float(Box->mCenter[2]) * mCenterCoeff.z);
    const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y,
    float(Box->mExtents[2]) * mExtentsCoeff.z);

    // Perform AABB-AABB overlap test
    if (!AABBAABBOverlap(Extents, Center))
        return;

    TEST_AABB_IN_BOX(Center, Extents)

    if (node->IsLeaf())
    {
        AABB_PRIM(node->GetPrimitive(), OPC_CONTACT)
    }
    else
    {
        _Collide(node->GetPos());

        if (ContactFound())
            return;

        _Collide(node->GetNeg());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBCollider::_Collide(const AABBNoLeafNode* node)
{
    // Perform AABB-AABB overlap test
    if (!AABBAABBOverlap(node->mAABB.mExtents, node->mAABB.mCenter))
        return;

    TEST_AABB_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

    if (node->HasLeaf())
    {
        AABB_PRIM(node->GetPrimitive(), OPC_CONTACT)
    }
    else
        _Collide(node->GetPos());

    if (ContactFound())
        return;

    if (node->HasLeaf2())
    {
        AABB_PRIM(node->GetPrimitive2(), OPC_CONTACT)
    }
    else
        _Collide(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBCollider::_Collide(const AABBQuantizedNoLeafNode* node)
{
    // Dequantize box
    const QuantizedAABB* Box = &node->mAABB;
    const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y,
    float(Box->mCenter[2]) * mCenterCoeff.z);
    const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y,
    float(Box->mExtents[2]) * mExtentsCoeff.z);

    // Perform AABB-AABB overlap test
    if (!AABBAABBOverlap(Extents, Center))
        return;

    TEST_AABB_IN_BOX(Center, Extents)

    if (node->HasLeaf())
    {
        AABB_PRIM(node->GetPrimitive(), OPC_CONTACT)
    }
    else
        _Collide(node->GetPos());

    if (ContactFound())
        return;

    if (node->HasLeaf2())
    {
        AABB_PRIM(node->GetPrimitive2(), OPC_CONTACT)
    }
    else
        _Collide(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for vanilla AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBCollider::_Collide(const AABBTreeNode* node)
{
    // Perform AABB-AABB overlap test
    Point Center, Extents;
    node->GetAABB()->GetCenter(Center);
    node->GetAABB()->GetExtents(Extents);
    if (!AABBAABBOverlap(Center, Extents))
        return;

    if (node->IsLeaf())
    {
        mTouchedPrimitives->Add(node->GetPrimitives(), node->GetNbPrimitives());
    }
    else
    {
        if (AABBContainsBox(Center, Extents))
        {
            mTouchedPrimitives->Add(node->GetPrimitives(), node->GetNbPrimitives());
            return;
        }

        _Collide(node->GetPos());
        _Collide(node->GetNeg());
    }
}
