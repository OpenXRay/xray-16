///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for a versatile AABB tree.
 *	\file		OPC_AABBTree.cpp
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a generic AABB tree node.
 *
 *	\class		AABBTreeNode
 *	\author		Pierre Terdiman
 *	\version	1.2
 *	\date		March, 20, 2001
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a generic AABB tree.
 *	This is a vanilla AABB tree, without any particular optimization. It contains anonymous references to
 *	user-provided primitives, which can theoretically be anything - triangles, boxes, etc. Each primitive
 *	is surrounded by an AABB, regardless of the primitive's nature. When the primitive is a triangle, the
 *	resulting tree can be converted into an optimized tree. If the primitive is a box, the resulting tree
 *	can be used for culling - VFC or occlusion -, assuming you cull on a mesh-by-mesh basis (modern way).
 *
 *	\class		AABBTree
 *	\author		Pierre Terdiman
 *	\version	1.2
 *	\date		March, 20, 2001
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

namespace Opcode {
#	include "OPC_TreeBuilders.h"
} // namespace Opcode

using namespace Opcode;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTreeNode::AABBTreeNode() : mP(null), mN(null), mNbPrimitives(0), mNodePrimitives(null)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTreeNode::~AABBTreeNode()
{
}

void  AABBTreeNode::destroy		(AABBTreeBuilder*	_tree)
{
	if (mP)	{ mP->destroy		(_tree); _tree->node_destroy	(mP); }
	if (mN)	{ mN->destroy		(_tree); _tree->node_destroy	(mN); }
	mNodePrimitives				= null;	// This was just a shortcut to the global list => no release
	mNbPrimitives				= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Splits the node along a given axis.
 *	The list of indices is reorganized according to the split values.
 *	\param		axis		[in] splitting axis index
 *	\param		builder		[in] the tree builder
 *	\return		the number of primitives assigned to the first child
 *	\warning	this method reorganizes the internal list of primitives
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
udword AABBTreeNode::Split(udword axis, AABBTreeBuilder* builder)
{
	// Get node split value
	float SplitValue = builder->GetSplittingValueEx(mNodePrimitives, mNbPrimitives, mBV, axis);

	udword NbPos = 0;
	// Loop through all node-related primitives. Their indices range from mNodePrimitives[0] to mNodePrimitives[mNbPrimitives-1].
	// Those indices map the global list in the tree builder.
	for(udword i=0;i<mNbPrimitives;i++)
	{
		// Get index in global list
		udword Index = mNodePrimitives[i];

		// Test against the splitting value. The primitive value is tested against the enclosing-box center.
		// [We only need an approximate partition of the enclosing box here.]
		float PrimitiveValue = builder->GetSplittingValue(Index, axis);

		// Reorganize the list of indices in this order: positive - negative.
		if(PrimitiveValue > SplitValue)
		{
			// Swap entries
			udword Tmp = mNodePrimitives[i];
			mNodePrimitives[i] = mNodePrimitives[NbPos];
			mNodePrimitives[NbPos] = Tmp;
			// Count primitives assigned to positive space
			NbPos++;
		}
	}
	return NbPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Subdivides the node.
 *	
 *	          N
 *	        /   \
 *	      /       \
 *	   N/2         N/2
 *	  /   \       /   \
 *	N/4   N/4   N/4   N/4
 *	(etc)
 *
 *	A well-balanced tree should have a O(log n) depth.
 *	A degenerate tree would have a O(n) depth.
 *	Note a perfectly-balanced tree is not well-suited to collision detection anyway.
 *
 *	\param		builder		[in] the tree builder
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBTreeNode::Subdivide(AABBTreeBuilder* builder)
{
	// Checkings
	if(!builder)	return false;

	// Stop subdividing if we reach a leaf node
	if(mNbPrimitives==1)	return true;

	// Check the user-defined limit
	if(mNbPrimitives<=builder->mLimit)	return true;

	bool ValidSplit = true;	// Optimism...
	udword NbPos	= 0;
	if(builder->mRules&SPLIT_LARGESTAXIS)
	{
		// Find the largest axis to split along
		Point Extents;	mBV.GetExtents(Extents);	// Box extents
		udword Axis	= Extents.LargestAxis();		// Index of largest axis

		// Split along the axis
		NbPos = Split(Axis, builder);

		// Check split validity
		if(!NbPos || NbPos==mNbPrimitives)	ValidSplit = false;
	}
	else if(builder->mRules&SPLIT_SPLATTERPOINTS)
	{
		// Compute the means
		Point Means(0.0f, 0.0f, 0.0f);
		for(udword i=0;i<mNbPrimitives;i++)
		{
			udword Index = mNodePrimitives[i];
			Means.x+=builder->GetSplittingValue(Index, 0);
			Means.y+=builder->GetSplittingValue(Index, 1);
			Means.z+=builder->GetSplittingValue(Index, 2);
		}
		Means/=float(mNbPrimitives);

		// Compute variances
		Point Vars(0.0f, 0.0f, 0.0f);
		for(i=0;i<mNbPrimitives;i++)
		{
			udword Index = mNodePrimitives[i];
			float Cx = builder->GetSplittingValue(Index, 0);
			float Cy = builder->GetSplittingValue(Index, 1);
			float Cz = builder->GetSplittingValue(Index, 2);
			Vars.x += (Cx - Means.x)*(Cx - Means.x);
			Vars.y += (Cy - Means.y)*(Cy - Means.y);
			Vars.z += (Cz - Means.z)*(Cz - Means.z);
		}
		Vars/=float(mNbPrimitives-1);

		// Choose axis with greatest variance
		udword Axis = Vars.LargestAxis();

		// Split along the axis
		NbPos = Split(Axis, builder);

		// Check split validity
		if(!NbPos || NbPos==mNbPrimitives)	ValidSplit = false;
	}
	else if(builder->mRules&SPLIT_BALANCED)
	{
		// Test 3 axis, take the best
		float Results[3];
		NbPos = Split(0, builder);	Results[0] = float(NbPos)/float(mNbPrimitives);
		NbPos = Split(1, builder);	Results[1] = float(NbPos)/float(mNbPrimitives);
		NbPos = Split(2, builder);	Results[2] = float(NbPos)/float(mNbPrimitives);
		Results[0]-=0.5f;	Results[0]*=Results[0];
		Results[1]-=0.5f;	Results[1]*=Results[1];
		Results[2]-=0.5f;	Results[2]*=Results[2];
		udword Min=0;
		if(Results[1]<Results[Min])	Min = 1;
		if(Results[2]<Results[Min])	Min = 2;
		
		// Split along the axis
		NbPos = Split(Min, builder);

		// Check split validity
		if(!NbPos || NbPos==mNbPrimitives)	ValidSplit = false;
	}
	else if(builder->mRules&SPLIT_BESTAXIS)
	{
		// Test largest, then middle, then smallest axis...

		// Sort axis
		Point Extents;	mBV.GetExtents(Extents);	// Box extents
		udword SortedAxis[] = { 0, 1, 2 };
		float* Keys = (float*)&Extents.x;
		for(udword j=0;j<3;j++)
		{
			for(udword i=0;i<2;i++)
			{
				if(Keys[SortedAxis[i]]<Keys[SortedAxis[i+1]])
				{
					udword Tmp = SortedAxis[i];
					SortedAxis[i] = SortedAxis[i+1];
					SortedAxis[i+1] = Tmp;
				}
			}
		}

		// Find the largest axis to split along
		udword CurAxis = 0;
		ValidSplit = false;
		while(!ValidSplit && CurAxis!=3)
		{
			NbPos = Split(SortedAxis[CurAxis], builder);
			// Check the subdivision has been successful
			if(!NbPos || NbPos==mNbPrimitives)	CurAxis++;
			else								ValidSplit = true;
		}
	}
	else if(builder->mRules&SPLIT_FIFTY)
	{
		// Don't even bother splitting (mainly a performance test)
		NbPos = mNbPrimitives>>1;
	}
	else return false;	// Unknown splitting rules

	// Check the subdivision has been successful
	if(!ValidSplit)
	{
		// Here, all boxes lie in the same sub-space. Two strategies:
		// - if the tree *must* be complete, make an arbitrary 50-50 split
		// - else stop subdividing
		if(builder->mRules&SPLIT_COMPLETE)
		{
			builder->IncreaseNbInvalidSplits();
			NbPos = mNbPrimitives>>1;
		}
		else return true;
	}

	// Now create children and assign their pointers.
	mP = builder->node_alloc();     CHECKALLOC(mP);
	mN = builder->node_alloc();		CHECKALLOC(mN);

	// Update stats
	builder->IncreaseCount	(2);

	// Assign children
	mP->mNodePrimitives	= &mNodePrimitives[0];
	mP->mNbPrimitives	= NbPos;
	mN->mNodePrimitives	= &mNodePrimitives[NbPos];
	mN->mNbPrimitives	= mNbPrimitives - NbPos;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive hierarchy building in a top-down fashion.
 *	\param		builder		[in] the tree builder
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBTreeNode::_BuildHierarchy(AABBTreeBuilder* builder)
{
	// 1) Compute the global box for current node. The box is stored in mBV.
	builder->ComputeGlobalBox(mNodePrimitives, mNbPrimitives, mBV);

	// 2) Subdivide current node
	Subdivide(builder);

	// 3) Recurse
	if(mP)	mP->_BuildHierarchy(builder);
	if(mN)	mN->_BuildHierarchy(builder);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTree::AABBTree() : mIndices(null), mTotalNbNodes(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTree::~AABBTree()
{
	CFREE(mIndices);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Builds a generic AABB tree from a tree builder.
 *	\param		builder		[in] the tree builder
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBTree::Build(AABBTreeBuilder* builder)
{
	// Checkings
	if(!builder || !builder->mNbPrimitives)	return false;

	// Init stats
	builder->SetCount(1);
	builder->SetNbInvalidSplits(0);

	// Initialize indices. This list will be modified during build.
	CFREE(mIndices);
	mIndices	= CALLOC(udword,builder->mNbPrimitives);
	CHECKALLOC(mIndices);
	for(udword i=0;i<builder->mNbPrimitives;i++)	mIndices[i] = i;

	// Setup initial box
	mNodePrimitives	= mIndices;
	mNbPrimitives	= builder->mNbPrimitives;

	// Build the hierarchy
	_BuildHierarchy(builder);

	// Get back total number of nodes
	mTotalNbNodes	= builder->GetCount();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes the depth of the tree.
 *	A well-balanced tree should have a log(n) depth. A degenerate tree O(n) depth.
 *	\return		depth of the tree
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
udword AABBTree::ComputeDepth() const
{
	udword Depth = 0;
	udword Current = 0;

	struct Local
	{
		static void _UpdateDepth(const AABBTreeNode* curnode, udword& depth, udword& current)
		{
			// Checkings
			if(!curnode)	return;
			// Entering a _new_ node => increase depth
			current++;
			// Keep track of max depth
			if(current>depth)	depth = current;
			// Recurse
			if(curnode->GetPos())	{ _UpdateDepth(curnode->GetPos(), depth, current);	current--;	}
			if(curnode->GetNeg())	{ _UpdateDepth(curnode->GetNeg(), depth, current);	current--;	}
		}
	};
	Local::_UpdateDepth(this, Depth, Current);
	return Depth;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes the number of bytes used by the tree.
 *	\return		number of bytes used
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
udword AABBTree::GetUsedBytes() const
{
	udword TotalSize = mTotalNbNodes*GetNodeSize();
	if(mIndices)	TotalSize+=mNbPrimitives*sizeof(udword);
	return TotalSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks the tree is a complete tree or not.
 *	A complete tree is made of 2*N-1 nodes, where N is the number of primitives in the tree.
 *	\return		true for complete trees
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBTree::IsComplete() const
{
	return (GetNbNodes()==GetNbPrimitives()*2-1);
}
