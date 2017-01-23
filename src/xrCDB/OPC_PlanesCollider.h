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
 *	\file		OPC_PlanesCollider.h
 *	\author		Pierre Terdiman
 *	\date		January, 1st, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_PLANESCOLLIDER_H__
#define __OPC_PLANESCOLLIDER_H__

	struct OPCODE_API PlanesCache : VolumeCache
	{
					PlanesCache()
					{
					}
	};

	class OPCODE_API PlanesCollider : public VolumeCollider
	{
		public:
		// Constructor / Destructor
											PlanesCollider();
		virtual								~PlanesCollider();
		// Generic collision query

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Generic collision query for generic OPCODE models. After the call, access the results:
		 *	- with GetContactStatus()
		 *	- with GetNbTouchedFaces()
		 *	- with GetTouchedFaces()
		 *
		 *	\param		cache			[in/out] a planes cache
		 *	\param		planes			[in] list of planes in world space
		 *	\param		nb_planes		[in] number of planes
		 *	\param		model			[in] Opcode model to collide with
		 *	\param		worldm			[in] model's world matrix, or null
		 *	\return		true if success
		 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							bool			Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, OPCODE_Model* model, const Matrix4x4* worldm=null);

		// Collision queries
							bool			Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBCollisionTree* tree,		const Matrix4x4* worldm=null);
							bool			Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBNoLeafTree* tree,			const Matrix4x4* worldm=null);
							bool			Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBQuantizedTree* tree,		const Matrix4x4* worldm=null);
							bool			Collide(PlanesCache& cache, const Plane* planes, udword nb_planes, const AABBQuantizedNoLeafTree* tree,	const Matrix4x4* worldm=null);

		// Mutant box-with-planes collision queries
		inline_				bool			Collide(PlanesCache& cache, const OBB& box, OPCODE_Model* model, const Matrix4x4* worldb=null, const Matrix4x4* worldm=null)
							{
								Plane PL[6];

								if(worldb)
								{
									// Create a _new_ OBB in world space
									OBB WorldBox;
									box.Rotate(*worldb, WorldBox);
									// Compute planes from the sides of the box
									WorldBox.ComputePlanes(PL);
								}
								else
								{
									// Compute planes from the sides of the box
									box.ComputePlanes(PL);
								}

								// Collide with box planes
								return Collide(cache, PL, 6, model, worldm);
							}
		// Settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Validates current settings. You should call this method after all the settings and callbacks have been defined for a collider.
		 *	\return		null if everything is ok, else a string describing the problem
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		override(Collider)	const char*		ValidateSettings();

		protected:
		// Planes in model space
							udword			mNbPlanes;
							Plane*			mPlanes;
		// Leaf description
							VertexPointers	mVP;
		// Internal methods
							void			_Collide(const AABBCollisionNode* node, udword clipmask);
							void			_Collide(const AABBNoLeafNode* node, udword clipmask);
							void			_Collide(const AABBQuantizedNode* node, udword clipmask);
							void			_Collide(const AABBQuantizedNoLeafNode* node, udword clipmask);
			// Overlap tests
		inline_				BOOL			PlanesAABBOverlap(const Point& center, const Point& extents, udword& out_clip_mask, udword in_clip_mask);
		inline_				BOOL			PlanesTriOverlap(udword in_clip_mask);
			// Init methods
							BOOL			InitQuery(PlanesCache& cache, const Plane* planes, udword nb_planes, const Matrix4x4* worldm=null);
	};

#endif // __OPC_PLANESCOLLIDER_H__
