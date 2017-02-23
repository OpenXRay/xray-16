/* lzo_ptr.c -- low-level pointer constructs

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


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(lzo_ptr_t)
__lzo_ptr_linear(const lzo_voidp ptr)
{
	lzo_ptr_t p;

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
    p = (((lzo_ptr_t)(_FP_SEG(ptr))) << (16 - __LZO_HShift)) + (_FP_OFF(ptr));
#else
    p = PTR_LINEAR(ptr);
#endif

	return p;
}


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(unsigned)
__lzo_align_gap(const lzo_voidp ptr, lzo_uint size)
{
	lzo_ptr_t p, s, n;

	assert(size > 0);

    p = __lzo_ptr_linear(ptr);
	s = (lzo_ptr_t) (size - 1);
#if 0
	assert((size & (size - 1)) == 0);
    n = ((p + s) & ~s) - p;
#else
	n = (((p + s) / size) * size) - p;
#endif

	assert((long)n >= 0);
	assert(n <= s);

	return (unsigned)n;
}



/*
vi:ts=4:et
*/
