/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Machine State dialog.
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
#include "BugTrapUtils.h"
#include "MachineStateDlg.h"
#include "LayoutManager.h"
#include "TextView.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Process list column identifiers.
enum PROCESS_LIST_COLUMN_ID
{
	/// PID.
	CID_PROCESS_ID,
	/// Process name.
	CID_PROCESS_NAME
};

/// Process list column identifiers.
enum PROCESS_MODULES_LIST_COLUMN_ID
{
	/// Module name.
	CID_MODULE_NAME,
	/// Module version.
	CID_MODULE_VERSION,
	/// Base address.
	CID_MODULE_BASE
};


/// Control layouts for Machine State dialog.
static LAYOUT_INFO g_arrMachineStateLayout[] =
{
	LAYOUT_INFO(IDC_PROCESS_LIST_FRAME,        ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_CENTER),
	LAYOUT_INFO(IDC_PROCESS_LIST,              ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_CENTER),
	LAYOUT_INFO(IDC_PROCESS_MODULES_LIST_FRAME, ALIGN_LEFT,  ALIGN_CENTER, ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDC_PROCESS_MODULES_LIST,       ALIGN_LEFT,  ALIGN_CENTER, ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDCANCEL,                      ALIGN_RIGHT, ALIGN_TOP,    ALIGN_RIGHT, ALIGN_TOP)
};

/// Dialog layout manager.
static CLayoutManager g_LayoutMgr;
/// Process list sort order.
static CListViewOrder g_ProcessListOrder;
/// Modules list sort order.
static CListViewOrder g_ModulesListOrder;

/**
 * @brief WM_COMMAND handler of Machine State dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void MachineStateDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	hwndCtl;
	switch (codeNotify)
	{
	case BN_CLICKED:
		switch (id)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;
		}
		break;
	}
}

/**
 * @brief WM_INITDIALOG handler of Machine State dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL MachineStateDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;

	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hBigAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_pResManager->m_hBigAppIcon);
	if (g_pResManager->m_hSmallAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_pResManager->m_hSmallAppIcon);
	CenterWindow(hwnd, GetParent(hwnd));
	g_LayoutMgr.InitLayout(hwnd, g_arrMachineStateLayout, countof(g_arrMachineStateLayout));

	HWND hwndProcessList = GetDlgItem(hwnd, IDC_PROCESS_LIST);
	ListView_SetExtendedListViewStyle(hwndProcessList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	HWND hwndModuleList = GetDlgItem(hwnd, IDC_PROCESS_MODULES_LIST);
	ListView_SetExtendedListViewStyle(hwndModuleList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	RECT rcList;
	GetClientRect(hwndProcessList, &rcList);
	rcList.right -= GetSystemMetrics(SM_CXHSCROLL);

	TCHAR szColumnTitle[64];
	LVCOLUMN lvc;
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = szColumnTitle;

	lvc.cx = rcList.right / 5;
	LoadString(g_hInstance, IDS_COLUMN_PID, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndProcessList, CID_PROCESS_ID, &lvc);

	lvc.cx = rcList.right * 4 / 5;
	LoadString(g_hInstance, IDS_COLUMN_PROCESS, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndProcessList, CID_PROCESS_NAME, &lvc);

	lvc.cx = rcList.right / 2;
	LoadString(g_hInstance, IDS_COLUMN_MODULE, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndModuleList, CID_MODULE_NAME, &lvc);

	lvc.cx = rcList.right / 4;
	LoadString(g_hInstance, IDS_COLUMN_VERSION, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndModuleList, CID_MODULE_VERSION, &lvc);

	lvc.cx = rcList.right / 4;
	LoadString(g_hInstance, IDS_COLUMN_BASE, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndModuleList, CID_MODULE_BASE, &lvc);

	CEnumProcess::CProcessEntry ProcEntry;
	if (g_pEnumProc->GetProcessFirst(ProcEntry))
	{
		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		int iItemPos = 0;
		do
		{
			TCHAR szProcessID[64];
			_ultot_s(ProcEntry.m_dwProcessID, szProcessID, countof(szProcessID), 10);
			lvi.iItem = iItemPos;
			lvi.pszText = szProcessID;
			lvi.lParam = ProcEntry.m_dwProcessID;
			ListView_InsertItem(hwndProcessList, &lvi);
			ListView_SetItemText(hwndProcessList, iItemPos, CID_PROCESS_NAME, ProcEntry.m_szProcessName);
			++iItemPos;
		}
		while (g_pEnumProc->GetProcessNext(ProcEntry));
	}

	// LVM_SETIMAGELIST resets header control image list
	g_ProcessListOrder.InitList(hwndProcessList);
	g_ModulesListOrder.InitList(hwndModuleList);
	return TRUE;
}

/**
 * @brief WM_DESTROY handler of Machine State dialog.
 * @param hwnd - window handle.
 */
static void MachineStateDlg_OnDestroy(HWND hwnd)
{
	hwnd;
	g_pEnumProc->Close();
}

/**
 * @brief WM_SIZE handler of Machine State dialog.
 * @param hwnd - window handle.
 * @param state - window state.
 * @param cx - window width.
 * @param cy - window height.
 */
static void MachineStateDlg_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	hwnd; cx; cy; state;
	g_LayoutMgr.ApplyLayout();
}

/**
 * @brief WM_GETMINMAXINFO handler of Machine State dialog.
 * @param hwnd - window handle.
 * @param pMinMaxInfo - window min-max info.
 */
static void MachineStateDlg_OnGetMinMaxInfo(HWND hwnd, PMINMAXINFO pMinMaxInfo)
{
	hwnd;
	pMinMaxInfo->ptMinTrackSize = g_LayoutMgr.GetMinTrackSize();
}

/**
 * @brief WM_NOTIFY handler of Machine State dialog.
 * @param hwnd - window handle.
 * @param idCtrl - identifier of the common control sending the message.
 * @param pnmh - pointer to an NMHDR structure that contains the notification code and additional information.
 */
static LRESULT MachineStateDlg_OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
	switch (pnmh->code)
	{
	case LVN_ITEMCHANGED:
		{
			if (idCtrl != IDC_PROCESS_LIST)
				break;
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
			if ((pnmv->uNewState & LVIS_SELECTED) != (pnmv->uOldState & LVIS_SELECTED))
			{
				if (pnmv->uNewState & LVIS_SELECTED)
				{
					CWaitCursor wait(true);
					HWND hwndModuleList = GetDlgItem(hwnd, IDC_PROCESS_MODULES_LIST);
					SendMessage(hwndModuleList, WM_SETREDRAW, FALSE, 0);
					DisplayWaitBanner(hwndModuleList);

					LVITEM lvi;
					ZeroMemory(&lvi, sizeof(lvi));
					lvi.mask = LVIF_TEXT;
					CEnumProcess::CModuleEntry module;
					if (g_pEnumProc->GetModuleFirst(pnmv->lParam, module))
					{
						int iItemPos = 0;
						do
						{
							lvi.iItem = iItemPos;
							lvi.pszText = module.m_szModuleName;
							ListView_InsertItem(hwndModuleList, &lvi);
							TCHAR szVersionString[64];
							if (CSymEngine::GetVersionString(module.m_szModuleName, szVersionString, countof(szVersionString)))
								ListView_SetItemText(hwndModuleList, iItemPos, CID_MODULE_VERSION, szVersionString);
							TCHAR szTempBuf[32];
							_stprintf_s(szTempBuf, countof(szTempBuf), _T("%08X"), (DWORD)module.m_pLoadBase);
							ListView_SetItemText(hwndModuleList, iItemPos, CID_MODULE_BASE, szTempBuf);
							++iItemPos;
						}
						while (g_pEnumProc->GetModuleNext(pnmv->lParam, module));
					}
					else
					{
						TCHAR szMessage[100];
						LoadString(g_hInstance, IDS_ERROR_NOT_AVAILABLE, szMessage, countof(szMessage));
						lvi.iItem = 0;
						lvi.pszText = szMessage;
						ListView_InsertItem(hwndModuleList, &lvi);
					}

					SendMessage(hwndModuleList, WM_SETREDRAW, TRUE, 0);
					InvalidateRect(hwndModuleList, NULL, TRUE);
				}
				else
				{
					HWND hwndModuleList = GetDlgItem(hwnd, IDC_PROCESS_MODULES_LIST);
					ListView_DeleteAllItems(hwndModuleList);
				}
			}
		}
		break;
	case LVN_COLUMNCLICK:
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
			if (idCtrl == IDC_PROCESS_LIST)
			{
				HWND hwndProcessList = GetDlgItem(hwnd, IDC_PROCESS_LIST);
				g_ProcessListOrder.ToggleSortParams(hwndProcessList, pnmv->iSubItem);
				LISTVIEW_SORT_PARAMS lvSortParams;
				lvSortParams.hwndList = hwndProcessList;
				lvSortParams.iColumnNumber = pnmv->iSubItem;
				lvSortParams.bAscending = g_ProcessListOrder.GetSortOrder();
				switch (pnmv->iSubItem)
				{
				case CID_PROCESS_ID:
					lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_INTEGER;
					break;
				case CID_PROCESS_NAME:
					lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_STRING;
					break;
				}
				ListView_SortItemsEx(hwndProcessList, &ListViewCompareFunc, (LPARAM)&lvSortParams);
			}
			else if (idCtrl == IDC_PROCESS_MODULES_LIST)
			{
				HWND hwndModuleList = GetDlgItem(hwnd, IDC_PROCESS_MODULES_LIST);
				g_ModulesListOrder.ToggleSortParams(hwndModuleList, pnmv->iSubItem);
				LISTVIEW_SORT_PARAMS lvSortParams;
				lvSortParams.hwndList = hwndModuleList;
				lvSortParams.iColumnNumber = pnmv->iSubItem;
				lvSortParams.bAscending = g_ModulesListOrder.GetSortOrder();
				switch (pnmv->iSubItem)
				{
				case CID_MODULE_NAME:
				case CID_MODULE_VERSION:
					lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_STRING;
					break;
				case CID_MODULE_BASE:
					lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_HEXADECIMAL;
					break;
				}
				ListView_SortItemsEx(hwndModuleList, &ListViewCompareFunc, (LPARAM)&lvSortParams);
			}
		}
		break;
	}
	return FALSE;
}

/**
 * @brief Dialog procedure of Machine State dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK MachineStateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, MachineStateDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_DESTROY, MachineStateDlg_OnDestroy);
	HANDLE_MSG(hwndDlg, WM_COMMAND, MachineStateDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_SIZE, MachineStateDlg_OnSize);
	HANDLE_MSG(hwndDlg, WM_GETMINMAXINFO, MachineStateDlg_OnGetMinMaxInfo);
	HANDLE_MSG(hwndDlg, WM_NOTIFY, MachineStateDlg_OnNotify);
	default: return FALSE;
	}
}

/** @} */
