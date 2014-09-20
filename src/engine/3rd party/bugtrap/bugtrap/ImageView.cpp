/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Image view control class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "ImageView.h"
#include "BugTrapUtils.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define USE_MEM_DC

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES 0x0068
#endif

#ifndef SPI_SETWHEELSCROLLLINES
#define SPI_SETWHEELSCROLLLINES 0x0069
#endif

/// Default number of wheel lines.
static const DWORD g_dwDefaultNumWheels = 3;

void CImageView::InitVars(void)
{
	m_hwnd = NULL;
	m_pfnOldImageViewWndProc = NULL;
	m_lOldStyle = 0;
	m_nWheelLines = g_dwDefaultNumWheels;
	ResetImage();
}

/**
 * @param hdc - drawing conImage.
 * @param prcPaint - the rectangle in which the painting is requested.
 */
void CImageView::DrawImageView(HDC hdc, RECT* prcPaint)
{
	_ASSERTE(g_pResManager != NULL);
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	if (prcPaint == NULL)
		prcPaint = &rcClient;
	if (IsRectEmpty(prcPaint))
		return;

#ifdef USE_MEM_DC
	int nClientWidth = prcPaint->right - prcPaint->left;
	int nClientHeight = prcPaint->bottom - prcPaint->top;
	HBITMAP hbmpMem;
	hbmpMem = CreateCompatibleBitmap(hdc, nClientWidth, nClientHeight);
	if (hbmpMem == NULL)
		return;
	HDC hdcMem;
	hdcMem = CreateCompatibleDC(hdc);
	if (hdcMem == NULL)
	{
		DeleteBitmap(hbmpMem);
		return;
	}
	SetViewportOrgEx(hdcMem, -prcPaint->left, -prcPaint->top, NULL);
	HBITMAP hbmpSafe = SelectBitmap(hdcMem, hbmpMem);
#else
	// CS_PARENTDC sets the clipping rectangle of the child window to that of the parent window
	// so that the child can draw on the parent. Text view inherits this style from sub-classed
	// static control. This causes problems with unclipped TabbedTextOut() output.
	HRGN hrgn = CreateRectRgnIndirect(prcPaint);
	SelectClipRgn(hdc, hrgn);
	DeleteRgn(hrgn);
	HDC hdcMem = hdc;
#endif

	FillRect(hdcMem, prcPaint, g_pResManager->m_hbrWindowBrush);
	HDC hdcTemp = CreateCompatibleDC(hdc);
	if (hdcTemp)
	{
		HBITMAP hbmpSafe2 = SelectBitmap(hdcTemp, m_hAdjustedBitmap);
		int nHorPos = GetScrollPos(m_hwnd, SB_HORZ);
		int nVertPos = GetScrollPos(m_hwnd, SB_VERT);
		int nImageLeft = rcClient.right > m_szAjustedBitmapSize.cx ? (rcClient.right - m_szAjustedBitmapSize.cx) / 2 : -nHorPos;
		int nImageTop = rcClient.bottom > m_szAjustedBitmapSize.cy ? (rcClient.bottom - m_szAjustedBitmapSize.cy) / 2 : -nVertPos;
		BitBlt(hdcMem, nImageLeft, nImageTop, m_szAjustedBitmapSize.cx, m_szAjustedBitmapSize.cy, hdcTemp, 0, 0, SRCCOPY);
		SelectBitmap(hdcTemp, hbmpSafe2);
		DeleteDC(hdcTemp);
	}

#ifdef USE_MEM_DC
	BitBlt(hdc, prcPaint->left, prcPaint->top, nClientWidth, nClientHeight, hdcMem, prcPaint->left, prcPaint->top, SRCCOPY);
	SelectBitmap(hdcMem, hbmpSafe);
	DeleteDC(hdcMem);
	DeleteBitmap(hbmpMem);
#endif
}

/**
 * @param bIgnoreScrollPos - pass true to disregard any existing scrollbar styles.
 */
void CImageView::ResizeImageView(BOOL bIgnoreScrollPos)
{
	SCROLLINFO sinfo;
	RECT rcClient;

	GetClientRect(m_hwnd, &rcClient);
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	if (! bIgnoreScrollPos)
	{
		sinfo.fMask = SIF_POS;
		GetScrollInfo(m_hwnd, SB_HORZ, &sinfo);
	}
	else
		sinfo.nPos = 0;
	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sinfo.nMin = 0;
	sinfo.nMax = m_szAjustedBitmapSize.cx > 0 ? m_szAjustedBitmapSize.cx - 1 : 0;
	sinfo.nPage = rcClient.right;
	ValidateScrollInfo(&sinfo);
	SetScrollInfo(m_hwnd, SB_HORZ, &sinfo, TRUE);

	GetClientRect(m_hwnd, &rcClient);
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	if (! bIgnoreScrollPos)
	{
		sinfo.fMask = SIF_POS;
		GetScrollInfo(m_hwnd, SB_VERT, &sinfo);
	}
	else
		sinfo.nPos = 0;
	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sinfo.nMin = 0;
	sinfo.nMax = m_szAjustedBitmapSize.cy > 0 ? m_szAjustedBitmapSize.cy - 1 : 0;
	sinfo.nPage = rcClient.bottom;
	ValidateScrollInfo(&sinfo);
	SetScrollInfo(m_hwnd, SB_VERT, &sinfo, TRUE);

	InvalidateRect(m_hwnd, NULL, FALSE);
}

/**
 * @param nScrollBarType - scrollbar type.
 * @param nScrollCode - scroll code.
 */
void CImageView::ScrollImageView(int nScrollBarType, int nScrollCode)
{
	_ASSERTE(nScrollBarType == SB_HORZ || nScrollBarType == SB_VERT);

	SCROLLINFO sinfo;
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	if (nScrollCode == SB_THUMBTRACK)
		sinfo.fMask |= SIF_TRACKPOS;
	GetScrollInfo(m_hwnd, nScrollBarType, &sinfo);
	int nOldPos = sinfo.nPos;

	int nLineOffset = LOWORD(GetDialogBaseUnits());

	switch (nScrollCode)
	{
	case SB_LINEUP:
	//case SB_LINELEFT:
		sinfo.nPos -= nLineOffset;
		break;
	case SB_LINEDOWN:
	//case SB_LINERIGHT:
		sinfo.nPos += nLineOffset;
		break;
	case SB_PAGEUP:
	//case SB_PAGELEFT:
		sinfo.nPos -= sinfo.nPage;
		break;
	case SB_PAGEDOWN:
	//case SB_PAGERIGHT:
		sinfo.nPos += sinfo.nPage;
		break;
	case SB_THUMBTRACK:
		sinfo.nPos = sinfo.nTrackPos;
		break;
	case SB_TOP:
	//case SB_LEFT:
		sinfo.nPos = 0;
		break;
	case SB_BOTTOM:
	//case SB_RIGHT:
		sinfo.nPos = sinfo.nMax;
		break;
	default:
		return;
	}
	ValidateScrollInfo(&sinfo);
	sinfo.fMask = SIF_POS;
	SetScrollInfo(m_hwnd, nScrollBarType, &sinfo, TRUE);

	if (sinfo.nPos != nOldPos)
	{
		int nHorOffset, nVertOffset;
		if (nScrollBarType == SB_HORZ)
		{
			nHorOffset = nOldPos - sinfo.nPos;
			nVertOffset = 0;
		}
		else
		{
			nHorOffset = 0;
			nVertOffset = nOldPos - sinfo.nPos;
		}

		RECT rcClient;
		GetClientRect(m_hwnd, &rcClient);
		// Explicitly clip output to override style CS_PARENTDC.
		ScrollWindowEx(m_hwnd, nHorOffset, nVertOffset, &rcClient, &rcClient, NULL, NULL, SW_INVALIDATE);
		UpdateWindow(m_hwnd);
	}
}

/**
 * @param hwnd - window handle.
 * @param uMsg - message identifier.
 * @param wParam - first message parameter.
 * @param lParam - second message parameter.
 */
LRESULT CALLBACK CImageView::ImageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int zDelta, zTotal, nScrollCode, nScrollBarType;
	LONG lWindowStyle;

	CImageView* _this  = (CImageView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	_ASSERTE(_this != NULL);
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		return 0;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		hdc = (HDC)wParam;
		if (! hdc)
		{
			hdc = BeginPaint(hwnd, &ps);
			if (hdc)
			{
				_this->DrawImageView(hdc, &ps.rcPaint);
				EndPaint(hwnd, &ps);
			}
		}
		else
			_this->DrawImageView(hdc, NULL);
		return 0;
	case WM_PRINTCLIENT:
		hdc = (HDC)wParam;
		_this->DrawImageView(hdc, NULL);
		return 0;
	case WM_SIZE:
		_this->ResizeImageView(FALSE);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			_this->ScrollImageView(SB_HORZ, SB_LINELEFT);
			break;
		case VK_UP:
			_this->ScrollImageView(SB_VERT, SB_LINEUP);
			break;
		case VK_RIGHT:
			_this->ScrollImageView(SB_HORZ, SB_LINERIGHT);
			break;
		case VK_DOWN:
			_this->ScrollImageView(SB_VERT, SB_LINEDOWN);
			break;
		case VK_HOME:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollImageView(SB_VERT, SB_TOP);
			else
				_this->ScrollImageView(SB_HORZ, SB_LEFT);
			break;
		case VK_END:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollImageView(SB_VERT, SB_BOTTOM);
			else
				_this->ScrollImageView(SB_HORZ, SB_RIGHT);
			break;
		case VK_PRIOR:
			_this->ScrollImageView(SB_VERT, SB_PAGEUP);
			break;
		case VK_NEXT:
			_this->ScrollImageView(SB_VERT, SB_PAGEDOWN);
			break;
		case VK_ADD:
			_this->ZoomIn();
			break;
		case VK_SUBTRACT:
			_this->ZoomOut();
			break;
		case VK_MULTIPLY:
			_this->FitImage();
			break;
		case VK_DIVIDE:
		case VK_NUMPAD0:
		case _T('0'):
			_this->ResetSize();
			break;
		}
		return 0;
	case WM_HSCROLL:
		_this->ScrollImageView(SB_HORZ, LOWORD(wParam));
		return 0;
	case WM_VSCROLL:
		_this->ScrollImageView(SB_VERT, LOWORD(wParam));
		return 0;
	case WM_MOUSEWHEEL:
		zDelta = (int)(short)HIWORD(wParam);
		if (_this->m_nWheelLines == WHEEL_PAGESCROLL)
			nScrollCode = zDelta > 0 ? SB_PAGEUP : SB_PAGEDOWN;
		else
			nScrollCode = zDelta > 0 ? SB_LINEUP : SB_LINEDOWN;
		zTotal = abs(zDelta) / WHEEL_DELTA;
		if (_this->m_nWheelLines != WHEEL_PAGESCROLL)
			zTotal *= _this->m_nWheelLines;
		lWindowStyle = GetWindowLong(hwnd, GWL_STYLE);

		if (lWindowStyle & WS_VSCROLL)
			nScrollBarType = SB_VERT;
		else if (lWindowStyle & WS_HSCROLL)
			nScrollBarType = SB_HORZ;
		else
			return 0;
		for (int i = 0; i < zTotal; ++i)
			_this->ScrollImageView(nScrollBarType, nScrollCode);
		return 0;
	case WM_SETTINGCHANGE:
		if (wParam == SPI_SETWHEELSCROLLLINES)
		{
			if (! SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &_this->m_nWheelLines, 0))
				_this->m_nWheelLines = g_dwDefaultNumWheels;
		}
		return 0;
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

/**
 * @param hwnd - image view window handle.
 */
void CImageView::Attach(HWND hwnd)
{
	_ASSERTE(hwnd != NULL);
	_ASSERTE(m_hwnd == NULL);
	_ASSERTE(g_pResManager != NULL);

	if (! SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_nWheelLines, 0))
		m_nWheelLines = g_dwDefaultNumWheels;

	m_hwnd = hwnd;
	m_pfnOldImageViewWndProc = SubclassWindow(hwnd, ImageViewWndProc);
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	// Preserve original window styles that could be modified by SetScrollInfo().
	m_lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
	ResizeImageView(TRUE);
}

void CImageView::Detach(void)
{
	if (m_pfnOldImageViewWndProc)
	{
		SubclassWindow(m_hwnd, m_pfnOldImageViewWndProc);
		SetWindowLongPtr(m_hwnd, GWL_USERDATA, NULL);

		SCROLLINFO sinfo;
		ZeroMemory(&sinfo, sizeof(sinfo));
		sinfo.cbSize = sizeof(sinfo);
		sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
		SetScrollInfo(m_hwnd, SB_HORZ, &sinfo, FALSE);
		SetScrollInfo(m_hwnd, SB_VERT, &sinfo, FALSE);
		SetWindowLong(m_hwnd, GWL_STYLE, m_lOldStyle);

		InvalidateRect(m_hwnd, NULL, TRUE);
		InitVars();
	}
}

/**
 * @param hBitmap - image handle.
 */
void CImageView::SetImage(HBITMAP hBitmap)
{
	_ASSERTE(hBitmap != NULL);
	_ASSERTE(m_hBitmap == NULL);
	_ASSERTE(m_hwnd != NULL);
	m_hBitmap = hBitmap;
	BITMAP bmpInfo;
	GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
	m_szBitmapSize.cx = bmpInfo.bmWidth;
	m_szBitmapSize.cy = bmpInfo.bmHeight;
	FitImage();
}

/**
 * @param prcClient - pointer to client rectangle.
 */
void CImageView::GetBiggestClientRect(RECT* prcClient)
{
	GetWindowRect(m_hwnd, prcClient);

	prcClient->right -= prcClient->left;
	prcClient->left = 0;
	int nCXEdge = GetSystemMetrics(SM_CXEDGE) * 2;
	if (prcClient->right >= nCXEdge)
		prcClient->right -= nCXEdge;
	else
		prcClient->right = 0;

	prcClient->bottom -= prcClient->top;
	prcClient->top = 0;
	int nCYEdge = GetSystemMetrics(SM_CYEDGE) * 2;
	if (prcClient->bottom >= nCYEdge)
		prcClient->bottom -= nCYEdge;
	else
		prcClient->bottom = 0;
}

void CImageView::FitImage(void)
{
	RECT rcClient;
	GetBiggestClientRect(&rcClient);

	if (rcClient.right < m_szBitmapSize.cx || rcClient.bottom < m_szBitmapSize.cy)
	{
		int nProduct1 = rcClient.right * m_szBitmapSize.cy;
		int nProduct2 = rcClient.bottom * m_szBitmapSize.cx;
		if (nProduct1 < nProduct2)
		{
			m_szAjustedBitmapSize.cx = rcClient.right;
			m_szAjustedBitmapSize.cy = nProduct1 / m_szBitmapSize.cx;
		}
		else
		{
			m_szAjustedBitmapSize.cx = nProduct2 / m_szBitmapSize.cy;
			m_szAjustedBitmapSize.cy = rcClient.bottom;
		}
	}
	else
		m_szAjustedBitmapSize = m_szBitmapSize;
	ResizeImage();
	ResizeImageView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CImageView::ResetSize(void)
{
	m_szAjustedBitmapSize = m_szBitmapSize;
	ResizeImage();
	ResizeImageView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CImageView::ResetImage(void)
{
	if (m_hAdjustedBitmap)
	{
		DeleteBitmap(m_hAdjustedBitmap);
		m_hAdjustedBitmap = NULL;
	}
	m_hBitmap = NULL;
}

void CImageView::ResizeImage(void)
{
	if (m_hAdjustedBitmap)
	{
		DeleteBitmap(m_hAdjustedBitmap);
		m_hAdjustedBitmap = NULL;
	}
	HDC hdc = GetDC(m_hwnd);
	if (hdc)
	{
		HDC hdcSrc = CreateCompatibleDC(hdc);
		if (hdcSrc)
		{
			HBITMAP hbmpSafe = SelectBitmap(hdcSrc, m_hBitmap);
			m_hAdjustedBitmap = CreateCompatibleBitmap(hdc, m_szAjustedBitmapSize.cx, m_szAjustedBitmapSize.cy);
			if (m_hAdjustedBitmap)
			{
				HDC hdcDest = CreateCompatibleDC(hdc);
				if (hdcDest)
				{
					HBITMAP hbmpSafe2 = SelectBitmap(hdcDest, m_hAdjustedBitmap);
					StretchBlt(hdcDest, 0, 0, m_szAjustedBitmapSize.cx, m_szAjustedBitmapSize.cy, hdcSrc, 0, 0, m_szBitmapSize.cx, m_szBitmapSize.cy, SRCCOPY);
					SelectBitmap(hdcDest, hbmpSafe2);
					DeleteDC(hdcDest);
				}
				else
				{
					DeleteBitmap(m_hAdjustedBitmap);
					m_hAdjustedBitmap = NULL;
				}
			}
			SelectBitmap(hdcSrc, hbmpSafe);
			DeleteDC(hdcSrc);
		}
		ReleaseDC(m_hwnd, hdc);
	}
}

void CImageView::ZoomIn(void)
{
	int nHorPos = GetScrollPos(m_hwnd, SB_HORZ);
	int nVertPos = GetScrollPos(m_hwnd, SB_VERT);
	float fltRatio = (float)m_szAjustedBitmapSize.cx / (float)m_szBitmapSize.cx * 3.0f / 2.0f;
	SIZE szOldAjustedBitmapSize = m_szAjustedBitmapSize;
	m_szAjustedBitmapSize.cx = (int)((float)m_szBitmapSize.cx * fltRatio);
	m_szAjustedBitmapSize.cy = (int)((float)m_szBitmapSize.cy * fltRatio);
	ResizeImage();
	if (m_hAdjustedBitmap != NULL)
	{
		ResizeImageView(FALSE);
		SetScrollPos(m_hwnd, SB_HORZ, nHorPos * 3 / 2, TRUE);
		SetScrollPos(m_hwnd, SB_VERT, nVertPos * 3 / 2, TRUE);
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	else
	{
		m_szAjustedBitmapSize = szOldAjustedBitmapSize;
		ResizeImage();
	}
}

void CImageView::ZoomOut(void)
{
	if (m_szAjustedBitmapSize.cx > 3 && m_szAjustedBitmapSize.cy > 3)
	{
		int nHorPos = GetScrollPos(m_hwnd, SB_HORZ);
		int nVertPos = GetScrollPos(m_hwnd, SB_VERT);
		float fltRatio = (float)m_szAjustedBitmapSize.cx / (float)m_szBitmapSize.cx * 2.0f / 3.0f;
		SIZE szOldAjustedBitmapSize = m_szAjustedBitmapSize;
		m_szAjustedBitmapSize.cx = (int)((float)m_szBitmapSize.cx * fltRatio);
		m_szAjustedBitmapSize.cy = (int)((float)m_szBitmapSize.cy * fltRatio);
		ResizeImage();
		if (m_hAdjustedBitmap != NULL)
		{
			ResizeImageView(FALSE);
			SetScrollPos(m_hwnd, SB_HORZ, nHorPos * 2 / 3, TRUE);
			SetScrollPos(m_hwnd, SB_VERT, nVertPos * 2 / 3, TRUE);
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
		else
		{
			m_szAjustedBitmapSize = szOldAjustedBitmapSize;
			ResizeImage();
		}
	}
}
