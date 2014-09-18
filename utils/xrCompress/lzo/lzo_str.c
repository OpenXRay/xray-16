/* lzo_str.c -- string functions for the the LZO library

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
// slow but portable <string.h> stuff, only used in assertions
************************************************************************/

LZO_PUBLIC(int)
lzo_memcmp(const lzo_voidp s1, const lzo_voidp s2, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCMP)
	return memcmp(s1,s2,len);
#else
	const lzo_byte *p1 = (const lzo_byte *) s1;
	const lzo_byte *p2 = (const lzo_byte *) s2;
	int d;

	if (len > 0) do
	{
		d = *p1 - *p2;
		if (d != 0)
			return d;
		p1++;
		p2++;
	}
	while (--len > 0);
	return 0;
#endif
}


LZO_PUBLIC(lzo_voidp)
lzo_memcpy(lzo_voidp dest, const lzo_voidp src, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCPY)
	return memcpy(dest,src,len);
#else
	lzo_byte *p1 = (lzo_byte *) dest;
	const lzo_byte *p2 = (const lzo_byte *) src;

	if (len <= 0 || p1 == p2)
		return dest;
	do
		*p1++ = *p2++;
	while (--len > 0);
	return dest;
#endif
}


LZO_PUBLIC(lzo_voidp)
lzo_memmove(lzo_voidp dest, const lzo_voidp src, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMMOVE)
	return memmove(dest,src,len);
#else
	lzo_byte *p1 = (lzo_byte *) dest;
	const lzo_byte *p2 = (const lzo_byte *) src;

	if (len <= 0 || p1 == p2)
		return dest;

	if (p1 < p2)
	{
		do
			*p1++ = *p2++;
		while (--len > 0);
	}
	else
	{
		p1 += len;
		p2 += len;
		do
			*--p1 = *--p2;
		while (--len > 0);
	}
	return dest;
#endif
}


LZO_PUBLIC(lzo_voidp)
lzo_memset(lzo_voidp s, int c, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)
	return memset(s,c,len);
#else
	lzo_byte *p = (lzo_byte *) s;

	if (len > 0) do
		*p++ = LZO_BYTE(c);
	while (--len > 0);
	return s;
#endif
}


/*
vi:ts=4:et
*/
