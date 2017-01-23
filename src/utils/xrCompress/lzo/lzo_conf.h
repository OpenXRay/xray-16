/* lzo_conf.h -- main internal configuration file for the the LZO library

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


#ifndef __LZO_CONF_H
#define __LZO_CONF_H

#if !defined(__LZO_IN_MINILZO)
#  ifndef __LZOCONF_H
#    include "lzoconf.h"
#  endif
#endif


/***********************************************************************
// memory checkers
************************************************************************/

#if defined(__BOUNDS_CHECKING_ON)
#  include <unchecked.h>
#else
#  define BOUNDS_CHECKING_OFF_DURING(stmt)      stmt
#  define BOUNDS_CHECKING_OFF_IN_EXPR(expr)     (expr)
#endif


/***********************************************************************
// autoconf section
************************************************************************/

#if !defined(LZO_HAVE_CONFIG_H)
#  include <stddef.h>			/* ptrdiff_t, size_t */
#  include <string.h>			/* memcpy, memmove, memcmp, memset */
#  if !defined(NO_STDLIB_H)
#    include <stdlib.h>
#  endif
#  define HAVE_MEMCMP
#  define HAVE_MEMCPY
#  define HAVE_MEMMOVE
#  define HAVE_MEMSET
#else
#  include <sys/types.h>
#  if defined(HAVE_STDDEF_H)
#    include <stddef.h>
#  endif
#  if defined(STDC_HEADERS)
#    include <string.h>
#    include <stdlib.h>
#  endif
#endif

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#  define HAVE_MALLOC_H
#  define HAVE_HALLOC
#endif


#undef NDEBUG
#if !defined(LZO_DEBUG)
#  define NDEBUG
#endif
#if defined(LZO_DEBUG) || !defined(NDEBUG)
#  if !defined(NO_STDIO_H)
#    include <stdio.h>
#  endif
#endif
#include <assert.h>


#if !defined(LZO_COMPILE_TIME_ASSERT)
#  define LZO_COMPILE_TIME_ASSERT(expr) \
        { typedef int __lzo_compile_time_assert_fail[1 - 2 * !(expr)]; }
#endif


#if !defined(LZO_UNUSED)
#  if 1
#    define LZO_UNUSED(var)     ((void)&var)
#  elif 0
#    define LZO_UNUSED(var)	    { typedef int __lzo_unused[sizeof(var) ? 2 : 1]; }
#  else
#    define LZO_UNUSED(parm)	(parm = parm)
#  endif
#endif


#if !defined(__inline__) && !defined(__GNUC__)
#  if defined(__cplusplus)
#    define __inline__		inline
#  else
#    define __inline__		/* nothing */
#  endif
#endif


#if defined(NO_MEMCMP)
#  undef HAVE_MEMCMP
#endif

#if !defined(HAVE_MEMCMP)
#  undef memcmp
#  define memcmp	lzo_memcmp
#endif
#if !defined(HAVE_MEMCPY)
#  undef memcpy
#  define memcpy	lzo_memcpy
#endif
#if !defined(HAVE_MEMMOVE)
#  undef memmove
#  define memmove	lzo_memmove
#endif
#if !defined(HAVE_MEMSET)
#  undef memset
#  define memset	lzo_memset
#endif


/***********************************************************************
//
************************************************************************/

#if 0
#  define LZO_BYTE(x)		((unsigned char) (x))
#else
#  define LZO_BYTE(x)		((unsigned char) ((x) & 0xff))
#endif

#define LZO_MAX(a,b)		((a) >= (b) ? (a) : (b))
#define LZO_MIN(a,b)		((a) <= (b) ? (a) : (b))
#define LZO_MAX3(a,b,c)		((a) >= (b) ? LZO_MAX(a,c) : LZO_MAX(b,c))
#define LZO_MIN3(a,b,c)		((a) <= (b) ? LZO_MIN(a,c) : LZO_MIN(b,c))

#define lzo_sizeof(type)	((lzo_uint) (sizeof(type)))

#define LZO_HIGH(array)		((lzo_uint) (sizeof(array)/sizeof(*(array))))

/* this always fits into 16 bits */
#define LZO_SIZE(bits)		(1u << (bits))
#define LZO_MASK(bits)		(LZO_SIZE(bits) - 1)

#define LZO_LSIZE(bits)		(1ul << (bits))
#define LZO_LMASK(bits)		(LZO_LSIZE(bits) - 1)

#define LZO_USIZE(bits)		((lzo_uint) 1 << (bits))
#define LZO_UMASK(bits)		(LZO_USIZE(bits) - 1)

/* Maximum value of a signed/unsigned type.
   Do not use casts, avoid overflows ! */
#define LZO_STYPE_MAX(b)	(((1l  << (8*(b)-2)) - 1l)  + (1l  << (8*(b)-2)))
#define LZO_UTYPE_MAX(b)	(((1ul << (8*(b)-1)) - 1ul) + (1ul << (8*(b)-1)))


/***********************************************************************
//
************************************************************************/

#if !defined(SIZEOF_UNSIGNED)
#  if (UINT_MAX == 0xffff)
#    define SIZEOF_UNSIGNED			2
#  elif (UINT_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED			4
#  elif (UINT_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED			8
#  else
#    error "SIZEOF_UNSIGNED"
#  endif
#endif

#if !defined(SIZEOF_UNSIGNED_LONG)
#  if (ULONG_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG	4
#  elif (ULONG_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG	8
#  else
#    error "SIZEOF_UNSIGNED_LONG"
#  endif
#endif


#if !defined(SIZEOF_SIZE_T)
#  define SIZEOF_SIZE_T				SIZEOF_UNSIGNED
#endif
#if !defined(SIZE_T_MAX)
#  define SIZE_T_MAX				LZO_UTYPE_MAX(SIZEOF_SIZE_T)
#endif


/***********************************************************************
// compiler and architecture specific stuff
************************************************************************/

/* Some defines that indicate if memory can be accessed at unaligned
 * memory addresses. You should also test that this is actually faster
 * even if it is allowed by your system.
 */

#if 1 && defined(__LZO_i386) && (UINT_MAX == LZO_0xffffffffL)
#  if !defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX == 0xffff)
#    define LZO_UNALIGNED_OK_2
#  endif
#  if !defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX == LZO_0xffffffffL)
#    define LZO_UNALIGNED_OK_4
#  endif
#endif

#if defined(LZO_UNALIGNED_OK_2) || defined(LZO_UNALIGNED_OK_4)
#  if !defined(LZO_UNALIGNED_OK)
#    define LZO_UNALIGNED_OK
#  endif
#endif

#if defined(__LZO_NO_UNALIGNED)
#  undef LZO_UNALIGNED_OK
#  undef LZO_UNALIGNED_OK_2
#  undef LZO_UNALIGNED_OK_4
#endif

#if defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX != 0xffff)
#  error "LZO_UNALIGNED_OK_2 must not be defined on this system"
#endif
#if defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_UNALIGNED_OK_4 must not be defined on this system"
#endif


/* Many modern processors can transfer 32bit words much faster than
 * bytes - this can significantly speed decompression.
 */

#if defined(__LZO_NO_ALIGNED)
#  undef LZO_ALIGNED_OK_4
#endif

#if defined(LZO_ALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_ALIGNED_OK_4 must not be defined on this system"
#endif


/* Definitions for byte order, according to significance of bytes, from low
 * addresses to high addresses. The value is what you get by putting '4'
 * in the most significant byte, '3' in the second most significant byte,
 * '2' in the second least significant byte, and '1' in the least
 * significant byte.
 * The byte order is only needed if we use LZO_UNALIGNED_OK.
 */

#define	LZO_LITTLE_ENDIAN		1234
#define	LZO_BIG_ENDIAN			4321
#define	LZO_PDP_ENDIAN			3412

#if !defined(LZO_BYTE_ORDER)
#  if defined(MFX_BYTE_ORDER)
#    define LZO_BYTE_ORDER		MFX_BYTE_ORDER
#  elif defined(__LZO_i386)
#    define LZO_BYTE_ORDER		LZO_LITTLE_ENDIAN
#  elif defined(BYTE_ORDER)
#    define LZO_BYTE_ORDER		BYTE_ORDER
#  elif defined(__BYTE_ORDER)
#    define LZO_BYTE_ORDER		__BYTE_ORDER
#  endif
#endif

#if defined(LZO_BYTE_ORDER)
#  if (LZO_BYTE_ORDER != LZO_LITTLE_ENDIAN) && \
      (LZO_BYTE_ORDER != LZO_BIG_ENDIAN)
#    error "invalid LZO_BYTE_ORDER"
#  endif
#endif

#if defined(LZO_UNALIGNED_OK) && !defined(LZO_BYTE_ORDER)
#  error "LZO_BYTE_ORDER is not defined"
#endif


/***********************************************************************
// optimization
************************************************************************/

/* gcc 2.6.3 and gcc 2.7.2 have a bug with 'register xxx __asm__("%yyy")' */
#define LZO_OPTIMIZE_GNUC_i386_IS_BUGGY

/* Help the gcc optimizer with register allocation. */
#if defined(NDEBUG) && !defined(LZO_DEBUG) && !defined(__LZO_CHECKER)
#  if defined(__GNUC__) && defined(__i386__)
#    if !defined(LZO_OPTIMIZE_GNUC_i386_IS_BUGGY)
#      define LZO_OPTIMIZE_GNUC_i386
#    endif
#  endif
#endif


/***********************************************************************
// some globals
************************************************************************/

__LZO_EXTERN_C int __lzo_init_done;
__LZO_EXTERN_C const lzo_byte __lzo_copyright[];
LZO_EXTERN(const lzo_byte *) lzo_copyright(void);
__LZO_EXTERN_C const lzo_uint32 _lzo_crc32_table[256];


/***********************************************************************
// ANSI C preprocessor macros
************************************************************************/

#define _LZO_STRINGIZE(x)			#x
#define _LZO_MEXPAND(x)				_LZO_STRINGIZE(x)

/* concatenate */
#define _LZO_CONCAT2(a,b)			a ## b
#define _LZO_CONCAT3(a,b,c)			a ## b ## c
#define _LZO_CONCAT4(a,b,c,d)		a ## b ## c ## d
#define _LZO_CONCAT5(a,b,c,d,e)		a ## b ## c ## d ## e

/* expand and concatenate (by using one level of indirection) */
#define _LZO_ECONCAT2(a,b)			_LZO_CONCAT2(a,b)
#define _LZO_ECONCAT3(a,b,c)		_LZO_CONCAT3(a,b,c)
#define _LZO_ECONCAT4(a,b,c,d)		_LZO_CONCAT4(a,b,c,d)
#define _LZO_ECONCAT5(a,b,c,d,e)	_LZO_CONCAT5(a,b,c,d,e)


/***********************************************************************
// Query-interface to the algorithms
************************************************************************/

#if 0

#define __LZO_IS_COMPRESS_QUERY(i,il,o,ol,w)	((lzo_voidp)(o) == (w))
#define __LZO_QUERY_COMPRESS(i,il,o,ol,w,n,s) \
				(*ol = (n)*(s), LZO_E_OK)

#define __LZO_IS_DECOMPRESS_QUERY(i,il,o,ol,w)	((lzo_voidp)(o) == (w))
#define __LZO_QUERY_DECOMPRESS(i,il,o,ol,w,n,s) \
				(*ol = (n)*(s), LZO_E_OK)

#define __LZO_IS_OPTIMIZE_QUERY(i,il,o,ol,w)	((lzo_voidp)(o) == (w))
#define __LZO_QUERY_OPTIMIZE(i,il,o,ol,w,n,s) \
				(*ol = (n)*(s), LZO_E_OK)

#endif


/***********************************************************************
//
************************************************************************/

#include "lzo_ptr.h"


/* Generate compressed data in a deterministic way.
 * This is fully portable, and compression can be faster as well.
 * A reason NOT to be deterministic is when the block size is
 * very small (e.g. 8kB) or the dictionary is big, because
 * then the initialization of the dictionary becomes a relevant
 * magnitude for compression speed.
 */
#define LZO_DETERMINISTIC


#define LZO_DICT_USE_PTR
#if defined(__LZO_DOS16) || defined(__LZO_WIN16) || defined(__LZO_STRICT_16BIT)
#  undef LZO_DICT_USE_PTR
#endif

#if defined(LZO_DICT_USE_PTR)
#  define lzo_dict_t	const lzo_bytep
#  define lzo_dict_p	lzo_dict_t __LZO_MMODEL *
#else
#  define lzo_dict_t	lzo_uint
#  define lzo_dict_p	lzo_dict_t __LZO_MMODEL *
#endif

#if !defined(lzo_moff_t)
/* must be unsigned */
#define lzo_moff_t		lzo_uint
#endif


#endif /* already included */

/*
vi:ts=4:et
*/

