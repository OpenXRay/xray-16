/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2001 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx.h"
#pragma warning(push)
#pragma warning(disable:4091) // 'typedef ': ignored on left of '' when no variable is declared
#include "MiniDump.h"
#include "StackTrace.h"
#pragma warning(pop)
#include <process.h>

// The structure I can package the data necessary to the dump writer thread.
typedef struct tag_DUMPTHREADPARAMS
{
    MINIDUMP_TYPE       eType;
    wchar_t *           szFileName;
    DWORD               dwThreadID;
    EXCEPTION_POINTERS* pExceptInfo;
    WriteMiniDumpResult eReturnValue;
    DWORD               dwMiniDumpWriteDumpLastError;
} DUMPTHREADPARAMS, *LPDUMPTHREADPARAMS;

// The dumper function.
unsigned __stdcall DumpThread(LPVOID pData)
{
    LPDUMPTHREADPARAMS pParams = (LPDUMPTHREADPARAMS)pData;

    // Create the file first.
    HANDLE hFile = CreateFileW(pParams->szFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION  stMDEI;
        MINIDUMP_EXCEPTION_INFORMATION* pMDEI = NULL;

        if (pParams->pExceptInfo != NULL)
        {
            stMDEI.ThreadId = pParams->dwThreadID;
            stMDEI.ExceptionPointers = pParams->pExceptInfo;
            stMDEI.ClientPointers = TRUE;
            pMDEI = &stMDEI;
        }
        // Got the file open.  Write it.
        BOOL bRet = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, pParams->eType, pMDEI, NULL, NULL);
        if (bRet == TRUE)
        {
            pParams->eReturnValue = WriteMiniDumpResult::DumpSucceeded;
        }
        else // Oops.
        {
            pParams->eReturnValue = WriteMiniDumpResult::MiniDumpWriteDumpFailed;
        }
        CloseHandle(hFile);
    }
    else // Could not open the file!
    {
        pParams->eReturnValue = WriteMiniDumpResult::OpenDumpFailed;
    }
    // Always save the last error value so I can set it in the original thread.
    pParams->dwMiniDumpWriteDumpLastError = GetLastError();
    return pParams->eReturnValue == WriteMiniDumpResult::DumpSucceeded;
}

WriteMiniDumpResult __stdcall WriteMiniDumpA(MINIDUMP_TYPE eType, char* szFileName, DWORD dwThread, EXCEPTION_POINTERS* pExceptInfo)
{
    // Check the string parameter because I am paranoid.
    if (IsBadStringPtrA(szFileName, MAX_PATH) == TRUE)
    {
        return WriteMiniDumpResult::BadParameter;
    }

    // The return value.
    WriteMiniDumpResult eRetVal = WriteMiniDumpResult::DumpSucceeded;

    // Allocate enough space to hold the converted string.
    int iLen = (lstrlenA(szFileName) + 1) * sizeof (wchar_t);
    wchar_t * pWideFileName = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, iLen);

    int iRet = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szFileName, -1, pWideFileName, iLen);
    if (iRet != 0)
    {
        // The conversion worked, call the wide function.
        eRetVal = WriteMiniDumpW(eType, pWideFileName, dwThread, pExceptInfo);
    }
    else
    {
        eRetVal = WriteMiniDumpResult::BadParameter;
    }
    if (pWideFileName != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pWideFileName);
    }
    return eRetVal;
}

WriteMiniDumpResult __stdcall WriteMiniDumpW(MINIDUMP_TYPE eType, wchar_t* szFileName, DWORD dwThread, EXCEPTION_POINTERS* pExceptInfo)
{
    // Check the string parameter because I am paranoid.  I can't check
    // the eType as that might change in the future.
    if (IsBadStringPtrW(szFileName, MAX_PATH) == TRUE)
    {
        return WriteMiniDumpResult::BadParameter;
    }
    // If an exception pointer blob was passed in.
    if (pExceptInfo != NULL)
    {
        if (IsBadReadPtr(pExceptInfo, sizeof(EXCEPTION_POINTERS)) == TRUE)
        {
            return WriteMiniDumpResult::BadParameter;
        }
    }

    // Package up the data for the dump writer thread.
    DUMPTHREADPARAMS stParams;
    stParams.eReturnValue = WriteMiniDumpResult::InvalidError;
    stParams.eType = eType;
    stParams.pExceptInfo = pExceptInfo;
    stParams.dwThreadID = dwThread;
    stParams.szFileName = szFileName;
    stParams.dwMiniDumpWriteDumpLastError = ERROR_SUCCESS;

    // Crank the writer thread.
    unsigned dwTID;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, DumpThread, &stParams, 0, &dwTID);
    if ((HANDLE)-1 != hThread)
    {
        // The thread is running.  Block until the thread ends.
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }
    else
    {
        stParams.dwMiniDumpWriteDumpLastError = GetLastError();
        stParams.eReturnValue = WriteMiniDumpResult::eDEATH_ERROR;
    }

    // Set the last error code based so it looks like this thread made
    // the call to MiniDumpWriteDump.
    SetLastError(stParams.dwMiniDumpWriteDumpLastError);
    return stParams.eReturnValue;
}
