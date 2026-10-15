///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 *
 *  OPCODE modifications for scaled model support (and other things)
 *  Copyright (C) 2004 Gilvan Maia (gilvan 'at' vdl.ufc.br)
 *	Check http://www.vdl.ufc.br/gilvan/coll/opcode/index.htm for updates.
 *
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for an LSS collider.
 *	\file		OPC_LSSCollider.h
 *	\author		Pierre Terdiman
 *	\date		December, 28, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_LSSCOLLIDER_H__
#define __OPC_LSSCOLLIDER_H__

	struct OPCODE_API LSSCache : VolumeCache
	{
					LSSCache()
					{
						Previous.mP0 = IceMaths::Point(0.0f, 0.0f, 0.0f);
						Previous.mP1 = IceMaths::Point(0.0f, 0.0f, 0.0f);
						Previous.mRadius = 0.0f;
						FatCoeff = 1.1f;
					}

		// Cached faces signature
		IceMaths::LSS			Previous;	//!< LSS used when performing the query resulting in cached faces
		// User settings
		float		FatCoeff;	//!< mRadius2 multiplier used to create a fat LSS
	};

	class OPCODE_API LSSCollider : public VolumeCollider
	{
		public:
		// Constructor / Destructor
											LSSCollider();
		virtual								~LSSCollider();

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Generic collision query for generic OPCODE models. After the call, access the results:
		 *	- with GetContactStatus()
		 *	- with GetNbTouchedPrimitives()
		 *	- with GetTouchedPrimitives()
		 *
		 *	\param		cache			[in/out] an lss cache
		 *	\param		lss				[in] collision lss in local space
		 *	\param		model			[in] Opcode model to collide with
		 *	\param		worldl			[in] lss world matrix, or null
		 *	\param		worldm			[in] model's world matrix, or null
		 *	\return		true if success
		 *	\warning	SCALE NOT SUPPORTED IN THE LSS MATRIX. The matrix must contain rotation & translation parts only.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							bool			Collide(LSSCache& cache, const IceMaths::LSS& lss, const Model& model, const IceMaths::Matrix4x4* worldl=null, const IceMaths::Matrix4x4* worldm=null);
		//
							bool			Collide(LSSCache& cache, const IceMaths::LSS& lss, const AABBTree* tree);
		protected:
		// LSS in model space
							IceMaths::Segment			mSeg;			//!< Segment
							float			mRadius2;		//!< LSS radius squared
		// Internal methods
							void			_Collide(const AABBCollisionNode* node);
							void			_Collide(const AABBNoLeafNode* node);
							void			_Collide(const AABBQuantizedNode* node);
							void			_Collide(const AABBQuantizedNoLeafNode* node);
							void			_Collide(const AABBTreeNode* node);
							void			_CollideNoPrimitiveTest(const AABBCollisionNode* node);
							void			_CollideNoPrimitiveTest(const AABBNoLeafNode* node);
							void			_CollideNoPrimitiveTest(const AABBQuantizedNode* node);
							void			_CollideNoPrimitiveTest(const AABBQuantizedNoLeafNode* node);
			// Overlap tests
		inline_				BOOL			LSSContainsBox(const IceMaths::Point& bc, const IceMaths::Point& be);
		inline_				BOOL			LSSAABBOverlap(const IceMaths::Point& center, const IceMaths::Point& extents);
		inline_				BOOL			LSSTriOverlap(const IceMaths::Point& vert0, const IceMaths::Point& vert1, const IceMaths::Point& vert2);
			// Init methods
							BOOL			InitQuery(LSSCache& cache, const IceMaths::LSS& lss, const IceMaths::Matrix4x4* worldl=null, const IceMaths::Matrix4x4* worldm=null);
	};

	class OPCODE_API HybridLSSCollider : public LSSCollider
	{
		public:
		// Constructor / Destructor
											HybridLSSCollider();
		virtual								~HybridLSSCollider();

							bool			Collide(LSSCache& cache, const IceMaths::LSS& lss, const HybridModel& model, const IceMaths::Matrix4x4* worldl=null, const IceMaths::Matrix4x4* worldm=null);
		protected:
							Container		mTouchedBoxes;
	};

#endif // __OPC_LSSCOLLIDER_H__
