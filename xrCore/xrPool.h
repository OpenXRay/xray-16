#ifndef xrPoolH
#define xrPoolH
//#pragma once

template <class T, int granularity>
class	poolSS
{
private:
	T*					list;
	xr_vector<T*>		blocks;
private:
	T**					access			(T* P)	{ return (T**) LPVOID(P);	}
	void				block_create	()
	{
		// Allocate
		VERIFY				(0==list);
		list				= xr_alloc<T>	(granularity);
		blocks.push_back	(list);

		// Partition
		for (int it=0; it<(granularity-1); it++)
		{
			T*		E			= list+it;
			*access(E)			= E+1;
		}
		*access(list+granularity-1)	= NULL;
	}
public:
	poolSS()
	{
		list				= 0;
	}
	~poolSS()
	{
		for (u32 b=0; b<blocks.size(); b++)
			xr_free	(blocks[b]);
	}
	T*					create			()
	{
		if (0==list)	block_create();

		T* E			= list;
		list			= *access(list);
		return			new (E) T();
	}
	void				destroy			(T* &P)
	{
		P->~T			();
		*access(P)		= list;
		list			= P;
		P				= NULL;
	}
    void				clear			()
    {
    	list			= 0;
		for (u32 b=0; b<blocks.size(); b++)
			xr_free	(blocks[b]);
        blocks.clear	();
    }
};
#endif
