///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for optimized trees.
 *	\file		OPC_OptimizedTree.h
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_OPTIMIZEDTREE_H__
#define __OPC_OPTIMIZEDTREE_H__

	//! Common interface for a node of an implicit tree
	#define IMPLEMENT_IMPLICIT_NODE(baseclass, volume)													\
		public:																							\
		/* Constructor / Destructor */																	\
		inline_								baseclass() : mData(0)	{}									\
		inline_								~baseclass()			{}									\
		/* Leaf test */																					\
		inline_			BOOL				IsLeaf()		const	{ return (BOOL)(mData&1);		}	\
		/* Data access */																				\
		inline_			const baseclass*	GetPos()		const	{ return (baseclass*)mData;		}	\
		inline_			const baseclass*	GetNeg()		const	{ return ((baseclass*)mData)+1;	}	\
		inline_			udword				GetPrimitive()	const	{ return udword(mData>>1);		}	\
		/* Stats */																						\
		inline_			size_t				GetNodeSize()	const	{ return SIZEOFOBJECT;			}	\
																										\
						volume				mAABB;														\
						uintptr_t			mData;

	//! Common interface for a node of a no-leaf tree
	#define IMPLEMENT_NOLEAF_NODE(baseclass, volume)													\
		public:																							\
		/* Constructor / Destructor */																	\
		inline_								baseclass() : mData(0), mData2(0)	{}						\
		inline_								~baseclass()						{}						\
		/* Leaf tests */																				\
		inline_			BOOL				HasLeaf()		const	{ return (BOOL)(mData&1);		}	\
		inline_			BOOL				HasLeaf2()		const	{ return (BOOL)(mData2&1);		}	\
		/* Data access */																				\
		inline_			const baseclass*	GetPos()		const	{ return (baseclass*)mData;		}	\
		inline_			const baseclass*	GetNeg()		const	{ return (baseclass*)mData2;	}	\
		inline_			udword				GetPrimitive()	const	{ return udword(mData>>1);		}	\
		inline_			udword				GetPrimitive2()	const	{ return udword(mData2>>1);		}	\
		/* Stats */																						\
		inline_			size_t				GetNodeSize()	const	{ return SIZEOFOBJECT;			}	\
																										\
						volume				mAABB;														\
						uintptr_t			mData;														\
						uintptr_t			mData2;

	class OPCODE_API AABBCollisionNode
	{
		IMPLEMENT_IMPLICIT_NODE(AABBCollisionNode, CollisionAABB)

		inline_			float				GetVolume()		const	{ return mAABB.mExtents.x * mAABB.mExtents.y * mAABB.mExtents.z;	}
		inline_			float				GetSize()		const	{ return mAABB.mExtents.SquareMagnitude();	}
		inline_			udword				GetRadius()		const
						{
							udword* Bits = (udword*)&mAABB.mExtents.x;
							udword Max = Bits[0];
							if(Bits[1]>Max)	Max = Bits[1];
							if(Bits[2]>Max)	Max = Bits[2];
							return Max;
						}

		// NB: using the square-magnitude or the true volume of the box, seems to yield better results
		// (assuming UNC-like informed traversal methods). I borrowed this idea from PQP. The usual "size"
		// otherwise, is the largest box extent. In SOLID that extent is computed on-the-fly each time it's
		// needed (the best approach IMHO). In RAPID the rotation matrix is permuted so that Extent[0] is
		// always the greatest, which saves looking for it at runtime. On the other hand, it yields matrices
		// whose determinant is not 1, i.e. you can't encode them anymore as unit quaternions. Not a very
		// good strategy.
	};

	class OPCODE_API AABBQuantizedNode
	{
		IMPLEMENT_IMPLICIT_NODE(AABBQuantizedNode, QuantizedAABB)

		inline_			uword				GetSize()		const
						{
							const uword* Bits = mAABB.mExtents;
							uword Max = Bits[0];
							if(Bits[1]>Max)	Max = Bits[1];
							if(Bits[2]>Max)	Max = Bits[2];
							return Max;
						}
		// NB: for quantized nodes I don't feel like computing a square-magnitude with integers all
		// over the place.......!
	};

	class OPCODE_API AABBNoLeafNode
	{
		IMPLEMENT_NOLEAF_NODE(AABBNoLeafNode, CollisionAABB)
	};

	class OPCODE_API AABBQuantizedNoLeafNode
	{
		IMPLEMENT_NOLEAF_NODE(AABBQuantizedNoLeafNode, QuantizedAABB)
	};

	//! Common interface for a collision tree
	#define IMPLEMENT_COLLISION_TREE(baseclass, volume)														\
		public:																								\
		/* Constructor / Destructor */																		\
											baseclass();													\
		virtual								~baseclass();													\
		/* Build from a standard tree */																	\
		virtual			bool				Build(AABBTree* tree);											\
		/* Data access */																					\
		inline_			const volume*		GetNodes()		const	{ return mNodes;					}	\
		/* Stats */																							\
		virtual			udword				GetUsedBytes()	const	{ return mNbNodes*sizeof(volume);	}	\
		private:																							\
						volume*				mNodes;

	class OPCODE_API AABBOptimizedTree
	{
		public:
		// Constructor / Destructor
											AABBOptimizedTree() : mNbNodes(0)		{}
		virtual								~AABBOptimizedTree()					{}

		// Data access
		inline_			udword				GetNbNodes()	const	{ return mNbNodes;	}

		virtual			udword				GetUsedBytes()	const	= 0;
		virtual			bool				Build(AABBTree* tree)	= 0;
		protected:
						udword				mNbNodes;
	};

	class OPCODE_API AABBCollisionTree : public AABBOptimizedTree
	{
		IMPLEMENT_COLLISION_TREE(AABBCollisionTree, AABBCollisionNode)
	};

	class OPCODE_API AABBNoLeafTree : public AABBOptimizedTree
	{
		IMPLEMENT_COLLISION_TREE(AABBNoLeafTree, AABBNoLeafNode)
	};

	class OPCODE_API AABBQuantizedTree : public AABBOptimizedTree
	{
		IMPLEMENT_COLLISION_TREE(AABBQuantizedTree, AABBQuantizedNode)

		public:
						Point				mCenterCoeff;
						Point				mExtentsCoeff;
	};

	class OPCODE_API AABBQuantizedNoLeafTree : public AABBOptimizedTree
	{
		IMPLEMENT_COLLISION_TREE(AABBQuantizedNoLeafTree, AABBQuantizedNoLeafNode)

		public:
						Point				mCenterCoeff;
						Point				mExtentsCoeff;
	};

#endif // __OPC_OPTIMIZEDTREE_H__
