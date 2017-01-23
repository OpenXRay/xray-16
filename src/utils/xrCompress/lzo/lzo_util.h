/* lzo_util.h -- utilities for the LZO library

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


#ifndef __LZO_UTIL_H
#define __LZO_UTIL_H

#ifndef __LZO_CONF_H
#  include "lzo_conf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// fast memcpy that copies multiples of 8 byte chunks.
// len is the number of bytes.
// note: all parameters must be lvalues, len >= 8
//       dest and src advance, len is undefined afterwards
************************************************************************/

#if 1 && defined(HAVE_MEMCPY)
#if !defined(__LZO_DOS16) && !defined(__LZO_WIN16)

#define MEMCPY8_DS(dest,src,len) \
	memcpy(dest,src,len); \
	dest += len; \
	src += len

#endif
#endif


#if 0 && !defined(MEMCPY8_DS)

#define MEMCPY8_DS(dest,src,len) \
	{ do { \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		len -= 8; \
	} while (len > 0); }

#endif


#if !defined(MEMCPY8_DS)

#define MEMCPY8_DS(dest,src,len) \
	{ register lzo_uint __l = (len) / 8; \
	do { \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
		*dest++ = *src++; \
	} while (--__l > 0); }

#endif


/***********************************************************************
// memcpy and pseudo-memmove
// len is the number of bytes.
// note: all parameters must be lvalues, len > 0
//       dest and src advance, len is undefined afterwards
************************************************************************/

#define MEMCPY_DS(dest,src,len) \
	do *dest++ = *src++; \
	while (--len > 0)

#define MEMMOVE_DS(dest,src,len) \
	do *dest++ = *src++; \
	while (--len > 0)


/***********************************************************************
// fast bzero that clears multiples of 8 pointers
// n is the number of pointers.
// note: n > 0
//       s and n are undefined afterwards
************************************************************************/

#if 0 && defined(LZO_OPTIMIZE_GNUC_i386)

#define BZERO8_PTR(s,l,n) \
__asm__ __volatile__( \
	"movl  %0,%%eax \n"             \
	"movl  %1,%%edi \n"             \
	"movl  %2,%%ecx \n"             \
	"cld \n"                        \
	"rep \n"                        \
	"stosl %%eax,(%%edi) \n"        \
	: /* no outputs */              \
	:"g" (0),"g" (s),"g" (n)	  	\
	:"eax","edi","ecx", "memory", "cc" \
)

#elif (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)

#if 1
#define BZERO8_PTR(s,l,n)	memset((s),0,(lzo_uint)(l)*(n))
#else
#define BZERO8_PTR(s,l,n)	memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))
#endif

#else

#define BZERO8_PTR(s,l,n) \
	lzo_memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))

#endif


/***********************************************************************
// rotate (currently not used)
************************************************************************/

#if 0
#if defined(__GNUC__) && defined(__i386__)

unsigned char lzo_rotr8(unsigned char value, int shift);
extern __inline__ unsigned char lzo_rotr8(unsigned char value, int shift)
{
	unsigned char result;

	__asm__ __volatile__ ("movb %b1, %b0; rorb %b2, %b0"
                      	: "=a"(result) : "g"(value), "c"(shift));
	return result;
}

unsigned short lzo_rotr16(unsigned short value, int shift);
extern __inline__ unsigned short lzo_rotr16(unsigned short value, int shift)
{
	unsigned short result;

	__asm__ __volatile__ ("movw %b1, %b0; rorw %b2, %b0"
                      	: "=a"(result) : "g"(value), "c"(shift));
	return result;
}

#endif
#endif



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

/*
vi:ts=4:et
*/
