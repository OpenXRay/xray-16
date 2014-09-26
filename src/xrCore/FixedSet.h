#ifndef _FIXEDSET_H
#define _FIXEDSET_H
#pragma once

template<class K, class allocator = xr_allocator>
class FixedSET
{
	enum	{
		SG_REALLOC_ADVANCE	= 64,
		SG_REALLOC_ALIGN	= 64
	};
public:
	struct TNode {
		K		key;
		TNode	*left,*right;
	};
	typedef void __fastcall callback(TNode*);
private:
	TNode*		nodes;
	u32			pool;
	u32			limit;

	IC u32		Size(u32 Count)
	{	return Count*sizeof(TNode);	}

	void		Realloc()
	{
		u32	newLimit = limit + SG_REALLOC_ADVANCE;
		VERIFY(newLimit%SG_REALLOC_ADVANCE == 0);
		TNode*	newNodes = (TNode*)allocator::alloc(sizeof(TNode)*newLimit);
		VERIFY(newNodes);

		ZeroMemory(newNodes, Size(newLimit));
		if (limit) CopyMemory(newNodes, nodes, Size(limit));

		for (u32 I=0; I<pool; I++)
		{
			VERIFY(nodes);
			TNode*	Nold	= nodes	+ I;
			TNode*	Nnew	= newNodes + I;

			if (Nold->left) {
				u32	Lid			= u32(Nold->left  - nodes);
				Nnew->left		= newNodes + Lid;
			}
			if (Nold->right) {
				u32	Rid			= u32(Nold->right - nodes);
				Nnew->right		= newNodes + Rid;
			}
		}
		if (nodes) allocator:dealloc(nodes);

		nodes = newNodes;
		limit = newLimit;
	}
	IC TNode*	Alloc(const K& key)
	{
		if (pool==limit) Realloc();
		TNode *node = nodes + pool;
		node->key	= key;
		node->right = node->left = 0;
		pool++;
		return node;
	}
	IC TNode*	CreateChild(TNode* &parent, const K& key)
	{
		u32 PID		= u32(parent-nodes);
		TNode*	N	= Alloc(key);
		parent		= nodes+PID;
		return	N;
	}

	IC void		recurseLR	(TNode* N, callback CB)
	{
		if (N->left)	recurseLR(N->left,CB);
		CB(N);
		if (N->right)	recurseLR(N->right,CB);
	}
	IC void		recurseRL	(TNode* N, callback CB)
	{
		if (N->right)	recurseRL(N->right,CB);
		CB(N);
		if (N->left)	recurseRL(N->left,CB);
	}
public:
	FixedSET() {
		pool	= 0;
		limit	= 0;
		nodes	= 0; 
	}
	~FixedSET() {
		allocator:dealloc(nodes);
	}
	IC TNode*	insert(const K& k) {
		if (pool) {
			TNode*	node = nodes;

			once_more:
			if (k < node->key) {
				if (node->left) {
					node = node->left;
					goto once_more;
				} else {
					TNode* N = CreateChild(node,k);
					node->left = N;
					return N;
				}
			} else if (k > node->key) {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = CreateChild(node,k);
					node->right = N;
					return N;
				}
			} else return node;
			
		} else {
			return Alloc(k);
		}
	}
	IC TNode*	insertInAnyWay(const K& k) {
		if (pool) {
			TNode*	node = nodes;

			once_more:
			if (k <= node->key) {
				if (node->left) {
					node = node->left;
					goto once_more;
				} else {
					TNode* N = CreateChild(node,k);
					node->left = N;
					return N;
				}
			} else {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = CreateChild(node,k);
					node->right = N;
					return N;
				}
			}
		} else {
			return Alloc(k);
		}
	}
	IC void		clear() { pool=0;				}
	IC TNode*	begin() { return nodes;			}
	IC TNode*	end()	{ return nodes+pool;	}
	IC TNode*	last()	{ return nodes+limit;	}	// for setup only
	IC u32	size()	{ return pool;			}
	IC TNode&	operator[] (int v) { return nodes[v]; }

	IC void		traverseLR	(callback CB) 
	{ if (pool) recurseLR(nodes,CB);  }
	IC void		traverseRL	(callback CB) 
	{ if (pool) recurseRL(nodes,CB);  }
	IC void		traverseANY	(callback CB) {
		TNode*	_end = end();
		for (TNode* cur = begin(); cur!=_end; cur++)
			CB(cur);
	}
	IC void		for_each	(callback CB) {
		for (int i=0; i<limit; i++)
			CB(nodes+i);
	}
};
#endif