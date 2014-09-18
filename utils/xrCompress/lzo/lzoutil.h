/* lzoutil.h -- utilitiy functions for use by applications

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


#ifndef __LZOUTIL_H
#define __LZOUTIL_H

#ifndef __LZOCONF_H
#include <lzoconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// portable memory allocation
//
// The LZO library does not allocate any memory (you always pass a
// pre-allocated pointer via the `wrkmem' parameter), but these
// functions are handy for application programs.
************************************************************************/

LZO_EXTERN(lzo_voidp) lzo_alloc(lzo_uint _nelems, lzo_uint _size);
LZO_EXTERN(lzo_voidp) lzo_malloc(lzo_uint _size);
LZO_EXTERN(void) lzo_free(lzo_voidp _ptr);

typedef lzo_voidp (__LZO_ENTRY *lzo_alloc_hook_t) (lzo_uint, lzo_uint);
typedef void (__LZO_ENTRY *lzo_free_hook_t) (lzo_voidp);

LZO_EXTERN_VAR(lzo_alloc_hook_t) lzo_alloc_hook;
LZO_EXTERN_VAR(lzo_free_hook_t) lzo_free_hook;


/***********************************************************************
// portable file io
************************************************************************/

#if !defined(LZO_FILEP)
#  define LZO_FILEP             void *
#endif

LZO_EXTERN(lzo_uint)
lzo_fread(LZO_FILEP f, lzo_voidp buf, lzo_uint size);
LZO_EXTERN(lzo_uint)
lzo_fwrite(LZO_FILEP f, const lzo_voidp buf, lzo_uint size);


#if (LZO_UINT_MAX <= UINT_MAX)
#  define lzo_fread(f,b,s)      (fread(b,1,s,f))
#  define lzo_fwrite(f,b,s)     (fwrite(b,1,s,f))
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

