/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Transfer Progress dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "TransferProgressDlg.h"
#include "BugTrapUtils.h"
#include "BugTrapUI.h"
#include "StrStream.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

#define IDC_DIALOG_PANE   100

/// Transfer thread handle.
static HANDLE g_hTransferThread = NULL;
/// Transfer thread parameters.
static CTransferThreadParams* g_pTransferThreadParams = NULL;

/**
 * @brief WM_INITDIALOG handler of Transfer Status pane.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @return true to setup focus to system-defined control.
 */
static BOOL TransferStatusPane_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM /*lParam*/)
{
	hwndFocus;

	CStrStream Stream(1024);
	TCHAR szMessageText[128];
	HICON hIcon;

	DWORD dwErrorCode = g_pTransferThreadParams->GetErrorCode();
	if (dwErrorCode == ERROR_SUCCESS)
	{
		hIcon = LoadIcon(NULL, IDI_INFORMATION);
		LoadString(g_hInstance, IDS_STATUS_REPORTSENT, szMessageText, countof(szMessageText));
		Stream << szMessageText;
	}
	else
	{
		hIcon = LoadIcon(NULL, IDI_ERROR);
		LoadString(g_hInstance, IDS_ERROR_TRANSFERFAILED, szMessageText, countof(szMessageText));
		Stream << szMessageText << _T('\n');
		PCTSTR pszErrorMessage = g_pTransferThreadParams->GetErrorMessage();
		if (pszErrorMessage == NULL)
		{
			TCHAR szErrorMessageTemplate[256];
			LoadString(g_hInstance, IDS_UNDEFINED_ERROR_EX, szErrorMessageTemplate, countof(szErrorMessageTemplate));
			TCHAR szErrorMessage[256];
			_stprintf_s(szErrorMessage, countof(szErrorMessage), szErrorMessageTemplate, dwErrorCode);
			pszErrorMessage = szErrorMessage;
		}
		Stream << pszErrorMessage;
	}

	HWND hwndCtl = GetDlgItem(hwnd, IDC_STATUSMESSAGE);
	SetWindowText(hwndCtl, Stream);
	ShowWindow(hwndCtl, SW_SHOW);

	hwndCtl = GetDlgItem(hwnd, IDC_STATUSICON);
	Static_SetIcon(hwndCtl, hIcon);
	ShowWindow(hwndCtl, SW_SHOW);

	return TRUE;
}

/**
 * @brief WM_COMMAND handler of Transfer Status pane.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void TransferStatusPane_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	codeNotify; hwndCtl;
	switch (id)
	{
	case IDCANCEL:
		HWND hwndParent = GetParent(hwnd);
		EndDialog(hwndParent, IDCANCEL);
		break;
	}
}

/**
 * @brief Dialog procedure of Transfer Status pane.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
static INT_PTR CALLBACK TransferStatusPaneProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, TransferStatusPane_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, TransferStatusPane_OnCommand);
	default:
		return FALSE;
	}
}

/**
 * @brief Start progress animation.
 * @param hwnd - window handle.
 */
static void StartProgressAnimation(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem(hwnd, IDC_UPLOAD_ANIMATION);
	Animate_Open(hwndCtl, MAKEINTRESOURCE(IDR_UPLOAD_ANIMATION));
	Animate_Play(hwndCtl, 0, -1, -1);
}

/**
 * @brief Stop progress animation.
 * @param hwnd - window handle.
 */
static void StopProgressAnimation(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem(hwnd, IDC_UPLOAD_ANIMATION);
	Animate_Stop(hwndCtl);
	Animate_Close(hwndCtl);
}

/**
 * @brief WM_INITDIALOG handler of Transfer Progress pane.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL TransferProgressPane_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;
	HWND hwndCtl;

	StartProgressAnimation(hwnd);
	hwndCtl = GetDlgItem(hwnd, IDC_CONNECTINGTOSERVER_CMARK);
	SendMessage(hwndCtl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_pResManager->m_hCheckMark);
	hwndCtl = GetDlgItem(hwnd, IDC_SENDINGREPORT_CMARK);
	SendMessage(hwndCtl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_pResManager->m_hCheckMark);
	hwndCtl = GetDlgItem(hwnd, IDC_CHECKINGERRORSTATUS_CMARK);
	SendMessage(hwndCtl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_pResManager->m_hCheckMark);

	return TRUE;
}

/**
 * @brief WM_DESTROY handler of Transfer Progress pane.
 * @param hwnd - window handle.
 */
static inline void TransferProgressPane_OnDestroy(HWND hwnd)
{
	StopProgressAnimation(hwnd);
}

/**
 * @brief WM_COMMAND handler of Transfer Progress pane.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void TransferProgressPane_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	hwnd; hwndCtl; codeNotify;
	switch (id)
	{
	case IDCANCEL:
		_ASSERTE(g_hTransferThread != NULL && g_pTransferThreadParams != NULL);
		HANDLE hCancellationEvent = g_pTransferThreadParams->GetCancellationEvent();
		SetEvent(hCancellationEvent);
		break;
	}
}

/**
 * @brief Dialog procedure of Transfer Progress pane.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
static INT_PTR CALLBACK TransferProgressPaneProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndCtl;
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, TransferProgressPane_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, TransferProgressPane_OnCommand);
	HANDLE_MSG(hwndDlg, WM_DESTROY, TransferProgressPane_OnDestroy);
	case UM_CONNECTINGTOSERVER:
		hwndCtl = GetDlgItem(hwndDlg, IDC_CONNECTINGTOSERVER_CMARK);
		ShowWindow(hwndCtl, SW_SHOW);
		return TRUE;
	case UM_SENDINGREPORT:
		hwndCtl = GetDlgItem(hwndDlg, IDC_SENDINGREPORT_CMARK);
		ShowWindow(hwndCtl, SW_SHOW);
		return TRUE;
	case UM_CHECKINGERRORSTATUS:
		hwndCtl = GetDlgItem(hwndDlg, IDC_CHECKINGERRORSTATUS_CMARK);
		ShowWindow(hwndCtl, SW_SHOW);
		return TRUE;
	default:
		return FALSE;
	}
}

/**
 * Create child dialog pane.
 * @param hwndParent - parent window handle.
 * @param uDialogID - dialog ID.
 * @param pfnDlgProc - dialog procedure.
 * @param lParam - additional dialog parameter.
 */
static HWND CreateDialogPane(HWND hwndParent, UINT uDialogID, DLGPROC pfnDlgProc, LPARAM lParam = 0)
{
	HWND hwndPane = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(uDialogID), hwndParent, pfnDlgProc, lParam);
	if (hwndPane)
	{
		SetWindowLong(hwndPane, GWL_ID, IDC_DIALOG_PANE);
		DWORD dwStyleEx = GetWindowLong(hwndPane, GWL_EXSTYLE);
		SetWindowLong(hwndPane, GWL_EXSTYLE, dwStyleEx | WS_EX_STATICEDGE);
		RECT rcClient;
		GetClientRect(hwndParent, &rcClient);
		RECT rcPane;
		GetWindowRect(hwndPane, &rcPane);
		ScreenToClient(hwndParent, (PPOINT)&rcPane);
		ScreenToClient(hwndParent, (PPOINT)&rcPane + 1);
		int x = (rcClient.right - (rcPane.right - rcPane.left)) / 2;
		int y = (rcClient.bottom - (rcPane.bottom - rcPane.top)) / 2;
		SetWindowPos(hwndPane, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
	}
	return hwndPane;
}

/**
 * @brief WM_INITDIALOG handler of Transfer Progress dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL TransferProgressDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;

	CenterWindow(hwnd, GetParent(hwnd));

	HWND hwndPane = CreateDialogPane(hwnd, IDD_TRANSFERPROGRESS_PANE, TransferProgressPaneProc);
	if (hwndPane == NULL)
		goto error;

	_ASSERTE(g_pTransferThreadParams == NULL);
	g_pTransferThreadParams = new CTransferThreadParams;
	if (g_pTransferThreadParams == NULL)
		goto error;
	g_pTransferThreadParams->SetSinkWnd(hwnd);
	_ASSERTE(g_hTransferThread == NULL);
	g_hTransferThread = StartTransferThread(g_pTransferThreadParams);
	if (g_hTransferThread == NULL)
		goto error;

	return TRUE;

error:
	EndDialog(hwnd, FALSE);
	return TRUE;
}

/**
 * @brief WM_DESTROY handler of Transfer Progress dialog.
 * @param hwnd - window handle.
 */
static void TransferProgressDlg_OnDestroy(HWND hwnd)
{
	hwnd;

	_ASSERTE(g_hTransferThread != NULL && g_pTransferThreadParams != NULL);

	CloseTransferThread(g_hTransferThread);
	g_hTransferThread = NULL;

	delete g_pTransferThreadParams;
	g_pTransferThreadParams = NULL;
}

/**
 * @brief Dialog procedure of Transfer Progress dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK TransferProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndPane;
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, TransferProgressDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_DESTROY, TransferProgressDlg_OnDestroy);
	case WM_COMMAND:
	case UM_CONNECTINGTOSERVER:
	case UM_SENDINGREPORT:
	case UM_CHECKINGERRORSTATUS:
		hwndPane = GetDlgItem(hwndDlg, IDC_DIALOG_PANE);
		if (hwndPane)
			SendMessage(hwndPane, uMsg, wParam, lParam);
		return TRUE;
	case UM_TRANSFERCOMPLETE:
		hwndPane = GetDlgItem(hwndDlg, IDC_DIALOG_PANE);
		if (hwndPane)
		{
			// on Windows 98 this causes an access violation in user32.dll
			// why does it crash? - because this is Windows 98
			//DestroyWindow(hwndPane);
			// rather then destroy the window, let's hide it
			ShowWindow(hwndPane, SW_HIDE);
			// stop progress animation
			StopProgressAnimation(hwndPane);
			// and of course panel ID should be changed to avoid conflicts
			SetWindowLong(hwndPane, GWL_ID, -1);
			// finally we can create a new panel
			hwndPane = CreateDialogPane(hwndDlg, IDD_TRANSFERSTATUS_PANE, TransferStatusPaneProc, wParam);
			if (hwndPane)
			{
				SetFocus(hwndPane);
				SendMessage(hwndPane, DM_SETDEFID, IDCANCEL, 0);
			}
			else
				EndDialog(hwndDlg, IDCANCEL);
		}
		return TRUE;
	default:
		return FALSE;
	}
}

/** @} */
