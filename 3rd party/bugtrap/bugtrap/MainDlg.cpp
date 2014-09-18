/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: BugTrap main dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "BugTrapUI.h"
#include "WaitCursor.h"
#include "MainDlg.h"
#include "MachineStateDlg.h"
#include "MachineInfoDlg.h"
#include "PreviewDlg.h"
#include "AboutDlg.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/// Stack trace list column identifiers.
enum STACK_WIN32_COLUMN_ID
{
	/// Stack frame address.
	CID_WIN32_ENTRY_ADDRESS,
	/// Function name.
	CID_WIN32_ENTRY_FUNCTION,
	/// Source file name.
	CID_WIN32_ENTRY_FILE,
	/// Entry line number.
	CID_WIN32_ENTRY_LINE,
	/// Module file name.
	CID_WIN32_ENTRY_MODULE
};

#ifdef _MANAGED

/// Stack trace list column identifiers.
enum STACK_NET_COLUMN_ID
{
	/// Type name.
	CID_NET_ENTRY_TYPE,
	/// Method name.
	CID_NET_ENTRY_METHOD,
	/// Source file name.
	CID_NET_ENTRY_FILE,
	/// Entry line number.
	CID_NET_ENTRY_LINE,
	/// Assembly name.
	CID_NET_ENTRY_ASSEMBLY
};

#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Tool-tip window handle.
static HWND g_hwndToolTip = NULL;
/// URL hyper-link control pointing to support site.
static CHyperLink g_hlURL;

#include "stdlib.h"

// Vista uses this hook for old-style save dialog
UINT_PTR CALLBACK OFNHookProcOldStyle(HWND , UINT , WPARAM , LPARAM )
{
	// let default hook work on this message
	return 0;
}

bool IsVista ()
{

	return true;

}

/**
 * @brief Save bug report on the disk.
 * @param hwndParent - parent window handle.
 */
static void SaveReport(HWND hwndParent)
{
// Was used for debugging:
// 	{
// 		TCHAR szFileName[400] = "";
// 		OPENFILENAME ofn;
// 		ZeroMemory(&ofn, sizeof(OPENFILENAME));
// 		ofn.lStructSize = sizeof(OPENFILENAME);
// 		ofn.lpfnHook = OFNHookProcOldStyle;
// 		ofn.hwndOwner = hwndParent;
// 		ofn.lpstrFile = szFileName;
// 		ofn.nMaxFile = 400;
// 		ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
// 
// 		ofn.lpstrFilter = _T("Zip Archives\0*.zip\0All Files\0*.*\0");
// 		ofn.lpstrDefExt = _T("zip");
// 
// 		GetSaveFileName(&ofn);
// 
// 		DWORD error = CommDlgExtendedError();
// 		char error_buffer[1000];
// 		_ltoa_s(error, error_buffer, 1000, 10);
// 		MessageBox(0, error_buffer, TEXT("error id"), 0);
// 	}

	TCHAR szFileName[MAX_PATH] = _T("");
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndParent;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = countof(szFileName);
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	OSVERSIONINFOEX	os_version_info;
	os_version_info.dwOSVersionInfoSize	= sizeof(os_version_info);
	GetVersionEx	((LPOSVERSIONINFO)&os_version_info);
	if (os_version_info.dwMajorVersion == 6)// && !os_version_info.wServicePackMajor)
	{
		ofn.Flags |= OFN_ENABLEHOOK;
		ofn.lpfnHook = OFNHookProcOldStyle;
	}

	if (g_dwFlags & BTF_DETAILEDMODE)
	{
		ofn.lpstrFilter = _T("Zip Archives\0*.zip\0All Files\0*.*\0");
		ofn.lpstrDefExt = _T("zip");
	}
	else if (g_eReportFormat == BTRF_TEXT)
	{
		ofn.lpstrFilter = _T("Log Files\0*.log\0Text Files\0*.txt\0All Files\0*.*\0");
		ofn.lpstrDefExt = _T("log");
	}
	else if (g_eReportFormat == BTRF_XML)
	{
		ofn.lpstrFilter = _T("Xml Files\0*.xml\0All Files\0*.*\0");
		ofn.lpstrDefExt = _T("xml");
	}
	else
	{
		_ASSERT(FALSE);
		return;
	}
	ofn.nFilterIndex = 1;

	g_pSymEngine->GetReportFileName(ofn.lpstrDefExt, szFileName, countof(szFileName));
	if (GetSaveFileName(&ofn))
	{
		UpdateWindow(hwndParent);
		CWaitCursor wait(true);
		CopyFile(g_szInternalReportFilePath, szFileName, FALSE);
	}

// Was used for debugging:
// 	DWORD error = CommDlgExtendedError();
// 	char error_buffer[1000];
// 	_ltoa_s(error, error_buffer, 1000, 10);
// 	MessageBox(0, error_buffer, TEXT("error id"), 0);
}

/**
* @brief Initialize CPU registers message.
* @param hwnd - parent window handle.
*/
static void InitReg(HWND hwnd)
{
	HWND hwndReg = GetDlgItem(hwnd, IDC_REGISTER);
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hFixedFont)
		SetWindowFont(hwndReg, g_pResManager->m_hFixedFont, FALSE);
	TCHAR szRegString[256];
	g_pSymEngine->GetRegistersString(szRegString, countof(szRegString));
	SetWindowText(hwndReg, szRegString);
}

/**
* @brief Initialize error reason message.
* @param hwnd - parent window handle.
*/
static void InitErrorInfo(HWND hwnd)
{
	HWND hwndError = GetDlgItem(hwnd, IDC_EXCEPTION);
	CStrStream Stream(1024);
	g_pSymEngine->GetErrorString(Stream);
	SetWindowText(hwndError, Stream);
}

/**
 * @brief Initialize call stack message.
 * @param hwnd - parent window handle.
 */
static void InitStackTrace(HWND hwnd)
{
	HWND hwndStack = GetDlgItem(hwnd, IDC_STACKTRACE);
	ListView_SetExtendedListViewStyle(hwndStack, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	TCHAR szColumnTitle[64];
	LVCOLUMN lvc;
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_TEXT;
	lvc.pszText = szColumnTitle;

#ifdef _MANAGED
	if (NetThunks::IsNetException())
	{
		LoadString(g_hInstance, IDS_COLUMN_TYPE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_NET_ENTRY_TYPE, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_METHOD, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_NET_ENTRY_METHOD, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_FILE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_NET_ENTRY_FILE, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_LINE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_NET_ENTRY_LINE, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_ASSEMBLY, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_NET_ENTRY_ASSEMBLY, &lvc);

		CNetStackTrace::CNetStackTraceEntry Entry;
		if (g_pSymEngine->GetFirstStackTraceEntry(Entry))
		{
			LVITEM lvi;
			ZeroMemory(&lvi, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			int iItemPos = 0;
			do
			{
				lvi.iItem = iItemPos;
				lvi.pszText = Entry.m_szType;
				ListView_InsertItem(hwndStack, &lvi);
				ListView_SetItemText(hwndStack, iItemPos, CID_NET_ENTRY_METHOD, Entry.m_szMethod);
				ListView_SetItemText(hwndStack, iItemPos, CID_NET_ENTRY_FILE, Entry.m_szSourceFile);
				ListView_SetItemText(hwndStack, iItemPos, CID_NET_ENTRY_LINE, Entry.m_szLineInfo);
				ListView_SetItemText(hwndStack, iItemPos, CID_NET_ENTRY_ASSEMBLY, Entry.m_szAssembly);
				++iItemPos;
			}
			while (g_pSymEngine->GetNextStackTraceEntry(Entry));
		}

		ListView_SetColumnWidth(hwndStack, CID_NET_ENTRY_TYPE, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_NET_ENTRY_METHOD, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_NET_ENTRY_FILE, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_NET_ENTRY_LINE, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_NET_ENTRY_ASSEMBLY, LVSCW_AUTOSIZE_USEHEADER);
	}
	else
	{
#endif

		LoadString(g_hInstance, IDS_COLUMN_ADDRESS, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_WIN32_ENTRY_ADDRESS, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_FUNCTION, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_WIN32_ENTRY_FUNCTION, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_FILE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_WIN32_ENTRY_FILE, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_LINE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_WIN32_ENTRY_LINE, &lvc);

		LoadString(g_hInstance, IDS_COLUMN_MODULE, szColumnTitle, countof(szColumnTitle));
		ListView_InsertColumn(hwndStack, CID_WIN32_ENTRY_MODULE, &lvc);

		CSymEngine::CStackTraceEntry Entry;
		if (g_pSymEngine->GetFirstStackTraceEntry(Entry))
		{
			LVITEM lvi;
			ZeroMemory(&lvi, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			int iItemPos = 0;
			do
			{
				lvi.iItem = iItemPos;
				lvi.pszText = Entry.m_szAddress;
				ListView_InsertItem(hwndStack, &lvi);
				ListView_SetItemText(hwndStack, iItemPos, CID_WIN32_ENTRY_FUNCTION, Entry.m_szFunctionInfo);
				ListView_SetItemText(hwndStack, iItemPos, CID_WIN32_ENTRY_FILE, Entry.m_szSourceFile);
				ListView_SetItemText(hwndStack, iItemPos, CID_WIN32_ENTRY_LINE, Entry.m_szLineInfo);
				ListView_SetItemText(hwndStack, iItemPos, CID_WIN32_ENTRY_MODULE, Entry.m_szModule);
				++iItemPos;
			}
			while (g_pSymEngine->GetNextStackTraceEntry(Entry));
		}

		ListView_SetColumnWidth(hwndStack, CID_WIN32_ENTRY_ADDRESS, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_WIN32_ENTRY_FUNCTION, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_WIN32_ENTRY_FILE, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_WIN32_ENTRY_LINE, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(hwndStack, CID_WIN32_ENTRY_MODULE, LVSCW_AUTOSIZE_USEHEADER);
#ifdef _MANAGED
	}
#endif

}

/**
 * @brief WM_COMMAND handler of main dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void MainDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	codeNotify; hwndCtl;

	switch (id)
	{
	case IDC_MACHINE_STATE:
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MACHINE_STATE_DLG), hwnd, MachineStateDlgProc);
		break;
	case IDC_MACHINE_INFO:
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MACHINE_INFO_DLG), hwnd, MachineInfoDlgProc);
		break;
	case IDC_PREVIEW_REPORT:
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_PREVIEW_DLG), hwnd, PreviewDlgProc);
		break;
	case IDC_MAILTO:
		SendReport(hwnd);
		break;
	case IDC_SUBMIT_BUG:
		if (*g_szSupportHost && g_nSupportPort)
			SubmitReport(hwnd);
		else if (*g_szSupportEMail)
			SendReport(hwnd);
		break;
	case IDC_SAVE_REPORT:
		SaveReport(hwnd);
		break;
	case IDCANCEL:
		EndDialog(hwnd, FALSE);
		break;
	}
}

/**
 * @brief Initialize dialog controls.
 * @param hwnd - window handle.
 */
static void InitControls(HWND hwnd)
{
	HWND hwndClose = GetDlgItem(hwnd, IDCANCEL);
	HWND hwndPreview = GetDlgItem(hwnd, IDC_PREVIEW_REPORT);
	HWND hwndSave = GetDlgItem(hwnd, IDC_SAVE_REPORT);
	HWND hwndMailTo = GetDlgItem(hwnd, IDC_MAILTO);
	HWND hwndSubmitBug = GetDlgItem(hwnd, IDC_SUBMIT_BUG);
	HWND hwndMachineInfo = GetDlgItem(hwnd, IDC_MACHINE_INFO);
	HWND hwndMachineState = GetDlgItem(hwnd, IDC_MACHINE_STATE);

	g_hwndToolTip = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX,
	                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                               hwnd, NULL, g_hInstance, 0l);
	if (g_hwndToolTip)
	{
		RECT rcCtl;
		GetClientRect(hwndClose, &rcCtl);
		SendMessage(g_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, rcCtl.right * 2);

		TOOLINFO tinfo;
		ZeroMemory(&tinfo, sizeof(tinfo));
		tinfo.cbSize = sizeof(tinfo);
		tinfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
		tinfo.hinst = g_hInstance;

		tinfo.uId = (UINT_PTR)hwndClose;
		tinfo.lpszText = (PTSTR)IDS_CLOSE_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndPreview;
		tinfo.lpszText = (PTSTR)IDS_PREVIEW_REPORT_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndSave;
		tinfo.lpszText = (PTSTR)IDS_SAVE_REPORT_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndMailTo;
		tinfo.lpszText = (PTSTR)IDS_MAILTO_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndSubmitBug;
		tinfo.lpszText = (PTSTR)IDS_SUBMIT_BUG_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndMachineInfo;
		tinfo.lpszText = (PTSTR)IDS_MACHINE_INFO_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndMachineState;
		tinfo.lpszText = (PTSTR)IDS_MACHINE_STATE_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);
	}

	if (*g_szSupportHost == _T('\0') || g_nSupportPort == 0)
	{
		EnableWindow(hwndMailTo, FALSE);
		if (*g_szSupportEMail == _T('\0'))
			EnableWindow(hwndSubmitBug, FALSE);
	}
	else
	{
		if (*g_szSupportEMail == _T('\0'))
			EnableWindow(hwndMailTo, FALSE);
	}

	HWND hwndFocus = IsWindowEnabled(hwndSubmitBug) ? hwndSubmitBug : hwndClose;
	SetFocus(hwndFocus);
	SendMessage(hwnd, DM_SETDEFID, GetDlgCtrlID(hwndFocus), 0l);
}

/**
 * @brief WM_INITDIALOG handler of main dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL MainDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;

	InitControls(hwnd);
	InitIntro(hwnd, g_hlURL);
	InitReg(hwnd);
	InitErrorInfo(hwnd);
	InitStackTrace(hwnd);
	InitAbout(hwnd);

	return FALSE;
}

/**
 * @brief WM_CTLCOLORXXX handler of main dialog.
 * @param hwnd - window handle.
 * @param hdc - device context.
 * @param hwndChild - child window handle.
 * @param type - message subtype.
 * @return brush handle.
 */
static HBRUSH MainDlg_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	hwnd;
	if (type == CTLCOLOR_STATIC)
	{
		int nChildID = GetDlgCtrlID(hwndChild);
		if (nChildID == IDC_INTRO_BKGND ||
			nChildID == IDC_INTRO1 ||
			nChildID == IDC_INTRO2 ||
			nChildID == IDC_URL ||
			nChildID == IDC_URL_PREFIX)
		{
			_ASSERTE(g_pResManager != NULL);
			SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
			return g_pResManager->m_hbrWindowBrush;
		}
	}
	return NULL;
}

/**
 * @brief WM_DESTROY handler of main dialog.
 * @param hwnd - window handle.
 */
static void MainDlg_OnDestroy(HWND hwnd)
{
	hwnd;

	g_hlURL.Detach();

	if (g_hwndToolTip)
	{
		DestroyWindow(g_hwndToolTip);
		g_hwndToolTip = NULL;
	}
}

/**
 * @brief WM_SYSCOMMAND handler of main dialog.
 * @param hwnd - window handle.
 * @param cmd - specifies the type of system command requested.
 * @param x - horizontal position of the cursor, in screen coordinates.
 * @param y - vertical position of the cursor, in screen coordinates.
 */
void MainDlg_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
	x; y;
	if ((cmd & 0xFFF0) == IDM_ABOUTBOX)
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT_BUGTRAP_DLG), hwnd, AboutDlgProc);
}

/**
 * @brief Dialog procedure of main dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, MainDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, MainDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_SYSCOMMAND, MainDlg_OnSysCommand);
	HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, MainDlg_OnCtlColor);
	HANDLE_MSG(hwndDlg, WM_DESTROY, MainDlg_OnDestroy);
	default: return FALSE;
	}
}

/** @} */
