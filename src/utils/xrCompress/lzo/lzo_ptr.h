/* lzo_ptr.h -- low-level pointer constructs

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


/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the library and is subject
   to change.
 */


#ifndef __LZO_PTR_H
#define __LZO_PTR_H

#ifdef __cplusplus
extern "C" {
#endif


/* This is the lowest part of the LZO library.
 * It deals with pointer representations at bit level.
 */


/***********************************************************************
// Includes
************************************************************************/

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#  include <dos.h>
#  if 1 && defined(__WATCOMC__)
#    include <i86.h>
     __LZO_EXTERN_C unsigned char _HShift;
#    define __LZO_HShift	_HShift
#  elif 1 && defined(_MSC_VER)
     __LZO_EXTERN_C unsigned short __near _AHSHIFT;
#    define __LZO_HShift	((unsigned) &_AHSHIFT)
#  elif defined(__LZO_WIN16)
#    define __LZO_HShift	3
#  else
#    define __LZO_HShift	12
#  endif
#  if !defined(_FP_SEG) && defined(FP_SEG)
#    define _FP_SEG			FP_SEG
#  endif
#  if !defined(_FP_OFF) && defined(FP_OFF)
#    define _FP_OFF			FP_OFF
#  endif
#endif


/***********************************************************************
// Integral types
************************************************************************/

/* ptrdiff_t */
#if !defined(lzo_ptrdiff_t)
#  if (UINT_MAX >= LZO_0xffffffffL)
     typedef ptrdiff_t        	lzo_ptrdiff_t;
#  else
     typedef long             	lzo_ptrdiff_t;
#  endif
#endif


/* Integral types that have *exactly* the same number of bits as a lzo_voidp */
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(lzo_ptr_t)
#    define __LZO_HAVE_PTR_T
#  endif
#endif

typedef uintptr_t	 	lzo_ptr_t;
typedef intptr_t		lzo_sptr_t;
#  define __LZO_HAVE_PTR_T


/***********************************************************************
//
************************************************************************/

/* Always use the integral version for pointer comparisions.
 *
 * Note that this only works if the representation and ordering
 * of the pointer and the integral is the same (at bit level).
 */

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#define PTR(a)				((lzo_bytep) (a))
/* only need the low bits of the pointer -> offset is ok */
#define PTR_ALIGNED_4(a)	((_FP_OFF(a) & 3) == 0)
#define PTR_ALIGNED2_4(a,b)	(((_FP_OFF(a) | _FP_OFF(b)) & 3) == 0)
#else
#define PTR(a)				((lzo_ptr_t) (a))
#define PTR_LINEAR(a)		PTR(a)
#define PTR_ALIGNED_4(a)	((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)	((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b)	(((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b)	(((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)
#endif

#define PTR_LT(a,b)			(PTR(a) < PTR(b))
#define PTR_GE(a,b)			(PTR(a) >= PTR(b))
#define PTR_DIFF(a,b)		((lzo_ptrdiff_t) (PTR(a) - PTR(b)))
#define pd(a,b)		        ((lzo_uint) ((a)-(b)))


LZO_EXTERN(lzo_ptr_t)
__lzo_ptr_linear(const lzo_voidp ptr);


typedef union
{
	char			a_char;
	unsigned char	a_uchar;
	short			a_short;
	unsigned short	a_ushort;
	int				a_int;
	unsigned int	a_uint;
	long			a_long;
	unsigned long	a_ulong;
	lzo_int			a_lzo_int;
	lzo_uint		a_lzo_uint;
	lzo_int32		a_lzo_int32;
	lzo_uint32		a_lzo_uint32;
	ptrdiff_t		a_ptrdiff_t;
	lzo_ptrdiff_t	a_lzo_ptrdiff_t;
	lzo_ptr_t		a_lzo_ptr_t;
	lzo_voidp		a_lzo_voidp;
    void *          a_void_p;
	lzo_bytep		a_lzo_bytep;
	lzo_bytepp		a_lzo_bytepp;
    lzo_uintp       a_lzo_uintp;
    lzo_uint *      a_lzo_uint_p;
    lzo_uint32p     a_lzo_uint32p;
    lzo_uint32 *    a_lzo_uint32_p;
    unsigned char * a_uchar_p;
    char *          a_char_p;
}
lzo_full_align_t;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

/*
vi:ts=4:et
*/

