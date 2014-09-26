/* lzo2a_d.ch -- implementation of the LZO2A decompression algorithm

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


#include "lzo1_d.ch"


#if defined(HAVE_ANY_IP) && defined(HAVE_ANY_OP)
   /* too many local variables, cannot allocate registers */
#  undef LZO_OPTIMIZE_GNUC_i386
#endif


/***********************************************************************
// decompress a block of data.
************************************************************************/

#define _NEEDBYTE	NEED_IP(1)
#define _NEXTBYTE	(*ip++)

LZO_PUBLIC(int)
DO_DECOMPRESS    ( const lzo_byte *in , lzo_uint  in_len,
						 lzo_byte *out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
#if defined(LZO_OPTIMIZE_GNUC_i386)
	register lzo_byte *op __asm__("%edi");
	register const lzo_byte *ip __asm__("%esi");
	register const lzo_byte *m_pos __asm__("%ebx");
#else
	register lzo_byte *op;
	register const lzo_byte *ip;
	register const lzo_byte *m_pos;
#endif

	lzo_uint t;
	const lzo_byte * const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
	lzo_byte * const op_end = out + *out_len;
#endif

	lzo_uint32 b = 0;		/* bit buffer */
	unsigned k = 0;			/* bits in bit buffer */

	LZO_UNUSED(wrkmem);

#if defined(__LZO_QUERY_DECOMPRESS)
	if (__LZO_IS_DECOMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
		return __LZO_QUERY_DECOMPRESS(in,in_len,out,out_len,wrkmem,0,0);
#endif

	op = out;
	ip = in;

	while (TEST_IP && TEST_OP)
	{
		NEEDBITS(1);
		if (MASKBITS(1) == 0)
		{
			DUMPBITS(1);
			/* a literal */
			NEED_IP(1); NEED_OP(1);
			*op++ = *ip++;
			continue;
		}
		DUMPBITS(1);

		NEEDBITS(1);
		if (MASKBITS(1) == 0)
		{
			DUMPBITS(1);
			/* a M1 match */
			NEEDBITS(2);
			t = M1_MIN_LEN + (lzo_uint) MASKBITS(2);
			DUMPBITS(2);
			NEED_IP(1); NEED_OP(t);
			m_pos = op - 1 - *ip++;
			assert(m_pos >= out); assert(m_pos < op);
			TEST_LOOKBEHIND(m_pos,out);
			MEMMOVE_DS(op,m_pos,t);
			continue;
		}
		DUMPBITS(1);

		NEED_IP(2);
		t = *ip++;
		m_pos = op;
		m_pos -= (t & 31) | (((lzo_uint) *ip++) << 5);
		t >>= 5;
		if (t == 0)
		{
#if (N >= 8192)
			NEEDBITS(1);
			t = MASKBITS(1);
			DUMPBITS(1);
			if (t == 0)
				t = 10 - 1;
			else
			{
				/* a M3 match */
				m_pos -= 8192;		/* t << 13 */
				t = M3_MIN_LEN - 1;
			}
#else
			t = 10 - 1;
#endif
			NEED_IP(1);
			while (*ip == 0)
			{
				t += 255;
				ip++;
				NEED_IP(1);
			}
			t += *ip++;
		}
		else
		{
#if defined(LZO_EOF_CODE)
			if (m_pos == op)
				goto eof_found;
#endif
			t += 2;
		}
		assert(m_pos >= out); assert(m_pos < op);
		TEST_LOOKBEHIND(m_pos,out);
		NEED_OP(t);
		MEMMOVE_DS(op,m_pos,t);
	}


#if defined(LZO_EOF_CODE)
#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
	/* no EOF code was found */
	*out_len = op - out;
	return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
	assert(t == 1);
#endif
	*out_len = op - out;
	return (ip == ip_end ? LZO_E_OK :
	       (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));


#if defined(HAVE_NEED_IP)
input_overrun:
	*out_len = op - out;
	return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
	*out_len = op - out;
	return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
	*out_len = op - out;
	return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}


/*
vi:ts=4:et
*/

