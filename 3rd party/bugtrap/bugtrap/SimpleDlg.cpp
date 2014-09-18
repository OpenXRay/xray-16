/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: BugTrap simplified dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "SimpleDlg.h"
#include "BugTrapUI.h"
#include "AboutDlg.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Tool-tip window handle.
static HWND g_hwndToolTip = NULL;
/// URL hyper-link control pointing to support site.
static CHyperLink g_hlURL;
/// URL hyper-link control pointing to support e-mail.
static CHyperLink g_hlMailTo;

/**
 * @brief WM_COMMAND handler of simplified dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void SimpleDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	codeNotify; hwndCtl;

	switch (id)
	{
	case IDC_MAILTO:
		SendReport(hwnd);
		break;
	case IDC_SUBMIT_BUG:
		if (*g_szSupportHost && g_nSupportPort)
			SubmitReport(hwnd);
		else if (*g_szSupportEMail)
			SendReport(hwnd);
		break;
	case IDC_MORE:
		EndDialog(hwnd, TRUE);
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
	HWND hwndMailTo = GetDlgItem(hwnd, IDC_MAILTO);
	HWND hwndSubmitBug = GetDlgItem(hwnd, IDC_SUBMIT_BUG);
	HWND hwndMore = GetDlgItem(hwnd, IDC_MORE);
	HWND hwndClose = GetDlgItem(hwnd, IDCANCEL);

	g_hlMailTo.Attach(hwndMailTo);
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

		tinfo.uId = (UINT_PTR)hwndMailTo;
		tinfo.lpszText = (PTSTR)IDS_MAILTO_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndSubmitBug;
		tinfo.lpszText = (PTSTR)IDS_SUBMIT_BUG_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndMore;
		tinfo.lpszText = (PTSTR)IDS_MORE_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);

		tinfo.uId = (UINT_PTR)hwndClose;
		tinfo.lpszText = (PTSTR)IDS_CLOSE_TIP;
		SendMessage(g_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tinfo);
	}

	if (*g_szSupportHost == _T('\0') || g_nSupportPort == 0)
	{
		ShowWindow(hwndMailTo, SW_HIDE);
		if (*g_szSupportEMail == _T('\0'))
			EnableWindow(hwndSubmitBug, FALSE);
	}
	else
	{
		if (*g_szSupportEMail == _T('\0'))
			ShowWindow(hwndMailTo, SW_HIDE);
	}

	HWND hwndFocus = IsWindowEnabled(hwndSubmitBug) ? hwndSubmitBug : hwndClose;
	SetFocus(hwndFocus);
	SendMessage(hwnd, DM_SETDEFID, GetDlgCtrlID(hwndFocus), 0l);
}

/**
 * @brief WM_INITDIALOG handler of simplified dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL SimpleDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;

	InitControls(hwnd);
	InitIntro(hwnd, g_hlURL);
	InitAbout(hwnd);

	return FALSE;
}

/**
 * @brief WM_CTLCOLORXXX handler of simplified dialog.
 * @param hwnd - window handle.
 * @param hdc - device context.
 * @param hwndChild - child window handle.
 * @param type - message subtype.
 * @return brush handle.
 */
static HBRUSH SimpleDlg_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	hwnd;
	if (type == CTLCOLOR_STATIC)
	{
		int nChildID = GetDlgCtrlID(hwndChild);
		if (nChildID == IDC_INTRO_BKGND ||
			nChildID == IDC_INTRO1 ||
			nChildID == IDC_INTRO2)
		{
			_ASSERTE(g_pResManager != NULL);
			SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
			return g_pResManager->m_hbrWindowBrush;
		}
	}
	return NULL;
}

/**
 * @brief WM_DESTROY handler of simplified dialog.
 * @param hwnd - window handle.
 */
static void SimpleDlg_OnDestroy(HWND hwnd)
{
	hwnd;

	g_hlURL.Detach();
	g_hlMailTo.Detach();

	if (g_hwndToolTip)
	{
		DestroyWindow(g_hwndToolTip);
		g_hwndToolTip = NULL;
	}
}

/**
 * @brief WM_SYSCOMMAND handler of simplified dialog.
 * @param hwnd - window handle.
 * @param cmd - specifies the type of system command requested.
 * @param x - horizontal position of the cursor, in screen coordinates.
 * @param y - vertical position of the cursor, in screen coordinates.
 */
void SimpleDlg_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
	x; y;
	if ((cmd & 0xFFF0) == IDM_ABOUTBOX)
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT_BUGTRAP_DLG), hwnd, AboutDlgProc);
}

/**
 * @brief Dialog procedure of simplified dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK SimpleDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, SimpleDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, SimpleDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_SYSCOMMAND, SimpleDlg_OnSysCommand);
	HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, SimpleDlg_OnCtlColor);
	HANDLE_MSG(hwndDlg, WM_DESTROY, SimpleDlg_OnDestroy);
	default: return FALSE;
	}
}

/** @} */
