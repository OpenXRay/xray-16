/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Layout manager class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "LayoutManager.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @param nCtlID - control ID.
 * @param nRatioX1 - left coordinate ratio.
 * @param nRatioY1 - top coordinate ratio.
 * @param nRatioX2 - right coordinate ratio.
 * @param nRatioY2 - bottom coordinate ratio.
 */
LAYOUT_INFO::LAYOUT_INFO(int nCtlID, int nRatioX1, int nRatioY1, int nRatioX2, int nRatioY2)
{
	m_nCtlID = nCtlID;
	m_nRatioX1 = nRatioX1;
	m_nRatioY1 = nRatioY1;
	m_nRatioX2 = nRatioX2;
	m_nRatioY2 = nRatioY2;
	ZeroMemory(&m_rcOriginal, sizeof(m_rcOriginal));
}

/**
 * @param hwndParent - parent window handle.
 * @param arrLayout - array of control layout info blocks.
 * @param nItemCount - number of control layout info blocks.
 * @param bAddSizeBox - pass true if size box must be added to the dialog.
 */
void CLayoutManager::InitLayout(HWND hwndParent, LAYOUT_INFO arrLayout[], int nItemCount, bool bAddSizeBox)
{
	LONG lStyle = GetWindowLong(hwndParent, GWL_STYLE);
	lStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	SetWindowLong(hwndParent, GWL_STYLE, lStyle);

	RECT rcWindow;
	GetWindowRect(hwndParent, &rcWindow);
	m_ptMinWindowSize.x = rcWindow.right - rcWindow.left;
	m_ptMinWindowSize.y = rcWindow.bottom - rcWindow.top;

	RECT rcClient;
	GetClientRect(hwndParent, &rcClient);
	m_ptMinClientSize.x = rcClient.right;
	m_ptMinClientSize.y = rcClient.bottom;

	m_hwndParent = hwndParent;
	m_arrLayout = arrLayout;
	m_nItemCount = nItemCount;

	for (int iPos = 0; iPos < nItemCount; ++iPos)
	{
		LAYOUT_INFO& rLayoutInfo = arrLayout[iPos];
		HWND hwndCtl = GetDlgItem(hwndParent, rLayoutInfo.m_nCtlID);
		RECT& rcOriginal = rLayoutInfo.m_rcOriginal;
		GetWindowRect(hwndCtl, &rcOriginal);
		ScreenToClient(hwndParent, (PPOINT)&rcOriginal);
		ScreenToClient(hwndParent, (PPOINT)&rcOriginal + 1);
	}

	m_hwndSizeBox = bAddSizeBox ?
			CreateWindow(WC_SCROLLBAR, NULL, WS_CHILD | WS_VISIBLE | WS_GROUP | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN | WS_CLIPSIBLINGS,
			             rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, hwndParent, NULL, g_hInstance, NULL) : NULL;
}

void CLayoutManager::ApplyLayout(void)
{
	if (! m_arrLayout)
		return;

	RECT rcClient;
	GetClientRect(m_hwndParent, &rcClient);
	const int nWindowDeltaX = rcClient.right - m_ptMinClientSize.x;
	const int nWindowDeltaY = rcClient.bottom - m_ptMinClientSize.y;

	BOOL bVisible = IsWindowVisible(m_hwndParent);
	if (bVisible)
		SendMessage(m_hwndParent, WM_SETREDRAW, FALSE, 0);

	int iPos;
	for (iPos = 0; iPos < m_nItemCount; ++iPos)
	{
		const LAYOUT_INFO& rLayoutInfo = m_arrLayout[iPos];
		const RECT& rcOriginal = rLayoutInfo.m_rcOriginal;
		RECT rcCtl;

		if (nWindowDeltaX > 0)
		{
			rcCtl.left   = rcOriginal.left   + nWindowDeltaX * rLayoutInfo.m_nRatioX1 / ALIGN_RANGE;
			rcCtl.right  = rcOriginal.right  + nWindowDeltaX * rLayoutInfo.m_nRatioX2 / ALIGN_RANGE;
		}
		else
		{
			rcCtl.left   = rcOriginal.left;
			rcCtl.right  = rcOriginal.right;
		}

		if (nWindowDeltaY > 0)
		{
			rcCtl.top    = rcOriginal.top    + nWindowDeltaY * rLayoutInfo.m_nRatioY1 / ALIGN_RANGE;
			rcCtl.bottom = rcOriginal.bottom + nWindowDeltaY * rLayoutInfo.m_nRatioY2 / ALIGN_RANGE;
		}
		else
		{
			rcCtl.top    = rcOriginal.top;
			rcCtl.bottom = rcOriginal.bottom;
		}

		int nHorScrollVal = GetScrollPos(m_hwndParent, SB_HORZ);
		rcCtl.left   -= nHorScrollVal;
		rcCtl.right  -= nHorScrollVal;
		int nVertScrollVal = GetScrollPos(m_hwndParent, SB_VERT);
		rcCtl.top    -= nVertScrollVal;
		rcCtl.bottom -= nVertScrollVal;

		HWND hwndCtl = GetDlgItem(m_hwndParent, rLayoutInfo.m_nCtlID);
		SetWindowPos(hwndCtl,
					 NULL,
					 rcCtl.left,
					 rcCtl.top,
					 rcCtl.right - rcCtl.left,
					 rcCtl.bottom - rcCtl.top,
					 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREDRAW);
	}

	if (m_hwndSizeBox)
	{
		RECT rcCtl;
		GetWindowRect(m_hwndSizeBox, &rcCtl);
		SetWindowPos(m_hwndSizeBox,
					 NULL,
					 rcClient.right - (rcCtl.right - rcCtl.left),
					 rcClient.bottom - (rcCtl.bottom - rcCtl.top),
					 0,
					 0,
					 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
		ShowWindow(m_hwndSizeBox, IsMaximized(m_hwndParent) ? SW_HIDE : SW_SHOW);
	}

	if (bVisible)
	{
		SendMessage(m_hwndParent, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(m_hwndParent, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
