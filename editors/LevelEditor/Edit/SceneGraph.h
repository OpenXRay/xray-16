#pragma once

template<class K, class T>
class FixedMAP
{
public:
	struct TNode {
		K		key;
		T		val;
		TNode	*left,*right;

		TNode() { right = left = 0; }
	};
	typedef void __fastcall callback(TNode*);
private:
	TNode*		nodes;
	DWORD		pool;
	DWORD		limit;

	IC TNode*	Alloc(const K& key, const T& val)
	{
		VERIFY(nodes);
		TNode *node = nodes + pool;
		node->key	= key;
		node->val	= val;
		node->right = node->left = 0;
		pool++;
		VERIFY(pool<limit);
		return node;
	}
	IC TNode*	Alloc(const K& key)
	{
		VERIFY(nodes);
		TNode *node = nodes + pool;
		node->key	= key;
		node->right = node->left = 0;
		pool++;
		VERIFY(pool<limit);
		return node;
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
	FixedMAP() {
		nodes	= 0;
		pool	= 0;
		limit	= 0;
	}
	~FixedMAP() {
		delete [] nodes;
	}
	IC void		init	(int maxnodes)
	{
		limit	= maxnodes;
		nodes	= new TNode[limit];
	}
	IC TNode*	insert(const K& k, const T& v)
	{
		if (pool) {
			TNode*	node = nodes;

			once_more:
			if (k < node->key) {
				if (node->left) {
					node = node->left;
					goto once_more;
				} else {
					TNode* N = Alloc(k,v);
					node->left = N;
					return N;
				}
			} else if (k > node->key) {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = Alloc(k,v);
					node->right = N;
					return N;
				}
			} else return node;

		} else {
			return Alloc(k,v);
		}
	}
	IC TNode*	insertInAnyWay(const K& k, const T& v)
	{
		if (pool) {
			TNode*	node = nodes;

			once_more:
			if (k <= node->key) {
				VERIFY(node->val != v);
				if (node->left) {
					node = node->left;
					goto once_more;
				} else {
					TNode* N = Alloc(k,v);
					node->left = N;
					return N;
				}
			} else {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = Alloc(k,v);
					node->right = N;
					return N;
				}
			}
		} else {
			return Alloc(k,v);
		}
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
					TNode* N = Alloc(k);
					node->left = N;
					return N;
				}
			} else if (k > node->key) {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = Alloc(k);
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
					TNode* N = Alloc(k);
					node->left = N;
					return N;
				}
			} else {
				if (node->right) {
					node = node->right;
					goto once_more;
				} else {
					TNode* N = Alloc(k);
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
	IC TNode*	size()	{ return pool;			}
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
	IC void		setup(callback CB) {
		for (int i=0; i<limit; i++)
			CB(nodes+i);
	}
};
