/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"
//#include "Internal.h"

/*//////////////////////////////////////////////////////////////////////
                           File Scope Globals
//////////////////////////////////////////////////////////////////////*/
// Indicates that the version information is valid.
static BOOL g_bHasVersion = FALSE ;
// Indicates NT or 95/98.
static BOOL g_bIsNT = TRUE ;

BOOL __stdcall IsNT ( void )
{
    if ( TRUE == g_bHasVersion )
    {
        return ( TRUE == g_bIsNT ) ;
    }

    OSVERSIONINFO stOSVI ;

    FillMemory ( &stOSVI , sizeof ( OSVERSIONINFO ), NULL ) ;
    stOSVI.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO ) ;

    BOOL bRet = GetVersionEx ( &stOSVI ) ;
    ASSERT ( TRUE == bRet ) ;
    if ( FALSE == bRet )
    {
        TRACE0 ( "GetVersionEx failed!\n" ) ;
        return ( FALSE ) ;
    }

    // Check the version and call the appropriate thing.
    if ( VER_PLATFORM_WIN32_NT == stOSVI.dwPlatformId )
    {
        g_bIsNT = TRUE ;
    }
    else
    {
        g_bIsNT = FALSE ;
    }
    g_bHasVersion = TRUE ;
    return ( TRUE == g_bIsNT ) ;
}


