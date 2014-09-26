/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"
#include "DiagAssert.h"

// The project internal header file.
//#include "Internal.h"

// STL will not compile at /W4 /WX.  Not good.
#ifndef _DEBUG
// function '' not inlined
#pragma warning ( disable : 4710 )
#endif
#pragma warning( push, 3 )
#include <vector>
using namespace std ;
#pragma warning( pop )

/*//////////////////////////////////////////////////////////////////////
                          File Scope Typedefs
//////////////////////////////////////////////////////////////////////*/
// The size of buffer that DiagAssert will use.  If you want to see more
// stack trace, make this a bigger number.
#define DIAGASSERT_BUFFSIZE 4096

/*//////////////////////////////////////////////////////////////////////
                          File Scope Typedefs
//////////////////////////////////////////////////////////////////////*/
// The typedef for the list of HMODULES that can possibly hold message
// resources.
typedef vector<HINSTANCE> HINSTVECTOR ;
// The address typedef.
typedef vector<ULONG> ADDRVECTOR ;

/*//////////////////////////////////////////////////////////////////////
                           File Scope Globals
//////////////////////////////////////////////////////////////////////*/
// The HMODULE vector.
#pragma warning(push)
#pragma warning(disable:4530)
static HINSTVECTOR g_HMODVector ;
#pragma warning(pop)

// The DiagAssert display options.
static DWORD g_DiagAssertOptions = DA_SHOWMSGBOX | DA_SHOWODS ;

// The symbol engine.
static CSymbolEngine g_cSym ;
// If TRUE, the symbol engine has been initialized.
static BOOL g_bSymIsInit = FALSE ;

// The handle for assertion file output.
static HANDLE g_hAssertFile = INVALID_HANDLE_VALUE ;

// The handle for tracing file output.
static HANDLE g_hTraceFile = INVALID_HANDLE_VALUE ;

/*//////////////////////////////////////////////////////////////////////
                         File Scope Prototypes
//////////////////////////////////////////////////////////////////////*/
// Handles doing the stack trace for DiagAssert.
void DoStackTrace ( LPTSTR szString  ,
                    DWORD  dwSize     ) ;

// The function that does the real assertions.
BOOL __stdcall RealAssert  ( DWORD  dwOverrideOpts  ,
                             LPCSTR szMsg           ,
                             BOOL   bAllowHalts      ) ;

/*//////////////////////////////////////////////////////////////////////
                            CODE STARTS HERE
//////////////////////////////////////////////////////////////////////*/

DWORD  __stdcall
    SetDiagAssertOptions ( DWORD dwOpts )
{
    if ( DA_USEDEFAULTS == dwOpts )
    {
        return ( DA_USEDEFAULTS ) ;
    }
    DWORD dwOld = g_DiagAssertOptions ;
    g_DiagAssertOptions = dwOpts ;
    return ( dwOld ) ;
}

HANDLE  __stdcall
    SetDiagAssertFile ( HANDLE hFile )
{
    HANDLE hRet = g_hAssertFile ;
    g_hAssertFile = hFile ;
    return ( hRet ) ;
}


BOOL  __stdcall
    AddDiagAssertModule ( HMODULE hMod )
{
    g_HMODVector.push_back ( hMod ) ;
    return ( TRUE ) ;
}

BOOL  __stdcall
    DiagAssertA ( DWORD  dwOverrideOpts ,
                  LPCSTR szMsg          ,
                  LPCSTR szFile         ,
                  DWORD  dwLine          )
{
    // First, save off the last error value.
    DWORD dwLastError = GetLastError ( ) ;

    // Format the C/C++ message.
    char szBuff [ 512 ] ;

    wsprintf ( szBuff               ,
               "File : %s\n"
               "Line : %d\n"
               "Expression : %s"    ,
               szFile               ,
               dwLine               ,
               szMsg                 ) ;

    // Set the error back and call the function that does all the work.
    SetLastError ( dwLastError ) ;
    return ( RealAssert ( dwOverrideOpts , szBuff , TRUE ) ) ;
}

BOOL  __stdcall
    DiagAssertW ( DWORD     dwOverrideOpts  ,
                  LPCWSTR   szMsg           ,
                  LPCSTR    szFile          ,
                  DWORD     dwLine           )
{
    // First, save off the last error value.
    DWORD dwLastError = GetLastError ( ) ;

    // Format the C/C++ message.
    char szBuff [ 512 ] ;

    wsprintf ( szBuff               ,
               "File : %s\n"
               "Line : %d\n"
               "Expression : %lS"   ,
               szFile               ,
               dwLine               ,
               szMsg                 ) ;

    // Set the error back and call the function that does all the work.
    SetLastError ( dwLastError ) ;
    return ( RealAssert ( dwOverrideOpts , szBuff , TRUE ) ) ;
}

BOOL  __stdcall
    DiagAssertVB ( DWORD   dwOverrideOpts  ,
                   BOOL    bAllowHalts     ,
                   LPCSTR  szMsg            )
{
    return ( RealAssert ( dwOverrideOpts , szMsg , bAllowHalts ) ) ;
}

// Turn off unreachable code error after ExitProcess.
#pragma warning ( disable : 4702 )

// The code that does the real assertion work.
BOOL __stdcall RealAssert  ( DWORD  dwOverrideOpts  ,
                             LPCSTR szMsg           ,
                             BOOL   bAllowHalts      )
{
    // The buffer used for the final message text.
    static char  szBuff [ DIAGASSERT_BUFFSIZE ] ;
    // The current position in szBuff ;
    LPSTR  pCurrPos = szBuff ;
    // The module name.
    char   szModName[ MAX_PATH + 1 ] ;
    // The decoded message from FormatMessage
    LPSTR  szFmtMsg = NULL ;
    // The options.
    DWORD  dwOpts = dwOverrideOpts ;
    // The last error value.  (Which is preserved across the call).
    DWORD  dwLastErr = GetLastError ( ) ;


    if ( DA_USEDEFAULTS == dwOverrideOpts )
    {
        dwOpts = g_DiagAssertOptions ;
    }

    // Look in any specified modules for the code.
    HINSTVECTOR::iterator loop ;
    for ( loop =  g_HMODVector.begin ( ) ;
          loop != g_HMODVector.end ( )   ;
          loop++                          )
    {
        if ( 0 != FormatMessageA ( FORMAT_MESSAGE_ALLOCATE_BUFFER    |
                                     FORMAT_MESSAGE_IGNORE_INSERTS   |
                                     FORMAT_MESSAGE_FROM_HMODULE      ,
                                   *loop                              ,
                                   dwLastErr                          ,
                                   0                                  ,
                                   (LPSTR)&szFmtMsg                   ,
                                   0                                  ,
                                   NULL                               ))
        {
            break ;
        }
    }

    // If the message was not translated, just look in the system.
    if ( NULL == szFmtMsg )
    {
        FormatMessageA ( FORMAT_MESSAGE_ALLOCATE_BUFFER    |
                           FORMAT_MESSAGE_IGNORE_INSERTS   |
                           FORMAT_MESSAGE_FROM_SYSTEM        ,
                         NULL                                ,
                         dwLastErr                           ,
                         0                                   ,
                         (LPSTR)&szFmtMsg                    ,
                         0                                   ,
                         NULL                                 ) ;
    }

    // Make sure the message got translated into something.
    LPSTR szRealLastErr ;
    if ( NULL != szFmtMsg )
    {
        szRealLastErr = szFmtMsg ;
    }
    else
    {
        szRealLastErr = "**Last error code does not exist!!!!" ;
    }

    // Get the module name.
    if ( 0 == GetModuleFileNameA ( NULL , szModName , MAX_PATH ) )
    {
        lstrcpy ( szModName , "<unknown application>" ) ;
    }

    // Build the message.
    pCurrPos += (wsprintfA ( szBuff                         ,
                             "Debug Assertion Failed!\n\n"
                             "Program : %s\n"
                             "%s\n"
                             "Last Error (0x%08X) : %s\n"   ,
                             szModName                      ,
                             szMsg                          ,
                             dwLastErr                      ,
                             szFmtMsg                        ) ) ;

    // Get rid of the allocated memory from FormatMessage.
    if ( NULL != szFmtMsg )
    {
        LocalFree ( (LPVOID)szFmtMsg ) ;
    }

    // Am I supposed to show the stack trace too?
    if ( DA_SHOWSTACKTRACE == ( DA_SHOWSTACKTRACE & dwOpts ) )
    {
        DoStackTrace ( pCurrPos ,
                       sizeof ( szBuff ) -
                                   ((DWORD)pCurrPos-(DWORD)szBuff) ) ;
    }

    // Is this supposed to go to ODS?
    if ( DA_SHOWODS == ( DA_SHOWODS & dwOpts ) )
    {
        OutputDebugStringA ( szBuff ) ;
    }

    // If the file handle is something, write to it.  I do not do any
    // error checking on purpose.
    if ( INVALID_HANDLE_VALUE != g_hAssertFile )
    {
        DWORD dwWritten ;
        WriteFile ( g_hAssertFile       ,
                    szBuff              ,
                    lstrlenA ( szBuff ) ,
                    &dwWritten          ,
                    NULL                 ) ;
    }

    // Check out the kind of buttons I am supposed to do.
    UINT uiType = MB_TASKMODAL | MB_SETFOREGROUND | MB_ICONERROR ;
    if ( TRUE == bAllowHalts )
    {
        uiType |= MB_ABORTRETRYIGNORE ;
    }
    else
    {
        uiType |= MB_OK ;
    }

    // By default, treat the return as an IGNORE.  This works best in
    // the case the user does not want the MessageBox.
    int iRet = IDIGNORE ;
    if ( DA_SHOWMSGBOX == ( DA_SHOWMSGBOX & dwOpts ) )
    {
        HWND hWndParent = GetActiveWindow ( ) ;
        if ( NULL != hWndParent )
        {
            hWndParent = GetLastActivePopup ( hWndParent ) ;
        }
        iRet = MessageBoxA ( hWndParent             ,
                            szBuff                  ,
                            "ASSERTION FAILURE..."  ,
                            uiType                   ) ;
    }

    // Put the incoming last error back.
    SetLastError ( dwLastErr ) ;

    // Figure out what to do on the return.
    if ( ( IDIGNORE == iRet ) || ( IDOK == iRet ) )
    {
        return ( FALSE ) ;
    }
    if ( IDRETRY == iRet )
    {
        // This will trigger DebugBreak!!
        return ( TRUE ) ;
    }

    // The return has to be Abort....
    ExitProcess ( (UINT)-1 ) ;
    return ( TRUE ) ;
}
// Turn on unreachable code error
#pragma warning ( default : 4702 )

HANDLE  __stdcall
    SetDiagOutputFile ( HANDLE hFile )
{
    HANDLE hRet = g_hTraceFile ;
    g_hTraceFile = hFile ;
    return ( hRet ) ;
}

void 
    DiagOutputA ( LPCSTR szFmt , ... )
{
    // Never corrupt the last error value.
    DWORD dwLastError = GetLastError ( ) ;

    static char szOutBuff [ 1024 ] ;

    va_list  args ;

    va_start ( args , szFmt ) ;

    wvsprintfA ( szOutBuff , szFmt , args ) ;

    OutputDebugStringA ( szOutBuff ) ;

    if ( INVALID_HANDLE_VALUE != g_hTraceFile )
    {
        DWORD dwWritten ;
        WriteFile ( g_hTraceFile           ,
                    szOutBuff              ,
                    lstrlenA ( szOutBuff ) ,
                    &dwWritten             ,
                    NULL                    ) ;
    }

    va_end ( args ) ;

    SetLastError ( dwLastError ) ;
}

void 
    DiagOutputW ( LPCWSTR szFmt , ... )
{
    // Never corrupt the last error value.
    DWORD dwLastError = GetLastError ( ) ;

    static wchar_t szOutBuff [ 1024 ] ;

    va_list  args ;

    va_start ( args , szFmt ) ;

    wvsprintfW ( szOutBuff , szFmt , args ) ;

    OutputDebugStringW ( szOutBuff ) ;

    if ( INVALID_HANDLE_VALUE != g_hTraceFile )
    {
        DWORD dwWritten ;
        WriteFile ( g_hTraceFile           ,
                    szOutBuff              ,
                    lstrlenW ( szOutBuff ) ,
                    &dwWritten             ,
                    NULL                    ) ;
    }

    va_end ( args ) ;

    SetLastError ( dwLastError ) ;
}

void  __stdcall
    DiagOutputVB ( LPCSTR szMsg )
{
    DiagOutputA ( szMsg ) ;
}


static DWORD __stdcall GetModBase ( HANDLE hProcess , DWORD dwAddr )
{
    // Check in the symbol engine first.
    IMAGEHLP_MODULE stIHM ;

    // This is what the MFC stack trace routines forgot to do so their
    // code will not get the info out of the symbol engine.
    stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;

    // Check to see if the module is already loaded.
    if ( g_cSym.SymGetModuleInfo ( dwAddr , &stIHM ) )
    {
        return ( stIHM.BaseOfImage ) ;
    }
    else
    {
        // The module is not loaded, so let's go fishing.
        MEMORY_BASIC_INFORMATION stMBI ;

        // Do the VirtualQueryEx to see if I can find the start of
        // this module.  Since the HMODULE is the start of a module
        // in memory, viola, this will give me the HMODULE.
        if ( 0 != VirtualQueryEx ( hProcess         ,
                                   (LPCVOID)dwAddr  ,
                                   &stMBI           ,
                                   sizeof ( stMBI )  ) )
        {
            // Try and load it.
            DWORD dwNameLen = 0 ;
            TCHAR szFile[ MAX_PATH ] ;

            // Using the address base for the memory location, try
            // to grab the module filename.
            dwNameLen = GetModuleFileName ( (HINSTANCE)
                                                stMBI.AllocationBase ,
                                            szFile                   ,
                                            MAX_PATH                  );

            HANDLE hFile = NULL ;

            if ( 0 != dwNameLen )
            {
                // Very cool, I found the DLL.  Now open it up for
                // reading.
                hFile = CreateFile ( szFile       ,
                                     GENERIC_READ    ,
                                     FILE_SHARE_READ ,
                                     NULL            ,
                                     OPEN_EXISTING   ,
                                     0               ,
                                     0                ) ;
            }
            // Go ahead and try to load the module anyway.
#ifdef _DEBUG
            DWORD dwRet =
#endif
            g_cSym.SymLoadModule ( hFile                            ,
                                   ( dwNameLen ? szFile : NULL )    ,
                                   NULL                             ,
                                   (DWORD)stMBI.AllocationBase      ,
                                   0                                 ) ;
#ifdef _DEBUG
            if ( 0 == dwRet )
            {
                TRACE ( "SymLoadModule failed : 0x%08X\n" ,
                        GetLastError ( )                   ) ;
            }
#endif  // _DEBUG
            return ( (DWORD)stMBI.AllocationBase ) ;
        }
    }
    return ( 0 ) ;
}

static DWORD ConvertAddress ( DWORD dwAddr , LPTSTR szOutBuff )
{
    char szTemp [ MAX_PATH + sizeof ( IMAGEHLP_SYMBOL ) ] ;

    PIMAGEHLP_SYMBOL pIHS = (PIMAGEHLP_SYMBOL)&szTemp ;

    IMAGEHLP_MODULE stIHM ;

    LPTSTR pCurrPos = szOutBuff ;

    ZeroMemory ( pIHS , MAX_PATH + sizeof ( IMAGEHLP_SYMBOL ) ) ;
    ZeroMemory ( &stIHM , sizeof ( IMAGEHLP_MODULE ) ) ;

    pIHS->SizeOfStruct = sizeof ( IMAGEHLP_SYMBOL ) ;
    pIHS->Address = dwAddr ;
    pIHS->MaxNameLength = MAX_PATH ;

    stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;

    // Always stick the address in first.
    pCurrPos += wsprintf ( pCurrPos , _T ( "0x%08X " ) , dwAddr ) ;

    // Get the module name.
    if ( 0 != g_cSym.SymGetModuleInfo ( dwAddr , &stIHM ) )
    {
        // Strip off the path.
        LPTSTR szName = _tcsrchr ( stIHM.ImageName , _T ( '\\' ) ) ;
        if ( NULL != szName )
        {
            szName++ ;
        }
        else
        {
            szName = stIHM.ImageName ;
        }
        pCurrPos += wsprintf ( pCurrPos , _T ( "%s: " ) , szName ) ;
    }
    else
    {
        pCurrPos += wsprintf ( pCurrPos , _T ( "<unknown module>: " ) );
    }

    // Get the function.
    DWORD dwDisp ;
    if ( 0 != g_cSym.SymGetSymFromAddr ( dwAddr , &dwDisp , pIHS ) )
    {
        if ( 0 == dwDisp )
        {
            pCurrPos += wsprintf ( pCurrPos , _T ( "%s" ) , pIHS->Name);
        }
        else
        {
            pCurrPos += wsprintf ( pCurrPos               ,
                                   _T ( "%s + %d bytes" ) ,
                                   pIHS->Name             ,
                                   dwDisp                  ) ;
        }

        // If I got a symbol, give the source and line a whirl.
        IMAGEHLP_LINE stIHL ;

        ZeroMemory ( &stIHL , sizeof ( IMAGEHLP_LINE ) ) ;

        stIHL.SizeOfStruct = sizeof ( IMAGEHLP_LINE ) ;

        if ( 0 != g_cSym.SymGetLineFromAddr ( dwAddr  ,
                                              &dwDisp ,
                                              &stIHL   ) )
        {
            // Put this on the next line and indented a bit.
            pCurrPos += wsprintf ( pCurrPos                  ,
                                  _T ( "\n\t\t%s, Line %d" ) ,
                                  stIHL.FileName             ,
                                  stIHL.LineNumber            ) ;
            if ( 0 != dwDisp )
            {
                pCurrPos += wsprintf ( pCurrPos             ,
                                       _T ( " + %d bytes" ) ,
                                       dwDisp                ) ;
            }
        }
    }
    else
    {
        pCurrPos += wsprintf ( pCurrPos , _T ( "<unknown symbol>" ) ) ;
    }

    // Tack on a CRLF.
    pCurrPos += wsprintf ( pCurrPos , _T ( "\n" ) ) ;

    return ( pCurrPos - szOutBuff ) ;
}


extern BOOL __stdcall CH_ReadProcessMemory ( HANDLE                      ,
                                      LPCVOID lpBaseAddress       ,
                                      LPVOID  lpBuffer            ,
                                      DWORD   nSize               ,
                                      LPDWORD lpNumberOfBytesRead  );

void DoStackTrace ( LPTSTR szString  ,
                    DWORD  dwSize     )
{
    HANDLE hProcess = GetCurrentProcess ( ) ;
    HANDLE hThread  = GetCurrentThread  ( ) ;

    // If the symbol engine is not initialized, do it now.
    if ( FALSE == g_bSymIsInit )
    {
        DWORD dwOpts = SymGetOptions ( ) ;

        // Turn on load lines.
        SymSetOptions ( dwOpts                |
                        SYMOPT_LOAD_LINES      ) ;

        if ( FALSE == g_cSym.SymInitialize ( hProcess ,
                                             NULL     ,
                                             FALSE     ) )
        {
#ifdef _DEBUG
            TRACE ( "DiagAssert : Unable to initialize the "
                    "symbol engine!!!\n" ) ;
            DebugBreak ( ) ;
#endif
        }
        else
        {
            g_bSymIsInit = TRUE ;
        }
    }

    // The symbol engine is initialized so do the stack walk.

    // The array of addresses.
#pragma warning(push)
#pragma warning(disable:4530)
    ADDRVECTOR vAddrs ;
#pragma warning(pop)

    // The thread information.
    CONTEXT    stCtx  ;

    stCtx.ContextFlags = CONTEXT_FULL ;

    // Get the thread context.  Since I am doing this on the CURRENT
    // executing thread, the context will be from down in the bowls of
    // KERNEL32.DLL.  Probably GetThreadContext itself.
    if ( GetThreadContext ( GetCurrentThread ( ) , &stCtx ) )
    {
        STACKFRAME stFrame ;
        DWORD      dwMachine ;

        ZeroMemory ( &stFrame , sizeof ( STACKFRAME ) ) ;

        stFrame.AddrPC.Mode = AddrModeFlat ;

#if defined (_M_IX86)
        dwMachine                = IMAGE_FILE_MACHINE_I386 ;

        stFrame.AddrPC.Offset    = stCtx.Eip    ;
        stFrame.AddrStack.Offset = stCtx.Esp    ;
        stFrame.AddrFrame.Offset = stCtx.Ebp    ;
        stFrame.AddrPC.Mode		 = AddrModeFlat ;
        stFrame.AddrStack.Mode   = AddrModeFlat ;
        stFrame.AddrFrame.Mode   = AddrModeFlat ;

#elif defined (_M_ALPHA)
        dwMachine                = IMAGE_FILE_MACHINE_ALPHA ;
        stFrame.AddrPC.Offset    = (unsigned long)stCtx.Fir ;
#else
#error ( "Unknown machine!" )
#endif

        // Loop for the first 512 stack elements.
        for ( DWORD i = 0 ; i < 512 ; i++ )
        {
            if ( FALSE == StackWalk ( dwMachine              ,
                                      hProcess               ,
                                      hThread                ,
                                      &stFrame               ,
                                      &stCtx                 ,
									  (PREAD_PROCESS_MEMORY_ROUTINE)CH_ReadProcessMemory ,
                                      SymFunctionTableAccess ,
                                      SymGetModuleBase       ,
                                      NULL                    ) )
            {
                break ;
            }
            // Also check that the address is not zero.  Sometimes
            // StackWalk returns TRUE with a frame of zero.
            if ( 0 != stFrame.AddrPC.Offset )
            {
                vAddrs.push_back ( stFrame.AddrPC.Offset ) ;
            }
        }

        // Now start converting the addresses.
        DWORD dwSizeLeft = dwSize ;
        DWORD dwSymSize ;

        TCHAR szSym [ MAX_PATH * 2 ] ;
        LPTSTR szCurrPos = szString ;

        BOOL bSeenDiagAssert = FALSE ;
        ADDRVECTOR::iterator loop ;
        for ( loop =  vAddrs.begin ( ) ;
              loop != vAddrs.end ( )   ;
              loop++                     )
        {
            dwSymSize = ConvertAddress ( *loop , szSym ) ;
            // Throw out everything with DiagAssert.cpp in it.
            if ( _tcsstr ( szSym , _T ( "DiagAssert.cpp" ) ) )
            {
                bSeenDiagAssert = TRUE ;
                continue ;
            }
            // Throw out anything before the functions in
            // DiagAssert.cpp
            if ( FALSE == bSeenDiagAssert )
            {
                continue ;
            }
            if ( dwSizeLeft < dwSymSize )
            {
                break ;
            }
            _tcscpy ( szCurrPos , szSym ) ;
            szCurrPos += dwSymSize ;
            dwSizeLeft -= dwSymSize ;
        }
    }
}
