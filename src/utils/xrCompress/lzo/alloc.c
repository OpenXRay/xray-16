/* alloc.c -- memory allocation

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
 */


#include "lzo_conf.h"
#include "lzoutil.h"

#if defined(HAVE_MALLOC_H)
#  include <malloc.h>
#endif
#if defined(__palmos__)
#  include <System/MemoryMgr.h>
#endif


#undef lzo_alloc_hook
#undef lzo_free_hook
#undef lzo_alloc
#undef lzo_malloc
#undef lzo_free


/***********************************************************************
// implementation
************************************************************************/

LZO_PRIVATE(lzo_voidp)
lzo_alloc_internal(lzo_uint nelems, lzo_uint size)
{
	lzo_voidp p = NULL;
	unsigned long s = (unsigned long) nelems * size;

	if (nelems <= 0 || size <= 0 || s < nelems || s < size)
		return NULL;

#if defined(__palmos__)
	p = (lzo_voidp) MemPtrNew(s);
#elif (LZO_UINT_MAX <= SIZE_T_MAX)
	if (s < SIZE_T_MAX)
		p = (lzo_voidp) malloc((size_t)s);
#elif defined(HAVE_HALLOC) && defined(__DMC__)
	if (size < SIZE_T_MAX)
		p = (lzo_voidp) _halloc(nelems,(size_t)size);
#elif defined(HAVE_HALLOC)
	if (size < SIZE_T_MAX)
		p = (lzo_voidp) halloc(nelems,(size_t)size);
#else
	if (s < SIZE_T_MAX)
		p = (lzo_voidp) malloc((size_t)s);
#endif

	return p;
}


LZO_PRIVATE(void)
lzo_free_internal(lzo_voidp p)
{
	if (!p)
		return;

#if defined(__palmos__)
	MemPtrFree(p);
#elif (LZO_UINT_MAX <= SIZE_T_MAX)
	free(p);
#elif defined(HAVE_HALLOC) && defined(__DMC__)
	_hfree(p);
#elif defined(HAVE_HALLOC)
	hfree(p);
#else
	free(p);
#endif
}


/***********************************************************************
// public interface using the global hooks
************************************************************************/

/* global allocator hooks */
LZO_PUBLIC_VAR(lzo_alloc_hook_t) lzo_alloc_hook = lzo_alloc_internal;
LZO_PUBLIC_VAR(lzo_free_hook_t) lzo_free_hook = lzo_free_internal;


LZO_PUBLIC(lzo_voidp)
lzo_alloc(lzo_uint nelems, lzo_uint size)
{
	if (!lzo_alloc_hook)
		return NULL;

	return lzo_alloc_hook(nelems,size);
}


LZO_PUBLIC(lzo_voidp)
lzo_malloc(lzo_uint size)
{
	if (!lzo_alloc_hook)
		return NULL;

#if defined(__palmos__)
	return lzo_alloc_hook(size,1);
#elif (LZO_UINT_MAX <= SIZE_T_MAX)
	return lzo_alloc_hook(size,1);
#elif defined(HAVE_HALLOC)
	/* use segment granularity by default */
    if (size + 15 > size)   /* avoid overflow */
		return lzo_alloc_hook((size+15)/16,16);
	return lzo_alloc_hook(size,1);
#else
	return lzo_alloc_hook(size,1);
#endif
}


LZO_PUBLIC(void)
lzo_free(lzo_voidp p)
{
	if (!lzo_free_hook)
		return;

	lzo_free_hook(p);
}


/*
vi:ts=4:et
*/
