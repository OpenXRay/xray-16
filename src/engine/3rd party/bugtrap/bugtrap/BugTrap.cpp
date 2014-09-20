/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: External routines and entry point.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "BugTrap.h"
#include "BugTrapUtils.h"
#include "BugTrapUI.h"
#include "Array.h"
#include "XmlReader.h"
#include "TextLogFile.h"
#include "XmlLogFile.h"
#include "Globals.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/// Critical section used for synchronizing log files management.
static CRITICAL_SECTION g_csMutualLogAccess;
/// Critical section used for synchronizing output to the console.
static CRITICAL_SECTION g_csConsoleAccess;
/// Critical section used for synchronous error handler execution.
static CRITICAL_SECTION g_csHandlerSync;
/// Event fired when pending log request is finished.
static HANDLE g_hLogRequestComplete = NULL;
/// True when logging activity is enabled.
static BOOL g_bLoggingEnabled = FALSE;
/// Number of active request to logging functions from different threads.
static DWORD g_dwNumLogRequests = 0;
/// Array of pointers to log descriptors.
static CArray<CLogFile*, CDynamicTraits<CLogFile*> > g_arrLogFiles;

#ifndef _MANAGED
/// Old filter of unhandled exception.
static PTOP_LEVEL_EXCEPTION_FILTER g_pfnOldExceptionFilter = NULL;
#endif

#if defined _CRTDBG_MAP_ALLOC && defined _DEBUG
/// Global memory state object. Tracks memory leaks.
static _CrtMemState g_MemState;
#endif

/**
 * @brief Request more privileges for current process.
 * @return true if operation was completed successfully.
 */
static BOOL RequestMorePrivileges(void)
{
	if (! g_bWinNT)
		return TRUE; // simulation for Win9x
	BOOL bResult = FALSE;
	HANDLE hCurrentProcess = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hCurrentProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		DWORD dwLength = 0;
		if (! GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwLength) &&
			GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			BYTE* pBuffer = new BYTE[dwLength];
			if (pBuffer)
			{
				if (GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLength, &dwLength))
				{
					TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
					for (DWORD dwPos = 0; dwPos < pPrivs->PrivilegeCount; ++dwPos)
						pPrivs->Privileges[dwPos].Attributes |= SE_PRIVILEGE_ENABLED;
					bResult = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
				}
				delete[] pBuffer;
			}
		}
		CloseHandle(hToken);
	}
	return bResult;
}

/**
 * Stop logging activity.
 */
static void StopLogging(void)
{
	EnterCriticalSection(&g_csMutualLogAccess);
	g_bLoggingEnabled = FALSE;
	while (g_dwNumLogRequests > 0)
	{
		LeaveCriticalSection(&g_csMutualLogAccess);
		WaitForSingleObject(g_hLogRequestComplete, INFINITE);
		EnterCriticalSection(&g_csMutualLogAccess);
	}
	LeaveCriticalSection(&g_csMutualLogAccess);
}

/**
 * Save all log link entries.
 * @param bCrash - true if crash has occurred.
 */
static void SaveLogLinkEntries(bool bCrash)
{
	int iNumLogLinks = g_arrLogLinks.GetCount();
	for (int iLogLinkPos = 0; iLogLinkPos < iNumLogLinks; ++iLogLinkPos)
	{
		CLogLink* pLogLink = g_arrLogLinks[iLogLinkPos];
		_ASSERTE(pLogLink != NULL);
		pLogLink->SaveEntries(bCrash);
	}
}

/**
 * Save entries in all open log files.
 * @param bCrash - true if crash has occurred.
 */
static void SaveLogFiles(bool bCrash)
{
	_ASSERTE(! g_bLoggingEnabled && ! g_dwNumLogRequests);
	int iNumLogFiles = g_arrLogFiles.GetCount();
	for (int iLogFilePos = 0; iLogFilePos < iNumLogFiles; ++iLogFilePos)
	{
		CLogFile* pLogFile = g_arrLogFiles[iLogFilePos];
		_ASSERTE(pLogFile != NULL);
		pLogFile->SaveEntries(bCrash);
	}
}
/**
 * Flush contents of all open log files.
 * @param bCrash - true if crash has occurred.
 */
static void FlushLogFiles(bool bCrash)
{
	StopLogging();
	SaveLogLinkEntries(bCrash);
	SaveLogFiles(bCrash);
}

/**
 * Log function prologue.
 * @return false if function can't be performed.
 */
static BOOL EnterLogFunction(void)
{
	EnterCriticalSection(&g_csMutualLogAccess);
	BOOL bLoggingEnabled = g_bLoggingEnabled;
	if (bLoggingEnabled)
		++g_dwNumLogRequests;
	LeaveCriticalSection(&g_csMutualLogAccess);
	return bLoggingEnabled;
}

/**
 * Log function epilogue.
 */
static void LeaveLogFunction(void)
{
	EnterCriticalSection(&g_csMutualLogAccess);
	_ASSERTE(g_dwNumLogRequests > 0);
	--g_dwNumLogRequests;
	SetEvent(g_hLogRequestComplete);
	LeaveCriticalSection(&g_csMutualLogAccess);
}

/**
 * Log function prologue.
 * @param pLogFile - log file object.
 * @return false if function can't be performed.
 */
static inline BOOL EnterLogFunction(CLogFile* pLogFile)
{
	if (! pLogFile || ! EnterLogFunction())
		return FALSE;
	if (g_arrLogFiles.LSearch(pLogFile) < 0)
	{
		LeaveLogFunction();
		return FALSE;
	}
	pLogFile->CaptureObject();
	return TRUE;
}

/**
 * Log function epilogue.
 * @param pLogFile - log file object.
 */
static inline void LeaveLogFunction(CLogFile* pLogFile)
{
	_ASSERTE(pLogFile != NULL);
	pLogFile->ReleaseObject();
	LeaveLogFunction();
}

/**
 * Free global variables to excludes this memory from the dump of memory leaks.
 */
static void FreeGlobalData(void)
{
	// Clear log files.
	g_arrLogLinks.DeleteAll(true);
	g_arrLogFiles.DeleteAll(true);
	// Deallocate user messages.
	g_strUserMessage.Free();
	g_strFirstIntroMesage.Free();
	g_strSecondIntroMesage.Free();
}

/**
 * Prepare report engine.
 * @param rParams - symbolic engine parameters.
 */
static void InitSymEngine(CSymEngine::CEngineParams& rParams)
{
	// Request additional privileges.
	RequestMorePrivileges();
#ifdef _MANAGED
	// Flush contents of all .NET trace listeners and close all log files.
	NetThunks::FlushTraceListeners();
#endif
	// Flush contents of all open log files ands disable logging.
	FlushLogFiles(true);
	_ASSERTE(! g_bLoggingEnabled && ! g_dwNumLogRequests);
	// All resource consuming objects are allocated dynamically.
	// So, they don't eat computer memory if application works as expected.
	if (g_pEnumProc == NULL)
		g_pEnumProc = new CEnumProcess();
	if (g_pSymEngine == NULL)
		g_pSymEngine = new CSymEngine(rParams);
	else
		g_pSymEngine->SetEngineParameters(rParams);
}

/**
 * Deallocate system engine object.
 */
static void FreeSymEngine(void)
{
	// Destroy allocated handles.
	delete g_pEnumProc;
	g_pEnumProc = NULL;
	delete g_pSymEngine;
	g_pSymEngine = NULL;
	delete g_pResManager;
	g_pResManager = NULL;
	// Certain actions may create global MAPI session object.
	delete g_pMapiSession;
	g_pMapiSession = NULL;
}

/**
 * Generic unhandled exception handler.
 * @param eExceptionType - type of exception.
 */
static void HandleException(CSymEngine::EXCEPTION_TYPE eExceptionType)
{
	// Initialize symbolic engine parameters.
	CSymEngine::CEngineParams params(g_pExceptionPointers, eExceptionType);
	// Flush log files and allocate symbolic engine.
	InitSymEngine(params);
	// Call user error handler before BugTrap user interface.
	if (g_pfnPreErrHandler != NULL)
		(*g_pfnPreErrHandler)(g_nPreErrHandlerParam);
#ifdef _MANAGED
	NetThunks::FireBeforeUnhandledExceptionEvent();
#endif
	// Read version info if application name is not specified.
	if (*g_szAppName == _T('\0'))
	{
#ifdef _MANAGED
		NetThunks::ReadVersionInfo();
#else
		BT_ReadVersionInfo(NULL);
#endif
	}
	// Execute BugTrap action.
	StartHandlerThread();
	// Call user error handler after BugTrap user interface.
#ifdef _MANAGED
	NetThunks::FireAfterUnhandledExceptionEvent();
#endif
	if (g_pfnPostErrHandler != NULL)
		(*g_pfnPostErrHandler)(g_nPostErrHandlerParam);
	// Deallocate system engine object.
	FreeSymEngine();
}

/**
 * Generic unhandled exception handler.
 * @param pExceptionPointers - pointer to the exception information.
 * @param eExceptionType - type of exception.
 * @return determines how the exception is handled.
 */
static LONG GenericFilter(PEXCEPTION_POINTERS pExceptionPointers, CSymEngine::EXCEPTION_TYPE eExceptionType)
{
	// Stack overflow is not handled by BugTrap.
	if (pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_CONTINUE_SEARCH;
	// Block other threads.
	EnterCriticalSection(&g_csHandlerSync);
	g_pExceptionPointers = pExceptionPointers;
	g_dwExceptionThreadID = GetCurrentThreadId();
	__try
	{
		// Handle the exception.
		HandleException(eExceptionType);
		// Free global data (to avoid false messages about memory leaks).
		FreeGlobalData();
	}
	// Catch any internal problems.
	__except (InternalFilter(GetExceptionInformation()))
	{
	}
#if defined _CRTDBG_MAP_ALLOC && defined _DEBUG
	// 1. Use DebugView of Mark Russinovich to check CRT output.
	// 2. Don't call _CrtDumpMemoryLeaks() because it warns about
	// memory allocated from static hash table used by CXmlReader.
	_CrtMemDumpAllObjectsSince(&g_MemState);
#endif
	// Force application to quit.
	HANDLE hCurrentProcess = GetCurrentProcess();
	TerminateProcess(hCurrentProcess, ~0u);
	// Unlock other threads
	// (this code won't be executed, but I have added it for clarity).
	LeaveCriticalSection(&g_csHandlerSync);
	// Exit from the exception filter.
	return EXCEPTION_EXECUTE_HANDLER;
}

/**
 * Write log entry to the file.
 * @param pLogFile - pointer to the log file info.
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param pszEntry - log entry text.
 */
static void WriteLogEntry(CLogFile* pLogFile, BUGTRAP_LOGLEVEL eLogLevel, CLogFile::ENTRY_MODE eEntryMode, PCTSTR pszEntry)
{
	if (pszEntry == NULL || ! EnterLogFunction(pLogFile))
		return;
	pLogFile->WriteLogEntry(eLogLevel, eEntryMode, g_csConsoleAccess, pszEntry);
	LeaveLogFunction(pLogFile);
}

/**
 * Write log entry to the file.
 * @param pLogFile - pointer to the log file info.
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param pszFormat - format string.
 * @param argList - variable argument list.
 */
static void WriteLogEntry(CLogFile* pLogFile, BUGTRAP_LOGLEVEL eLogLevel, CLogFile::ENTRY_MODE eEntryMode, PCTSTR pszFormat, va_list argList)
{
	if (pszFormat == NULL || ! EnterLogFunction(pLogFile))
		return;
	pLogFile->WriteLogEntryV(eLogLevel, eEntryMode, g_csConsoleAccess, pszFormat, argList);
	LeaveLogFunction(pLogFile);
}

/**
 * @brief Set default report path (report is located in Application Data folder by default).
 */
static void SetDefaultReportPath(void)
{
	TCHAR szAppDataPath[MAX_PATH];
	if (SHGetSpecialFolderPath(NULL, szAppDataPath, CSIDL_APPDATA, TRUE))
	{
		TCHAR szAppName[MAX_PATH];
		if (GetCanonicalAppName(szAppName, countof(szAppName), TRUE))
		{
			PathCombine(g_szReportFilePath, szAppDataPath, szAppName);
			PathAppend(g_szReportFilePath, _T("reports"));
		}
	}
}

/**
 * @brief Attach library to the process.
 */
static void AttachProcess(void)
{
	INITCOMMONCONTROLSEX InitCtrls;
	// Initialize synchronization objects.
	InitializeCriticalSection(&g_csHandlerSync);
	InitializeCriticalSection(&g_csMutualLogAccess);
	InitializeCriticalSection(&g_csConsoleAccess);
	// Synchronize log completion.
	g_hLogRequestComplete = CreateEvent(NULL, FALSE, FALSE, NULL); // non-signaled auto-reset event
	// Initialize common controls.
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	// Setup unhandled exception handler.
	BT_InstallSehFilter();
	// Logging is enabled.
	g_bLoggingEnabled = TRUE;
}

/**
 * @brief Detach library from the process.
 */
static void DetachProcess(void)
{
	// Flush log files to the disk.
	FlushLogFiles(false);
	_ASSERTE(! g_bLoggingEnabled && ! g_dwNumLogRequests);
	// Restore exception handler.
	BT_UninstallSehFilter();
	// Close log event.
	CloseHandle(g_hLogRequestComplete);
	g_hLogRequestComplete = NULL;
	// In managed world DllMain() is called after object destructors.
#ifndef _MANAGED
	// Free global data (to avoid false messages about memory leaks).
	FreeGlobalData();
#endif
	// Delete synchronization objects.
	DeleteCriticalSection(&g_csConsoleAccess);
	DeleteCriticalSection(&g_csMutualLogAccess);
	DeleteCriticalSection(&g_csHandlerSync);
}

/**
 * @param hModule - handle to the DLL module.
 * @param fdwReason - reason for calling function.
 * @param pvReserved - reserved.
 * @return true if initialization/de-initialization has been performed normally.
 */
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, PVOID pvReserved)
{
	pvReserved;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
#if defined _CRTDBG_MAP_ALLOC && defined _DEBUG
#ifndef _MANAGED
		// Avoid incorrect and annoying message about memory leaks
		// in static hash table used by CXmlReader.
		CXmlReader::InitStdEntities();
#endif
		// Watch for memory leaks.
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
		_CrtMemCheckpoint(&g_MemState);
#endif
		g_hInstance = hModule;
		AttachProcess();
		break;
	case DLL_PROCESS_DETACH:
		DetachProcess();
		g_hInstance = NULL;
#if defined _CRTDBG_MAP_ALLOC && defined _DEBUG
		// 1. Use DebugView by Mark Russinovich to check CRT output.
		// 2. Don't call _CrtDumpMemoryLeaks() because it warns about
		// memory allocated from static hash table used by CXmlReader.
		_CrtMemDumpAllObjectsSince(&g_MemState);
#endif
		break;
	}
	return TRUE;
}

/**
 * @return current BugTrap options.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetFlags(void)
{
	return g_dwFlags;
}

/**
 * @param dwFlags - new BugTrap options.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetFlags(DWORD dwFlags)
{
	g_dwFlags = dwFlags;
}

/**
 * @param pExceptionPointers - pointer to the exception information.
 * @return determines how the exception is handled.
 */
extern "C" BUGTRAP_API LONG CALLBACK BT_SehFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
	return GenericFilter(pExceptionPointers, CSymEngine::WIN32_EXCEPTION);
}

/**
 * @param pExceptionPointers - pointer to the exception information.
 * @return determines how the exception is handled.
 */
extern "C" BUGTRAP_API LONG CALLBACK BT_CppFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
	return GenericFilter(pExceptionPointers, CSymEngine::CPP_EXCEPTION);
}

/**
 * @param pExceptionPointers - pointer to the exception information.
 * @return determines how the exception is handled.
 */
extern "C" BUGTRAP_API LONG CALLBACK BT_NetFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
#ifdef _MANAGED
	return GenericFilter(pExceptionPointers, CSymEngine::NET_EXCEPTION);
#else
	pExceptionPointers;
	return 0;
#endif
}

/**
 * @return application name.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetAppName(void)
{
	return g_szAppName;
}

/**
 * @param pszAppName - application name.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetAppName(PCTSTR pszAppName)
{
	if (pszAppName)
		_tcscpy_s(g_szAppName, countof(g_szAppName), pszAppName);
	else
		*g_szAppName = _T('\0');
}

/**
 * @return user-defined application version number.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetAppVersion(void)
{
	return g_szAppVersion;
}

/**
 * @param pszAppVersion - user-defined application version number.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetAppVersion(PCTSTR pszAppVersion)
{
	if (pszAppVersion)
		_tcscpy_s(g_szAppVersion, countof(g_szAppVersion), pszAppVersion);
	else
		*g_szAppVersion = _T('\0');
}

/**
 * @return web address of product support site.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetSupportURL(void)
{
	return g_szSupportURL;
}

/**
 * @param pszSupportURL - web address of product support site.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetSupportURL(PCTSTR pszSupportURL)
{
	if (pszSupportURL)
		_tcscpy_s(g_szSupportURL, countof(g_szSupportURL), pszSupportURL);
	else
		*g_szSupportURL = _T('\0');
}

/**
 * @return e-mail address of product support.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetSupportEMail(void)
{
	return g_szSupportEMail;
}

/**
 * @param pszSupportEMail - e-mail address of product support.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetSupportEMail(PCTSTR pszSupportEMail)
{
	if (pszSupportEMail)
		_tcscpy_s(g_szSupportEMail, countof(g_szSupportEMail), pszSupportEMail);
	else
		*g_szSupportEMail = _T('\0');
}

/**
 * @return e-mail address of error notification.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetNotificationEMail(void)
{
	return g_szNotificationEMail;
}

/**
 * @param pszNotificationEMail - e-mail address of error notification.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetNotificationEMail(PCTSTR pszNotificationEMail)
{
	if (pszNotificationEMail)
		_tcscpy_s(g_szNotificationEMail, countof(g_szNotificationEMail), pszNotificationEMail);
	else
		*g_szNotificationEMail = _T('\0');
}

/**
 * @return host name of product support server where BugTrapServer is installed.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetSupportHost(void)
{
	return g_szSupportHost;
}

/**
 * @return port number of product support server where BugTrapServer is installed.
 */
extern "C" BUGTRAP_API SHORT APIENTRY BT_GetSupportPort(void)
{
	return g_nSupportPort;
}

/**
 * @param pszSupportHost - host name of product support server where BugTrapServer is installed.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetSupportHost(LPCTSTR pszSupportHost)
{
	if (pszSupportHost)
		_tcscpy_s(g_szSupportHost, countof(g_szSupportHost), pszSupportHost);
	else
		*g_szSupportHost = _T('\0');
}

/**
 * @param nSupportPort - port number of product support server where BugTrapServer is installed.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetSupportPort(SHORT nSupportPort)
{
	g_nSupportPort = nSupportPort;
}

/**
 * @param pszSupportHost - host name of product support server where BugTrapServer is installed.
 * @param nSupportPort - port number of product support server where BugTrapServer is installed.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetSupportServer(PCTSTR pszSupportHost, SHORT nSupportPort)
{
	BT_SetSupportHost(pszSupportHost);
	BT_SetSupportPort(nSupportPort);
}

extern "C" BUGTRAP_API void APIENTRY BT_ClearLogFiles(void)
{
	g_arrLogLinks.DeleteAll(true);
}

/**
 * @return number of log files attached to the report.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogFilesCount(void)
{
	return g_arrLogLinks.GetCount();
}

/**
 * @param pszLogFile - custom log file name.
 * @return index of matching long file name if present.
 */
static int FindLogLink(PCTSTR pszLogFile)
{
	TCHAR szLogFileName[MAX_PATH];
	if (GetCompleteLogFileName(szLogFileName, pszLogFile, NULL))
	{
		int nItemCount = g_arrLogLinks.GetCount();
		for (int nItemPos = 0; nItemPos < nItemCount; ++nItemPos)
		{
			CLogLink* pLogLink = g_arrLogLinks[nItemPos];
			_ASSERTE(pLogLink != NULL);
			if (_tcsicmp(pLogLink->GetLogFileName(), szLogFileName) == 0)
				return nItemPos;
		}
	}
	return -1;
}

/**
 * @param nLogFileIndexOrName - log entry index of name.
 * @param bGetByIndex - specifies type of @a nLogFileIndexOrName parameter.
 * @param peLogType - log entry type.
 * @param pdwLogEntrySize - log entry size.
 * @param pLogEntry - log entry data.
 * @return error status.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogFileEntry(INT_PTR nLogFileIndexOrName, BOOL bGetByIndex, BUGTRAP_LOGTYPE* peLogType, PDWORD pdwLogEntrySize, PVOID pLogEntry)
{
	if (bGetByIndex)
	{
		if (nLogFileIndexOrName < 0 || nLogFileIndexOrName >= g_arrLogLinks.GetCount())
			return ERROR_NO_MORE_ITEMS;
	}
	else
	{
		if (nLogFileIndexOrName == 0)
			return ERROR_BAD_ARGUMENTS;

		nLogFileIndexOrName = FindLogLink((PCTSTR)nLogFileIndexOrName);
		if (nLogFileIndexOrName < 0)
			return ERROR_NOT_FOUND;
	}
	CLogLink* pLogLink = g_arrLogLinks[nLogFileIndexOrName];
	_ASSERTE(pLogLink != NULL);
	BUGTRAP_LOGTYPE eLogType = pLogLink->GetLogType();
	if (pLogEntry != NULL)
	{
		if (peLogType == NULL ||
			*peLogType != eLogType ||
			pdwLogEntrySize == NULL)
		{
			return ERROR_BAD_ARGUMENTS;
		}
		switch (eLogType)
		{
		case BTLT_LOGFILE:
			if (*pdwLogEntrySize != sizeof(BUGTRAP_LOGFILEENTRY))
				return ERROR_INCORRECT_SIZE;
			{
				BUGTRAP_LOGFILEENTRY* pLogFileEntry = (BUGTRAP_LOGFILEENTRY*)pLogEntry;
				_tcscpy_s(pLogFileEntry->szLogFileName, countof(pLogFileEntry->szLogFileName), pLogLink->GetLogFileName());
			}
			break;
		case BTLT_REGEXPORT:
			if (*pdwLogEntrySize != sizeof(BUGTRAP_REGEXPORTENTRY))
				return ERROR_INCORRECT_SIZE;
			{
				BUGTRAP_REGEXPORTENTRY* pRegExportEntry = (BUGTRAP_REGEXPORTENTRY*)pLogEntry;
				CRegLink* pRegLink = (CRegLink*)pLogLink;
				_tcscpy_s(pRegExportEntry->szLogFileName, countof(pRegExportEntry->szLogFileName), pRegLink->GetLogFileName());
				_tcscpy_s(pRegExportEntry->szRegKey, countof(pRegExportEntry->szRegKey), pRegLink->GetRegKey());
			}
			break;
		default:
			_ASSERT(FALSE);
			return ERROR_INTERNAL_ERROR;
		}
	}
	else
	{
		if (peLogType != NULL)
			*peLogType = eLogType;
		if (pdwLogEntrySize != NULL)
		{
			switch (eLogType)
			{
			case BTLT_LOGFILE:
				*pdwLogEntrySize = sizeof(BUGTRAP_LOGFILEENTRY);
				break;
			case BTLT_REGEXPORT:
				*pdwLogEntrySize = sizeof(BUGTRAP_REGEXPORTENTRY);
				break;
			default:
				_ASSERT(FALSE);
				return ERROR_INTERNAL_ERROR;
			}
		}
	}
	return ERROR_SUCCESS;
}

/**
 * @param pszLogFile - custom log file name.
 */
extern "C" BUGTRAP_API void APIENTRY BT_AddLogFile(PCTSTR pszLogFile)
{
	if (pszLogFile && *pszLogFile)
	{
		if (FindLogLink(pszLogFile) < 0)
		{
			CLogLink* pLogLink = new CLogLink(pszLogFile);
			if (pLogLink != NULL)
				g_arrLogLinks.AddItem(pLogLink);
		}
	}
}

/**
 * @param pszRegFile - custom log file name.
 * @param pszRegKey - registry key to be exported.
 */
extern "C" BUGTRAP_API void APIENTRY BT_AddRegFile(LPCTSTR pszRegFile, PCTSTR pszRegKey)
{
	if (pszRegFile && *pszRegFile)
	{
		if (FindLogLink(pszRegFile) < 0)
		{
			CRegLink* pRegLink = new CRegLink(pszRegFile, pszRegKey);
			if (pRegLink != NULL)
				g_arrLogLinks.AddItem(pRegLink);
		}
	}
}

/**
 * @param pszLogFile - custom log file name.
 */
extern "C" BUGTRAP_API void APIENTRY BT_DeleteLogFile(PCTSTR pszLogFile)
{
	if (pszLogFile && *pszLogFile)
	{
		int nItemPos = FindLogLink(pszLogFile);
		if (nItemPos >= 0)
			g_arrLogLinks.DeleteItem(nItemPos);
	}
}

/**
 * @return address of error handler called before BugTrap dialog.
 */
extern "C" BUGTRAP_API BT_ErrHandler APIENTRY BT_GetPreErrHandler(void)
{
	return g_pfnPreErrHandler;
}

/**
 * @param pfnPreErrHandler - address of error handler called before BugTrap dialog.
 * @param nPreErrHandlerParam - user-defined parameter of error handler called before BugTrap dialog.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetPreErrHandler(BT_ErrHandler pfnPreErrHandler, INT_PTR nPreErrHandlerParam)
{
	g_pfnPreErrHandler = pfnPreErrHandler;
	g_nPreErrHandlerParam = nPreErrHandlerParam;
}

/**
 * @return address of error handler called after BugTrap dialog.
 */
extern "C" BUGTRAP_API BT_ErrHandler APIENTRY BT_GetPostErrHandler(void)
{
	return g_pfnPostErrHandler;
}

/**
 * @param pfnPostErrHandler - address of error handler called after BugTrap dialog.
 * @param nPostErrHandlerParam - user-defined parameter of error handler called after BugTrap dialog.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetPostErrHandler(BT_ErrHandler pfnPostErrHandler, INT_PTR nPostErrHandlerParam)
{
	g_pfnPostErrHandler = pfnPostErrHandler;
	g_nPostErrHandlerParam = nPostErrHandlerParam;
}

/**
 * @param iHandle - log file handle.
 * @return custom log file name.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetLogFileName(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return NULL;
	PCTSTR pszLogFileName = pLogFile->GetLogFileName();
	LeaveLogFunction(pLogFile);
	return pszLogFileName;
}

/**
 * @param iHandle - log file handle.
 * @return maximum size of log file in records.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogSizeInEntries(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return 0;
	DWORD dwLogSizeInEntries = pLogFile->GetLogSizeInEntries();
	LeaveLogFunction(pLogFile);
	return dwLogSizeInEntries;
}

/**
 * @param iHandle - log file handle.
 * @param dwLogSizeInEntries - maximum size of log file in records; pass MAXDWORD for unlimited log.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetLogSizeInEntries(INT_PTR iHandle, DWORD dwLogSizeInEntries)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SetLogSizeInEntries(dwLogSizeInEntries);
	LeaveLogFunction(pLogFile);
}

/**
 * @param iHandle - log file handle.
 * @return maximum size of log file in records.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogSizeInBytes(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return 0;
	DWORD dwLogSizeInBytes = pLogFile->GetLogSizeInBytes();
	LeaveLogFunction(pLogFile);
	return dwLogSizeInBytes;
}

/**
 * @param iHandle - log file handle.
 * @param dwLogSizeInBytes - maximum size of log file in records; pass MAXDWORD for unlimited log.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetLogSizeInBytes(INT_PTR iHandle, DWORD dwLogSizeInBytes)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SetLogSizeInBytes(dwLogSizeInBytes);
	LeaveLogFunction(pLogFile);
}

/**
 * @param iHandle - log file handle.
 * @return current set of log flags.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogFlags(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return BTLF_NONE;
	DWORD dwLogFlags = pLogFile->GetLogFlags();
	LeaveLogFunction(pLogFile);
	return dwLogFlags;
}

/**
 * @param iHandle - log file handle.
 * @param dwLogFlags - set of log flags.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetLogFlags(INT_PTR iHandle, DWORD dwLogFlags)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SetLogFlags(dwLogFlags);
	LeaveLogFunction(pLogFile);
}

/**
 * @param iHandle - log file handle.
 * @return minimal log level accepted by tracing functions.
 */
extern "C" BUGTRAP_API BUGTRAP_LOGLEVEL APIENTRY BT_GetLogLevel(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return BTLL_NONE;
	BUGTRAP_LOGLEVEL eLogLevel = pLogFile->GetLogLevel();
	LeaveLogFunction(pLogFile);
	return eLogLevel;
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - minimal logl level accepted by tracing functions.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetLogLevel(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SetLogLevel(eLogLevel);
	LeaveLogFunction(pLogFile);
}

/**
 * @param iHandle - log file handle.
 * @return current echo mode.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetLogEchoMode(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return BTLE_NONE;
	DWORD dwLogEchoMode = pLogFile->GetLogEchoMode();
	LeaveLogFunction(pLogFile);
	return dwLogEchoMode;
}

/**
 * @param iHandle - log file handle.
 * @param dwLogEchoMode - new echo mode.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetLogEchoMode(INT_PTR iHandle, DWORD dwLogEchoMode)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SetLogEchoMode(dwLogEchoMode);
	LeaveLogFunction(pLogFile);
}

extern "C" BUGTRAP_API void CDECL BT_CallSehFilter(void)
{
	__try {
		RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
	} __except(BT_SehFilter(GetExceptionInformation())) {
	}
}

extern "C" BUGTRAP_API void CDECL BT_CallCppFilter(void)
{
	__try {
		RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
	} __except(BT_CppFilter(GetExceptionInformation())) {
	}
}

extern "C" BUGTRAP_API void CDECL BT_CallNetFilter(void)
{
#ifdef _MANAGED
	__try {
		RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
	} __except(BT_NetFilter(GetExceptionInformation())) {
	}
#endif
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszEntry - text of message.
 */
extern "C" BUGTRAP_API void APIENTRY BT_InsLogEntry(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszEntry)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_INSERT, pszEntry);
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszEntry - text of message.
 */
extern "C" BUGTRAP_API void APIENTRY BT_AppLogEntry(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszEntry)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_APPEND, pszEntry);
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszFormat - printf-like message format.
 */
extern "C" BUGTRAP_API void CDECL BT_AppLogEntryF(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_APPEND, pszFormat, argList);
	va_end(argList);
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszFormat - printf-like message format.
 * @param argList - variable length argument list.
 */
extern "C" BUGTRAP_API void APIENTRY BT_AppLogEntryV(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszFormat, va_list argList)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_APPEND, pszFormat, argList);
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszFormat - printf-like message format.
 */
extern "C" BUGTRAP_API void CDECL BT_InsLogEntryF(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_INSERT, pszFormat, argList);
	va_end(argList);
}

/**
 * @param iHandle - log file handle.
 * @param eLogLevel - log level number.
 * @param pszFormat - printf-like message format.
 * @param argList - variable length argument list.
 */
extern "C" BUGTRAP_API void APIENTRY BT_InsLogEntryV(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, PCTSTR pszFormat, va_list argList)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	WriteLogEntry(pLogFile, eLogLevel, CLogFile::EM_INSERT, pszFormat, argList);
}

/**
 * @param iHandle - log file handle.
 */
extern "C" BUGTRAP_API void APIENTRY BT_ClearLog(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	// remove entries from the memory
	pLogFile->FreeEntries();
	LeaveLogFunction(pLogFile);
}

/**
 * @return type of produced mini-dump.
 */
extern "C" BUGTRAP_API DWORD APIENTRY BT_GetDumpType(void)
{
	return g_eDumpType;
}

/**
 * @param dwDumpType - type of produced mini-dump.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetDumpType(DWORD dwDumpType)
{
	g_eDumpType = (MINIDUMP_TYPE)dwDumpType;
}

/**
 * @return the type of BugTrap action.
 */
extern "C" BUGTRAP_API BUGTRAP_ACTIVITY APIENTRY BT_GetActivityType(void)
{
	return g_eActivityType;
}

/**
 * @return format of error report.
 */
extern "C" BUGTRAP_API BUGTRAP_REPORTFORMAT APIENTRY BT_GetReportFormat(void)
{
	return g_eReportFormat;
}

/**
 * @param eReportFormat - format of error report.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetReportFormat(BUGTRAP_REPORTFORMAT eReportFormat)
{
	g_eReportFormat = eReportFormat;
}

/**
 * @param eActivityType - the type of BugTrap action.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetActivityType(BUGTRAP_ACTIVITY eActivityType)
{
	g_eActivityType = eActivityType;
}

/**
 * @return path to report file.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetReportFilePath(void)
{
	if (*g_szReportFilePath == _T('\0'))
		SetDefaultReportPath();
	return g_szReportFilePath;
}

/**
 * @param pszReportFilePath - new report path.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetReportFilePath(PCTSTR pszReportFilePath)
{
	if (pszReportFilePath && *pszReportFilePath)
		_tcscpy_s(g_szReportFilePath, countof(g_szReportFilePath), pszReportFilePath);
	else
		SetDefaultReportPath();
}

/**
 * @return name of MAPI profile.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetMailProfile(void)
{
	return g_szMailProfile;
}

/**
 * @param pszMailProfile - name of MAPI profile.
 * @param pszMailPassword - password of MAPI profile.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetMailProfile(PCTSTR pszMailProfile, PCTSTR pszMailPassword)
{
	if (pszMailProfile)
		_tcscpy_s(g_szMailProfile, countof(g_szMailProfile), pszMailProfile);
	else
		*g_szMailProfile = _T('\0');
	if (pszMailPassword)
		_tcscpy_s(g_szMailPassword, countof(g_szMailPassword), pszMailPassword);
	else
		*g_szMailPassword = _T('\0');
}

/**
 * @param pszLogFileName - pointer to log file name or NULL pointer for default log file name value.
 * @param eLogFormat - log file format (at this moment only text format is supported).
 * @return handle of opened log file or NULL value.
 */
extern "C" BUGTRAP_API INT_PTR APIENTRY BT_OpenLogFile(PCTSTR pszLogFileName, BUGTRAP_LOGFORMAT eLogFormat)
{
	CLogFile* pLogFile;
	switch (eLogFormat)
	{
	case BTLF_TEXT:
		pLogFile = new CTextLogFile();
		break;
	case BTLF_XML:
		pLogFile = new CXmlLogFile();
		break;
	default:
		_ASSERT(FALSE);
		return NULL;
	}
	BOOL bResult = FALSE;
	if (pLogFile != NULL)
	{
		EnterLogFunction();
		pLogFile->SetLogFileName(pszLogFileName);
		pszLogFileName = pLogFile->GetLogFileName();
		if (CreateParentFolder(pszLogFileName) &&
			pLogFile->LoadEntries())
		{
			EnterCriticalSection(&g_csMutualLogAccess);
			g_arrLogFiles.AddItem(pLogFile);
			LeaveCriticalSection(&g_csMutualLogAccess);
			bResult = TRUE;
		}
		LeaveLogFunction();
	}
	if (! bResult)
	{
		delete pLogFile;
		pLogFile = NULL;
	}
	return (INT_PTR)pLogFile;
}

/**
 * @param iHandle - log file handle.
 */
extern "C" BUGTRAP_API void APIENTRY BT_CloseLogFile(INT_PTR iHandle)
{
	BT_FlushLogFile(iHandle);
	// remove file entry from the list
	EnterCriticalSection(&g_csMutualLogAccess);
	int iLogFilePos = g_arrLogFiles.LSearch((CLogFile*)iHandle);
	if (iLogFilePos >= 0)
		g_arrLogFiles.DeleteItem(iLogFilePos);
	LeaveCriticalSection(&g_csMutualLogAccess);
}

/**
 * @param iHandle - log file handle.
 */
extern "C" BUGTRAP_API void APIENTRY BT_FlushLogFile(INT_PTR iHandle)
{
	CLogFile* pLogFile = (CLogFile*)iHandle;
	if (! EnterLogFunction(pLogFile))
		return;
	pLogFile->SaveEntries(false);
	LeaveLogFunction(pLogFile);
}

/**
 * @return user message.
 */
extern "C" BUGTRAP_API PCTSTR APIENTRY BT_GetUserMessage(void)
{
	return g_strUserMessage;
}

/**
 * @param pszUserMessage - user message.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetUserMessage(PCTSTR pszUserMessage)
{
	g_strUserMessage = pszUserMessage;
}

/**
 * @param dwErrorCode - Win32 error code.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetUserMessageFromCode(DWORD dwErrorCode)
{
	PTSTR pszMessageBuffer = NULL;
	DWORD dwNumChars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
	                                 NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
	                                 (PTSTR)&pszMessageBuffer, 0, NULL);
	CStrStream strTemp(dwNumChars ? 32 + dwNumChars : 128);
	TCHAR szErrorCode[32];
	_stprintf_s(szErrorCode, countof(szErrorCode), _T("0x%08lX - "), dwErrorCode);
	strTemp << szErrorCode;
	if (pszMessageBuffer)
	{
		TrimSpaces(pszMessageBuffer);
		strTemp << pszMessageBuffer;
		LocalFree(pszMessageBuffer);
	}
	else
	{
		TCHAR szErrorMessage[64];
		LoadString(g_hInstance, IDS_UNDEFINED_ERROR, szErrorMessage, countof(szErrorMessage));
		strTemp << szErrorMessage;
	}
	g_strUserMessage = strTemp;
}

/**
 * @param hModule - module instance handle. Can be set to NULL for the main executable.
 * @return true operation has been completed successfully.
 */
extern "C" BUGTRAP_API BOOL APIENTRY BT_ReadVersionInfo(HMODULE hModule)
{
	TCHAR szModuleFileName[MAX_PATH];
	if (GetModuleFileName(hModule, szModuleFileName, countof(szModuleFileName)) == 0)
		return FALSE;
	DWORD dwSize, dwHandle;
	dwSize = GetFileVersionInfoSize(szModuleFileName, &dwHandle);
	if (dwSize == 0)
		return FALSE;
	PBYTE pVersionInfo = new BYTE[dwSize];
	if (pVersionInfo == NULL)
		return FALSE;
	BOOL bResult = FALSE;
	struct LANGANDCODEPAGE
	{
		LANGID wLanguage;
		WORD wCodePage;
	};
	LANGANDCODEPAGE* pTranslation;
	UINT uTranslationLength;
	if (GetFileVersionInfo(szModuleFileName, dwHandle, dwSize, pVersionInfo) &&
		VerQueryValue(pVersionInfo, _T("\\VarFileInfo\\Translation"), (void**)&pTranslation, &uTranslationLength))
	{
		LANGID lidDefault = GetUserDefaultLangID();
		int iNumItems = uTranslationLength / sizeof(LANGANDCODEPAGE);
		int iItemPos = 0, iDefaultItem = -1, iNeutralItem = -1;
		while (iItemPos < iNumItems)
		{
			if (pTranslation[iItemPos].wLanguage == lidDefault)
				iDefaultItem = iItemPos;
			else if (pTranslation[iItemPos].wLanguage == MAKELANGID(LANG_NEUTRAL, LANG_NEUTRAL))
				iNeutralItem = iItemPos;
			++iItemPos;
		}
		if (iDefaultItem >= 0)
			iItemPos = iDefaultItem;
		else if (iNeutralItem >= 0)
			iItemPos = iNeutralItem;
		else if (iNumItems > 0)
			iItemPos = 0;
		else
			iItemPos = -1;
		if (iItemPos >= 0)
		{
			PTSTR pszValue;
			UINT uValueLength;
			TCHAR szSubBlock[128];
			_stprintf_s(szSubBlock, countof(szSubBlock), _T("\\StringFileInfo\\%04x%04x\\AppName"), pTranslation[iItemPos].wLanguage, pTranslation[iItemPos].wCodePage);
			if (VerQueryValue(pVersionInfo, szSubBlock, (void**)&pszValue, &uValueLength))
			{
				bResult = TRUE;
				BT_SetAppName(pszValue);
				_stprintf_s(szSubBlock, countof(szSubBlock), _T("\\StringFileInfo\\%04x%04x\\AppVersion"), pTranslation[iItemPos].wLanguage, pTranslation[iItemPos].wCodePage);
				if (VerQueryValue(pVersionInfo, szSubBlock, (void**)&pszValue, &uValueLength))
					BT_SetAppVersion(pszValue);
				else
					BT_SetAppVersion(NULL);
			}
		}
	}
	delete[] pVersionInfo;
	return bResult;
}

/**
 * @return the address of the previous exception filter established with the function.
 */
extern "C" BUGTRAP_API LPTOP_LEVEL_EXCEPTION_FILTER APIENTRY BT_InstallSehFilter(void)
{
#ifndef _MANAGED
	// Setup unhandled exception handler.
	LPTOP_LEVEL_EXCEPTION_FILTER pfnOldExceptionFilter = SetUnhandledExceptionFilter(BT_SehFilter);
	if (pfnOldExceptionFilter != BT_SehFilter)
		g_pfnOldExceptionFilter = pfnOldExceptionFilter;
	return g_pfnOldExceptionFilter;
#else
	return NULL;
#endif
}

extern "C" BUGTRAP_API void APIENTRY BT_UninstallSehFilter(void)
{
#ifndef _MANAGED
	SetUnhandledExceptionFilter(g_pfnOldExceptionFilter);
	g_pfnOldExceptionFilter = NULL;
#endif
}

/**
 * @param eDialogMessage - message type.
 * @return pointer to message text.
 */
static CStrHolder* GetDialogMessage(BUGTRAP_DIALOGMESSAGE eDialogMessage)
{
	switch (eDialogMessage)
	{
	case BTDM_INTRO1:
		return &g_strFirstIntroMesage;
	case BTDM_INTRO2:
		return &g_strSecondIntroMesage;
	default:
		return NULL;
	}
}

/**
 * @param eDialogMessage - message type.
 * @return message text.
 */
extern "C" BUGTRAP_API LPCTSTR APIENTRY BT_GetDialogMessage(BUGTRAP_DIALOGMESSAGE eDialogMessage)
{
	CStrHolder* pstrDialogMessage = GetDialogMessage(eDialogMessage);
	if (pstrDialogMessage == NULL || pstrDialogMessage->IsEmpty())
		return NULL;
	else
		return (PCTSTR)*pstrDialogMessage;
}

/**
 * @param eDialogMessage - message type.
 * @param pszMessage - message text.
 */
extern "C" BUGTRAP_API void APIENTRY BT_SetDialogMessage(BUGTRAP_DIALOGMESSAGE eDialogMessage, LPCTSTR pszMessage)
{
	CStrHolder* pstrDialogMessage = GetDialogMessage(eDialogMessage);
	if (pstrDialogMessage != NULL)
	{
		if (pszMessage && *pszMessage)
			*pstrDialogMessage = pszMessage;
		else
			pstrDialogMessage->Free();
	}
}

/**
 * @brief Parse registry key and return root key and sub-key.
 * @param pszRegKey - registry key path.
 * @param hKey - root key.
 * @param pszRootName - root name.
 * @param pszRegPath - registry path.
 */
static BOOL GetRootKey(PCTSTR pszRegKey, HKEY& hRootKey, PCTSTR& pszRootName, PCTSTR& pszRegPath)
{
	/// Registry key synonym.
	static struct REGKEY_SYNONYM
	{
		/// Registry key handle.
		HKEY hKey;
		/// Name of the key.
		PCTSTR pszName;
		/// Full name of the key.
		PCTSTR pszFullName;
	}
	arrRegKeySynonyms[] =
	{
		{ HKEY_CLASSES_ROOT,   _T("HKEY_CLASSES_ROOT"),   _T("HKEY_CLASSES_ROOT")   },
		{ HKEY_CLASSES_ROOT,   _T("HKCR"),                _T("HKEY_CLASSES_ROOT")   },
		{ HKEY_CURRENT_USER,   _T("HKEY_CURRENT_USER"),   _T("HKEY_CURRENT_USER")   },
		{ HKEY_CURRENT_USER,   _T("HKCU"),                _T("HKEY_CURRENT_USER")   },
		{ HKEY_LOCAL_MACHINE,  _T("HKEY_LOCAL_MACHINE"),  _T("HKEY_LOCAL_MACHINE")  },
		{ HKEY_LOCAL_MACHINE,  _T("HKLM"),                _T("HKEY_LOCAL_MACHINE")  },
		{ HKEY_USERS,          _T("HKEY_USERS"),          _T("HKEY_USERS")          },
		{ HKEY_USERS,          _T("HKU"),                 _T("HKEY_USERS")          },
		{ HKEY_CURRENT_CONFIG, _T("HKEY_CURRENT_CONFIG"), _T("HKEY_CURRENT_CONFIG") },
		{ HKEY_CURRENT_CONFIG, _T("HKCC"),                _T("HKEY_CURRENT_CONFIG") }
	};
	for (int iItemNum = 0; iItemNum < countof(arrRegKeySynonyms); ++iItemNum)
	{
		const REGKEY_SYNONYM* pRegKeySynonym = arrRegKeySynonyms + iItemNum;
		PCTSTR pszName = pRegKeySynonym->pszName;
		DWORD dwNameLength = (DWORD)_tcslen(pszName);
		if (_tcsnicmp(pszRegKey, pszName, dwNameLength) == 0)
		{
			if (pszRegKey[dwNameLength] == _T('\\'))
				pszRegPath = pszRegKey + dwNameLength + 1;
			else if (pszRegKey[dwNameLength] == _T('\0'))
				pszRegPath = pszRegKey + dwNameLength;
			else
				continue;
			hRootKey = pRegKeySynonym->hKey;
			pszRootName = pRegKeySynonym->pszFullName;
			return TRUE;
		}
	}
	return FALSE;
}

/// Registry export data.
struct REGEXPORT_DATA
{
	/// Contains full registry path.
	TCHAR szRegPathBuffer[MAX_PATH];
	/// Root name.
	PCTSTR pszRootName;
	/// File handle.
	HANDLE hFile;
	/// Output file buffer.
	TCHAR arrFileBuffer[1024];
	/// Position in the buffer.
	DWORD dwFileBufferPos;
	/// Dynamically allocated value buffer.
	PBYTE pbValueBuffer;
	/// Value size.
	DWORD dwValueSize;
	/// Size of value buffer.
	DWORD dwValueBufferSize;
#ifndef _UNICODE
	/// Auxiliary buffer.
	PBYTE pbAuxiliaryBuffer;
	/// Size of auxiliary value.
	DWORD dwAuxiliarySize;
	/// Size of auxiliary buffer.
	DWORD dwAuxiliaryBufferSize;
#endif
	/// Temporarily stored key or value name.
	TCHAR szName[MAX_PATH];
	/// Name size.
	DWORD dwNameSize;
	/// Temporary buffer.
	TCHAR szAuxiliaryBuffer[128];
	/// Temporary time buffer.
	FILETIME ftime;
};

/**
 * @brief Flush buffer to the disk if necessary.
 * @param rRegData - registry data block.
 */
static void FlushRegData(REGEXPORT_DATA& rRegData)
{
	if (rRegData.dwFileBufferPos > 0)
	{
		DWORD dwNumWritten;
		WriteFile(rRegData.hFile, rRegData.arrFileBuffer, rRegData.dwFileBufferPos * sizeof(TCHAR), &dwNumWritten, NULL);
		rRegData.dwFileBufferPos = 0;
	}
}

#ifdef _UNICODE

/**
 * @brief Write data to the buffer and flush buffer to the disk if necessary.
 * @param rRegData - registry data block.
 * @param pbData - binary data to be written.
 * @param dwDataSize - data size.
 * @return number of bytes written to the file.
 */
static DWORD WriteRegData(REGEXPORT_DATA& rRegData, const BYTE* pbData, DWORD dwDataSize)
{
	DWORD dwNumBytes = 0;
	while (dwDataSize > 0)
	{
		if (rRegData.dwFileBufferPos >= countof(rRegData.arrFileBuffer))
			FlushRegData(rRegData);
		rRegData.arrFileBuffer[rRegData.dwFileBufferPos++] = pbData[dwNumBytes++];
		--dwDataSize;
	}
	return dwNumBytes;
}

#endif

/**
 * @brief Write data to the buffer and flush buffer to the disk if necessary.
 * @param rRegData - registry data block.
 * @param pszText - text to be written.
 * @return number of characters written to the file.
 */
static DWORD WriteRegData(REGEXPORT_DATA& rRegData, PCTSTR pszText)
{
	DWORD dwNumChars = 0;
	while (*pszText)
	{
		if (rRegData.dwFileBufferPos >= countof(rRegData.arrFileBuffer))
			FlushRegData(rRegData);
		rRegData.arrFileBuffer[rRegData.dwFileBufferPos++] = *pszText++;
		++dwNumChars;
	}
	return dwNumChars;
}

/**
 * @brief Write data to the buffer and flush buffer to the disk if necessary.
 * @param rRegData - registry data block.
 * @param dwValueType - type code.
 * @param pbValueData - value data.
 * @param dwValueSize - value size.
 * @param dwPosition - initial cursor position.
 * @return number of characters written to the file.
 */
static DWORD WriteRegData(REGEXPORT_DATA& rRegData, DWORD dwValueType, PBYTE pbValueData, DWORD dwValueSize, DWORD dwPosition)
{
	DWORD dwNumChars;
	if (dwValueType != REG_NONE)
	{
		_stprintf_s(rRegData.szAuxiliaryBuffer, countof(rRegData.szAuxiliaryBuffer), _T("hex(%02x):"), dwValueType);
		dwNumChars = WriteRegData(rRegData, rRegData.szAuxiliaryBuffer);
	}
	else
		dwNumChars = WriteRegData(rRegData, _T("hex:"));
	dwPosition += dwNumChars;
	for (;;)
	{
		while (dwValueSize > 0 && dwPosition <= 76)
		{
			if (dwValueSize > 1)
				_stprintf_s(rRegData.szAuxiliaryBuffer, countof(rRegData.szAuxiliaryBuffer), _T("%02x,"), *pbValueData);
			else
				_stprintf_s(rRegData.szAuxiliaryBuffer, countof(rRegData.szAuxiliaryBuffer), _T("%02x"), *pbValueData);
			DWORD dwResult = WriteRegData(rRegData, rRegData.szAuxiliaryBuffer);
			dwNumChars += dwResult;
			dwPosition += dwResult;
			++pbValueData;
			--dwValueSize;
		}
		if (dwValueSize > 0)
		{
			dwNumChars += WriteRegData(rRegData, _T("\\\r\n  "));
			dwPosition = 2;
		}
		else
			break;
	}
	return dwNumChars;

}

/**
 * @brief Write data to the buffer and flush buffer to the disk if necessary.
 * @param rRegData - registry data block.
 * @param dwValueType - type code.
 * @param dwPosition - initial cursor position.
 * @return number of characters written to the file.
 */
static inline DWORD WriteRegData(REGEXPORT_DATA& rRegData, DWORD dwValueType, DWORD dwPosition)
{
	return WriteRegData(rRegData, dwValueType, rRegData.pbValueBuffer, rRegData.dwValueSize, dwPosition);
}

#ifndef _UNICODE

/**
 * @brief Convert ANSI text to Unicode.
 * @param rRegData - registry data block.
 * @return true if conversion was completed successfully.
 */
static BOOL ConvertRegBuffer(REGEXPORT_DATA& rRegData)
{
	int nNewDataSize = MultiByteToWideChar(CP_ACP, 0, (PCSTR)rRegData.pbValueBuffer, rRegData.dwValueSize, NULL, 0);
	int nNewDataSize2 = nNewDataSize * sizeof(WCHAR);
	if (nNewDataSize2 > (int)rRegData.dwAuxiliaryBufferSize)
	{
		PBYTE pbNewData = (PBYTE)malloc(nNewDataSize2);
		if (pbNewData != NULL)
		{
			if (rRegData.pbAuxiliaryBuffer != NULL)
				free(rRegData.pbAuxiliaryBuffer);
			rRegData.pbAuxiliaryBuffer = pbNewData;
			rRegData.dwAuxiliaryBufferSize = nNewDataSize2;
		}
		else
			return FALSE;
	}
	MultiByteToWideChar(CP_ACP, 0, (PCSTR)rRegData.pbValueBuffer, rRegData.dwValueSize, (PWSTR)rRegData.pbAuxiliaryBuffer, nNewDataSize);
	rRegData.dwAuxiliarySize = nNewDataSize2;
	return TRUE;
}

#endif

/**
 * @brief Export registry values to the file.
 * @param rRegData - registry data.
 * @param hKey - registry key.
 */
static void ExportRegValues(REGEXPORT_DATA& rRegData, HKEY hKey)
{
	for (DWORD dwRegIndex = 0; ; ++dwRegIndex)
	{
		DWORD dwValueType;
		rRegData.dwNameSize = countof(rRegData.szName);
		rRegData.dwValueSize = rRegData.dwValueBufferSize;
		LONG lResult = RegEnumValue(hKey, dwRegIndex, rRegData.szName, &rRegData.dwNameSize, NULL, &dwValueType, rRegData.pbValueBuffer, &rRegData.dwValueSize);
		if (lResult == ERROR_NO_MORE_ITEMS)
			break;
		if (lResult == ERROR_MORE_DATA)
		{
			DWORD dwNewDataSize = (rRegData.dwValueSize + 0x3FF) & ~0x3FF;
			PBYTE pbNewData = (PBYTE)malloc(dwNewDataSize);
			if (pbNewData != NULL)
			{
				free(rRegData.pbValueBuffer);
				rRegData.pbValueBuffer = pbNewData;
				rRegData.dwValueBufferSize = dwNewDataSize;
			}
			else
				continue;
			lResult = RegEnumValue(hKey, dwRegIndex, rRegData.szName, &rRegData.dwNameSize, NULL, &dwValueType, rRegData.pbValueBuffer, &rRegData.dwValueSize);
		}
		if (lResult == ERROR_SUCCESS)
		{
			DWORD dwNumChars = 0;
			if (*rRegData.szName)
			{
				dwNumChars += WriteRegData(rRegData, _T("\""));
				dwNumChars += WriteRegData(rRegData, rRegData.szName);
				dwNumChars += WriteRegData(rRegData, _T("\"="));
			}
			else
				dwNumChars += WriteRegData(rRegData, _T("@="));
			switch (dwValueType)
			{
			case REG_BINARY:
				WriteRegData(rRegData, (DWORD)REG_NONE, dwNumChars);
				break;
			case REG_SZ:
				WriteRegData(rRegData, _T("\""));
				WriteRegData(rRegData, (PTSTR)rRegData.pbValueBuffer);
				WriteRegData(rRegData, _T("\""));
				break;
			case REG_DWORD:
				_stprintf_s(rRegData.szAuxiliaryBuffer, countof(rRegData.szAuxiliaryBuffer), _T("dword:%08x"), *(PDWORD)rRegData.pbValueBuffer);
				WriteRegData(rRegData, rRegData.szAuxiliaryBuffer);
				break;
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
#ifdef _UNICODE
				WriteRegData(rRegData, dwValueType, dwNumChars);
#else
				if (ConvertRegBuffer(rRegData))
					WriteRegData(rRegData, dwValueType, rRegData.pbAuxiliaryBuffer, rRegData.dwAuxiliarySize, dwNumChars);
#endif
				break;
			default:
				WriteRegData(rRegData, dwValueType, dwNumChars);
			}
			WriteRegData(rRegData, _T("\r\n"));
		}
	}
}

/// Forward declaration of function that exports registry key to the file.
static void ExportRegKey(REGEXPORT_DATA& rRegData, HKEY hKey, DWORD dwRegPathLength);

/**
 * @brief Export registry keys to the file.
 * @param hKey - registry key.
 * @param dwRegPathLength - pre-calculated registry path length.
 * @param rRegData - registry data.
 */
static void ExportRegKeys(REGEXPORT_DATA& rRegData, HKEY hKey, DWORD dwRegPathLength)
{
	if (dwRegPathLength + 1 >= countof(rRegData.szRegPathBuffer))
		return;
	rRegData.szRegPathBuffer[dwRegPathLength] = _T('\\');
	++dwRegPathLength;
	PTSTR pszRegPathEnd = rRegData.szRegPathBuffer + dwRegPathLength;
	for (DWORD dwRegIndex = 0; ; ++dwRegIndex)
	{
		rRegData.dwNameSize = countof(rRegData.szName);
		LONG lResult = RegEnumKeyEx(hKey, dwRegIndex, rRegData.szName, &rRegData.dwNameSize, NULL, NULL, NULL, &rRegData.ftime);
		if (lResult == ERROR_NO_MORE_ITEMS)
			break;
		if (lResult == ERROR_SUCCESS)
		{
			_tcscpy_s(pszRegPathEnd, countof(rRegData.szRegPathBuffer) - dwRegPathLength, rRegData.szName);
			HKEY hNestedKey;
			if (RegOpenKeyEx(hKey, rRegData.szName, 0l, KEY_READ, &hNestedKey) == ERROR_SUCCESS)
			{
				ExportRegKey(rRegData, hNestedKey, dwRegPathLength + rRegData.dwNameSize);
				RegCloseKey(hNestedKey);
			}
		}
	}
	--dwRegPathLength;
	rRegData.szRegPathBuffer[dwRegPathLength] = _T('\0');
}

/**
 * @brief Export registry key to the file.
 * @param rRegData - registry data.
 * @param hKey - registry key.
 * @param dwRegPathLength - pre-calculated registry path length.
 */
static void ExportRegKey(REGEXPORT_DATA& rRegData, HKEY hKey, DWORD dwRegPathLength)
{
	WriteRegData(rRegData, _T("["));
	WriteRegData(rRegData, rRegData.pszRootName);
	WriteRegData(rRegData, _T("\\"));
	WriteRegData(rRegData, rRegData.szRegPathBuffer);
	WriteRegData(rRegData, _T("]\r\n"));
	ExportRegValues(rRegData, hKey);
	WriteRegData(rRegData, _T("\r\n"));
	ExportRegKeys(rRegData, hKey, dwRegPathLength);
}

/**
 * @brief Write error code & description to the REG-file.
 * @param rRegData - registry data.
 * @param pszRegKey - registry key that wasn't exported.
 * @param lResult - error code.
 */
static void WriteRegError(REGEXPORT_DATA& rRegData, PCTSTR pszRegKey, LONG lResult)
{
	WriteRegData(rRegData, _T("; Error: cannot open registry key ["));
	WriteRegData(rRegData, pszRegKey);
	WriteRegData(rRegData, _T("]\r\n; Error code: "));
	_stprintf_s(rRegData.szAuxiliaryBuffer, countof(rRegData.szAuxiliaryBuffer), _T("0x%08lX"), lResult);
	WriteRegData(rRegData, rRegData.szAuxiliaryBuffer);
	PTSTR pszMessageBuffer = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
	              NULL, lResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
	              (PTSTR)&pszMessageBuffer, 0, NULL);
	if (pszMessageBuffer)
	{
		TrimSpaces(pszMessageBuffer);
		WriteRegData(rRegData, _T("\r\n; Description: "));
		WriteRegData(rRegData, pszMessageBuffer);
		LocalFree(pszMessageBuffer);
	}
	WriteRegData(rRegData, _T("\r\n"));
}

/**
 * @param pszRegFileName - output reg-file name.
 * @param pszRegKey - registry key path.
 * @return > 0 - if registry key was exported; 0 - if registry key was not exported, but error info was stored to a file. < 0 - if function ws not able to allocate memory, create file, etc.
 */
extern "C" BUGTRAP_API int APIENTRY BT_ExportRegistryKey(LPCTSTR pszRegFile, LPCTSTR pszRegKey)
{
	int nResult = -1;
	HKEY hRootKey;
	PCTSTR pszRegPath;
	REGEXPORT_DATA RegData;
	if (GetRootKey(pszRegKey, hRootKey, RegData.pszRootName, pszRegPath))
	{
		TCHAR szRegFileName[MAX_PATH];
		if (GetCompleteLogFileName(szRegFileName, pszRegFile, _T(".reg")) &&
			CreateParentFolder(szRegFileName))
		{
			RegData.dwValueSize = 0;
			RegData.dwFileBufferPos = 0;
			RegData.dwValueBufferSize = 1024;
			RegData.pbValueBuffer = (PBYTE)malloc(RegData.dwValueBufferSize);
			if (RegData.pbValueBuffer)
			{
#ifndef _UNICODE
				RegData.pbAuxiliaryBuffer = NULL;
				RegData.dwAuxiliarySize = 0;
				RegData.dwAuxiliaryBufferSize = 0;
#endif
				RegData.hFile = CreateFile(szRegFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (RegData.hFile != INVALID_HANDLE_VALUE)
				{
#ifdef _UNICODE
					WriteRegData(RegData, g_arrUTF16LEPreamble, sizeof(g_arrUTF16LEPreamble));
#endif
					WriteRegData(RegData, _T("Windows Registry Editor Version 5.00\r\n\r\n"));
					_tcscpy_s(RegData.szRegPathBuffer, countof(RegData.szRegPathBuffer), pszRegPath);
					HKEY hKey;
					LONG lResult = RegOpenKeyEx(hRootKey, RegData.szRegPathBuffer, 0l, KEY_READ, &hKey);
					if (lResult == ERROR_SUCCESS)
					{
						ExportRegKey(RegData, hKey, _tcslen(RegData.szRegPathBuffer));
						RegCloseKey(hKey);
						nResult = +1;
					}
					else
					{
						WriteRegError(RegData, pszRegKey, lResult);
						nResult = 0;
					}
					FlushRegData(RegData);
					CloseHandle(RegData.hFile);
				}
				free(RegData.pbValueBuffer);
#ifndef _UNICODE
				if (RegData.pbAuxiliaryBuffer)
					free(RegData.pbAuxiliaryBuffer);
#endif
			}
		}
	}
	return nResult;
}

/**
 * Generate report file name based on user criteria.
 * @param pszFileName - user-supplied file name.
 * @param szFileNameBuffer - file name buffer that is used to store default report name.
 * @return pointer to generated report name.
 */
static PCTSTR GetReportFileName(PCTSTR pszFileName, PTSTR szFileNameBuffer)
{
	_ASSERTE(g_pSymEngine != NULL);
	bool bGenerateFileName = pszFileName == NULL || *pszFileName == _T('\0');
	if (bGenerateFileName || ! PathIsRoot(pszFileName))
	{
		PCTSTR pszReportFolder = BT_GetReportFilePath();
		if (bGenerateFileName)
		{
			TCHAR szDefaultReportName[MAX_PATH];
			g_pSymEngine->GetReportFileName(szDefaultReportName, countof(szDefaultReportName));
			PathCombine(szFileNameBuffer, pszReportFolder, szDefaultReportName);
		}
		else
			PathCombine(szFileNameBuffer, pszReportFolder, pszFileName);
		pszFileName = szFileNameBuffer;
	}
	return pszFileName;
}

/**
 * @param pszFileName - snapshot file name or NULL, if you want to generate file name automatically.
 * @param true if operation has been completed successfully.
 */
extern "C" BUGTRAP_API BOOL APIENTRY BT_MakeSnapshot(PCTSTR pszFileName)
{
	BOOL bFreeSymEngine;
	if (g_pExceptionPointers == NULL)
	{
		CSymEngine::CEngineParams params;
		// Initialize symbolic engine.
		InitSymEngine(params);
		bFreeSymEngine = TRUE;
	}
	else
		bFreeSymEngine = FALSE;
	BOOL bResult = g_pSymEngine != NULL && g_pEnumProc != NULL;
	if (bResult)
	{
		// Generate report file name when necessary.
		TCHAR szReportFilePath[MAX_PATH];
		pszFileName = GetReportFileName(pszFileName, szReportFilePath);
		// Create parent report folder.
		CreateParentFolder(pszFileName);
		// Store report files to user supplied location.
		bResult = g_pSymEngine->WriteReport(pszFileName, g_pEnumProc);
	}
	// Deallocate system engine object.
	if (bFreeSymEngine)
		FreeSymEngine();
	return bResult;
}
