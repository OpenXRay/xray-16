///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 *
 *	OPCODE modifications for scaled model support (and some minor things)
 *	Copyright (C) 2004-2005 Gilvan Maia (gilvan 'at' vdl.ufc.br)
 *	Check http://www.vdl.ufc.br/gilvan/coll/opcode/index.htm  for news, more information and updates.
 *
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains compilation flags.
 *	\file		OPC_Settings.h
 *	\author		Pierre Terdiman
 *	\date		May, 12, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_SETTINGS_H__
#define __OPC_SETTINGS_H__

	//! NEW!!!	Use double-precision vertices. Remember that this is a "storage-only" option, because
	//			every test performed in OPCODE for collision is done against a triangle. This means a
	//			double precision triangle is 'cast' into a single precision one before true collision
	//			tests are performed, trees built, etc.
	// #define OPC_DOUBLE_PRECISION

	//! Use CPU comparisons (comment that line to use standard FPU compares)
	#define OPC_CPU_COMPARE

	//! Use FCOMI / FCMOV on Pentium-Pro based processors (comment that line to use plain C++)
#if !defined(_M_X64) && defined(_MSC_VER)
	#define OPC_USE_FCOMI
#endif

	//! Use epsilon value in tri-tri overlap test
	#define OPC_TRITRI_EPSILON_TEST

	//! Use tree-coherence or not [not implemented yet]
//	#define OPC_USE_TREE_COHERENCE

	//! Use callbacks or direct pointers. Using callbacks might be a bit slower (but probably not much)
//	#define OPC_USE_CALLBACKS

	//! Support triangle and vertex strides or not. Using strides might be a bit slower (but probably not much)
	//! HINT: Enable strides only if your vertices are NOT "tighly packed", ie, there are other info (not only
	//!		  vertex positions but colors, normals, etc) stored at each vertex. In this case, vertex strides
	//!		  must be set to the 'sizeof' the 'struct' holding your vertex positions in an array - if you used
	//!		  OpenGL vertex arrays, you should understand this fine.
	//!		  Thus, if you always have an array of 'xzy' coordinates and an array for indices, it could be better
	//!		  if you disable strides.
	#define OPC_USE_STRIDE

	//! Discard negative pointer in vanilla trees
//	#define OPC_NO_NEG_VANILLA_TREE

	//! Use a callback in the ray collider
	//#define OPC_RAYHIT_CALLBACK

	// NB: no compilation flag to enable/disable stats since they're actually needed in the box/box overlap test.
	//     Such flag will be added in the 1.3.3 version and will make things faster. Just wait.

#endif //__OPC_SETTINGS_H__
