/*----------------------------------------------------------------------
Bugslayer Column - MSDN Magazine - John Robbins
Copyright (c) 2000 - 2001 John Robbins -- All rights reserved.

The minidump function wrappers.  MiniDumpWriteDump can easily be called
to write out a dump of the current process.  The only problem is that
the dump for the current thread will show that it's coming from deep
inside MiniDumpWriteDump.  Unfortunately, with DBGHELP.DLL being updated
each release of WinDBG, we are missing symbols.  That means it's nearly
impossible to walk the stack back to your code.  These wrappers fix that
by spawning a thread to do the write.  While not a perfect solution, at
least you'll get a cleaner dump.  Keep in mind you might have to walk
the stack quite a bit to get back to your code, especially if used from
something like my SUPERASSERT (v2) dialog.

Note that these functions do NO checking if the older broken versions of
the mini dump commands are there.  The broken versions would hang if
called inside the process space.  Any version of DBGHELP.DLL from WinDBG
3.0.20 or later, or Windows XP will work just fine.

----------------------------------------------------------------------*/
#pragma once

#include <windows.h>
#include <DbgHelp.h>

/*//////////////////////////////////////////////////////////////////////
                         Typedefs and Structures
//////////////////////////////////////////////////////////////////////*/
// The return values for CreateCurrentProcMiniDump.
enum class WriteMiniDumpResult
{
    // Everything worked.
    DumpSucceeded,
    // DBGHELP.DLL could not be found at all in the path.
    DbghelpNotFound,
    // The mini dump exports are not in the version of DBGHELP.DLL in memory.
    DbghelpMissingExports,
    // A parameter was bad.
    BadParameter,
    // Unable to open the dump file requested.
    OpenDumpFailed,
    // MiniDumpWriteDump failed.  Call GetLastError to see why.
    MiniDumpWriteDumpFailed,
    // Death error.  Thread failed to crank up.
    eDEATH_ERROR,
    // The invalid error value.
    InvalidError,
};

/*----------------------------------------------------------------------
FUNCTION        :   CreateCurrentProcessMiniDump
DISCUSSION      :
    Creates a minidump of the current process.
PARAMETERS      :
    eType       - The type of mini dump to do.
    szFileName  - The complete path and filename to write the dump.
                  Traditionally, the extension for dump files is .DMP.
                  If the file exists, it will be overwritten.
    dwThread    - The optional id of the thread that crashed.
    pExceptInfo - The optional exception information.  This can be NULL
                  to indicate no exception information is to be added
                  to the dump.
RETURNS         :
    FALSE - Mini dump functions are not available.
    TRUE  - Mini dump functions are there.
----------------------------------------------------------------------*/
WriteMiniDumpResult __stdcall WriteMiniDumpA(MINIDUMP_TYPE eType, char* szFileName, DWORD dwThread, EXCEPTION_POINTERS* pExceptInfo);
WriteMiniDumpResult __stdcall WriteMiniDumpW(MINIDUMP_TYPE eType, wchar_t* szFileName, DWORD dwThread, EXCEPTION_POINTERS* pExceptInfo);

#ifdef UNICODE
#define WriteMiniDump WriteMiniDumpW
#else
#define WriteMiniDump WriteMiniDumpA
#endif
