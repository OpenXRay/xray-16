/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Machine Info dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "MachineInfoDlg.h"
#include "BugTrapUtils.h"
#include "BugTrapUI.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/**
 * @brief WM_COMMAND handler of Machine Info dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void MachineInfoDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	codeNotify; hwndCtl;
	switch (id)
	{
	case IDOK:
	case IDCANCEL:
		EndDialog(hwnd, FALSE);
		break;
	}
}

/**
 * @brief WM_INITDIALOG handler of Machine Info dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL MachineInfoDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hBigAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_pResManager->m_hBigAppIcon);
	if (g_pResManager->m_hSmallAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_pResManager->m_hSmallAppIcon);
	CenterWindow(hwnd, GetParent(hwnd));

	HWND hwndCtl;
	TCHAR szTempBuf[256];
	CStrStream Stream(8 * 1024);

	hwndCtl = GetDlgItem(hwnd, IDC_CPU_TEXT);
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hFixedFont)
		SetWindowFont(hwndCtl, g_pResManager->m_hFixedFont, FALSE);
	Stream.Reset();
	CSymEngine::GetCpuString(Stream);
	SetWindowText(hwndCtl, Stream);

	hwndCtl = GetDlgItem(hwnd, IDC_OS_TEXT);
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hFixedFont)
		SetWindowFont(hwndCtl, g_pResManager->m_hFixedFont, FALSE);
	CSymEngine::GetOsString(szTempBuf, countof(szTempBuf));
	SetWindowText(hwndCtl, szTempBuf);

	hwndCtl = GetDlgItem(hwnd, IDC_MEM_TEXT);
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hFixedFont)
		SetWindowFont(hwndCtl, g_pResManager->m_hFixedFont, FALSE);
	CSymEngine::GetMemString(szTempBuf, countof(szTempBuf));
	SetWindowText(hwndCtl, szTempBuf);

	hwndCtl = GetDlgItem(hwnd, IDC_ENVIRONMENT);
	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hFixedFont)
		SetWindowFont(hwndCtl, g_pResManager->m_hFixedFont, FALSE);
	Stream.Reset();
	CSymEngine::GetEnvironmentStrings(Stream);
	SetWindowText(hwndCtl, Stream);
	return TRUE;
}

/**
 * @brief WM_CTLCOLORXXX handler of Machine Info dialog.
 * @param hwnd - window handle.
 * @param hdc - device context.
 * @param hwndChild - child window handle.
 * @param type - message subtype.
 * @return brush handle.
 */
static HBRUSH MachineInfoDlg_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	hwnd; hdc;
	if (type == CTLCOLOR_STATIC)
	{
		int nChildID = GetDlgCtrlID(hwndChild);
		if (nChildID == IDC_ENVIRONMENT)
		{
			_ASSERTE(g_pResManager != NULL);
			SetBkColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
			return g_pResManager->m_hbrControlLight;
		}
	}
	return NULL;
}

/**
 * @brief Dialog procedure of Machine Info dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK MachineInfoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, MachineInfoDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, MachineInfoDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, MachineInfoDlg_OnCtlColor);
	default: return FALSE;
	}
}

/** @} */
