/* lzo1_99.c -- implementation of the LZO1-99 algorithm

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



#include "lzoconf.h"
#if !defined(LZO_99_UNSUPPORTED)

#define COMPRESS_ID		99

#define DDBITS			3
#define CLEVEL			9


/***********************************************************************
//
************************************************************************/

#define LZO_NEED_DICT_H
#include "config1.h"


/***********************************************************************
// compression internal entry point.
************************************************************************/

static int
_lzo1_do_compress ( const lzo_byte *in,  lzo_uint  in_len,
                          lzo_byte *out, lzo_uintp out_len,
                          lzo_voidp wrkmem,
                          lzo_compress_t func )
{
	int r;

	/* don't try to compress a block that's too short */
	if (in_len <= 0)
	{
		*out_len = 0;
		r = LZO_E_OK;
	}
	else if (in_len <= MIN_LOOKAHEAD + 1)
	{
#if defined(LZO_RETURN_IF_NOT_COMPRESSIBLE)
		*out_len = 0;
		r = LZO_E_NOT_COMPRESSIBLE;
#else
		*out_len = STORE_RUN(out,in,in_len) - out;
		r = (*out_len > in_len) ? LZO_E_OK : LZO_E_ERROR;
#endif
	}
	else
		r = func(in,in_len,out,out_len,wrkmem);

	return r;
}


/***********************************************************************
//
************************************************************************/

#if !defined(COMPRESS_ID)
#define COMPRESS_ID		_LZO_ECONCAT2(DD_BITS,CLEVEL)
#endif


#define LZO_CODE_MATCH_INCLUDE_FILE		"lzo1_cm.ch"
#include "lzo1b_c.ch"


/***********************************************************************
//
************************************************************************/

#define LZO_COMPRESS \
	_LZO_ECONCAT3(lzo1_,COMPRESS_ID,_compress)

#define LZO_COMPRESS_FUNC \
	_LZO_ECONCAT3(_lzo1_,COMPRESS_ID,_compress_func)


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
LZO_COMPRESS ( const lzo_byte *in,  lzo_uint  in_len,
                     lzo_byte *out, lzo_uintp out_len,
                     lzo_voidp wrkmem )
{
#if defined(__LZO_QUERY_COMPRESS)
	if (__LZO_IS_COMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
		return __LZO_QUERY_COMPRESS(in,in_len,out,out_len,wrkmem,D_SIZE,lzo_sizeof(lzo_dict_t));
#endif

	return _lzo1_do_compress(in,in_len,out,out_len,wrkmem,do_compress);
}

#endif

/*
vi:ts=4:et
*/
