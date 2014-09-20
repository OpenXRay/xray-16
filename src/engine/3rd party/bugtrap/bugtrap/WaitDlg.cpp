/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Wait dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "WaitDlg.h"
#include "BugTrapUtils.h"
#include "ResManager.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/**
 * @param hwndParent - parent window handle.
 */
CWaitDialog::CWaitDialog(HWND hwndParent)
{
	InitVars();
	BeginWait(hwndParent);
}

/**
 * @param hwndParent - parent window handle.
 */
void CWaitDialog::BeginWait(HWND hwndParent)
{
	_ASSERTE(m_hwndWait == NULL && m_hOldCursor == NULL);
	m_hOldCursor = SetCursor(g_pResManager->m_hWaitCursor);
	m_hwndWait = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_WAIT_DLG), hwndParent, NULL);
	if (m_hwndWait)
	{
		CenterWindow(m_hwndWait, hwndParent);
		ShowWindow(m_hwndWait, SW_SHOW);
		UpdateWindow(m_hwndWait);
	}
}

void CWaitDialog::EndWait(void)
{
	if (m_hwndWait)
	{
		DestroyWindow(m_hwndWait);
		m_hwndWait = NULL;
	}
	if (m_hOldCursor)
	{
		SetCursor(m_hOldCursor);
		m_hOldCursor = NULL;
	}
}

/** @} */
