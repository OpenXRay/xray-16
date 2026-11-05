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
 *	Contains a mesh interface.
 *	\file		OPC_MeshInterface.h
 *	\author		Pierre Terdiman
 *	\date		November, 27, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_MESHINTERFACE_H__
#define __OPC_MESHINTERFACE_H__

	struct VertexPointers
	{
		const IceMaths::Point*	Vertex[3];

		bool BackfaceCulling(const IceMaths::Point& source)
		{
			const IceMaths::Point& p0 = *Vertex[0];
			const IceMaths::Point& p1 = *Vertex[1];
			const IceMaths::Point& p2 = *Vertex[2];

			// Compute normal direction
			IceMaths::Point Normal = (p2 - p1)^(p0 - p1);

			// Backface culling
			return (Normal | (source - p0)) >= 0.0f;
		}
	};

#ifdef OPC_USE_CALLBACKS
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	User-callback, called by OPCODE to request vertices from the app.
	 *	\param		triangle_index	[in] face index for which the system is requesting the vertices
	 *	\param		triangle		[out] triangle's vertices (must be provided by the user)
	 *	\param		user_data		[in] user-defined data from SetCallback()
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef void	(*RequestCallback)	(udword triangle_index, VertexPointers& triangle, void* user_data);
#endif

	/// Enumerates mesh types
    enum MeshInterfaceType
	{
		/// Terrain mesh
		MESH_TERRAIN,
		/// Simple, triangle mesh
		MESH_TRIANGLE,
		/// Triangle strip
		MESH_TRIANGLE_STRIP,
		/// Triangle fan
		MESH_TRIANGLE_FAN
	};

	class OPCODE_API MeshInterface
	{
		public:
		// Constructor / Destructor
											MeshInterface();
											~MeshInterface();
		// Common settings
		inline_			MeshInterfaceType   GetInterfaceType()	const	{ return mMIType;	}
		inline_			udword				GetNbTriangles()	const	{ return mNbTris;	}
		inline_			udword				GetNbVertices()		const	{ return mNbVerts;	}
		inline_			udword				GetNbRows()			const	{ return 1 + mNbTris/(2*(mNbVerts-1));	}
		inline_			void				SetNbTriangles(udword nb)	{ mNbTris = nb;		}
		inline_			void				SetNbVertices(udword nb)	{ mNbVerts = nb;	}
		inline_			void				SetInterfaceType(MeshInterfaceType mit)	{ mMIType = mit;	}

#ifdef OPC_USE_CALLBACKS
		// Callback settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Callback control: setups object callback. Must provide triangle-vertices for a given triangle index.
		 *	\param		callback	[in] user-defined callback
		 *	\param		user_data	[in] user-defined data
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						bool				SetCallback(RequestCallback callback, void* user_data);
		inline_			void*				GetUserData()		const	{ return mUserData;		}
		inline_			RequestCallback		GetCallback()		const	{ return mObjCallback;	}
#else
		// Pointers settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Pointers control: setups object pointers. Must provide access to faces and vertices for a given object.
		 *	\param		tris	[in] pointer to triangles
		 *	\param		verts	[in] pointer to vertices
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						bool				SetPointers(const IceMaths::IndexedTriangle* tris, const IceMaths::Point* verts);
		inline_	const	IceMaths::IndexedTriangle*	GetTris()			const	{ return mTris;			}
		inline_	const	IceMaths::Point*				GetVerts()			const	{ return mVerts;		}

	#ifdef OPC_USE_STRIDE
		// Strides settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Strides control
		 *	\param		tri_stride		[in] size of a triangle in bytes. The first sizeof(IndexedTriangle) bytes are used to get vertex indices.
		 *	\param		vertex_stride	[in] size of a vertex in bytes. The first sizeof(Point) bytes are used to get vertex position.
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						bool				SetStrides(udword tri_stride=sizeof(IceMaths::IndexedTriangle), udword vertex_stride=sizeof(IceMaths::Point));
		inline_			udword				GetTriStride()		const	{ return mTriStride;	}
		inline_			udword				GetVertexStride()	const	{ return mVertexStride;	}
	#endif
#endif

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Fetches a triangle given a triangle index.
		 *	\param		vp		[out] required triangle's vertex pointers
		 *	\param		index	[in] triangle index
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_			void				GetTriangle(VertexPointers& vp, udword index)	const
											{
#ifdef OPC_USE_CALLBACKS
												// if we are using callbacks there is nothing to do!
												(mObjCallback)(index, vp, mUserData);
#else
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// START OF STRIDE CODE!
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#ifdef OPC_USE_STRIDE

		#ifndef OPC_DOUBLE_PRECISION
			//! UTILITY macro for unpacking the K-th triangle vertex from a given index (using strides)
			#define OPC_UNPACK_VERTEX(K, ind )		\
				vp.Vertex[K] = (const IceMaths::Point*)(((ubyte*)mVerts) + (ind) * mVertexStride);
		#else
			//! UTILITY macro for shorter code when using double-precision vertices
			#define OPC_COPY_VERTEX(K)				\
					VertexCache[K].x = (float)v[0];	\
					VertexCache[K].x = (float)v[1];	\
					VertexCache[K].x = (float)v[2];	\
					vp.Vertex[K] = &VertexCache[K];

			//! UTILITY macro for unpacking the K-th triangle vertex from a given index (using strides)
			#define OPC_UNPACK_VERTEX(K, ind )		\
			{										\
				const double* v = (const double*)(((ubyte*)mVerts) + (ind) * mVertexStride);
				OPC_COPY_VERTEX(K)
			}
		#endif

											switch( mMIType )
											{
											case MESH_TRIANGLE:
												if(mTris)
												{
													const IceMaths::IndexedTriangle* T = (const IceMaths::IndexedTriangle*)( ((ubyte*)mTris) + index * mTriStride);

													OPC_UNPACK_VERTEX(0, T->mVRef[0] )
													OPC_UNPACK_VERTEX(1, T->mVRef[1] )
													OPC_UNPACK_VERTEX(2, T->mVRef[2] )

												}else
												{
													// suppport for non-indexed meshes
													index *= 3;

													OPC_UNPACK_VERTEX(0, index   )
													OPC_UNPACK_VERTEX(1, index+1 )
													OPC_UNPACK_VERTEX(2, index+2 )
												}//else
											break;
											case MESH_TRIANGLE_STRIP:
												if(mTris)
												{
													const IceMaths::IndexedTriangle* T = (const IceMaths::IndexedTriangle*)(((ubyte*)mTris) + index * mTriStride);

													// unpack indexed double-precision tri-trip
													OPC_UNPACK_VERTEX(0, T->mVRef[0]  )
													OPC_UNPACK_VERTEX(1, T->mVRef[index%2 ? 2 : 1] )
													OPC_UNPACK_VERTEX(2, T->mVRef[index%2 ? 1 : 2])
												}
												else
												{
													// unpack non-indexed double-precision tri-trip
													OPC_UNPACK_VERTEX(0, index )
													OPC_UNPACK_VERTEX(1, (index%2 ? index+2 : index+1)  )
													OPC_UNPACK_VERTEX(2, (index%2 ? index+1 : index+2)  )
												}
											break;

											case MESH_TRIANGLE_FAN:
												if(mTris)
												{
													const IceMaths::IndexedTriangle* T = (const IceMaths::IndexedTriangle*)(((ubyte*)mTris) + index * mTriStride);
													//const udword* inds = (const udword*)mTris;

													// unpack indexed double-precision tri-fan
													OPC_UNPACK_VERTEX(0, mTris[0].mVRef[0] )
													OPC_UNPACK_VERTEX(1, (T->mVRef[1]) )
													OPC_UNPACK_VERTEX(2, (T->mVRef[2]) )
												}
												else
												{
													// unpack non-indexed double-precision tri-trip
													OPC_UNPACK_VERTEX(0, 0		 )
													OPC_UNPACK_VERTEX(1, index+1 )
													OPC_UNPACK_VERTEX(2, index+2 )
												}
											break;

											case MESH_TERRAIN:
												// NOTE: Terrain models are always non-indexed, so indices won't be used here.
												//		 What we do is compute indices. We introduce a little overhead, but using
												//		 integer operations... shall we test performance??
												{
													udword trisPerRow = ((mNbVerts-1)<<1);

													udword row  = index / trisPerRow;
													udword coll = index % trisPerRow;
													udword i0 = row*mNbVerts + (coll>>1);

													// here we use a lookup table for a good tesselation
													udword lookup[4][3] =
													{
															{0,mNbVerts+1,1},		// case 0
															{0,mNbVerts,mNbVerts+1},// case 1
															{mNbVerts,mNbVerts+1,1},// case 2
															{0,mNbVerts,1}			// case 3
													};

													// compute key into lookup table
													udword key = (row%2) ? (coll%2) | ((i0%2)<<1) : (3- ((coll%2) | ((i0%2)<<1)));

													// unpack terrain mesh here
													OPC_UNPACK_VERTEX(0, (i0 + lookup[key][0]) )
													OPC_UNPACK_VERTEX(1, (i0 + lookup[key][1]) )
													OPC_UNPACK_VERTEX(2, (i0 + lookup[key][2]) )
												}
											break;
											}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// END OF STRIDE CODE!
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#else
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// START OF NON-STRIDE CODE!
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		#ifndef OPC_DOUBLE_PRECISION
			//! UTILITY macro for unpacking the K-th triangle vertex from a given index (not using strides)
			#define OPC_UNPACK_VERTEX(K, ind )												 \
				vp.Vertex[K] = &mVerts[ind];
		#else
			//! UTILITY macro for shorter code when using double-precision vertices
			#define OPC_COPY_VERTEX(K)				\
					VertexCache[K].x = (float)v[0];	\
					VertexCache[K].x = (float)v[1];	\
					VertexCache[K].x = (float)v[2];	\
					vp.Vertex[K] = &VertexCache[K];

			//! UTILITY macro for unpacking the K-th triangle vertex from a given index (not using strides)
			#define OPC_UNPACK_VERTEX(K, ind )												 \
			{																				 \
				const double* v = (const double*)(((ubyte*)mVerts) + (ind) * 3*sizeof(double));\
				OPC_COPY_VERTEX(K)															 \
			}
		#endif
												switch( mMIType )
												{
												case MESH_TRIANGLE:
													if(mTris)
													{
														const IndexedTriangle* T = &mTris[index];

														OPC_UNPACK_VERTEX(0,T->mVRef[0])
														OPC_UNPACK_VERTEX(1,T->mVRef[1])
														OPC_UNPACK_VERTEX(2,T->mVRef[2])
													}else
													{// support for non-indexed meshes
														index *= 3;

														OPC_UNPACK_VERTEX(0,index  )
														OPC_UNPACK_VERTEX(1,index+1)
														OPC_UNPACK_VERTEX(2,index+2)
													}
												break;
												case MESH_TRIANGLE_STRIP:
													if(mTris)
													{
														const udword* inds = (const udword*)mTris;

														// unpack indexed double-precision tri-trip
														OPC_UNPACK_VERTEX(0,inds[index]  )
														OPC_UNPACK_VERTEX(1,inds[index%2 ? index+2 : index+1])
														OPC_UNPACK_VERTEX(2,inds[index%2 ? index+1 : index+2])
                                                     }
													else
													{

														// unpack non-indexed double-precision tri-trip
														OPC_UNPACK_VERTEX(0, index )
														OPC_UNPACK_VERTEX(1, (index%2 ? index+2 : index+1)  )
														OPC_UNPACK_VERTEX(2, (index%2 ? index+1 : index+2)  )
                                                    }
												break;

												case MESH_TRIANGLE_FAN:
													if(mTris)
													{
														const udword* inds = (const udword*)mTris;

														// unpack indexed double-precision tri-fan
														OPC_UNPACK_VERTEX(0, inds[0] )
														OPC_UNPACK_VERTEX(1, inds[index+1] )
														OPC_UNPACK_VERTEX(2, inds[index+2] )
													}
													else
													{
														// unpack non-indexed double-precision tri-trip
														OPC_UNPACK_VERTEX(0, 0 )
														OPC_UNPACK_VERTEX(1, (index+1) )
														OPC_UNPACK_VERTEX(2, (index+2) )
													}
												break;

												case MESH_TERRAIN:
													// NOTE: Terrain models are always non-indexed, so indices won't be used here.
													//		 What we do is compute indices. We introduce a little overhead, but using
													//		 integer operations... shall we test performance??
													{
														udword trisPerRow = ((mNbVerts-1)<<1);

														udword row  = index / trisPerRow;
														udword coll = index % trisPerRow;
														udword i0 = row*mNbVerts + (coll>>1);

														// here we use a lookup table for a good tesselation
														udword lookup[4][3] =
														{
																{0,mNbVerts+1,1},		// case 0
																{0,mNbVerts,mNbVerts+1},// case 1
																{mNbVerts,mNbVerts+1,1},// case 2
																{0,mNbVerts,1}			// case 3
														};

														// compute key into lookup table
														udword key = (row%2) ? (coll%2) | ((i0%2)<<1) : (3- ((coll%2) | ((i0%2)<<1)));

														// unpack terrain mesh here
														OPC_UNPACK_VERTEX(0, (i0 + lookup[key][0]) )
														OPC_UNPACK_VERTEX(1, (i0 + lookup[key][1]) )
														OPC_UNPACK_VERTEX(2, (i0 + lookup[key][2]) )
													}
												break;
												}
	#endif
#endif
											}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Remaps client's mesh according to a permutation.
		 *	\param		nb_indices	[in] number of indices in the permutation (will be checked against number of triangles)
		 *	\param		permutation	[in] list of triangle indices
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		bool				RemapClient(udword nb_indices, const udword* permutation)	const;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Checks the mesh interface is valid, i.e. things have been setup correctly.
		 *	\return		true if valid
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						bool				IsValid()		const;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Checks the mesh itself is valid.
		 *	Currently we only look for degenerate faces.
		 *	\return		number of degenerate faces
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						udword				CheckTopology()	const;
		private:

						udword				mNbTris;			//!< Number of triangles in the input model
						udword				mNbVerts;			//!< Number of vertices in the input model (if this model is a terrain one, this holds the number of vertices per row)

						MeshInterfaceType   mMIType;			//!< Mesh interface type
#ifdef OPC_USE_CALLBACKS
		// User callback
						void*				mUserData;			//!< User-defined data sent to callback
						RequestCallback		mObjCallback;		//!< Object callback
#else
		// User pointers
				const	IceMaths::IndexedTriangle*	mTris;				//!< Array of indexed triangles
				const	IceMaths::Point*			mVerts;				//!< Array of vertices
	#ifdef OPC_USE_STRIDE
						udword				mTriStride;			//!< Possible triangle stride in bytes [Opcode 1.3]
						udword				mVertexStride;		//!< Possible vertex stride in bytes [Opcode 1.3]
	#endif

		private:
						static IceMaths::Point VertexCache[3];
#endif
	};

#endif //__OPC_MESHINTERFACE_H__
