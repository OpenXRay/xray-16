/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"
#include "Internal.h"

// The Win95 version of GetModuleBaseName.
static DWORD __stdcall Win95GetModuleBaseName ( HANDLE  hProcess   ,
                                                HMODULE hModule    ,
                                                LPSTR   lpBaseName ,
                                                DWORD   nSize       ) ;

/*//////////////////////////////////////////////////////////////////////
                        Function Implementation
//////////////////////////////////////////////////////////////////////*/

DWORD __stdcall BSUGetModuleBaseName ( HANDLE  hProcess   ,
                                       HMODULE hModule    ,
                                       LPTSTR  lpBaseName ,
                                       DWORD   nSize       )
{
    if ( TRUE == IsNT ( ) )
    {
        // Call the NT version.  It is in NT4ProcessInfo because that is
        // where all the PSAPI wrappers are kept.
        return ( NTGetModuleBaseName ( hProcess     ,
                                       hModule      ,
                                       lpBaseName   ,
                                       nSize         ) ) ;
    }
    return ( Win95GetModuleBaseName ( hProcess     ,
                                      hModule      ,
                                      lpBaseName   ,
                                      nSize         ) ) ;

}

static DWORD __stdcall
            Win95GetModuleBaseName ( HANDLE  /*hProcess*/  ,
                                     HMODULE hModule       ,
                                     LPSTR   lpBaseName    ,
                                     DWORD   nSize          )
{
    ASSERT ( FALSE == IsBadWritePtr ( lpBaseName , nSize ) ) ;
    if ( TRUE == IsBadWritePtr ( lpBaseName , nSize ) )
    {
        TRACE0 ( "Win95GetModuleBaseName Invalid string buffer\n" ) ;
        SetLastError ( ERROR_INVALID_PARAMETER ) ;
        return ( 0 ) ;
    }

    // This could blow the stack...
    char szBuff[ MAX_PATH + 1 ] ;
    DWORD dwRet = GetModuleFileName ( hModule , szBuff , MAX_PATH ) ;
    ASSERT ( 0 != dwRet ) ;
    if ( 0 == dwRet )
    {
        return ( 0 ) ;
    }

    // Find the last '\' mark.
    char * pStart = strrchr ( szBuff , '\\' ) ;
    int iMin ;
    if ( NULL != pStart )
    {
        // Move up one character.
        pStart++ ;
        //lint -e666
        iMin = min ( (int)nSize , (lstrlen ( pStart ) + 1) ) ;
        //lint +e666
        lstrcpyn ( lpBaseName , pStart , iMin ) ;
    }
    else
    {
        // Copy the szBuff buffer in.
        //lint -e666
        iMin = min ( (int)nSize , (lstrlen ( szBuff ) + 1) ) ;
        //lint +e666
        lstrcpyn ( lpBaseName , szBuff , iMin ) ;
    }
    // Always NULL terminate.
    lpBaseName[ iMin ] = '\0' ;
    return ( (DWORD)(iMin - 1) ) ;
}

DWORD  __stdcall
        BSUSymInitialize ( DWORD  dwPID          ,
                           HANDLE hProcess       ,
                           PSTR   UserSearchPath ,
                           BOOL   fInvadeProcess  )
{
    // If this is any flavor of NT or fInvadeProcess is FALSE, just call
    // SymInitialize itself
    if ( ( TRUE == IsNT ( ) ) || ( FALSE == fInvadeProcess ) )
    {
        return ( ::SymInitialize ( hProcess       ,
                                   UserSearchPath ,
                                   fInvadeProcess  ) ) ;
    }
    else
    {
        // This is Win9x and the user wants to invade!

        // The first step is to initialize the symbol engine.  If it
        // fails, there is not much I can do.
        BOOL bSymInit = ::SymInitialize ( hProcess       ,
                                          UserSearchPath ,
                                          fInvadeProcess  ) ;
        ASSERT ( FALSE != bSymInit ) ;
        if ( FALSE == bSymInit )
        {
            return ( FALSE ) ;
        }

        DWORD dwCount ;
        // Find out how many modules there are.  This is a BSU function.
        if ( FALSE == GetLoadedModules ( dwPID    ,
                                         0        ,
                                         NULL     ,
                                         &dwCount  ) )
        {
            ASSERT ( !"GetLoadedModules failed" ) ;
            // Clean up the symbol engine and leave.
            VERIFY ( ::SymCleanup ( hProcess ) ) ;
            return ( FALSE ) ;
        }
        // Allocate something big enough to hold the list.
        HMODULE * paMods = new HMODULE[ dwCount ] ;

        // Get the list for real.
        if ( FALSE == GetLoadedModules ( dwPID    ,
                                         dwCount  ,
                                         paMods   ,
                                         &dwCount  ) )
        {
            ASSERT ( !"GetLoadedModules failed" ) ;
            // Clean up the symbol engine and leave.
            VERIFY ( ::SymCleanup ( hProcess ) ) ;
            // Free the memory that I allocated earlier.
            delete [] paMods ;
            return ( FALSE ) ;
        }
        // The module filename.
        TCHAR szModName [ MAX_PATH ] ;
        for ( UINT uiCurr = 0 ; uiCurr < dwCount ; uiCurr++ )
        {
            // Get the module's filename.
            if ( FALSE == GetModuleFileName ( paMods[ uiCurr ]     ,
                                              szModName            ,
                                              sizeof ( szModName )  ) )
            {
                ASSERT ( !"GetModuleFileName failed!" ) ;
                // Clean up the symbol engine and leave.
                VERIFY ( ::SymCleanup ( hProcess ) ) ;
                // Free the memory that I allocated earlier.
                delete [] paMods ;
                return ( FALSE ) ;
            }

            // In order to get the symbol engine to work outside a
            // debugger, it needs a handle to the image.  Yes, this
            // will leak but the OS will close it down when the process
            // ends.
            HANDLE hFile = CreateFile ( szModName       ,
                                        GENERIC_READ    ,
                                        FILE_SHARE_READ ,
                                        NULL            ,
                                        OPEN_EXISTING   ,
                                        0               ,
                                        0                ) ;

            // For whatever reason, SymLoadModule can return zero, but
            // it still loads the modules.  Sheez.
            if ( FALSE == SymLoadModule ( hProcess               ,
                                          hFile                  ,
                                          szModName              ,
                                          NULL                   ,
                                         (DWORD)paMods[ uiCurr ] ,
                                          0                       ) )
            {
                // Check the last error value.  If it is zero, then all
                // I can assume is that it worked.
                DWORD dwLastErr = GetLastError ( ) ;
                ASSERT ( ERROR_SUCCESS == dwLastErr ) ;
                if ( ERROR_SUCCESS != dwLastErr )
                {
                    // Clean up the symbol engine and leave.
                    VERIFY ( ::SymCleanup ( hProcess ) ) ;
                    // Free the memory that I allocated earlier.
                    delete [] paMods ;
                    return ( FALSE ) ;
                }
            }
        }
        delete [] paMods ;
    }
    return ( TRUE ) ;
}

DWORD __stdcall BSUGetModuleFileNameEx ( DWORD     dwPID        ,
                                         HANDLE    hProcess     ,
                                         HMODULE   hModule      ,
                                         LPTSTR    szFilename   ,
                                         DWORD     nSize         )
{
    if ( TRUE == IsNT ( ) )
    {
        return ( NTGetModuleFileNameEx ( dwPID      ,
                                         hProcess   ,
                                         hModule    ,
                                         szFilename ,
                                         nSize       ) ) ;
    }
    return ( TLHELPGetModuleFileNameEx ( dwPID      ,
                                         hProcess   ,
                                         hModule    ,
                                         szFilename ,
                                         nSize       ) ) ;
}

