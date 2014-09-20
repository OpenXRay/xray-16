/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Splitter control class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "Splitter.h"
#include "BugTrapUtils.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @param eDirection - splitter direction.
 * @param bProportionalMode - proportional mode.
 */
CSplitter::CSplitter(SPLITTER_DIRECTION eDirection, bool bProportionalMode)
{
	ClearPanels();
	m_pfnOldSplitterWndProc = NULL;
	m_bProportionalMode = bProportionalMode;
	SetDirection(eDirection);
	m_nSplitterSize = 0;
}

/**
 * @param eDirection - new splitter direction.
 */
void CSplitter::SetDirection(SPLITTER_DIRECTION eDirection)
{
	m_eDirection = eDirection;
	if (m_hwnd)
		ResetSplitterPos();
	else
		ClearSplitterPos();
}

void CSplitter::ResetSplitterPos(void)
{
	_ASSERTE(m_hwnd != NULL);
	m_nSplitterSize = CalcSplitterSize();
	m_nSplitterPos = m_nIdealPos = m_nSplitterSize / 2;
	UpdateLayout();
}

/**
 * @param rect - splitter bar coordinates.
 */
void CSplitter::GetSplitterBarRect(RECT& rect) const
{
	_ASSERTE(m_hwnd != NULL);
	GetClientRect(m_hwnd, &rect);
	int nBarSize = GetSystemMetrics(m_eDirection == SD_VERTICAL ? SM_CYFRAME : SM_CXFRAME);
	int nSplitterBarEnd = m_nSplitterPos + nBarSize;
	if (m_eDirection == SD_VERTICAL)
	{
		rect.top = m_nSplitterPos;
		if (rect.bottom > nSplitterBarEnd)
			rect.bottom = nSplitterBarEnd;
	}
	else
	{
		rect.left = m_nSplitterPos;
		if (rect.right > nSplitterBarEnd)
			rect.right = nSplitterBarEnd;
	}
}

/**
 * @param point - point coordinates.
 * @return true if point appears in boundaries of Splitter text.
 */
BOOL CSplitter::HitTest(const POINT& point) const
{
	_ASSERTE(m_hwnd != NULL);
	RECT rect;
	GetSplitterBarRect(rect);
	return PtInRect(&rect, point);
}

/**
 * @param x - point x-coordinate.
 * @param y - point y-coordinate.
 * @return true if point appears in boundaries of Splitter text.
 */
BOOL CSplitter::HitTest(int x, int y) const
{
	POINT point;
	point.x = x;
	point.y = y;
	return HitTest(point);
}

/**
 * @param hdc - drawing context.
 * @param iPanel - panel number.
 * @return true if panel index is valid.
 */
BOOL CSplitter::DrawPanel(HDC hdc, int iPanel) const
{
	_ASSERTE(m_hwnd != NULL);
	_ASSERTE(g_pResManager != NULL);
	if (iPanel >= 0 && iPanel < NUM_PANELS)
	{
		RECT rect;
		CalcPanelRect(iPanel, rect, false);
		HWND hwndPanel = m_arrPanels[iPanel];
		if (hwndPanel == NULL || (GetWindowLong(hwndPanel, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) == 0)
		{
			int nCXEdge = GetSystemMetrics(SM_CXEDGE) * 2;
			int nCYEdge = GetSystemMetrics(SM_CYEDGE) * 2;
			if (rect.right - rect.left >= nCXEdge && rect.bottom - rect.top >= nCYEdge)
				DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
		}
		if (hwndPanel == NULL)
			FillRect(hdc, &rect, g_pResManager->m_hbrAppWorkspace);
		return TRUE;
	}
	return FALSE;
}

/**
 * @param hdc - drawing context.
 */
void CSplitter::DrawSplitter(HDC hdc) const
{
	_ASSERTE(m_hwnd != NULL);
	DrawSplitterBar(hdc);
	DrawPanel(hdc, 0);
	DrawPanel(hdc, 1);
}

/**
 * @param hdc - drawing context.
 */
void CSplitter::DrawSplitterBar(HDC hdc) const
{
	_ASSERTE(m_hwnd != NULL);
	RECT rect;
	GetSplitterBarRect(rect);
	_ASSERTE(g_pResManager != NULL);
	FillRect(hdc, &rect, g_pResManager->m_hbrButtonFaceBrush);
}

/**
 * @param iPanel - panel number.
 * @param hwndPanel - panel window handle.
 * @return true if panel index is valid.
 */
BOOL CSplitter::SetPanel(int iPanel, HWND hwndPanel)
{
	_ASSERTE(m_hwnd != NULL);
	if (iPanel >= 0 && iPanel < NUM_PANELS)
	{
		m_arrPanels[iPanel] = hwndPanel;
		if (hwndPanel)
		{
			RECT rect;
			CalcPanelRect(iPanel, rect, true);
			HWND hwndPanelParent = GetParent(hwndPanel);
			MapWindowRect(m_hwnd, hwndPanelParent, &rect);
			SetWindowPos(hwndPanel, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @param iPanel - panel number.
 * @param rect - panel rectangle.
 * @param bShrinkRect - true if rectangle must be shrinked.
 * @return true if panel index is valid.
 */
BOOL CSplitter::CalcPanelRect(int iPanel, RECT& rect, bool bShrinkRect) const
{
	_ASSERTE(m_hwnd != NULL);
	if (iPanel >= 0 && iPanel < NUM_PANELS)
	{
		GetClientRect(m_hwnd, &rect);
		if (m_eDirection == SD_VERTICAL)
		{
			if (iPanel == 0)
				rect.bottom = m_nSplitterPos;
			else
			{
				int nBarSize = GetSystemMetrics(SM_CYFRAME);
				int nSplitterBarEnd = m_nSplitterPos + nBarSize;
				rect.top = rect.bottom > nSplitterBarEnd ? nSplitterBarEnd : rect.bottom;
			}
		}
		else
		{
			if (iPanel == 0)
				rect.right = m_nSplitterPos;
			else
			{
				int nBarSize = GetSystemMetrics(SM_CXFRAME);
				int nSplitterBarEnd = m_nSplitterPos + nBarSize;
				rect.left = rect.right > nSplitterBarEnd ? nSplitterBarEnd : rect.right;
			}
		}
		if (bShrinkRect)
		{
			HWND hwndPanel = m_arrPanels[iPanel];
			if (hwndPanel == NULL || (GetWindowLong(hwndPanel, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) == 0)
			{
				int nCXEdge = GetSystemMetrics(SM_CXEDGE);
				int nCYEdge = GetSystemMetrics(SM_CYEDGE);
				if (rect.right - rect.left >= nCXEdge * 2)
				{
					rect.left += nCXEdge;
					rect.right -= nCXEdge;
				}
				if (rect.bottom - rect.top >= nCYEdge * 2)
				{
					rect.top += nCYEdge;
					rect.bottom -= nCYEdge;
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}

void CSplitter::UpdateLayout(void)
{
	_ASSERTE(m_hwnd != NULL);
	InvalidateRect(m_hwnd, NULL, FALSE);
	HDWP hdwp = BeginDeferWindowPos(NUM_PANELS);
	if (hdwp)
	{
		for (int iPanel = 0; iPanel < NUM_PANELS; ++iPanel)
		{
			HWND hwndPanel = m_arrPanels[iPanel];
			if (hwndPanel)
			{
				RECT rect;
				CalcPanelRect(iPanel, rect, true);
				HWND hwndPanelParent = GetParent(hwndPanel);
				MapWindowRect(m_hwnd, hwndPanelParent, &rect);
				DeferWindowPos(hdwp, hwndPanel, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			}
		}
		EndDeferWindowPos(hdwp);
	}
}

/**
 * @param nSplitterSize - new splitter size.
 */
void CSplitter::ResizeSplitter(int nSplitterSize)
{
	int nEdgeType, nFrameType;
	if (m_eDirection == SD_VERTICAL)
	{
		nEdgeType = SM_CYEDGE;
		nFrameType = SM_CYFRAME;
	}
	else
	{
		nEdgeType = SM_CXEDGE;
		nFrameType = SM_CXFRAME;
	}
	int nBarSize = GetSystemMetrics(nFrameType);
	int nEdgeSize = GetSystemMetrics(nEdgeType) * 2;
	nSplitterSize -= nBarSize;
	nSplitterSize -= nEdgeSize;
	if (nSplitterSize < 0)
		nSplitterSize = 0;
	if (m_bProportionalMode)
		m_nSplitterPos = m_nSplitterSize > 0 ? MulDiv(m_nIdealPos, nSplitterSize, m_nSplitterSize) : nSplitterSize / 2;
	if (m_nSplitterPos < nEdgeSize)
		m_nSplitterPos = nEdgeSize;
	if (m_nSplitterPos > nSplitterSize)
		m_nSplitterPos = nSplitterSize;
	UpdateLayout();
}

/**
 * @param nSplitterPos - new splitter position.
 */
void CSplitter::SetSplitterPos(int nSplitterPos)
{
	_ASSERTE(m_hwnd != NULL);
	int nEdgeSize = GetSystemMetrics(m_eDirection == SD_VERTICAL ? SM_CYEDGE : SM_CXEDGE) * 2;
	int nSplitterSize = CalcSplitterSize();
	if (nSplitterPos < nEdgeSize)
		nSplitterPos = nEdgeSize;
	if (nSplitterPos > nSplitterSize)
		nSplitterPos = nSplitterSize;
	if (m_nSplitterPos != nSplitterPos)
	{
		m_nSplitterSize = nSplitterSize;
		m_nIdealPos = m_nSplitterPos = nSplitterPos;
		UpdateLayout();
	}
}

/**
 * @param hwnd - window handle.
 * @param uMsg - message identifier.
 * @param wParam - first message parameter.
 * @param lParam - second message parameter.
 */
LRESULT CALLBACK CSplitter::SplitterWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int x, y, nSplitterPos, nSplitterSize;
	POINT point;
	HCURSOR hCursor;
	PAINTSTRUCT ps;
	HDC hdc;
	DWORD dwResult;

	CSplitter* _this  = (CSplitter*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	_ASSERTE(_this != NULL);
	switch(uMsg)
	{
	case WM_NCHITTEST:
		if (GetCapture() == hwnd)
			return HTCLIENT;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		ScreenToClient(hwnd, &point);
		return (_this->HitTest(point) ? HTCLIENT : HTTRANSPARENT);
	case WM_SETCURSOR:
		_ASSERTE(g_pResManager != NULL);
		dwResult = GetMessagePos();
		point.x = GET_X_LPARAM(dwResult);
		point.y = GET_Y_LPARAM(dwResult);
		ScreenToClient(hwnd, &point);
		if (_this->HitTest(point))
			hCursor = _this->m_eDirection == SD_VERTICAL ? g_pResManager->m_hUpDownCursor : g_pResManager->m_hLeftRightCursor;
		else
			hCursor = g_pResManager->m_hArrowCursor;
		if (hCursor)
		{
			SetCursor(hCursor);
			return TRUE;
		}
		return FALSE;
	case WM_LBUTTONDOWN:
		if (GetCapture() != hwnd)
		{
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			if (_this->HitTest(x, y))
				SetCapture(hwnd);
		}
		return 0;
	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd)
		{
			nSplitterPos = _this->m_eDirection == SD_VERTICAL ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam);
			_this->SetSplitterPos(nSplitterPos);
		}
		return 0;
	case WM_LBUTTONUP:
		if (GetCapture() == hwnd)
			ReleaseCapture();
		return 0;
	case WM_CAPTURECHANGED:
		return 0;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_SIZE:
		nSplitterSize = _this->m_eDirection == SD_VERTICAL ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam);
		_this->ResizeSplitter(nSplitterSize);
		return 0;
	case WM_PAINT:
		hdc = (HDC)wParam;
		if (! hdc)
		{
			hdc = BeginPaint(hwnd, &ps);
			if (hdc)
			{
				_this->DrawSplitter(hdc);
				EndPaint(hwnd, &ps);
			}
		}
		else
			_this->DrawSplitter(hdc);
		return 0;
	case WM_PRINTCLIENT:
		hdc = (HDC)wParam;
		_this->DrawSplitter(hdc);
		return 0;
	case WM_GETDLGCODE:
		return DLGC_STATIC;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

/**
 * @param hwnd - splitter window handle.
 */
void CSplitter::Attach(HWND hwnd)
{
	_ASSERTE(hwnd != NULL);
	_ASSERTE(m_hwnd == NULL);
	m_hwnd = hwnd;
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	m_pfnOldSplitterWndProc = SubclassWindow(hwnd, SplitterWndProc);
	ResetSplitterPos();
	InvalidateRect(hwnd, NULL, TRUE);
}

void CSplitter::Detach(void)
{
	if (m_pfnOldSplitterWndProc)
	{
		SubclassWindow(m_hwnd, m_pfnOldSplitterWndProc);
		SetWindowLongPtr(m_hwnd, GWL_USERDATA, NULL);
		ClearSplitterPos();
		ClearPanels();
		InvalidateRect(m_hwnd, NULL, TRUE);
		m_pfnOldSplitterWndProc = NULL;
		m_hwnd = NULL;
	}
}

void CSplitter::ClearSplitterPos(void)
{
	m_nIdealPos = m_nSplitterPos = m_nSplitterSize = 0;
}

void CSplitter::ClearPanels(void)
{
	m_arrPanels[0] = NULL;
	m_arrPanels[1] = NULL;
}

/**
 * @return splitter size value.
 */
int CSplitter::CalcSplitterSize(void) const
{
	_ASSERTE(m_hwnd != NULL);
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	int nSplitterSize, nEdgeType, nFrameType;
	if (m_eDirection == SD_VERTICAL)
	{
		nSplitterSize = rect.bottom;
		nEdgeType = SM_CYEDGE;
		nFrameType = SM_CYFRAME;
	}
	else
	{
		nSplitterSize = rect.right;
		nEdgeType = SM_CXEDGE;
		nFrameType = SM_CXFRAME;
	}
	int nEdgeSize = GetSystemMetrics(nEdgeType) * 2;
	int nBarSize = GetSystemMetrics(nFrameType);
	nSplitterSize -= nBarSize;
	nSplitterSize -= nEdgeSize;
	if (nSplitterSize < 0)
		nSplitterSize = 0;
	return nSplitterSize;
}
