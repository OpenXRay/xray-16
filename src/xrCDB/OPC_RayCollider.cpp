///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for a ray collider.
 *	\file		OPC_RayCollider.cpp
 *	\author		Pierre Terdiman
 *	\date		June, 2, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a ray-vs-tree collider.
 *	This class performs a stabbing query on an AABB tree, i.e. does a ray-mesh collision.
 *
 *	HIGHER DISTANCE BOUND:
 *
 *		If P0 and P1 are two 3D points, let's define:
 *		- d = distance between P0 and P1
 *		- Origin	= P0
 *		- Direction	= (P1 - P0) / d = normalized direction vector3
 *		- A parameter t such as a point P on the line (P0,P1) is P = Origin + t * Direction
 *		- t = 0  -->  P = P0
 *		- t = d  -->  P = P1
 *
 *		Then we can define a general "ray" as:
 *
 *			struct Ray
 *			{
 *				Point	Origin;
 *				Point	Direction;
 *			};
 *
 *		But it actually maps three different things:
 *		- a segment,   when 0 <= t <= d
 *		- a half-line, when 0 <= t < +infinity, or -infinity < t <= d
 *		- a line,      when -infinity < t < +infinity
 *
 *		In Opcode, we support segment queries, which yield half-line queries by setting d = +infinity.
 *		We don't support line-queries. If you need them, shift the origin along the ray by an appropriate margin.
 *
 *		In short, the lower bound is always 0, and you can setup the higher bound "d" with RayCollider::SetMaxDist().
 *
 *		Query	|segment			|half-line		|line
 *		--------|-------------------|---------------|----------------
 *		Usages	|-shadow feelers	|-raytracing	|-
 *				|-sweep tests		|-in/out tests	|
 *
 *	FIRST CONTACT:
 *
 *		- You can setup "first contact" mode or "all contacts" mode with RayCollider::SetFirstContact().
 *		- In "first contact" mode we return as soon as the ray hits one face. If can be useful e.g. for shadow feelers, where
 *		you want to know whether the path to the light is _free or not (a boolean answer is enough).
 *		- In "all contacts" mode we return all faces hit by the ray.
 *
 *	TEMPORAL COHERENCE:
 *
 *		- You can enable or disable temporal coherence with RayCollider::SetTemporalCoherence().
 *		- It currently only works in "first contact" mode.
 *		- If temporal coherence is enabled, the previously hit triangle is cached during the first query. Then, next queries
 *		start by colliding the ray against the cached triangle. If they still collide, we return immediately.
 *
 *	CLOSEST HIT:
 *
 *		- You can enable or disable "closest hit" with RayCollider::SetClosestHit().
 *		- It currently only works in "all contacts" mode.
 *		- If closest hit is enabled, faces are sorted by distance on-the-fly and the closest one only is reported.
 *
 *	BACKFACE CULLING:
 *
 *		- You can enable or disable backface culling with RayCollider::SetCulling().
 *		- If culling is enabled, ray will not hit back faces (only front faces).
 *		
 *
 *
 *	\class		RayCollider
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

#include "OPC_RayAABBOverlap.h"
#include "OPC_RayTriOverlap.h"

#define HANDLE_CONTACT(prim)																	\
	/* Set contact status */																	\
	mFlags |= OPC_CONTACT;																		\
																								\
	if(mStabbedFaces)																			\
	{																							\
		if(!mClosestHit || !mStabbedFaces->GetNbFaces())										\
		{																						\
			mStabbedFace.mFaceID = prim;														\
			mStabbedFaces->AddFace(mStabbedFace);												\
		}																						\
		else																					\
		{																						\
			CollisionFace* Current = const_cast<CollisionFace*>(mStabbedFaces->GetFaces());		\
			if(Current && mStabbedFace.mDistance<Current->mDistance)							\
			{																					\
				mStabbedFace.mFaceID = prim;													\
				*Current = mStabbedFace;														\
			}																					\
		}																						\
	}

#ifdef OPC_USE_CALLBACKS
	#define STAB_PRIM(prim)																		\
		/* Request vertices from the app */														\
		VertexPointers VP;	(mObjCallback)(prim, VP, mUserData);								\
																								\
		/* Perform ray-tri overlap test and return */											\
		if(RayTriOverlap(*VP.Vertex[0], *VP.Vertex[1], *VP.Vertex[2]))							\
		{																						\
			/* Intersection point is valid if: */												\
			/* - distance is positive (else it can just be a face behind the orig point) */		\
			/* - distance is smaller than a given max distance (useful for shadow feelers) */	\
			if(!IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))										\
			{																					\
				mNbIntersections++;																\
				if(IR(mStabbedFace.mDistance)<IR(mMaxDist))										\
				{																				\
					HANDLE_CONTACT(prim)														\
				}																				\
			}																					\
		}
#else
	#define STAB_PRIM(prim)																		\
		const IndexedTriangle* Tri = &mFaces[prim];												\
																								\
		/* Perform ray-tri overlap test and return */											\
		if(RayTriOverlap(mVerts[Tri->mVRef[0]], mVerts[Tri->mVRef[1]], mVerts[Tri->mVRef[2]]))	\
		{																						\
			/* Intersection point is valid if: */												\
			/* - distance is positive (else it can just be a face behind the orig point) */		\
			/* - distance is smaller than a given max distance (useful for shadow feelers) */	\
			if(!IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))										\
			{																					\
				mNbIntersections++;																\
				if(IR(mStabbedFace.mDistance)<IR(mMaxDist))										\
				{																				\
					HANDLE_CONTACT(prim)														\
				}																				\
			}																					\
		}
#endif

#ifdef OPC_USE_CALLBACKS
	#define UNBOUNDED_STAB_PRIM(prim)															\
		/* Request vertices from the app */														\
		VertexPointers VP;	(mObjCallback)(prim, VP, mUserData);								\
																								\
		/* Perform ray-tri overlap test and return */											\
		if(RayTriOverlap(*VP.Vertex[0], *VP.Vertex[1], *VP.Vertex[2]))							\
		{																						\
			/* Intersection point is valid if: */												\
			/* - distance is positive (else it can just be a face behind the orig point) */		\
			if(!IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))										\
			{																					\
				mNbIntersections++;																\
				HANDLE_CONTACT(prim)															\
			}																					\
		}
#else
	#define UNBOUNDED_STAB_PRIM(prim)															\
		const IndexedTriangle* Tri = &mFaces[prim];												\
																								\
		/* Perform ray-tri overlap test and return */											\
		if(RayTriOverlap(mVerts[Tri->mVRef[0]], mVerts[Tri->mVRef[1]], mVerts[Tri->mVRef[2]]))	\
		{																						\
			/* Intersection point is valid if: */												\
			/* - distance is positive (else it can just be a face behind the orig point) */		\
			if(!IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))										\
			{																					\
				mNbIntersections++;																\
				HANDLE_CONTACT(prim)															\
			}																					\
		}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RayCollider::RayCollider()
 :	mNbRayBVTests		(0),
	mNbRayPrimTests		(0),
	mNbIntersections	(0),
	mClosestHit			(false),
	mCulling			(true),
#ifdef OPC_USE_CALLBACKS
	mUserData			(0),
	mObjCallback		(null),
#else
	mFaces				(null),
	mVerts				(null),
#endif
	mStabbedFaces		(null),
	mMaxDist			(flt_max)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RayCollider::~RayCollider()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Validates current settings. You should call this method after all the settings and callbacks have been defined.
 *	\return		null if everything is ok, else a string describing the problem
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* RayCollider::ValidateSettings()
{
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)											return "Object callback must be defined! Call: SetCallback().";
#else
	if(!mFaces || !mVerts)										return "Object pointers must be defined! Call: SetPointers().";
#endif
	if(mMaxDist<0.0f)											return "Higher distance bound must be positive!";
	if(TemporalCoherenceEnabled() && !FirstContactEnabled())	return "Temporal coherence only works with ""First contact"" mode!";
	if(mClosestHit && FirstContactEnabled())					return "Closest hit doesn't work with ""First contact"" mode!";
	return null;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Generic stabbing query for generic OPCODE models. After the call, access the results:
 *	- with GetContactStatus()
 *	- in the user-provided destination array
 *
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		model			[in] Opcode model to collide with
 *	\param		world			[in] model's world matrix, or null
 *	\param		cache			[in] a possibly cached face index, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, OPCODE_Model* model, const Matrix4x4* world, udword* cache)
{
	// Checkings
	if(!model)	return false;

	// Simple double-dispatch
	if(!model->HasLeafNodes())
	{
		if(model->IsQuantized())	return Collide(world_ray, (const AABBQuantizedNoLeafTree*)model->GetTree(), world, cache);
		else						return Collide(world_ray, (const AABBNoLeafTree*)model->GetTree(), world, cache);
	}
	else
	{
		if(model->IsQuantized())	return Collide(world_ray, (const AABBQuantizedTree*)model->GetTree(), world, cache);
		else						return Collide(world_ray, (const AABBCollisionTree*)model->GetTree(), world, cache);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes a stabbing query :
 *	- reset stats & contact status
 *	- compute ray in local space
 *	- check temporal coherence
 *
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		world			[in] object's world matrix, or null
 *	\param		faceid		[in] index of previously stabbed triangle
 *	\return		contact status
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RayCollider::InitQuery(const Ray& world_ray, const Matrix4x4* world, udword* faceid)
{
	// Reset stats & contact status
	Collider::InitQueryEx();
	mNbRayBVTests		= 0;
	mNbRayPrimTests		= 0;
	mNbIntersections	= 0;
	if(mStabbedFaces)	mStabbedFaces->Reset();

	// Compute ray in local space
	// The (Origin/Dir) form is needed for the ray-triangle test anyway
	if(world)
	{
		Matrix3x3 InvWorld = *world;
		mDir = InvWorld * world_ray.mDir;

		Matrix4x4 World;
		InvertPRMatrix(World, *world);
		mOrigin = world_ray.mOrig * World;
	}
	else
	{
		mDir	= world_ray.mDir;
		mOrigin	= world_ray.mOrig;
	}

	// Precompute data
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)
	{
		// For Segment-AABB overlap
		mData = 0.5f * mDir * mMaxDist;
		mData2 = mOrigin + mData;

		// Precompute mFDir;
		mFDir.x = _abs(mData.x);
		mFDir.y = _abs(mData.y);
		mFDir.z = _abs(mData.z);
	}
	else
	{
		// For Ray-AABB overlap
//		udword x = SIR(mDir.x)-1;
//		udword y = SIR(mDir.y)-1;
//		udword z = SIR(mDir.z)-1;
//		mData.x = FR(x);
//		mData.y = FR(y);
//		mData.z = FR(z);

		// Precompute mFDir;
		mFDir.x = _abs(mDir.x);
		mFDir.y = _abs(mDir.y);
		mFDir.z = _abs(mDir.z);
	}

	// Check temporal coherence :

//## voir mClosest

	// Test previously colliding primitives first
	if(TemporalCoherenceEnabled() && FirstContactEnabled() && faceid && *faceid!=INVALID_ID)
	{
		// Request vertices from the app
		VertexPointers VP;
#ifdef OPC_USE_CALLBACKS
		(mObjCallback)(*faceid, VP, mUserData);
#else
		const IndexedTriangle* Tri = &mFaces[*faceid];
		VP.Vertex[0] = &mVerts[Tri->mVRef[0]];
		VP.Vertex[1] = &mVerts[Tri->mVRef[1]];
		VP.Vertex[2] = &mVerts[Tri->mVRef[2]];
#endif
		// Perform ray-cached tri overlap
		if(RayTriOverlap(*VP.Vertex[0], *VP.Vertex[1], *VP.Vertex[2]))
		{
			// Intersection point is valid if:
			// - distance is positive (else it can just be a face behind the orig point)
			// - distance is smaller than a given max distance (useful for shadow feelers)
			if(mStabbedFace.mDistance>0.0f && mStabbedFace.mDistance<mMaxDist)
			{
				// Set contact status
				mFlags |= OPC_CONTACT;

				mStabbedFace.mFaceID = *faceid;

				if(mStabbedFaces)	mStabbedFaces->AddFace(mStabbedFace);
			}
		}
	}
	return GetContactStatus();
}

#define UPDATE_CACHE												\
	if(cache && GetContactStatus() && mStabbedFaces)				\
	{																\
		const CollisionFace* Current = mStabbedFaces->GetFaces();	\
		if(Current)	*cache	= Current->mFaceID;						\
		else		*cache	= INVALID_ID;							\
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stabbing query for normal trees.
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		tree			[in] object's AABB tree
 *	\param		world			[in] object's world matrix, or null
 *	\param		cache			[in] a possibly cached face index, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, const AABBCollisionTree* tree, const Matrix4x4* world, udword* cache)
{
	// Checkings
	if(!tree)					return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)			return false;
#else
	if(!mFaces || !mVerts)		return false;
#endif
	// Init collision query
	if(InitQuery(world_ray, world, cache))	return true;

	// Perform stabbing query
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)	_Stab(tree->GetNodes());
	else								_UnboundedStab(tree->GetNodes());

	// Update cache if needed
	UPDATE_CACHE
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stabbing query for no-leaf trees.
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		tree			[in] object's AABB tree
 *	\param		world			[in] object's world matrix, or null
 *	\param		cache			[in] a possibly cached face index, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, const AABBNoLeafTree* tree, const Matrix4x4* world, udword* cache)
{
	// Checkings
	if(!tree)					return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)			return false;
#else
	if(!mFaces || !mVerts)		return false;
#endif

	// Init collision query
	if(InitQuery(world_ray, world, cache))	return true;

	// Perform stabbing query
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)	_Stab(tree->GetNodes());
	else								_UnboundedStab(tree->GetNodes());

	// Update cache if needed
	UPDATE_CACHE
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stabbing query for quantized trees.
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		tree			[in] object's AABB tree
 *	\param		world			[in] object's world matrix, or null
 *	\param		cache			[in] a possibly cached face index, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, const AABBQuantizedTree* tree, const Matrix4x4* world, udword* cache)
{
	// Checkings
	if(!tree)					return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)			return false;
#else
	if(!mFaces || !mVerts)		return false;
#endif

	// Init collision query
	if(InitQuery(world_ray, world, cache))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform stabbing query
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)	_Stab(tree->GetNodes());
	else								_UnboundedStab(tree->GetNodes());

	// Update cache if needed
	UPDATE_CACHE
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stabbing query for quantized no-leaf trees.
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		tree			[in] object's AABB tree
 *	\param		world			[in] object's world matrix, or null
 *	\param		cache			[in] a possibly cached face index, or null
 *	\return		true if success
 *	\warning	SCALE NOT SUPPORTED. The matrix must contain rotation & translation parts only.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, const AABBQuantizedNoLeafTree* tree, const Matrix4x4* world, udword* cache)
{
	// Checkings
	if(!tree)					return false;
#ifdef OPC_USE_CALLBACKS
	if(!mObjCallback)			return false;
#else
	if(!mFaces || !mVerts)		return false;
#endif

	// Init collision query
	if(InitQuery(world_ray, world, cache))	return true;

	// Setup dequantization coeffs
	mCenterCoeff	= tree->mCenterCoeff;
	mExtentsCoeff	= tree->mExtentsCoeff;

	// Perform stabbing query
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)	_Stab(tree->GetNodes());
	else								_UnboundedStab(tree->GetNodes());

	// Update cache if needed
	UPDATE_CACHE
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stabbing query for vanilla AABB trees.
 *	\param		world_ray		[in] stabbing ray in world space
 *	\param		tree			[in] AABB tree
 *	\param		box_indices		[out] indices of stabbed boxes
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RayCollider::Collide(const Ray& world_ray, const AABBTree* tree, Container& box_indices)
{
	// ### bad design here

	// This is typically called for a scene tree, full of -AABBs-, not full of triangles.
	// So we don't really have "primitives" to deal with. Hence it doesn't work with
	// "FirstContact" + "TemporalCoherence".
	ASSERT( !(FirstContactEnabled() && TemporalCoherenceEnabled()) );

	// Checkings
	if(!tree)					return false;

	// Init collision query
	// Basically this is only called to initialize precomputed data
	if(InitQuery(world_ray))	return true;

	// Perform stabbing query
	if(IR(mMaxDist)!=IEEE_MAX_FLOAT)	_Stab(tree, box_indices);
	else								_UnboundedStab(tree, box_indices);

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for normal AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_Stab(const AABBCollisionNode* node)
{
	// Perform Segment-AABB overlap test
	if(!SegmentAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	if(node->IsLeaf())
	{
		STAB_PRIM(node->GetPrimitive())
	}
	else
	{
		_Stab(node->GetPos());

		if(ContactFound()) return;

		_Stab(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for quantized AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_Stab(const AABBQuantizedNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Segment-AABB overlap test
	if(!SegmentAABBOverlap(Center, Extents))	return;

	if(node->IsLeaf())
	{
		STAB_PRIM(node->GetPrimitive())
	}
	else
	{
		_Stab(node->GetPos());

		if(ContactFound()) return;

		_Stab(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_Stab(const AABBNoLeafNode* node)
{
	// Perform Segment-AABB overlap test
	if(!SegmentAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	if(node->HasLeaf())
	{
		STAB_PRIM(node->GetPrimitive())
	}
	else _Stab(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())
	{
		STAB_PRIM(node->GetPrimitive2())
	}
	else _Stab(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_Stab(const AABBQuantizedNoLeafNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Segment-AABB overlap test
	if(!SegmentAABBOverlap(Center, Extents))	return;

	if(node->HasLeaf())
	{
		STAB_PRIM(node->GetPrimitive())
	}
	else _Stab(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())
	{
		STAB_PRIM(node->GetPrimitive2())
	}
	else _Stab(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for vanilla AABB trees.
 *	\param		node		[in] current collision node
 *	\param		box_indices	[out] indices of stabbed boxes
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_Stab(const AABBTreeNode* node, Container& box_indices)
{
	// Test the box against the segment
	Point Center, Extents;
	node->GetAABB()->GetCenter(Center);
	node->GetAABB()->GetExtents(Extents);
	if(!SegmentAABBOverlap(Center, Extents))	return;

	if(node->IsLeaf())
	{
		box_indices.Add(node->GetPrimitives(), node->GetNbPrimitives());
	}
	else
	{
		_Stab(node->GetPos(), box_indices);
		_Stab(node->GetNeg(), box_indices);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for normal AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_UnboundedStab(const AABBCollisionNode* node)
{
	// Perform Ray-AABB overlap test
	if(!RayAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	if(node->IsLeaf())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive())
	}
	else
	{
		_UnboundedStab(node->GetPos());

		if(ContactFound()) return;

		_UnboundedStab(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for quantized AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_UnboundedStab(const AABBQuantizedNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Ray-AABB overlap test
	if(!RayAABBOverlap(Center, Extents))	return;

	if(node->IsLeaf())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive())
	}
	else
	{
		_UnboundedStab(node->GetPos());

		if(ContactFound()) return;

		_UnboundedStab(node->GetNeg());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_UnboundedStab(const AABBNoLeafNode* node)
{
	// Perform Ray-AABB overlap test
	if(!RayAABBOverlap(node->mAABB.mCenter, node->mAABB.mExtents))	return;

	if(node->HasLeaf())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive())
	}
	else _UnboundedStab(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive2())
	}
	else _UnboundedStab(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for quantized no-leaf AABB trees.
 *	\param		node	[in] current collision node
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_UnboundedStab(const AABBQuantizedNoLeafNode* node)
{
	// Dequantize box
	const QuantizedAABB* Box = &node->mAABB;
	const Point Center(float(Box->mCenter[0]) * mCenterCoeff.x, float(Box->mCenter[1]) * mCenterCoeff.y, float(Box->mCenter[2]) * mCenterCoeff.z);
	const Point Extents(float(Box->mExtents[0]) * mExtentsCoeff.x, float(Box->mExtents[1]) * mExtentsCoeff.y, float(Box->mExtents[2]) * mExtentsCoeff.z);

	// Perform Ray-AABB overlap test
	if(!RayAABBOverlap(Center, Extents))	return;

	if(node->HasLeaf())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive())
	}
	else _UnboundedStab(node->GetPos());

	if(ContactFound()) return;

	if(node->HasLeaf2())
	{
		UNBOUNDED_STAB_PRIM(node->GetPrimitive2())
	}
	else _UnboundedStab(node->GetNeg());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive stabbing query for vanilla AABB trees.
 *	\param		node		[in] current collision node
 *	\param		box_indices	[out] indices of stabbed boxes
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RayCollider::_UnboundedStab(const AABBTreeNode* node, Container& box_indices)
{
	// Test the box against the ray
	Point Center, Extents;
	node->GetAABB()->GetCenter(Center);
	node->GetAABB()->GetExtents(Extents);
	if(!RayAABBOverlap(Center, Extents))	return;

	if(node->IsLeaf())
	{
		box_indices.Add(node->GetPrimitives(), node->GetNbPrimitives());
	}
	else
	{
		_UnboundedStab(node->GetPos(), box_indices);
		_UnboundedStab(node->GetNeg(), box_indices);
	}
}
