/* lzo1x_d3.c -- LZO1X decompression with preset dictionary

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


#include "config1x.h"

#if 0
#undef NDEBUG
#include <assert.h>
#endif

#define LZO_TEST_DECOMPRESS_OVERRUN

#if 0 && defined(__linux__)
#  include <linux/string.h>
#endif


#define SLOW_MEMCPY(a,b,l)		{ do *a++ = *b++; while (--l > 0); }
#if 1 && defined(HAVE_MEMCPY)
#  if !defined(__LZO_DOS16) && !defined(__LZO_WIN16)
#    define FAST_MEMCPY(a,b,l)	{ memcpy(a,b,l); a += l; }
#  endif
#endif

#if 1 && defined(FAST_MEMCPY)
#  define DICT_MEMMOVE(op,m_pos,m_len,m_off) \
		if (m_off >= (m_len)) \
			FAST_MEMCPY(op,m_pos,m_len) \
		else \
			SLOW_MEMCPY(op,m_pos,m_len)
#else
#  define DICT_MEMMOVE(op,m_pos,m_len,m_off) \
		SLOW_MEMCPY(op,m_pos,m_len)
#endif

#if !defined(FAST_MEMCPY)
#  define FAST_MEMCPY	SLOW_MEMCPY
#endif


#define COPY_DICT_DICT(m_len,m_off) \
	{ \
		register const lzo_byte *m_pos; \
		m_off -= (lzo_moff_t) (op - out); assert(m_off > 0); \
		if (m_off > dict_len) goto lookbehind_overrun; \
		m_pos = dict_end - m_off; \
		if (m_len > m_off) \
		{ \
			m_len -= m_off; \
			FAST_MEMCPY(op,m_pos,m_off) \
			m_pos = out; \
			SLOW_MEMCPY(op,m_pos,m_len) \
		} \
		else \
			FAST_MEMCPY(op,m_pos,m_len) \
	}

#define COPY_DICT(m_len,m_off) \
	assert(m_len >= 2); assert(m_off > 0); assert(op > out); \
	if (m_off <= (lzo_moff_t) (op - out)) \
	{ \
		register const lzo_byte *m_pos = op - m_off; \
		DICT_MEMMOVE(op,m_pos,m_len,m_off) \
	} \
	else \
		COPY_DICT_DICT(m_len,m_off)




LZO_PUBLIC(int)
lzo1x_decompress_dict_safe ( const lzo_byte *in,  lzo_uint  in_len,
                                   lzo_byte *out, lzo_uintp out_len,
                                   lzo_voidp wrkmem /* NOT USED */,
                             const lzo_byte *dict, lzo_uint dict_len)


#include "lzo1x_d.ch"


/*
vi:ts=4:et
*/

