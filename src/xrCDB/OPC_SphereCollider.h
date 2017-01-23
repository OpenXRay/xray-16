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
 *	\file		OPC_SphereCollider.h
 *	\author		Pierre Terdiman
 *	\date		June, 2, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_SPHERECOLLIDER_H__
#define __OPC_SPHERECOLLIDER_H__

	struct OPCODE_API SphereCache : VolumeCache
	{
					SphereCache() : Center(0.0f,0.0f,0.0f), FatRadius2(0.0f), FatCoeff(1.1f)
					{
					}

		// Cached faces signature
		Point		Center;		//!< Sphere used when performing the query resulting in cached faces
		float		FatRadius2;	//!< Sphere used when performing the query resulting in cached faces
		// User settings
		float		FatCoeff;	//!< mRadius2 multiplier used to create a fat sphere
	};

	class OPCODE_API SphereCollider : public VolumeCollider
	{
		public:
		// Constructor / Destructor
											SphereCollider();
		virtual								~SphereCollider();
		// Generic collision query

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Generic collision query for generic OPCODE models. After the call, access the results:
		 *	- with GetContactStatus()
		 *	- in the user-provided destination array
		 *
		 *	\param		cache			[in/out] a sphere cache
		 *	\param		sphere			[in] collision sphere in local space
		 *	\param		model			[in] Opcode model to collide with
		 *	\param		worlds			[in] sphere's world matrix, or null
		 *	\param		worldm			[in] model's world matrix, or null
		 *	\return		true if success
		 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							bool			Collide(SphereCache& cache, const Sphere& sphere, OPCODE_Model* model, const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);

		// Collision queries
							bool			Collide(SphereCache& cache, const Sphere& sphere, const AABBCollisionTree* tree,		const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);
							bool			Collide(SphereCache& cache, const Sphere& sphere, const AABBNoLeafTree* tree,			const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);
							bool			Collide(SphereCache& cache, const Sphere& sphere, const AABBQuantizedTree* tree,		const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);
							bool			Collide(SphereCache& cache, const Sphere& sphere, const AABBQuantizedNoLeafTree* tree,	const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);
							bool			Collide(SphereCache& cache, const Sphere& sphere, const AABBTree* tree);
		// Settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Validates current settings. You should call this method after all the settings and callbacks have been defined for a collider.
		 *	\return		null if everything is ok, else a string describing the problem
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		override(Collider)	const char*		ValidateSettings();

		protected:
		// Sphere in model space
							Point			mCenter;			//!< Sphere center
							float			mRadius2;			//!< Sphere radius squared
		// Internal methods
							void			_Collide(const AABBCollisionNode* node);
							void			_Collide(const AABBNoLeafNode* node);
							void			_Collide(const AABBQuantizedNode* node);
							void			_Collide(const AABBQuantizedNoLeafNode* node);
							void			_Collide(const AABBTreeNode* node);
			// Overlap tests
		inline_				BOOL			SphereContainsBox(const Point& bc, const Point& be);
		inline_				BOOL			SphereAABBOverlap(const Point& center, const Point& extents);
							BOOL			SphereTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2);
			// Init methods
							BOOL			InitQuery(SphereCache& cache, const Sphere& sphere, const Matrix4x4* worlds=null, const Matrix4x4* worldm=null);
	};

#endif // __OPC_SPHERECOLLIDER_H__
