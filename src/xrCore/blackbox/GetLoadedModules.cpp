/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"

// The project internal header file.
#include "Internal.h"

// The documentation for this function is in BugslayerUtil.h.
BOOL  __stdcall
                           GetLoadedModules ( DWORD     dwPID        ,
                                              UINT      uiCount      ,
                                              HMODULE * paModArray   ,
                                              LPDWORD   pdwRealCount   )
{
    // Do the debug checking.
    ASSERT ( NULL != pdwRealCount ) ;
    ASSERT ( FALSE == IsBadWritePtr ( pdwRealCount , sizeof ( UINT ) ));
#ifdef _DEBUG
    if ( 0 != uiCount )
    {
        ASSERT ( NULL != paModArray ) ;
        ASSERT ( FALSE == IsBadWritePtr ( paModArray                   ,
                                          uiCount *
                                                sizeof ( HMODULE )   ));
    }
#endif

    // Do the parameter checking for real.  Note that I only check the
    // memory in paModArray if uiCount is > 0.  The user can pass zero
    // in uiCount if they are just interested in the total to be
    // returned so they could dynamically allocate a buffer.
    if ( ( TRUE == IsBadWritePtr ( pdwRealCount , sizeof(UINT) ) )    ||
         ( ( uiCount > 0 ) &&
           ( TRUE == IsBadWritePtr ( paModArray ,
                                     uiCount * sizeof(HMODULE) ) ) )   )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // Figure out which OS we are on.
    OSVERSIONINFO stOSVI ;

    FillMemory ( &stOSVI ,sizeof ( OSVERSIONINFO ), NULL  ) ;
    stOSVI.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO ) ;

    BOOL bRet = GetVersionEx ( &stOSVI ) ;
    ASSERT ( TRUE == bRet ) ;
    if ( FALSE == bRet )
    {
        TRACE0 ( "GetVersionEx failed!\n" ) ;
        return ( FALSE ) ;
    }

    // Check the version and call the appropriate thing.
    if ( ( VER_PLATFORM_WIN32_NT == stOSVI.dwPlatformId ) &&
         ( 4 == stOSVI.dwMajorVersion                   )    )
    {
        // This is NT 4 so call its specific version in PSAPI.DLL
        return ( NT4GetLoadedModules ( dwPID        ,
                                       uiCount      ,
                                       paModArray   ,
                                       pdwRealCount  ) );
    }
    else
    {
        // Win9x and Win2K go through tool help.
        return ( TLHELPGetLoadedModules ( dwPID         ,
                                          uiCount       ,
                                          paModArray    ,
                                          pdwRealCount   ) ) ;
    }
}

