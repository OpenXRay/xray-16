/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Send Mail dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "SendMailDlg.h"
#include "BugTrapUI.h"
#include "BugTrapUtils.h"
#include "WaitDlg.h"
#include "LayoutManager.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Control layouts for Send Mail dialog.
static LAYOUT_INFO g_arrSendMailLayout[] =
{
	LAYOUT_INFO(IDC_RECIPIENT,  ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_TOP),
	LAYOUT_INFO(IDC_ATTACHMENT, ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_TOP),
	LAYOUT_INFO(IDC_SUBJECT,    ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_TOP),
	LAYOUT_INFO(IDC_BODY,       ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDOK,           ALIGN_RIGHT, ALIGN_BOTTOM, ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDCANCEL,       ALIGN_RIGHT, ALIGN_BOTTOM, ALIGN_RIGHT, ALIGN_BOTTOM)
};

/// Dialog layout manager.
static CLayoutManager g_LayoutMgr;

/**
 * @brief WM_COMMAND handler of Send Mail dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void SendMailDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	int nTextLength;
	PTSTR pszSubject, pszMessage;
	switch (codeNotify)
	{
	case BN_CLICKED:
		switch (id)
		{
		case IDOK:
			hwndCtl = GetDlgItem(hwnd, IDC_SUBJECT);
			nTextLength = GetWindowTextLength(hwndCtl) + 1;
			pszSubject = new TCHAR[nTextLength];
			if (! pszSubject)
				return;
			GetWindowText(hwndCtl, pszSubject, nTextLength);

			hwndCtl = GetDlgItem(hwnd, IDC_BODY);
			nTextLength = GetWindowTextLength(hwndCtl) + 1;
			pszMessage = new TCHAR[nTextLength];
			if (! pszMessage)
			{
				delete[] pszSubject;
				return;
			}
			GetWindowText(hwndCtl, pszMessage, nTextLength);

			if (! SendEMail(hwnd, pszSubject, pszMessage))
			{
				SetForegroundWindow(hwnd);
				TCHAR szProjectName[32], szMessageText[128];
				LoadString(g_hInstance, IDS_BUGTRAP_NAME, szProjectName, countof(szProjectName));
				LoadString(g_hInstance, IDS_ERROR_TRANSFERFAILED, szMessageText, countof(szMessageText));
				::MessageBox(hwnd, szMessageText, szProjectName, MB_ICONERROR | MB_OK);
			}
			else
				EndDialog(hwnd, TRUE);
			delete[] pszSubject;
			delete[] pszMessage;
			break;
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;
		}
		break;
	}
}

/**
 * @brief WM_INITDIALOG handler of Send Mail dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL SendMailDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;
	HWND hwndCtl;
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hBigAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_pResManager->m_hBigAppIcon);
	if (g_pResManager->m_hSmallAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_pResManager->m_hSmallAppIcon);
	CenterWindow(hwnd, GetParent(hwnd));
	g_LayoutMgr.InitLayout(hwnd, g_arrSendMailLayout, countof(g_arrSendMailLayout));

	if (g_dwFlags & BTF_ATTACHREPORT)
	{
		TCHAR szFileName[MAX_PATH];
		g_pSymEngine->GetReportFileName(szFileName, countof(szFileName));
		hwndCtl = GetDlgItem(hwnd, IDC_ATTACHMENT);
		SetWindowText(hwndCtl, szFileName);
	}

	hwndCtl = GetDlgItem(hwnd, IDC_RECIPIENT);
	SetWindowText(hwndCtl, g_szSupportEMail);

	TCHAR szSubject[MAX_PATH];
	GetDefaultMailSubject(szSubject, countof(szSubject));
	hwndCtl = GetDlgItem(hwnd, IDC_SUBJECT);
	SetWindowText(hwndCtl, szSubject);

	hwndCtl = GetDlgItem(hwnd, IDC_BODY);
	SetFocus(hwndCtl);
	return FALSE;
}

/**
 * @brief WM_SIZE handler of Send Mail dialog.
 * @param hwnd - window handle.
 * @param state - window state.
 * @param cx - window width.
 * @param cy - window height.
 */
static void SendMailDlg_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	hwnd; cx; cy; state;
	g_LayoutMgr.ApplyLayout();
}

/**
 * @brief WM_GETMINMAXINFO handler of Send Mail State dialog.
 * @param hwnd - window handle.
 * @param pMinMaxInfo - window min-max info.
 */
static void SendMailDlg_OnGetMinMaxInfo(HWND hwnd, PMINMAXINFO pMinMaxInfo)
{
	hwnd;
	pMinMaxInfo->ptMinTrackSize = g_LayoutMgr.GetMinTrackSize();
}

/**
 * @brief Dialog procedure of Send Mail dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK SendMailDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, SendMailDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, SendMailDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_SIZE, SendMailDlg_OnSize);
	HANDLE_MSG(hwndDlg, WM_GETMINMAXINFO, SendMailDlg_OnGetMinMaxInfo);
	default: return FALSE;
	}
}

/** @} */
