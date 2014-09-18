///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains various caches.
 *	\file		OPC_BVTCache.h
 *	\author		Pierre Terdiman
 *	\date		May, 12, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_BVTCACHE_H__
#define __OPC_BVTCACHE_H__

	//! This structure holds cached information used by the algorithm.
	//! Two model pointers and two colliding primitives are cached. Model pointers are assigned
	//! to their respective meshes, and the pair of colliding primitives is used for temporal
	//! coherence. That is, in case temporal coherence is enabled, those two primitives are
	//! tested for overlap before everything else. If they still collide, we're done before
	//! even entering the recursive collision code.
	struct OPCODE_API BVTCache : Pair
	{
		//! Constructor
		inline_			BVTCache()
						{
							ResetCache();
							ResetCountDown();
						}

						void ResetCache()
						{
							Model0			= null;
							Model1			= null;
							id0				= 0;
							id1				= 1;
#ifdef __MESHMERIZER_H__	// Collision hulls only supported within ICE !
							HullTest		= true;
							SepVector.pid	= 0;
							SepVector.qid	= 0;
							SepVector.SV	= Point(1.0f, 0.0f, 0.0f);
#endif // __MESHMERIZER_H__
						}

		inline_			void ResetCountDown()
						{
#ifdef __MESHMERIZER_H__	// Collision hulls only supported within ICE !
							CountDown		= 50;
#endif // __MESHMERIZER_H__
						}

		OPCODE_Model*	Model0;	//!< Model for first object
		OPCODE_Model*	Model1;	//!< Model for second object

#ifdef __MESHMERIZER_H__	// Collision hulls only supported within ICE !
		SVCache			SepVector;
		udword			CountDown;
		bool			HullTest;
#endif // __MESHMERIZER_H__
	};

#endif // __OPC_BVTCACHE_H__