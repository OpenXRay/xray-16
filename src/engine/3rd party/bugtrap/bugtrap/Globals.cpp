/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Global variables definitions.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Get the type of current platform.
 * @return true for Windows NT platform and false for Windows 9x platform.
 */
static BOOL IsNT(void)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
	return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

/// BugTrap module handle.
HINSTANCE g_hInstance = NULL;
/// Application name.
TCHAR g_szAppName[MAX_PATH] = _T("");
/// Application version number.
TCHAR g_szAppVersion[MAX_PATH] = _T("");
/// Web address of product support site.
TCHAR g_szSupportURL[MAX_PATH] = _T("");
/// E-mail address of product support.
TCHAR g_szSupportEMail[MAX_PATH] = _T("");
/// Host name of product support server where BugTrapServer is installed.
TCHAR g_szSupportHost[MAX_PATH] = _T("");
/// E-mail address of error notification service.
TCHAR g_szNotificationEMail[MAX_PATH] = _T("");
/// Name of MAPI profile.
TCHAR g_szMailProfile[MAX_PATH] = _T("");
/// Password of MAPI profile.
TCHAR g_szMailPassword[MAX_PATH] = _T("");
/// Port number of product support server where BugTrapServer is installed.
SHORT g_nSupportPort = 0;
/// Path to error report specified by user.
TCHAR g_szReportFilePath[MAX_PATH] = _T("");
/// Internal path to a folder with report files.
TCHAR g_szInternalReportFolder[MAX_PATH] = _T("");
/// Internal path to single report file.
TCHAR g_szInternalReportFilePath[MAX_PATH] = _T("");
/// BugTrap configuration flags.
DWORD g_dwFlags = BTF_NONE;
/// Type of action which is performed in response to the error.
BUGTRAP_ACTIVITY g_eActivityType = BTA_SHOWUI;
/// Type of produced mini-dump. See @a MINIDUMP_TYPE for details.
MINIDUMP_TYPE g_eDumpType = MiniDumpWithDataSegs;
/// Format of error report.
BUGTRAP_REPORTFORMAT g_eReportFormat = BTRF_XML;
/// Address of error handler called before BugTrap dialog.
BT_ErrHandler g_pfnPreErrHandler = NULL;
/// User-defined parameter of error handler called before BugTrap dialog.
INT_PTR g_nPreErrHandlerParam = 0;
/// Address of error handler called after BugTrap dialog.
BT_ErrHandler g_pfnPostErrHandler = NULL;
/// User-defined parameter of error handler called after BugTrap dialog.
INT_PTR g_nPostErrHandlerParam = 0;
/// Pointer to the exception information.
PEXCEPTION_POINTERS g_pExceptionPointers = NULL;
/// Exception thread ID.
DWORD g_dwExceptionThreadID = 0;
/// True if application runs on Windows NT platform.
BOOL g_bWinNT = IsNT();
/// Custom resources manager.
CResManager* g_pResManager = NULL;
/// Pointer to Simple MAPI session object.
CMapiSession* g_pMapiSession = NULL;
/// Pointer to symbolic engine.
CSymEngine* g_pSymEngine = NULL;
/// Pointer to process enumerator.
CEnumProcess* g_pEnumProc = NULL;
/// Array of custom log file names attached to the report.
CArray<CLogLink*, CDynamicTraits<CLogLink*> > g_arrLogLinks;
/// User defined message printed to the log file.
CStrHolder g_strUserMessage;
/// 1st introduction message displayed on the dialog.
CStrHolder g_strFirstIntroMesage;
/// 2nd introduction message displayed on the dialog.
CStrHolder g_strSecondIntroMesage;
