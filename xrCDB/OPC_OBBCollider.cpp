///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for an OBB collider.
 *	\file		OPC_OBBCollider.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 1st, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains an OBB-vs-tree collider.
 *
 *	\class		OBBCollider
 *	\author		Pierre Terdiman
 *	\version	1.2
 *	\date		January, 1st, 2002
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace Opcode;

#include "OPC_BoxBoxOverlap.h"
#include "OPC_TriBoxOverlap.h"

//! OBB-triangle test
#ifdef OPC_USE_CALLBACKS
	#define OBB_PRIM(primindex)															\
		/* Request vertices from the app */												\
		VertexPointers VP;	(mObjCallback)(primindex, VP, mUserData);					\
		/* Transform them in a common space */											\
		TransformPoint(mLeafVerts[0], *VP.Vertex[0], mRModelToBox, mTModelToBox);		\
		TransformPoint(mLeafVerts[1], *VP.Vertex[1], mRModelToBox, mTModelToBox);		\
		TransformPoint(mLeafVerts[2], *VP.Vertex[2], mRModelToBox, mTModelToBox);		\
		/* Perform triangle-box overlap test */											\
		if(TriBoxOverlap())																\
		{																				\
			/* Set contact status */													\
			mFlags |= OPC_CONTACT;														\
			mTouchedPrimitives->Add(primindex);											\
		}
#else
	#define OBB_PRIM(primindex)															\
		/* Direct access to vertices */													\
		const IndexedTriangle* T = &mFaces[primindex];									\
		/* Transform them in a common space */											\
		TransformPoint(mLeafVerts[0], mVerts[T->mVRef[0]], mRModelToBox, mTModelToBox);	\
		TransformPoint(mLeafVerts[1], mVerts[T->mVRef[1]], mRModelToBox, mTModelToBox);	\
		TransformPoint(mLeafVerts[2], mVerts[T->mVRef[2]], mRModelToBox, mTModelToBox);	\
		/* Perform triangle-box overlap test */											\
		if(TriBoxOverlap())																\
		{																				\
			/* Set contact status */													\
			mFlags |= OPC_CONTACT;														\
			mTouchedPrimitives->Add(primindex);											\
		}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OBBCollider::OBBCollider() : mFullBoxBoxTest(true)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OBBCollider::~OBBCollider()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Validates current settings. You should call this method after all the settings and callbacks have been defined.
 *	\return		null if everything is ok, else a string describing the problem
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* OBBCollider::ValidateSettings()
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
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision OBB in local space
 *	\param		model		[in] Opcode model to collide with
 *	\param		worldb		[in] OBB's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OBBCollider::Collide(OBBCache& cache, const OBB& box, OPCODE_Model* model, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// Checkings
	if(!model)	return false;

	// Simple double-dispatch
	if(!model->HasLeafNodes())
	{
		if(model->IsQuantized())	return Collide(cache, box, (const AABBQuantizedNoLeafTree*)model->GetTree(), worldb, worldm);
		else						return Collide(cache, box, (const AABBNoLeafTree*)model->GetTree(), worldb, worldm);
	}
	else
	{
		if(model->IsQuantized())	return Collide(cache, box, (const AABBQuantizedTree*)model->GetTree(), worldb, worldm);
		else						return Collide(cache, box, (const AABBCollisionTree*)model->GetTree(), worldb, worldm);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes a collision query :
 *	- reset stats & contact status
 *	- setup matrices
 *	- check temporal coherence
 *
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] obb in local space
 *	\param		worldb		[in] obb's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		contact status
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OBBCollider::InitQuery(OBBCache& cache, const OBB& box, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// 1) Call the base method
	VolumeCollider::InitQueryEx();

	// 2) Compute obb in world space
	mBoxExtents = box.mExtents;

	Matrix4x4 WorldB;

	if(worldb)
	{
		WorldB = Matrix4x4( box.mRot2 * Matrix3x3(*worldb) );
		WorldB.SetTrans(box.mCenter * *worldb);
	}
	else
	{
		WorldB = box.mRot2;
		WorldB.SetTrans(box.mCenter);
	}

	// Setup matrices
	Matrix4x4 InvWorldB;
	InvertPRMatrix(InvWorldB, WorldB);

	if(worldm)
	{
		Matrix4x4 InvWorldM;
		InvertPRMatrix(InvWorldM, *worldm);

		Matrix4x4 WorldBtoM = WorldB * InvWorldM;
		Matrix4x4 WorldMtoB = *worldm * InvWorldB;

		mRModelToBox = WorldMtoB;		WorldMtoB.GetTrans(mTModelToBox);
		mRBoxToModel = WorldBtoM;		WorldBtoM.GetTrans(mTBoxToModel);
	}
	else
	{
		mRModelToBox = InvWorldB;	InvWorldB.GetTrans(mTModelToBox);
		mRBoxToModel = WorldB;		WorldB.GetTrans(mTBoxToModel);
	}

	// Precompute absolute box-to-model rotation matrix
	for(udword i=0;i<3;i++)
	{
		for(udword j=0;j<3;j++)
		{
			// Epsilon value prevents floating-point inaccuracies (strategy borrowed from RAPID)
			mAR.m[i][j] = 1e-6f + _abs(mRBoxToModel.m[i][j]);
		}
	}

	// Precompute bounds for box-in-box test
	mB0 = mBoxExtents - mTModelToBox;
	mB1 = - mBoxExtents - mTModelToBox;

	// Precompute box-box data - Courtesy of Erwin de Vries
	Point Box = mBoxExtents;
	mBBx1 = Box.x*mAR.m[0][0] + Box.y*mAR.m[1][0] + Box.z*mAR.m[2][0];
	mBBy1 = Box.x*mAR.m[0][1] + Box.y*mAR.m[1][1] + Box.z*mAR.m[2][1];
	mBBz1 = Box.x*mAR.m[0][2] + Box.y*mAR.m[1][2] + Box.z*mAR.m[2][2];

	mBB_1 = Box.y*mAR.m[2][0] + Box.z*mAR.m[1][0];
	mBB_2 = Box.x*mAR.m[2][0] + Box.z*mAR.m[0][0];
	mBB_3 = Box.x*mAR.m[1][0] + Box.y*mAR.m[0][0];
	mBB_4 = Box.y*mAR.m[2][1] + Box.z*mAR.m[1][1];
	mBB_5 = Box.x*mAR.m[2][1] + Box.z*mAR.m[0][1];
	mBB_6 = Box.x*mAR.m[1][1] + Box.y*mAR.m[0][1];
	mBB_7 = Box.y*mAR.m[2][2] + Box.z*mAR.m[1][2];
	mBB_8 = Box.x*mAR.m[2][2] + Box.z*mAR.m[0][2];
	mBB_9 = Box.x*mAR.m[1][2] + Box.y*mAR.m[0][2];

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

				// Perform overlap test between the cached triangle and the box (and set contact status if needed)
				OBB_PRIM(PreviouslyTouchedFace)
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
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision OBB in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worldb		[in] OBB's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OBBCollider::Collide(OBBCache& cache, const OBB& box, const AABBCollisionTree* tree, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, box, worldb, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for no-leaf trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision OBB in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worldb		[in] OBB's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OBBCollider::Collide(OBBCache& cache, const OBB& box, const AABBNoLeafTree* tree, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, box, worldb, worldm))	return true;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Collision query for quantized trees.
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision OBB in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worldb		[in] OBB's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OBBCollider::Collide(OBBCache& cache, const OBB& box, const AABBQuantizedTree* tree, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, box, worldb, worldm))	return true;

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
 *	\param		cache		[in/out] a box cache
 *	\param		box			[in] collision OBB in local space
 *	\param		tree		[in] model's AABB tree
 *	\param		worldb		[in] OBB's world matrix, or null
 *	\param		worldm		[in] model's world matrix, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OBBCollider::Collide(OBBCache& cache, const OBB& box, const AABBQuantizedNoLeafTree* tree, const Matrix4x4* worldb, const Matrix4x4* worldm)
{
	// Checkings
	if(!tree)				return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)		return false;
#else
	if(!mFaces || !mVerts)	return false;
#endif

	// Init collision query
	if(InitQuery(cache, box, worldb, worldm))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform collision query
	_Collide(tree->GetNodes());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks the OBB completely contains the box. In which case we can end the query sooner.
 *	\param		bc	[in] box center
 *	\param		be	[in] box extents
 *	\return		true if the OBB contains the whole box
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL OBBCollider::OBBContainsBox(const Point& bc, const Point& be)
{
	// I assume if all 8 box vertices are inside the OBB, so does the whole box.
	// Sounds ok but maybe there's a better way?
/*
#define TEST_PT(a,b,c)																												\
	p.x=a;	p.y=b;	p.z=c;		p+=bc;																								\
	f = p.x * mRModelToBox.m[0][0] + p.y * mRModelToBox.m[1][0] + p.z * mRModelToBox.m[2][0];	if(f>mB0.x || f<mB1.x) return FALSE;\
	f = p.x * mRModelToBox.m[0][1] + p.y * mRModelToBox.m[1][1] + p.z * mRModelToBox.m[2][1];	if(f>mB0.y || f<mB1.y) return FALSE;\
	f = p.x * mRModelToBox.m[0][2] + p.y * mRModelToBox.m[1][2] + p.z * mRModelToBox.m[2][2];	if(f>mB0.z || f<mB1.z) return FALSE;

	Point p;
	float f;

	TEST_PT(be.x, be.y, be.z)
	TEST_PT(-be.x, be.y, be.z)
	TEST_PT(be.x, -be.y, be.z)
	TEST_PT(-be.x, -be.y, be.z)
	TEST_PT(be.x, be.y, -be.z)
	TEST_PT(-be.x, be.y, -be.z)
	TEST_PT(be.x, -be.y, -be.z)
	TEST_PT(-be.x, -be.y, -be.z)

	return TRUE;
*/

	// Yes there is:
	// - compute model-box's AABB in OBB space
	// - test AABB-in-AABB
	float NCx = bc.x * mRModelToBox.m[0][0] + bc.y * mRModelToBox.m[1][0] + bc.z * mRModelToBox.m[2][0];
	float NEx = _abs(mRModelToBox.m[0][0] * be.x) + _abs(mRModelToBox.m[1][0] * be.y) + _abs(mRModelToBox.m[2][0] * be.z);

	if(mB0.x < NCx+NEx)	return FALSE;
	if(mB1.x > NCx-NEx)	return FALSE;

	float NCy = bc.x * mRModelToBox.m[0][1] + bc.y * mRModelToBox.m[1][1] + bc.z * mRModelToBox.m[2][1];
	float NEy = _abs(mRModelToBox.m[0][1] * be.x) + _abs(mRModelToBox.m[1][1] * be.y) + _abs(mRModelToBox.m[2][1] * be.z);

	if(mB0.y < NCy+NEy)	return FALSE;
	if(mB1.y > NCy-NEy)	return FALSE;

	float NCz = bc.x * mRModelToBox.m[0][2] + bc.y * mRModelToBox.m[1][2] + bc.z * mRModelToBox.m[2][2];
	float NEz = _abs(mRModelToBox.m[0][2] * be.x) + _abs(mRModelToBox.m[1][2] * be.y) + _abs(mRModelToBox.m[2][2] * be.z);

	if(mB0.z < NCz+NEz)	return FALSE;
	if(mB1.z > NCz-NEz)	return FALSE;

	return TRUE;
}

#define TEST_OBB_IN_BOX(center, extents)	\
	if(OBBContainsBox(center, extents))		\
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
void OBBCollider::_Collide(const AABBCollisionNode* node)
{
	// Perform OBB-AABB overlap test
	if(!BoxBoxOverlap(node->mAABB.mExtents, node->mAABB.mCenter))	return;

	TEST_OBB_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

	if(node->IsLeaf())
	{
		OBB_PRIM(node->GetPrimitive())
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
void OBBCollider::_Collide(const AABBQuantizedNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform OBB-AABB overlap test
	if(!BoxBoxOverlap(Extents, Center))	return;

	TEST_OBB_IN_BOX(Center, Extents)

	if(node->IsLeaf())
	{
		OBB_PRIM(node->GetPrimitive())
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
void OBBCollider::_Collide(const AABBNoLeafNode* node)
{
	// Perform OBB-AABB overlap test
	if(!BoxBoxOverlap(node->mAABB.mExtents, node->mAABB.mCenter))	return;

	TEST_OBB_IN_BOX(node->mAABB.mCenter, node->mAABB.mExtents)

	if(node->HasLeaf())		{ OBB_PRIM(node->GetPrimitive()) }
	else					_Collide(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ OBB_PRIM(node->GetPrimitive2()) }
	else					_Collide(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive collision query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OBBCollider::_Collide(const AABBQuantizedNoLeafNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform OBB-AABB overlap test
	if(!BoxBoxOverlap(Extents, Center))	return;

	TEST_OBB_IN_BOX(Center, Extents)

	if(node->HasLeaf())		{ OBB_PRIM(node->GetPrimitive()) }
	else					_Collide(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())	{ OBB_PRIM(node->GetPrimitive2()) }
	else					_Collide(node->GetNeg());
}
