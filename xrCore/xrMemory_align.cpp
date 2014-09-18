#include "stdafx.h"
#pragma hdrstop

#include <errno.h>
#include <malloc.h>

#include "xrMemory_align.h"

/***
*align.c - Aligned allocation, reallocation or freeing of memory in the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the _aligned_malloc(),
*                   _aligned_realloc(),
*                   _aligned_offset_malloc(),
*                   _aligned_offset_realloc() and
*                   _aligned_free() functions.
*
*******************************************************************************/

#define IS_2_POW_N(X)   (((X)&(X-1)) == 0)
#define PTR_SZ          sizeof(void *)

#ifdef __BORLANDC__
	typedef _W64 unsigned int  uintptr_t;
#endif
/***
*
* |1|___6___|2|3|4|_________5__________|_6_|
*
* 1 -> Pointer to start of the block allocated by malloc.
* 2 -> Value of 1.
* 3 -> Gap used to get 1 aligned on sizeof(void *).
* 4 -> Pointer to the start of data block.
* 4+5 -> Data block.
* 6 -> Wasted memory at rear of data block.
* 6 -> Wasted memory.
*
*******************************************************************************/

/***
* void * xr_aligned_malloc(size_t size, size_t alignment)
*       - Get a block of aligned memory from the heap.
*
* Purpose:
*       Allocate of block of aligned memory aligned on the alignment of at least
*       size bytes from the heap and return a pointer to it.
*
* Entry:
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*
* Exit:
*       Sucess: Pointer to memory block
*       Faliure: Null
*******************************************************************************/

void * __stdcall xr_aligned_malloc(
									size_t size,
									size_t alignment
									)
{
	return xr_aligned_offset_malloc(size, alignment, 0);
}

/***
* void *xr_aligned_offset_malloc(size_t size, size_t alignment, int offset)
*       - Get a block of memory from the heap.
*
* Purpose:
*       Allocate a block of memory which is shifted by offset from alignment of
*       at least size bytes from the heap and return a pointer to it.
*
* Entry:
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*
* Exit:
*       Sucess: Pointer to memory block
*       Faliure: Null
*
*******************************************************************************/


void * __stdcall xr_aligned_offset_malloc(
	size_t size,
	size_t align,
	size_t offset
	)
{
	uintptr_t ptr, retptr, gap;

	if (!IS_2_POW_N(align))
	{
		errno = EINVAL;
		return NULL;
	}
	if ( offset >= size && offset != 0)
		size	= offset+1;

	align = (align > PTR_SZ ? align : PTR_SZ) -1;

	/* gap = number of bytes needed to round up offset to align with PTR_SZ*/
	gap = (0 - offset)&(PTR_SZ -1);

	if ( (ptr =(uintptr_t)malloc(PTR_SZ +gap +align +size)) == (uintptr_t)NULL)
		return NULL;

	retptr =((ptr +PTR_SZ +gap +align +offset)&~align)- offset;
	((uintptr_t *)(retptr - gap))[-1] = ptr;

	return (void *)retptr;
}

/***
*
* void * xr_aligned_realloc(void * memblock, size_t size, size_t alignment)
*       - Reallocate a block of aligned memory from the heap.
*
* Purpose:
*       Reallocates of block of aligned memory aligned on the alignment of at
*       least size bytes from the heap and return a pointer to it. Size can be
*       either greater or less than the original size of the block.
*       The reallocation may result in moving the block as well as changing the
*       size.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*
* Exit:
*       Sucess: Pointer to re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/

void * __stdcall xr_aligned_realloc(
									 void *memblock,
									 size_t size,
									 size_t alignment
									 )
{
	return xr_aligned_offset_realloc(memblock, size, alignment, 0);
}


/***
*
* void *xr_aligned_offset_realloc (void * memblock, size_t size,
*                                     size_t alignment, int offset)
*       - Reallocate a block of memory from the heap.
*
* Purpose:
*       Reallocates a block of memory which is shifted by offset from
*       alignment of at least size bytes from the heap and return a pointer
*       to it. Size can be either greater or less than the original size of the
*       block.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*
* Exit:
*       Sucess: Pointer to the re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/

void * __stdcall xr_aligned_offset_realloc(
	void *memblock,
	size_t size,
	size_t align,
	size_t offset
	)
{
	uintptr_t ptr, retptr, gap, stptr, diff;
	uintptr_t movsz, reqsz;
	int bFree = 0;

	if (memblock == NULL)
	{
		return xr_aligned_offset_malloc(size, align, offset);
	}
	if ( size == 0)
	{
		xr_aligned_free(memblock);
		return NULL;
	}
	if ( offset >= size && offset != 0)
	{
		errno = EINVAL;
		return NULL;
	}

	stptr = (uintptr_t)memblock;

	/* ptr points to the pointer to starting of the memory block */
	stptr = (stptr & ~(PTR_SZ -1)) - PTR_SZ;

	/* ptr is the pointer to the start of memory block*/
	stptr = *((uintptr_t *)stptr);

	if (!IS_2_POW_N(align))
	{
		errno = EINVAL;
		return NULL;
	}

	align = (align > PTR_SZ ? align : PTR_SZ) -1;
	/* gap = number of bytes needed to round up offset to align with PTR_SZ*/
	gap = (0 -offset)&(PTR_SZ -1);

	diff = (uintptr_t)memblock - stptr;
	/* Mov size is min of the size of data available and sizw requested.
	*/
	movsz = _msize((void *)stptr) - ((uintptr_t)memblock - stptr);
	movsz = movsz > size? size: movsz;
	reqsz = PTR_SZ +gap +align +size;

	/* First check if we can expand(reducing or expanding using expand) data
	* safely, ie no data is lost. eg, reducing alignment and keeping size
	* same might result in loss of data at the tail of data block while
	* expanding.
	*
	* If no, use malloc to allocate the new data and move data.
	*
	* If yes, expand and then check if we need to move the data.
	*/
	if ((stptr +align +PTR_SZ +gap)<(uintptr_t)memblock)
	{
		if ((ptr = (uintptr_t)malloc(reqsz)) == (uintptr_t) NULL)
			return NULL;
		bFree = 1;
	}
	else
	{
		if ((ptr = (uintptr_t)_expand((void *)stptr, reqsz)) == (uintptr_t)NULL)
		{
			if ((ptr = (uintptr_t)malloc(reqsz)) == (uintptr_t) NULL)
				return NULL;
			bFree = 1;
		}
		else
			stptr = ptr;
	}


	if ( ptr == ((uintptr_t)memblock - diff)
		&& !( ((size_t)memblock + gap +offset) & ~(align) ))
	{
		return memblock;
	}

	retptr =((ptr +PTR_SZ +gap +align +offset)&~align)- offset;
	memmove((void *)retptr, (void *)(stptr + diff), movsz);
	if ( bFree)
		free ((void *)stptr);

	((uintptr_t *)(retptr - gap))[-1] = ptr;
	return (void *)retptr;
}


/***
*
* void *xr_aligned_free(void *memblock)
*       - Free the memory which was allocated using _aligned_malloc or
*       _aligned_offset_memory
*
* Purpose:
*       Frees the aligned memory block which was allocated using _aligned_malloc
*       or _aligned_memory.
*
* Entry:
*       void * memblock - pointer to the block of memory
*
*******************************************************************************/

void __stdcall xr_aligned_free(void *memblock)
{
	uintptr_t ptr;

	if (memblock == NULL)
		return;

	ptr = (uintptr_t)memblock;

	/* ptr points to the pointer to starting of the memory block */
	ptr = (ptr & ~(PTR_SZ -1)) - PTR_SZ;

	/* ptr is the pointer to the start of memory block*/
	ptr = *((uintptr_t *)ptr);
	free((void *)ptr);
}

u32 __stdcall xr_aligned_msize(void *memblock)
{
	uintptr_t ptr;

	if (memblock == NULL)
		return	0;

	ptr = (uintptr_t)memblock;

	/* ptr points to the pointer to starting of the memory block */
	ptr = (ptr & ~(PTR_SZ -1)) - PTR_SZ;

	/* ptr is the pointer to the start of memory block*/
	ptr = *((uintptr_t *)ptr);
	return	(u32)	_msize	((void *)ptr);
}
