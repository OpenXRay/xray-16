#include "stdafx.h"
#pragma hdrstop

/* lzo_init.c -- initialization of the LZO library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2005 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License,
   version 2, as published by the Free Software Foundation.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


#include "rt_lzo_conf.h"


/***********************************************************************
// Runtime check of the assumptions about the size of builtin types,
// memory model, byte order and other low-level constructs.
//
// We are really paranoid here - LZO should either fail
// at startup or not at all.
//
// Because of inlining much of these functions evaluates to nothing.
//
// And while many of the tests seem highly obvious and redundant they are
// here to catch compiler/optimizer bugs. Yes, these do exist.
************************************************************************/

#if !defined(__LZO_IN_MINILZO)

#define ACC_WANT_ACC_CHK_CH 1
#undef ACCCHK_ASSERT
#include "rt_miniacc.h"

    ACCCHK_ASSERT_IS_SIGNED_T(lzo_int)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uint)

    ACCCHK_ASSERT_IS_SIGNED_T(lzo_int32)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uint32)
    ACCCHK_ASSERT((LZO_UINT32_C(1) << (int)(8*sizeof(LZO_UINT32_C(1))-1)) > 0)
    ACCCHK_ASSERT(sizeof(lzo_uint32) >= 4)

#if !defined(__LZO_UINTPTR_T_IS_POINTER)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uintptr_t)
#endif
    ACCCHK_ASSERT(sizeof(lzo_uintptr_t) >= sizeof(lzo_voidp))

    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_xint)
    ACCCHK_ASSERT(sizeof(lzo_xint) >= sizeof(lzo_uint32))
    ACCCHK_ASSERT(sizeof(lzo_xint) >= sizeof(lzo_uint))
    ACCCHK_ASSERT(sizeof(lzo_xint) == sizeof(lzo_uint32) || sizeof(lzo_xint) == sizeof(lzo_uint))

#endif
#undef ACCCHK_ASSERT


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
_lzo_config_check(void)
{
    lzo_bool r = 1;
    union { unsigned char c[2*sizeof(lzo_xint)]; lzo_xint l[2]; } u;

#if !defined(LZO_CFG_NO_CONFIG_CHECK)
#if defined(LZO_ABI_BIG_ENDIAN)
    u.l[0] = u.l[1] = 0; u.c[sizeof(lzo_xint) - 1] = 128;
    r &= (u.l[0] == 128);
#endif
#if defined(LZO_ABI_LITTLE_ENDIAN)
    u.l[0] = u.l[1] = 0; u.c[0] = 128;
    r &= (u.l[0] == 128);
#endif
#if defined(LZO_UNALIGNED_OK_2)
    u.l[0] = u.l[1] = 0;
    r &= ((* (const lzo_ushortp) (const lzo_voidp) &u.c[1]) == 0);
#endif
#if defined(LZO_UNALIGNED_OK_4)
    u.l[0] = u.l[1] = 0;
    r &= ((* (const lzo_uint32p) (const lzo_voidp) &u.c[1]) == 0);
#endif
#endif

    LZO_UNUSED(u);
    return r == 1 ? LZO_E_OK : LZO_E_ERROR;
}


/***********************************************************************
//
************************************************************************/

int __lzo_init_done = 0;

LZO_PUBLIC(int)
__lzo_init_v2(unsigned v, int s1, int s2, int s3, int s4, int s5,
                          int s6, int s7, int s8, int s9)
{
    int r;

#if defined(__LZO_IN_MINILZO)
#elif (LZO_CC_MSC && ((_MSC_VER) < 700))
#else
#define ACC_WANT_ACC_CHK_CH 1
#undef ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)  LZO_COMPILE_TIME_ASSERT(expr)
#include "rt_miniacc.h"
#endif
#undef ACCCHK_ASSERT

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
        (s9 == -1 || s9 == (int) sizeof(lzo_callback_t));
    if (!r)
        return LZO_E_ERROR;

    r = _lzo_config_check();
    if (r != LZO_E_OK)
        return r;

    return r;
}


#if !defined(__LZO_IN_MINILZO)
#include "rt_lzo_dll.ch"
#endif


/*
vi:ts=4:et
*/
