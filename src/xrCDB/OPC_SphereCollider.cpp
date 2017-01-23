///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for a sphere collider.
 *	\file		OPC_SphereCollider.cpp
 *	\author		Pierre Terdiman
 *	\date		June, 2, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a sphere-vs-tree collider.
 *	This class performs a collision test between a sphere and an AABB tree. You can use this to do a standard player vs world collision,
 *	in a Nettle/Telemachos way. It doesn't suffer from all reported bugs in those two classic codes - the "_new_" one by Paul Nettle is a
 *	debuggued version I think. Collision response can be driven by reported collision data - it works extremely well for me. In sake of
 *	efficiency, all meshes (that is, all AABB trees) should of course also be kept in an extra hierarchical structure (octree, whatever).
 *
 *	\class		SphereCollider
 *	\author		Pierre Terdiman
 *	\version	1.2
 *	\date		June, 2, 2001
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace Opcode;

#include "OPC_SphereAABBOverlap.h"
#include "OPC_SphereTriOverlap.h"

//! Sphere-triangle overlap test
#ifdef OPC_USE_CALLBACKS
	#define SPHERE_PRIM(prim, flag)																	\
		/* Request vertices from the app */															\
		VertexPointers VP;	(mObjCallback)(prim, VP, mUserData);									\
																									\
		/* Perform sphere-tri overlap test */														\
		if(SphereTriOverlap(*VP.Vertex[0], *VP.Vertex[1], *VP.Vertex[2]))							\
		{																							\
			/* Set contact status */																\
			mFlags |= flag;																			\
			mTouchedPrimitives->Add(prim);															\
		}
#else
	#define SPHERE_PRIM(prim, flag)																	\
		/* Direct access to vertices */																\
		const IndexedTriangle* Tri = &mFaces[prim];													\
																									\
		/* Perform sphere-tri overlap test */														\
		if(SphereTriOverlap(mVerts[Tri->mVRef[0]], mVerts[Tri->mVRef[1]], mVerts[Tri->mVRef[2]]))	\
		{																							\
			/* Set contact status */																\
			mFlags |= flag;																			\
			mTouchedPrimitives->Add(prim);															\
		}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SphereCollider::SphereCollider()
{
	mCenter.Zero();
	mRadius2 = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SphereCollider::~SphereCollider()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Validates current settings. You should call this method after all the settings and callbacks have been defined.
 *	\return		null if everything is ok, else a string describing the problem
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* SphereCollider::ValidateSettings()
{
	return VolumeCollider::ValidateSettings();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Generic collision query for generic OPCODE models. After the call, access the results:
 *	- with GetContactStatus()
 *	- with GetNbTouchedFaces()
 *	- with GetTouchedFaces()
 *
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in local space
 *	\param		model		[in] Opcode model to collide with
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, OPCODE_Model* model, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// Checkings
	if(!model)	return false;

	// Simple double-dispatch
	if(!model->HasLeafNodes())
	{
		if(model->IsQuantized())	return Collide(cache, sphere, (const AABBQuantizedNoLeafTree*)model->GetTree(), worlds, worldm);
		else						return Collide(cache, sphere, (const AABBNoLeafTree*)model->GetTree(), worlds, worldm);
	}
	else
	{
		if(model->IsQuantized())	return Collide(cache, sphere, (const AABBQuantizedTree*)model->GetTree(), worlds, worldm);
		else						return Collide(cache, sphere, (const AABBCollisionTree*)model->GetTree(), worlds, worldm);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes a collision query :
 *	- reset stats & contact status
 *	- setup matrices
 *	- check temporal coherence
 *
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] sphere in local space
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		contact status
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SphereCollider::InitQuery(SphereCache& cache, const Sphere& sphere, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// 1) Call the base method
	VolumeCollider::InitQueryEx();

	// 2) Compute sphere in model space:
	// - Precompute R^2
	mRadius2 = sphere.mRadius * sphere.mRadius;
	// - Compute center position
	mCenter = sphere.mCenter;
	// -> to world space
	if(worlds)	mCenter *= *worlds;
	// -> to model space
	if(worldm)
	{
		// Invert model matrix
		Matrix4x4 InvWorldM;
		InvertPRMatrix(InvWorldM, *worldm);

		mCenter *= InvWorldM;
	}

	// 3) Setup destination pointer
	mTouchedPrimitives = &cache.TouchedPrimitives;

	// 4) Check temporal coherence :
	if(TemporalCoherenceEnabled())
	{
		// Here we use temporal coherence
		// => check results from previous frame before performing the collision query
		if(FirstContactEnabled())
		{
			// We're only interested in the first contact found => test the unique previously touched face
			if(mTouchedPrimitives->GetNbEntries())
			{
				// Get index of previously touched face = the first entry in the array
				udword PreviouslyTouchedFace = mTouchedPrimitives->GetEntry(0);

				// Then reset the array:
				// - if the overlap test below is successful, the index we'll get added back anyway
				// - if it isn't, then the array should be reset anyway for the normal query
				mTouchedPrimitives->Reset();

				// Perform overlap test between the cached triangle and the sphere (and set contact status if needed)
				SPHERE_PRIM(PreviouslyTouchedFace, OPC_TEMPORAL_CONTACT)
			}
			// else no face has been touched during previous query
			// => we'll have to perform a normal query
		}
		else
		{
			// We're interested in all contacts =>test the _new_ real sphere N(ew) against the previous fat sphere P(revious):
			float r = _sqrt(cache.FatRadius2) - sphere.mRadius;
			if(cache.Center.SquareDistance(mCenter) < r*r)
			{
				// - if N is included in P, return previous list
				// => we simply leave the list (mTouchedFaces) unchanged

				// Set contact status if needed
				if(mTouchedPrimitives->GetNbEntries())	mFlags |= OPC_TEMPORAL_CONTACT;
			}
			else
			{
				// - else do the query using a fat N

				// Reset cache since we'll about to perform a real query
				mTouchedPrimitives->Reset();

				// Make a fat sphere so that coherence will work for subsequent frames
				mRadius2 *= cache.FatCoeff;

				// Update cache with query data (signature for cached faces)
				cache.Center = mCenter;
				cache.FatRadius2 = mRadius2;
			}
		}
	}
	else
	{
		// Here we don't use temporal coherence => do a normal query
		mTouchedPrimitives->Reset();
	}

	return GetContactStatus();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for normal trees.
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, const AABBCollisionTree* tree, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, sphere, worlds, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for no-leaf trees.
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, const AABBNoLeafTree* tree, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, sphere, worlds, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized trees.
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, const AABBQuantizedTree* tree, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, sphere, worlds, worldm))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized no-leaf trees.
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worlds		[in] sphere's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, const AABBQuantizedNoLeafTree* tree, const Matrix4x4* worlds, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, sphere, worlds, worldm))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for vanilla AABB trees.
 *	\param		cache		[in/out] a sphere cache
 *	\param		sphere		[in] collision sphere in world space
 *	\param		tree		[in] AABB tree
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SphereCollider::Collide(SphereCache& cache, const Sphere& sphere, const AABBTree* tree)
{
	// This is typically called for a scene tree, full of -AABBs-, not full of triangles.
	// So we don't really have "primitives" to deal with. Hence it doesn't work with
	// "FirstContact" + "TemporalCoherence".
	ASSERT( !(FirstContactEnabled() && TemporalCoherenceEnabled()) );

	// Checkings
	if(!tree)	return false;

	// Init collision query
	if(InitQuery(cache, sphere))	return true;

	// Perform collision query
	_Collide(tree);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks the sphere completely contains the box. In which case we can end the query sooner.
 *	\param		bc	[in] box center
 *	\param		be	[in] box extents
 *	\return		true if the sphere contains the whole box
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL SphereCollider::SphereContainsBox(const Point& bc, const Point& be)
{
	// I assume if all 8 box vertices are inside the sphere, so does the whole box.
	// Sounds ok but maybe there's a better way?
	Point p;
	p.x=bc.x+be.x; p.y=bc.y+be.y; p.z=bc.z+be.z;	if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x-be.x;									if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x+be.x; p.y=bc.y-be.y;					if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x-be.x;									if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x+be.x; p.y=bc.y+be.y; p.z=bc.z-be.z;	if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x-be.x;									if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x+be.x; p.y=bc.y-be.y;					if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;
	p.x=bc.x-be.x;									if(mCenter.SquareDistance(p)>=mRadius2)	return FALSE;

	return TRUE;
}

#define TEST_SPHERE_IN_BOX(center, extents)	\
	if(SphereContainsBox(center, extents))	\
	{										\
		/* Set contact status */			\
		mFlags |= OPC_CONTACT;				\
		_Dump(node);						\
		return;								\
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for normal AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::_Collide(const AABBCollisionNode* node)
{
	// Perform Sphere-AABB overlap test
	if(!SphereAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	TEST_SPHERE_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

	if(node->IsLeaf())
	{
		SPHERE_PRIM(node->GetPrimitive(), OPC_CONTACT)
	}
	else
	{
		_Collide(node->GetPos());

		if(ContactFound()) return;

		_Collide(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::_Collide(const AABBQuantizedNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Sphere-AABB overlap test
	if(!SphereAABBOverlap(Center, Extents))	return;

	TEST_SPHERE_IN_BOX(Center, Extents)

	if(node->IsLeaf())
	{
		SPHERE_PRIM(node->GetPrimitive(), OPC_CONTACT)
	}
	else
	{
		_Collide(node->GetPos());

		if(ContactFound()) return;

		_Collide(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::_Collide(const AABBNoLeafNode* node)
{
	// Perform Sphere-AABB overlap test
	if(!SphereAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	TEST_SPHERE_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

	if(node->HasLeaf())		{ SPHERE_PRIM(node->GetPrimitive(), OPC_CONTACT) }
	else					_Collide(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ SPHERE_PRIM(node->GetPrimitive2(), OPC_CONTACT) }
	else					_Collide(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::_Collide(const AABBQuantizedNoLeafNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Sphere-AABB overlap test
	if(!SphereAABBOverlap(Center, Extents))	return;

	TEST_SPHERE_IN_BOX(Center, Extents)

	if(node->HasLeaf())		{ SPHERE_PRIM(node->GetPrimitive(), OPC_CONTACT) }
	else					_Collide(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ SPHERE_PRIM(node->GetPrimitive2(), OPC_CONTACT) }
	else					_Collide(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for vanilla AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::_Collide(const AABBTreeNode* node)
{
	// Perform Sphere-AABB overlap test
	Point Center, Extents;
	node->GetAABB()->GetCenter(Center);
	node->GetAABB()->GetExtents(Extents);
	if(!SphereAABBOverlap(Center, Extents))	return;

	if(node->IsLeaf())
	{
		mTouchedPrimitives->Add(node->GetPrimitives(), node->GetNbPrimitives());
	}
	else
	{
		if(SphereContainsBox(Center, Extents))
		{
			mTouchedPrimitives->Add(node->GetPrimitives(), node->GetNbPrimitives());
			return;
		}

		_Collide(node->GetPos());
		_Collide(node->GetNeg());
	}
}
