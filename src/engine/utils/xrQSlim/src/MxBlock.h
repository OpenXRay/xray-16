#ifndef MXBLOCK_INCLUDED // -*- C++ -*-
#define MXBLOCK_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  MxBlock provides typed access a contiguous block of elements.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxBlock.h,v 1.18.2.1 2004/07/01 18:38:41 garland Exp $

 ************************************************************************/

// This trivial allocator is a trick we use to resize the array
// without destructing and reconstructing all the elements.
//
enum _array_alloc_policy { ARRAY_ALLOC_INPLACE };
inline void *operator new(size_t, void *p, _array_alloc_policy) { return p; }

#if _MSC_VER >= 1200
//
// This matching delete operator is necessary to avoid warnings in
// VC++ 6.0.  For some reason, it seems to periodically cause internal
// compiler errors in some GCC 2.95.x compilers.
//
inline void operator delete(void *mem, void *spot, _array_alloc_policy) { }
#endif

template<class T>
class MxBlock
{
private:
    int N;
    T *block;

protected:
    MxBlock() { }
    void init_block(int n)
	{
	    // Allocate memory for block
	    N=n; block = xr_alloc<T>(n);

	    // Initialize each entry
	    for(int i=0;i<n;i++) new((void *)&block[i],ARRAY_ALLOC_INPLACE) T;
	}
    void resize_block(int n)
	{
	    T *old = block;

	    // Allocate new block
	    block = (T *)xr_realloc(old, sizeof(T)*n);

	    // Initialize all the newly allocated entries
	    for(int i=N;i<n;i++) new((void *)&block[i],ARRAY_ALLOC_INPLACE) T;

	    N = n;
	}
    void free_block()
	{
/*
//.
#if defined(_MSC_VER)
	    // The Microsoft compiler has a problem with the 
	    // standard syntax below.  For some reason,
	    // expanding it into the following pointer-based
	    // version makes it work.  Don't ask me why.
	    //
		for(int i=0; i<N; i++)  { T *p = &block[i]; p->~T(); }
	    xr_free(block);
#else
*/
	    // Call the relevant destructors for each element before
	    // freeing the block.  Has now effect for types like 'int'.
	    //
	    for(int i=0; i<N; i++)  block[i].~T();
	    xr_free(block);
//#endif
	}

public:
    MxBlock(int n) { init_block(n); }
    ~MxBlock() { free_block(); }

    operator const T*() const { return block; }
    operator       T*()       { return block; }
    int length() const { return N; }

    // These parenthesized accessors are included for backwards
    // compatibility.  Their continued use is discouraged.
    //
    T&       operator()(int i)       { return (*this)[i]; }
    const T& operator()(int i) const { return (*this)[i]; }

    // Primitive methods for altering the data block
    //
    void resize(int n) { resize_block(n); }
    void bitcopy(const T *a, int n) // copy bits directly
	{ CopyMemory(block, a, _min(n,N)*sizeof(T)); }
    void copy(const T *a, const int n) // copy using assignment operator
	{ for(int i=0; i<_min(n,N); i++) block[i] = a[i]; }
    void bitcopy(const MxBlock<T>& b) { bitcopy(b, b.length()); }
    void copy(const MxBlock<T>& b) { copy(b, b.length()); }

    // Restricted STL-like interface for interoperability with
    // STL-based code.
    //
    typedef T value_type;
    typedef value_type *iterator;
    typedef value_type *const_iterator;

    int size() const { return length(); }

    iterator       begin()       { return block; }
    const_iterator begin() const { return block; }
    iterator       end()       { return begin()+size(); }
    const_iterator end() const { return begin()+size(); }
};

// MXBLOCK_INCLUDED
#endif
