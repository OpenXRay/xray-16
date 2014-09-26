/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
------------------------------------------------------------------------
FILE        :   WarningsOff.h
DISCUSSION  :
    Turns off warnings so standard headers will compile.  Any warning
turned off, must be turned back in in WarningsOn.h
----------------------------------------------------------------------*/

#ifndef _WARNINGSOFF_H
#define _WARNINGSOFF_H

/* Unreferenced in inline function removed */
#pragma warning ( disable : 4514 )

#ifndef _DEBUG
/* function '' not inlined. */
#pragma warning ( disable : 4710 )
#endif

/* This is really a dumb warning.... */
/* 'blah' : identifier was truncated to '255' characters in the debug
   information */
#pragma warning ( disable : 4786 )


/* Force everything to warning level 3. */
#pragma warning ( push , 3 )

#endif  /* _WARNINGSOFF_H */


