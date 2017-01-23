/* lzo2a_9x.c -- implementation of the LZO2A-999 compression algorithm

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
#if !defined(LZO_999_UNSUPPORTED)

#include <stdio.h>
#include "config2a.h"

#if 0
#undef NDEBUG
#include <assert.h>
#endif


/***********************************************************************
//
************************************************************************/

#define THRESHOLD	    1			/* lower limit for match length */
#define F		     2048			/* upper limit for match length */


#define LZO2A
#define LZO_COMPRESS_T	lzo2a_999_t
#define lzo_swd_t		lzo2a_999_swd_t
#include "lzo_mchw.ch"



/***********************************************************************
//
************************************************************************/

#define putbyte(x)		*op++ = LZO_BYTE(x)

#define putbits(j,x) \
	if (k == 0) bitp = op++; \
	SETBITS(j,x); \
	if (k >= 8) { *bitp = LZO_BYTE(MASKBITS(8)); DUMPBITS(8); \
					if (k > 0) bitp = op++; }

#define putbit(x)		putbits(1,x)


/***********************************************************************
// this is a public function, but there is no prototype in a header file
************************************************************************/

LZO_EXTERN(int)
lzo2a_999_compress_callback ( const lzo_byte *in , lzo_uint  in_len,
                                    lzo_byte *out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                                    lzo_progress_callback_t cb,
                                    lzo_uint max_chain );

LZO_PUBLIC(int)
lzo2a_999_compress_callback ( const lzo_byte *in , lzo_uint  in_len,
                                    lzo_byte *out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                                    lzo_progress_callback_t cb,
                                    lzo_uint max_chain )
{
	lzo_byte *op;
	lzo_byte *bitp = 0;
	lzo_uint m_len, m_off;
	LZO_COMPRESS_T cc;
	LZO_COMPRESS_T * const c = &cc;
	lzo_swd_t * const swd = (lzo_swd_t *) wrkmem;
	int r;

	lzo_uint32 b = 0;		/* bit buffer */
	unsigned k = 0;			/* bits in bit buffer */

#if defined(__LZO_QUERY_COMPRESS)
	if (__LZO_IS_COMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
		return __LZO_QUERY_COMPRESS(in,in_len,out,out_len,wrkmem,1,lzo_sizeof(lzo_swd_t));
#endif

	/* sanity check */
	if (!lzo_assert(LZO2A_999_MEM_COMPRESS >= lzo_sizeof(lzo_swd_t)))
		return LZO_E_ERROR;

	c->init = 0;
	c->ip = c->in = in;
	c->in_end = in + in_len;
	c->cb = cb;
	c->m1 = c->m2 = c->m3 = c->m4 = 0;

	op = out;

	r = init_match(c,swd,NULL,0,0);
	if (r != 0)
		return r;
	if (max_chain > 0)
		swd->max_chain = max_chain;

	r = find_match(c,swd,0,0);
	if (r != 0)
		return r;
	while (c->look > 0)
	{
		int lazy_match_min_gain = 0;
		int extra1 = 0;
		int extra2 = 0;
		lzo_uint ahead = 0;

		LZO_UNUSED(extra1);

		m_len = c->m_len;
		m_off = c->m_off;

#if (N >= 8192)
		if (m_off >= 8192)
		{
			if (m_len < M3_MIN_LEN)
				m_len = 0;
			else
				lazy_match_min_gain = 1;
		}
		else
#endif
		if (m_len >= M1_MIN_LEN && m_len <= M1_MAX_LEN && m_off <= 256)
		{
			lazy_match_min_gain = 2;
			extra1 = 3;
			extra2 = 2;
		}
		else if (m_len >= 10)
			lazy_match_min_gain = 1;
		else if (m_len >= 3)
		{
			lazy_match_min_gain = 1;
			extra1 = 1;
		}
		else
			m_len = 0;


		/* try a lazy match */
		if (lazy_match_min_gain > 0 && c->look > m_len)
		{
			int lit = swd->b_char;

			r = find_match(c,swd,1,0);
			assert(r == 0);
			assert(c->look > 0);

#if (N >= 8192)
			if (m_off < 8192 && c->m_off >= 8192)
				lazy_match_min_gain += extra1;
			else
#endif
			if (m_len >= M1_MIN_LEN && m_len <= M1_MAX_LEN && m_off <= 256)
			{
				if (!(c->m_len >= M1_MIN_LEN &&
				      c->m_len <= M1_MAX_LEN && c->m_off <= 256))
					lazy_match_min_gain += extra2;
			}
			if (c->m_len >= M1_MIN_LEN &&
			    c->m_len <= M1_MAX_LEN && c->m_off <= 256)
			{
					lazy_match_min_gain -= 1;
			}

			if (lazy_match_min_gain < 1)
				lazy_match_min_gain = 1;

			if (c->m_len >= m_len + lazy_match_min_gain)
			{
				c->lazy++;
#if !defined(NDEBUG)
				m_len = c->m_len;
				m_off = c->m_off;
				assert(lzo_memcmp(c->ip - c->look, c->ip - c->look - m_off,
			                      m_len) == 0);
				assert(m_len >= 3 || (m_len >= 2 && m_off <= 256));
#endif
				/* code literal */
				putbit(0);
				putbyte(lit);
				c->lit_bytes++;
				continue;
			}
			else
				ahead = 1;
			assert(m_len > 0);
		}


		if (m_len == 0)
		{
			/* a literal */
			putbit(0);
			putbyte(swd->b_char);
			c->lit_bytes++;
			r = find_match(c,swd,1,0);
			assert(r == 0);
		}
		else
		{
			assert(m_len >= M1_MIN_LEN);
			assert(m_off > 0);
			assert(m_off <= N);

			/* 2 - code match */
			if (m_len >= M1_MIN_LEN && m_len <= M1_MAX_LEN && m_off <= 256)
			{
				putbit(1);
				putbit(0);
				putbits(2,m_len - M1_MIN_LEN);
				putbyte(m_off - 1);
				c->m1++;
			}
#if (N >= 8192)
			else if (m_off >= 8192)
			{
				unsigned len = m_len;
				assert(m_len >= M3_MIN_LEN);
				putbit(1);
				putbit(1);
				putbyte(m_off & 31);
				putbyte(m_off >> 5);
				putbit(1);
				len -= M3_MIN_LEN - 1;
				while (len > 255)
				{
					len -= 255;
					putbyte(0);
				}
				putbyte(len);
				c->m4++;
			}
#endif
			else
			{
				assert(m_len >= 3);

				putbit(1);
				putbit(1);
				if (m_len <= 9)
				{
					putbyte(((m_len - 2) << 5) | (m_off & 31));
					putbyte(m_off >> 5);
					c->m2++;
				}
				else
				{
					lzo_uint len = m_len;
					putbyte(m_off & 31);
					putbyte(m_off >> 5);
#if (N >= 8192)
					putbit(0);
#endif
					len -= 10 - 1;
					while (len > 255)
					{
						len -= 255;
						putbyte(0);
					}
					putbyte(len);
					c->m3++;
				}
			}
			r = find_match(c,swd,m_len,1+ahead);
			assert(r == 0);
		}

		c->codesize = op - out;
	}

#if defined(LZO_EOF_CODE)
	/* code EOF code */
	putbit(1);
	putbit(1);
	putbyte(1 << 5);
	putbyte(0);
#endif

	/* flush remaining bits */
	assert(k < CHAR_BIT);
	if (k > 0)
	{
		assert(b == MASKBITS(k));
		assert(op - bitp > 1);
		*bitp = LZO_BYTE(MASKBITS(k));
		DUMPBITS(k);
		assert(b == 0);
		assert(k == 0);
	}

	assert(c->textsize == in_len);
	c->codesize = op - out;

	*out_len = op - out;

	if (c->cb)
		(*c->cb)(c->textsize,c->codesize);

#if 0
	printf("%ld -> %ld: %ld %ld %ld %ld %ld %ld\n",
		(long) c->textsize, (long) c->codesize,
		c->lit_bytes, c->m1, c->m2, c->m3, c->m4, c->lazy);
#endif
	return LZO_E_OK;
}



/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
lzo2a_999_compress  ( const lzo_byte *in , lzo_uint  in_len,
                            lzo_byte *out, lzo_uintp out_len,
                            lzo_voidp wrkmem )
{
	return lzo2a_999_compress_callback(in,in_len,out,out_len,wrkmem,
									   (lzo_progress_callback_t) 0, 0);
}


#endif /* !defined(LZO_999_UNSUPPORTED) */

/*
vi:ts=4:et
*/

