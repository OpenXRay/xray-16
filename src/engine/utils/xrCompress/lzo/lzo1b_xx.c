/* lzo1b_xx.c -- LZO1B compression public entry point

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


#include "config1b.h"


/***********************************************************************
//
************************************************************************/

static const lzo_compress_t * const c_funcs [9] =
{
	&_lzo1b_1_compress_func,
	&_lzo1b_2_compress_func,
	&_lzo1b_3_compress_func,
	&_lzo1b_4_compress_func,
	&_lzo1b_5_compress_func,
	&_lzo1b_6_compress_func,
	&_lzo1b_7_compress_func,
	&_lzo1b_8_compress_func,
	&_lzo1b_9_compress_func
};


lzo_compress_t _lzo1b_get_compress_func(int clevel)
{
	const lzo_compress_t *f;

	if (clevel < LZO1B_BEST_SPEED || clevel > LZO1B_BEST_COMPRESSION)
	{
		if (clevel == LZO1B_DEFAULT_COMPRESSION)
			clevel = LZO1B_BEST_SPEED;
		else
			return 0;
	}
	f = c_funcs[clevel-1];
	assert(f && *f);
	return *f;
}


LZO_PUBLIC(int)
lzo1b_compress ( const lzo_byte *src, lzo_uint  src_len,
                       lzo_byte *dst, lzo_uintp dst_len,
                       lzo_voidp wrkmem,
                       int clevel )
{
	lzo_compress_t f;

	f = _lzo1b_get_compress_func(clevel);
	if (!f)
		return LZO_E_ERROR;
	return _lzo1b_do_compress(src,src_len,dst,dst_len,wrkmem,f);
}



/*
vi:ts=4:et
*/

