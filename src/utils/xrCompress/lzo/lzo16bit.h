/* lzo16bit.h -- configuration for the strict 16-bit memory model

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
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
   http://www.oberhumer.com/opensource/lzo/
 */


/*
 * NOTE:
 *   the strict 16-bit memory model is *not* officially supported.
 *   This file is only included for the sake of completeness.
 */


#ifndef __LZOCONF_H
#  include <lzoconf.h>
#endif

#ifndef __LZO16BIT_H
#define __LZO16BIT_H

#if defined(__LZO_STRICT_16BIT)
#if (UINT_MAX < LZO_0xffffffffL)

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

#ifndef LZO_99_UNSUPPORTED
#define LZO_99_UNSUPPORTED
#endif
#ifndef LZO_999_UNSUPPORTED
#define LZO_999_UNSUPPORTED
#endif

typedef unsigned int        lzo_uint;
typedef int                 lzo_int;
#define LZO_UINT_MAX        UINT_MAX
#define LZO_INT_MAX         INT_MAX

#define lzo_sizeof_dict_t   sizeof(lzo_uint)


/***********************************************************************
//
************************************************************************/

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)

#if 0
#define __LZO_MMODEL        __far
#else
#define __LZO_MMODEL
#endif

#endif /* defined(__LZO_DOS16) || defined(__LZO_WIN16) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* (UINT_MAX < LZO_0xffffffffL) */
#endif /* defined(__LZO_STRICT_16BIT) */

#endif /* already included */

