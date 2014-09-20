/* lzo_init.c -- initialization of the LZO library

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
#include "lzo_util.h"


#if 0
#  define IS_SIGNED(type)		(((type) (1ul << (8 * sizeof(type) - 1))) < 0)
#  define IS_UNSIGNED(type)		(((type) (1ul << (8 * sizeof(type) - 1))) > 0)
#else
#  define IS_SIGNED(type)		(((type) (-1)) < ((type) 0))
#  define IS_UNSIGNED(type)		(((type) (-1)) > ((type) 0))
#endif

#define IS_POWER_OF_2(x)		(((x) & ((x) - 1)) == 0)


/***********************************************************************
// Runtime check of the assumptions about the size of builtin types,
// memory model, byte order and other low-level constructs.
//
// We are really paranoid here - LZO should either fail (or crash)
// at startup or not at all.
//
// Because of inlining much of these functions evaluates to nothing.
************************************************************************/

static lzo_bool schedule_insns_bug(void);	/* avoid inlining */
static lzo_bool strength_reduce_bug(int *);	/* avoid inlining */


#if 0 || defined(LZO_DEBUG)
#include <stdio.h>
static lzo_bool __lzo_assert_fail(const char *s, unsigned line)
{
#if defined(__palmos__)
	printf("LZO assertion failed in line %u: '%s'\n",line,s);
#else
	fprintf(stderr,"LZO assertion failed in line %u: '%s'\n",line,s);
#endif
	return 0;
}
#  define __lzo_assert(x)	((x) ? 1 : __lzo_assert_fail(#x,__LINE__))
#else
#  define __lzo_assert(x)	((x) ? 1 : 0)
#endif


#undef COMPILE_TIME_ASSERT
#if 0
#  define COMPILE_TIME_ASSERT(expr)     r &= __lzo_assert(expr)
#else
#  define COMPILE_TIME_ASSERT(expr)     LZO_COMPILE_TIME_ASSERT(expr)
#endif


/***********************************************************************
// The next two functions should get completely optimized out of existance.
// Some assertions are redundant - but included for clarity.
************************************************************************/

static lzo_bool basic_integral_check(void)
{
    lzo_bool r = 1;

	/* paranoia */
	COMPILE_TIME_ASSERT(CHAR_BIT == 8);
	COMPILE_TIME_ASSERT(sizeof(char) == 1);
	COMPILE_TIME_ASSERT(sizeof(short) >= 2);
	COMPILE_TIME_ASSERT(sizeof(long) >= 4);
	COMPILE_TIME_ASSERT(sizeof(int) >= sizeof(short));
	COMPILE_TIME_ASSERT(sizeof(long) >= sizeof(int));

	COMPILE_TIME_ASSERT(sizeof(lzo_uint) == sizeof(lzo_int));
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == sizeof(lzo_int32));

    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= sizeof(unsigned));
#if defined(__LZO_STRICT_16BIT)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) == 2);
#else
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= sizeof(unsigned));
#endif

	/* paranoia - check <limits.h> header */
#if (USHRT_MAX == 65535u)
	COMPILE_TIME_ASSERT(sizeof(short) == 2);
#elif (USHRT_MAX == LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(short) == 4);
#elif (USHRT_MAX >= LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(short) > 4);
#endif
#if (UINT_MAX == 65535u)
	COMPILE_TIME_ASSERT(sizeof(int) == 2);
#elif (UINT_MAX == LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(int) == 4);
#elif (UINT_MAX >= LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(int) > 4);
#endif
#if (ULONG_MAX == 65535ul)
	COMPILE_TIME_ASSERT(sizeof(long) == 2);
#elif (ULONG_MAX == LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(long) == 4);
#elif (ULONG_MAX >= LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(long) > 4);
#endif

#if defined(SIZEOF_UNSIGNED)
	COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED == sizeof(unsigned));
#endif
#if defined(SIZEOF_UNSIGNED_LONG)
	COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED_LONG == sizeof(unsigned long));
#endif
#if defined(SIZEOF_UNSIGNED_SHORT)
	COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED_SHORT == sizeof(unsigned short));
#endif
#if !defined(__LZO_IN_MINILZO)
#if defined(SIZEOF_SIZE_T)
	COMPILE_TIME_ASSERT(SIZEOF_SIZE_T == sizeof(size_t));
#endif
#endif

	/* assert the signedness of our integral types */
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned char));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned short));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned long));
	COMPILE_TIME_ASSERT(IS_SIGNED(short));
	COMPILE_TIME_ASSERT(IS_SIGNED(int));
	COMPILE_TIME_ASSERT(IS_SIGNED(long));

	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint32));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int32));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int));

	COMPILE_TIME_ASSERT(INT_MAX    == LZO_STYPE_MAX(sizeof(int)));
	COMPILE_TIME_ASSERT(UINT_MAX   == LZO_UTYPE_MAX(sizeof(unsigned)));
	COMPILE_TIME_ASSERT(LONG_MAX   == LZO_STYPE_MAX(sizeof(long)));
	COMPILE_TIME_ASSERT(ULONG_MAX  == LZO_UTYPE_MAX(sizeof(unsigned long)));
	COMPILE_TIME_ASSERT(SHRT_MAX   == LZO_STYPE_MAX(sizeof(short)));
	COMPILE_TIME_ASSERT(USHRT_MAX  == LZO_UTYPE_MAX(sizeof(unsigned short)));
	COMPILE_TIME_ASSERT(LZO_UINT32_MAX == LZO_UTYPE_MAX(sizeof(lzo_uint32)));
	COMPILE_TIME_ASSERT(LZO_UINT_MAX   == LZO_UTYPE_MAX(sizeof(lzo_uint)));
#if !defined(__LZO_IN_MINILZO)
	COMPILE_TIME_ASSERT(SIZE_T_MAX     == LZO_UTYPE_MAX(sizeof(size_t)));
#endif

	r &= __lzo_assert(LZO_BYTE(257) == 1);

	return r;
}


static lzo_bool basic_ptr_check(void)
{
	lzo_bool r = 1;

	COMPILE_TIME_ASSERT(sizeof(char *) >= sizeof(int));
	COMPILE_TIME_ASSERT(sizeof(lzo_byte *) >= sizeof(char *));

	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_byte *));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_voidpp));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_bytepp));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) >= sizeof(lzo_uint));

	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_voidp));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_sptr_t));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) >= sizeof(lzo_uint));

	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(ptrdiff_t));

	COMPILE_TIME_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(lzo_uint));

#if defined(SIZEOF_CHAR_P)
	COMPILE_TIME_ASSERT(SIZEOF_CHAR_P == sizeof(char *));
#endif
#if defined(SIZEOF_PTRDIFF_T)
	COMPILE_TIME_ASSERT(SIZEOF_PTRDIFF_T == sizeof(ptrdiff_t));
#endif

	/* assert the signedness of our integral types */
	COMPILE_TIME_ASSERT(IS_SIGNED(ptrdiff_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(size_t));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_ptrdiff_t));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_sptr_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_ptr_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_moff_t));

	return r;
}


/***********************************************************************
//
************************************************************************/

static lzo_bool ptr_check(void)
{
	lzo_bool r = 1;
	int i;
	char _wrkmem[10 * sizeof(lzo_byte *) + sizeof(lzo_full_align_t)];
	lzo_bytep wrkmem;
	lzo_bytepp dict;
	unsigned char x[4 * sizeof(lzo_full_align_t)];
	long d;
	lzo_full_align_t a;
	lzo_full_align_t u;

	for (i = 0; i < (int) sizeof(x); i++)
		x[i] = LZO_BYTE(i);

	wrkmem = LZO_PTR_ALIGN_UP((lzo_byte *)_wrkmem,sizeof(lzo_full_align_t));

#if 0
	dict = (lzo_bytepp) wrkmem;
#else
    /* Avoid a compiler warning on architectures that
     * do not allow unaligned access. */
    u.a_lzo_bytep = wrkmem; dict = u.a_lzo_bytepp;
#endif

	d = (long) ((const lzo_bytep) dict - (const lzo_bytep) _wrkmem);
	r &= __lzo_assert(d >= 0);
	r &= __lzo_assert(d < (long) sizeof(lzo_full_align_t));

	memset(&a,0,sizeof(a));
	r &= __lzo_assert(a.a_lzo_voidp == NULL);

	memset(&a,0xff,sizeof(a));
	r &= __lzo_assert(a.a_ushort == USHRT_MAX);
	r &= __lzo_assert(a.a_uint == UINT_MAX);
	r &= __lzo_assert(a.a_ulong == ULONG_MAX);
	r &= __lzo_assert(a.a_lzo_uint == LZO_UINT_MAX);
	r &= __lzo_assert(a.a_lzo_uint32 == LZO_UINT32_MAX);

	/* sanity check of the memory model */
	if (r == 1)
	{
		for (i = 0; i < 8; i++)
			r &= __lzo_assert((const lzo_voidp) (&dict[i]) == (const lzo_voidp) (&wrkmem[i * sizeof(lzo_byte *)]));
	}

	/* check BZERO8_PTR and that NULL == 0 */
	memset(&a,0,sizeof(a));
	r &= __lzo_assert(a.a_char_p == NULL);
	r &= __lzo_assert(a.a_lzo_bytep == NULL);
	r &= __lzo_assert(NULL == (void *)0);
	if (r == 1)
	{
		for (i = 0; i < 10; i++)
			dict[i] = wrkmem;
		BZERO8_PTR(dict+1,sizeof(dict[0]),8);
		r &= __lzo_assert(dict[0] == wrkmem);
		for (i = 1; i < 9; i++)
			r &= __lzo_assert(dict[i] == NULL);
		r &= __lzo_assert(dict[9] == wrkmem);
	}

	/* check that the pointer constructs work as expected */
	if (r == 1)
	{
		unsigned k = 1;
		const unsigned n = (unsigned) sizeof(lzo_uint32);
		lzo_byte *p0;
		lzo_byte *p1;

		k += __lzo_align_gap(&x[k],n);
		p0 = (lzo_bytep) &x[k];
#if defined(PTR_LINEAR)
		r &= __lzo_assert((PTR_LINEAR(p0) & (n-1)) == 0);
#else
		r &= __lzo_assert(n == 4);
		r &= __lzo_assert(PTR_ALIGNED_4(p0));
#endif

		r &= __lzo_assert(k >= 1);
		p1 = (lzo_bytep) &x[1];
		r &= __lzo_assert(PTR_GE(p0,p1));

		r &= __lzo_assert(k < 1+n);
		p1 = (lzo_bytep) &x[1+n];
		r &= __lzo_assert(PTR_LT(p0,p1));

		/* now check that aligned memory access doesn't core dump */
		if (r == 1)
		{
            lzo_uint32 v0, v1;
#if 0
			v0 = * (lzo_uint32 *) &x[k];
			v1 = * (lzo_uint32 *) &x[k+n];
#else
            /* Avoid compiler warnings on architectures that
             * do not allow unaligned access. */
            u.a_uchar_p = &x[k];
            v0 = *u.a_lzo_uint32_p;
            u.a_uchar_p = &x[k+n];
            v1 = *u.a_lzo_uint32_p;
#endif
			r &= __lzo_assert(v0 > 0);
			r &= __lzo_assert(v1 > 0);
		}
	}

	return r;
}


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
_lzo_config_check(void)
{
	lzo_bool r = 1;
	int i;
	union {
		lzo_uint32 a;
		unsigned short b;
		lzo_uint32 aa[4];
		unsigned char x[4*sizeof(lzo_full_align_t)];
	} u;

	/* check that the compiler correctly casts signed to unsigned */
	COMPILE_TIME_ASSERT( (int) ((unsigned char) ((signed char) -1)) == 255);
	/* check that the compiler correctly promotes integrals */
	COMPILE_TIME_ASSERT( (((unsigned char)128) << (int)(8*sizeof(int)-8)) < 0);

#if 0
	/* paranoia - the following is guaranteed by definition anyway */
	r &= __lzo_assert((const void *)&u == (const void *)&u.a);
	r &= __lzo_assert((const void *)&u == (const void *)&u.b);
	r &= __lzo_assert((const void *)&u == (const void *)&u.x[0]);
	r &= __lzo_assert((const void *)&u == (const void *)&u.aa[0]);
#endif

	r &= basic_integral_check();
	r &= basic_ptr_check();
	if (r != 1)
		return LZO_E_ERROR;

	u.a = 0; u.b = 0;
	for (i = 0; i < (int) sizeof(u.x); i++)
		u.x[i] = LZO_BYTE(i);

	/* check LZO_BYTE_ORDER */
#if defined(LZO_BYTE_ORDER)
	if (r == 1)
	{
#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		lzo_uint32 a = (lzo_uint32) (u.a & LZO_0xffffffffL);
		unsigned short b = (unsigned short) (u.b & 0xffff);
		r &= __lzo_assert(a == 0x03020100L);
		r &= __lzo_assert(b == 0x0100);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		lzo_uint32 a = u.a >> (8 * sizeof(u.a) - 32);
		unsigned short b = u.b >> (8 * sizeof(u.b) - 16);
		r &= __lzo_assert(a == 0x00010203L);
		r &= __lzo_assert(b == 0x0001);
#  else
#    error "invalid LZO_BYTE_ORDER"
#  endif
	}
#endif

	/* check that unaligned memory access works as expected */
#if defined(LZO_UNALIGNED_OK_2)
	COMPILE_TIME_ASSERT(sizeof(short) == 2);
	if (r == 1)
	{
		unsigned short b[4];

        for (i = 0; i < 4; i++)
            b[i] = * (const unsigned short *) &u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		r &= __lzo_assert(b[0] == 0x0100);
		r &= __lzo_assert(b[1] == 0x0201);
		r &= __lzo_assert(b[2] == 0x0302);
		r &= __lzo_assert(b[3] == 0x0403);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		r &= __lzo_assert(b[0] == 0x0001);
		r &= __lzo_assert(b[1] == 0x0102);
		r &= __lzo_assert(b[2] == 0x0203);
		r &= __lzo_assert(b[3] == 0x0304);
#  endif
	}
#endif

#if defined(LZO_UNALIGNED_OK_4)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
	if (r == 1)
	{
		lzo_uint32 a[4];

		for (i = 0; i < 4; i++)
			a[i] = * (const lzo_uint32 *) &u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		r &= __lzo_assert(a[0] == 0x03020100L);
		r &= __lzo_assert(a[1] == 0x04030201L);
		r &= __lzo_assert(a[2] == 0x05040302L);
		r &= __lzo_assert(a[3] == 0x06050403L);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		r &= __lzo_assert(a[0] == 0x00010203L);
		r &= __lzo_assert(a[1] == 0x01020304L);
		r &= __lzo_assert(a[2] == 0x02030405L);
		r &= __lzo_assert(a[3] == 0x03040506L);
#  endif
	}
#endif

#if defined(LZO_ALIGNED_OK_4)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
#endif

	COMPILE_TIME_ASSERT(lzo_sizeof_dict_t == sizeof(lzo_dict_t));

	/* save space and don't require linking in the lzo_adler32() function */
#if defined(__LZO_IN_MINLZO)
	/* check the lzo_adler32() function */
	if (r == 1)
	{
		lzo_uint32 adler;
		adler = lzo_adler32(0, NULL, 0);
		adler = lzo_adler32(adler, lzo_copyright(), 200);
		r &= __lzo_assert(adler == 0xc76f1751L);
	}
#endif

	/* check for the gcc schedule-insns optimization bug */
	if (r == 1)
	{
		r &= __lzo_assert(!schedule_insns_bug());
	}

	/* check for the gcc strength-reduce optimization bug */
	if (r == 1)
	{
		static int x[3];
		static unsigned xn = 3;
		register unsigned j;

		for (j = 0; j < xn; j++)
			x[j] = (int)j - 3;
		r &= __lzo_assert(!strength_reduce_bug(x));
	}

	/* now for the low-level pointer checks */
	if (r == 1)
	{
		r &= ptr_check();
	}

	return r == 1 ? LZO_E_OK : LZO_E_ERROR;
}


static lzo_bool schedule_insns_bug(void)
{
#if defined(__LZO_CHECKER)
	/* for some reason checker complains about uninitialized memory access */
	return 0;
#else
	const int clone[] = {1, 2, 0};
	const int *q;
	q = clone;
	return (*q) ? 0 : 1;
#endif
}


static lzo_bool strength_reduce_bug(int *x)
{
	return x[0] != -3 || x[1] != -2 || x[2] != -1;
}


#undef COMPILE_TIME_ASSERT


/***********************************************************************
//
************************************************************************/

int __lzo_init_done = 0;

LZO_PUBLIC(int)
__lzo_init2(unsigned v, int s1, int s2, int s3, int s4, int s5,
                        int s6, int s7, int s8, int s9)
{
	int r;

	__lzo_init_done = 1;

	if (v == 0)
		return LZO_E_ERROR;

	r = (s1 == -1 || s1 == (int) sizeof(short)) &&
	    (s2 == -1 || s2 == (int) sizeof(int)) &&
	    (s3 == -1 || s3 == (int) sizeof(long)) &&
	    (s4 == -1 || s4 == (int) sizeof(lzo_uint32)) &&
	    (s5 == -1 || s5 == (int) sizeof(lzo_uint)) &&
	    (s6 == -1 || s6 == (int) lzo_sizeof_dict_t) &&
	    (s7 == -1 || s7 == (int) sizeof(char *)) &&
	    (s8 == -1 || s8 == (int) sizeof(lzo_voidp)) &&
	    (s9 == -1 || s9 == (int) sizeof(lzo_compress_t));
	if (!r)
		return LZO_E_ERROR;

	r = _lzo_config_check();
	if (r != LZO_E_OK)
		return r;

	return r;
}


/***********************************************************************
// backward compatibility with v1.01
************************************************************************/

#if !defined(__LZO_IN_MINILZO)

LZO_EXTERN(int)
__lzo_init(unsigned v,int s1,int s2,int s3,int s4,int s5,int s6,int s7);

LZO_PUBLIC(int)
__lzo_init(unsigned v,int s1,int s2,int s3,int s4,int s5,int s6,int s7)
{
	if (v == 0 || v > 0x1010)
		return LZO_E_ERROR;
	return __lzo_init2(v,s1,s2,s3,s4,s5,-1,-1,s6,s7);
}

#endif


/*
vi:ts=4:et
*/
