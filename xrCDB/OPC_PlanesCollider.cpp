///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for a planes collider.
 *	\file		OPC_PlanesCollider.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 1st, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace Opcode;

#include "OPC_PlanesAABBOverlap.h"
#include "OPC_PlanesTriOverlap.h"

//! Planes-triangle test
#ifdef OPC_USE_CALLBACKS
	#define PLANES_PRIM(primindex)						\
		/* Request vertices from the app */				\
		(mObjCallback)(primindex, mVP, mUserData);		\
		/* Perform triangle-box overlap test */			\
		if(PlanesTriOverlap(clipmask))					\
		{												\
			/* Set contact status */					\
			mFlags |= OPC_CONTACT;						\
			mTouchedPrimitives->Add(primindex);			\
		}
#else
	#define PLANES_PRIM(primindex)						\
		/* Direct access to vertices */					\
		const IndexedTriangle* T = &mFaces[primindex];	\
		mVP.Vertex[0] = &mVerts[T->mVRef[0]];			\
		mVP.Vertex[1] = &mVerts[T->mVRef[1]];			\
		mVP.Vertex[2] = &mVerts[T->mVRef[2]];			\
		/* Perform triangle-box overlap test */			\
		if(PlanesTriOverlap(clipmask))					\
		{												\
			/* Set contact status */					\
			mFlags |= OPC_CONTACT;						\
			mTouchedPrimitives->Add(primindex);			\
		}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PlanesCollider::PlanesCollider() :
	mPlanes		(null),
	mNbPlanes	(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PlanesCollider::~PlanesCollider()
{
	CFREE(mPlanes);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Validates current settings. You should call this method after all the settings and callbacks have been defined.
 *	\return		null if everything is ok, else a string describing the problem
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* PlanesCollider::ValidateSettings()
{
	if(TemporalCoherenceEnabled() && !FirstContactEnabled())	return "Temporal coherence only works with ""First contact"" mode!";

	return VolumeCollider::ValidateSettings();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Generic collision query for generic OPCODE models. After the call, access the results:
 *	- with GetContactStatus()
 *	- with GetNbTouchedFaces()
 *	- with GetTouchedFaces()
 *
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes in world space
 *	\param		nb_planes	[in] number of planes
 *	\param		model		[in] Opcode model to collide with
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PlanesCollider::Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, OPCODE_Model* model, const Matrix4x4* worldm)
{
	// Checkings
	if(!planes || !model)	return false;

	// Simple double-dispatch
	if(!model->HasLeafNodes())
	{
		if(model->IsQuantized())	return Collide(cache, planes, nb_planes, (const AABBQuantizedNoLeafTree*)model->GetTree(), worldm);
		else						return Collide(cache, planes, nb_planes, (const AABBNoLeafTree*)model->GetTree(), worldm);
	}
	else
	{
		if(model->IsQuantized())	return Collide(cache, planes, nb_planes, (const AABBQuantizedTree*)model->GetTree(), worldm);
		else						return Collide(cache, planes, nb_planes, (const AABBCollisionTree*)model->GetTree(), worldm);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes a collision query :
 *	- reset stats & contact status
 *	- compute planes in model space
 *	- check temporal coherence
 *
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes
 *	\param		nb_planes	[in] number of planes
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		contact status
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PlanesCollider::InitQuery(PlanesCache& cache, const Plane* planes, udword nb_planes, const Matrix4x4* worldm)
{
	// 1) Call the base method
	VolumeCollider::InitQueryEx();

	// 2) Compute planes in model space
	if(nb_planes>mNbPlanes)
	{
		CFREE(mPlanes);
		mPlanes = CALLOC(Plane,nb_planes);
	}
	mNbPlanes = nb_planes;

	if(worldm)
	{
		Matrix4x4 InvWorldM;
		InvertPRMatrix(InvWorldM, *worldm);

//		for(udword i=0;i<nb_planes;i++)	mPlanes[i] = planes[i] * InvWorldM;
		for(udword i=0;i<nb_planes;i++)	TransformPlane(mPlanes[i], planes[i], InvWorldM);
	}
	else CopyMemory(mPlanes, planes, nb_planes*sizeof(Plane));

	// 3) Setup destination pointer
	mTouchedPrimitives = &cache.TouchedPrimitives;

	// 4) Check temporal coherence:
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

				// Perform overlap test between the cached triangle and the planes (and set contact status if needed)
				udword clipmask = (1<<mNbPlanes)-1;

				PLANES_PRIM(PreviouslyTouchedFace)
			}
			// else no face has been touched during previous query
			// => we'll have to perform a normal query
		}
		else mTouchedPrimitives->Reset();
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
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes
 *	\param		nb_planes	[in] number of planes
 *	\param		tree		[in] model's AABB tree
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PlanesCollider::Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBCollisionTree* tree, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree || !planes || !nb_planes)	return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)					return false;
#else
	if(!mFaces || !mVerts)				return false;
#endif

	// Init collision query
	if(InitQuery(cache, planes, nb_planes, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes(), (1<<nb_planes)-1);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for no-leaf trees.
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes
 *	\param		nb_planes	[in] number of planes
 *	\param		tree		[in] model's AABB tree
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PlanesCollider::Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBNoLeafTree* tree, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree || !planes || !nb_planes)	return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)					return false;
#else
	if(!mFaces || !mVerts)				return false;
#endif

	// Init collision query
	if(InitQuery(cache, planes, nb_planes, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes(), (1<<nb_planes)-1);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized trees.
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes
 *	\param		nb_planes	[in] number of planes
 *	\param		tree		[in] model's AABB tree
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PlanesCollider::Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBQuantizedTree* tree, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree || !planes || !nb_planes)	return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)					return false;
#else
	if(!mFaces || !mVerts)				return false;
#endif

	// Init collision query
	if(InitQuery(cache, planes, nb_planes, worldm))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform collision query
	_Collide(tree->GetNodes(), (1<<nb_planes)-1);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized no-leaf trees.
 *	\param		cache		[in/out] a planes cache
 *	\param		planes		[in] list of planes
 *	\param		nb_planes	[in] number of planes
 *	\param		tree		[in] model's AABB tree
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PlanesCollider::Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBQuantizedNoLeafTree* tree, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree || !planes || !nb_planes)	return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)					return false;
#else
	if(!mFaces || !mVerts)				return false;
#endif

	// Init collision query
	if(InitQuery(cache, planes, nb_planes, worldm))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform collision query
	_Collide(tree->GetNodes(), (1<<nb_planes)-1);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for normal AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PlanesCollider::_Collide(const AABBCollisionNode* node, udword clipmask)
{
	// Test the box against the planes. If the box is completely culled, so are its children, hence we exit.
	udword OutClipMask;
	if(!PlanesAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents, OutClipMask, clipmask))	return;

	// If the box is completely included, so are its children. We don't need to do extra tests, we
	// can immediately output a list of visible children. Those ones won't need to be clipped.
	if(!OutClipMask)
	{
		// Set contact status
		mFlags |= OPC_CONTACT;
		_Dump(node);
		return;
	}

	// Else the box straddles one or several planes, so we need to recurse down the tree.

	if(node->IsLeaf())
	{
		PLANES_PRIM(node->GetPrimitive())
	}
	else
	{
		_Collide(node->GetPos(), OutClipMask);

		if(ContactFound()) return;

		_Collide(node->GetNeg(), OutClipMask);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PlanesCollider::_Collide(const AABBQuantizedNode* node, udword clipmask)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Test the box against the planes. If the box is completely culled, so are its children, hence we exit.
	udword OutClipMask;
	if(!PlanesAABBOverlap(Center, Extents, OutClipMask, clipmask))	return;

	// If the box is completely included, so are its children. We don't need to do extra tests, we
	// can immediately output a list of visible children. Those ones won't need to be clipped.
	if(!OutClipMask)
	{
		// Set contact status
		mFlags |= OPC_CONTACT;
		_Dump(node);
		return;
	}

	// Else the box straddles one or several planes, so we need to recurse down the tree.

	if(node->IsLeaf())
	{
		PLANES_PRIM(node->GetPrimitive())
	}
	else
	{
		_Collide(node->GetPos(), OutClipMask);

		if(ContactFound()) return;

		_Collide(node->GetNeg(), OutClipMask);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PlanesCollider::_Collide(const AABBNoLeafNode* node, udword clipmask)
{
	// Test the box against the planes. If the box is completely culled, so are its children, hence we exit.
	udword OutClipMask;
	if(!PlanesAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents, OutClipMask, clipmask))	return;

	// If the box is completely included, so are its children. We don't need to do extra tests, we
	// can immediately output a list of visible children. Those ones won't need to be clipped.
	if(!OutClipMask)
	{
		// Set contact status
		mFlags |= OPC_CONTACT;
		_Dump(node);
		return;
	}

	// Else the box straddles one or several planes, so we need to recurse down the tree.

	if(node->HasLeaf())		{ PLANES_PRIM(node->GetPrimitive()) }
	else					_Collide(node->GetPos(), OutClipMask);

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ PLANES_PRIM(node->GetPrimitive2()) }
	else					_Collide(node->GetNeg(), OutClipMask);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PlanesCollider::_Collide(const AABBQuantizedNoLeafNode* node, udword clipmask)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Test the box against the planes. If the box is completely culled, so are its children, hence we exit.
	udword OutClipMask;
	if(!PlanesAABBOverlap(Center, Extents, OutClipMask, clipmask))	return;

	// If the box is completely included, so are its children. We don't need to do extra tests, we
	// can immediately output a list of visible children. Those ones won't need to be clipped.
	if(!OutClipMask)
	{
		// Set contact status
		mFlags |= OPC_CONTACT;
		_Dump(node);
		return;
	}

	// Else the box straddles one or several planes, so we need to recurse down the tree.

	if(node->HasLeaf())		{ PLANES_PRIM(node->GetPrimitive()) }
	else					_Collide(node->GetPos(), OutClipMask);

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ PLANES_PRIM(node->GetPrimitive2()) }
	else					_Collide(node->GetNeg(), OutClipMask);
}
