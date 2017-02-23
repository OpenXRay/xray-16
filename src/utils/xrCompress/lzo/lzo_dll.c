/* lzo_dll.c -- DLL initialization of the LZO library

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

#if defined(LZO_BUILD_DLL)


/***********************************************************************
// Windows 16 bit + Watcom C + DLL
************************************************************************/

#if defined(__LZO_WIN16) && defined(__WATCOMC__) && defined(__SW_BD)

/* don't pull in <windows.h> - we don't need it */
#if 0
#include <windows.h>
#endif

#pragma off (unreferenced);
#if 0 && defined(WINVER)
BOOL FAR PASCAL LibMain ( HANDLE hInstance, WORD wDataSegment,
                          WORD wHeapSize, LPSTR lpszCmdLine )
#else
int __far __pascal LibMain ( int a, short b, short c, long d )
#endif
#pragma on (unreferenced);
{
	return 1;
}

#endif


#endif /* defined(LZO_BUILD_DLL) */

/*
vi:ts=4:et
*/
