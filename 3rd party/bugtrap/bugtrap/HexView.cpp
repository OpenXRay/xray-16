/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Hex view control class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "HexView.h"
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

void CHexView::InitVars(void)
{
	m_hwnd = NULL;
	m_pfnOldHexViewWndProc = NULL;
	m_lOldStyle = 0;
	m_nWheelLines = g_dwDefaultNumWheels;
	ResetFile();
}

/**
 * @param hdc - drawing conHex.
 * @param prcPaint - the rectangle in which the painting is requested.
 */
void CHexView::DrawHexView(HDC hdc, RECT* prcPaint)
{
	_ASSERTE(g_pResManager != NULL);
	if (prcPaint == NULL)
	{
		RECT rcClient;
		GetClientRect(m_hwnd, &rcClient);
		prcPaint = &rcClient;
	}
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

	COLORREF rgbOldTextColor = SetTextColor(hdcMem, GetSysColor(COLOR_WINDOWTEXT));
	COLORREF rgbOldBackground = SetBkColor(hdcMem, GetSysColor(COLOR_WINDOW));
	HFONT hOldFont = g_pResManager->m_hFixedFont ? SelectFont(hdcMem, g_pResManager->m_hFixedFont) : NULL;
	TEXTMETRIC tmetr;
	::GetTextMetrics(hdcMem, &tmetr);

	DWORD dwNumLines = m_dwFileSize / LINE_WIDTH;
	if (m_dwFileSize % LINE_WIDTH)
		++dwNumLines;
	DWORD dwTopLineNum = GetScrollPos(m_hwnd, SB_VERT);
	DWORD dwTopVisLineNum = dwTopLineNum + prcPaint->top / tmetr.tmHeight;

	if (dwTopVisLineNum < dwNumLines)
	{
		int nHorPos = tmetr.tmAveCharWidth - GetScrollPos(m_hwnd, SB_HORZ);
		int nVertPos = prcPaint->top - prcPaint->top % tmetr.tmHeight, nTopPos = nVertPos;
		DWORD dwNumVisLines = prcPaint->bottom / tmetr.tmHeight;
		if (prcPaint->bottom % tmetr.tmHeight)
			++dwNumVisLines;
		DWORD dwBottomVisLineNum = dwTopLineNum + dwNumVisLines - 1;
		if (dwBottomVisLineNum >= dwNumLines)
			dwBottomVisLineNum = dwNumLines - 1;
		int nBottomPos = nVertPos + (dwBottomVisLineNum - dwTopVisLineNum + 1) * tmetr.tmHeight;

		int nOffsetWidth = 10 * tmetr.tmAveCharWidth;
		int nLineWidth = (3 * LINE_WIDTH + 2) * tmetr.tmAveCharWidth;
		int nLinePos = nOffsetWidth + nLineWidth - tmetr.tmAveCharWidth * 3 / 2;

		for (DWORD dwLineNum = dwTopVisLineNum; dwLineNum <= dwBottomVisLineNum; ++dwLineNum)
		{
			CacheLine(dwLineNum);
			TCHAR szHexLine[3 * LINE_WIDTH], szTextLine[2 * LINE_WIDTH];
			PBYTE pbLine = m_pbCache + (dwLineNum - m_dwFirstCachedLine) * LINE_WIDTH;
			DWORD dwOffset = dwLineNum * LINE_WIDTH, dwNumBytes = m_dwFileSize - dwOffset;
			if (dwNumBytes > LINE_WIDTH)
				dwNumBytes = LINE_WIDTH;
			DWORD dwLinePos = 0, dwCharPos = 0;
			for (DWORD dwBytePos = 0; dwBytePos < dwNumBytes; ++dwBytePos)
			{
				BYTE bValue = pbLine[dwBytePos];
				BYTE bDigit = (bValue >> 4) & 0x0F;
				szHexLine[dwLinePos++] = bDigit < 0x0A ? bDigit + _T('0') : bDigit - 0x0A + _T('A');
				bDigit = bValue & 0x0F;
				szHexLine[dwLinePos++] = bDigit < 0x0A ? bDigit + _T('0') : bDigit - 0x0A + _T('A');
				szHexLine[dwLinePos++] = _T(' ');
				WORD wCharType = 0;
				GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, (PCSTR)&bValue, 1, &wCharType);
				if (wCharType & (C1_ALPHA | C1_DIGIT | C1_PUNCT) || bValue == ' ')
				{
#ifdef _UNICODE
					dwCharPos += MultiByteToWideChar(CP_ACP, 0, (PCSTR)&bValue, 1, szTextLine + dwCharPos, 2);
#else
					szTextLine[dwCharPos++] = bValue;
#endif
				}
				else
					szTextLine[dwCharPos++] = _T('.');
			}

			TCHAR szOffset[11];
			_stprintf_s(szOffset, countof(szOffset), _T("%08X: "), dwOffset);
			int nPosition = nHorPos;
			TextOut(hdcMem, nPosition, nVertPos, szOffset, 10);
			nPosition += nOffsetWidth;
			TextOut(hdcMem, nPosition, nVertPos, szHexLine, dwLinePos);
			nPosition += nLineWidth;
			TextOut(hdcMem, nPosition, nVertPos, szTextLine, dwCharPos);
			nVertPos += tmetr.tmHeight;
		}

		int nPosition = nHorPos + nLinePos;
		MoveToEx(hdcMem, nPosition, nTopPos, NULL);
		LineTo(hdcMem, nPosition, nBottomPos);
	}

	SetTextColor(hdcMem, rgbOldTextColor);
	SetBkColor(hdcMem, rgbOldBackground);
	if (hOldFont)
		SelectFont(hdcMem, hOldFont);

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
void CHexView::ResizeHexView(BOOL bIgnoreScrollPos)
{
	TEXTMETRIC tmetr;
	GetTextMetrics(&tmetr);
	SCROLLINFO sinfo;
	RECT rcClient;

	GetClientRect(m_hwnd, &rcClient);
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_POS | SIF_RANGE;
	GetScrollInfo(m_hwnd, SB_HORZ, &sinfo);
	int nOldHorPos = ! bIgnoreScrollPos ? sinfo.nPos : 0;
	int nWidth = (10 + 3 * LINE_WIDTH + 2 + LINE_WIDTH) * tmetr.tmAveCharWidth;

	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sinfo.nMin = 0;
	if (sinfo.nMax < nWidth - 1)
		sinfo.nMax = nWidth - 1;
	sinfo.nPage = rcClient.right;
	ValidateScrollInfo(&sinfo);
	int nHorOffset = nOldHorPos - sinfo.nPos;
	SetScrollInfo(m_hwnd, SB_HORZ, &sinfo, TRUE);

	GetClientRect(m_hwnd, &rcClient);
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	int nOldVertPos;
	if (! bIgnoreScrollPos)
	{
		sinfo.fMask = SIF_POS;
		GetScrollInfo(m_hwnd, SB_VERT, &sinfo);
		nOldVertPos = sinfo.nPos;
	}
	else
		nOldVertPos = sinfo.nPos = 0;
	int nNumLines = m_dwFileSize / LINE_WIDTH;
	if (m_dwFileSize % LINE_WIDTH)
		++nNumLines;
	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sinfo.nMin = 0;
	sinfo.nMax = nNumLines > 0 ? nNumLines - 1 : 0;
	sinfo.nPage = rcClient.bottom / tmetr.tmHeight;
	ValidateScrollInfo(&sinfo);
	int nVertOffset = (nOldVertPos - sinfo.nPos) * tmetr.tmHeight;
	SetScrollInfo(m_hwnd, SB_VERT, &sinfo, TRUE);

	GetClientRect(m_hwnd, &rcClient);
	// Explicitly clip output to override style CS_PARENTDC.
	ScrollWindowEx(m_hwnd, nHorOffset, nVertOffset, &rcClient, &rcClient, NULL, NULL, SW_INVALIDATE);
	UpdateWindow(m_hwnd);
}

/**
 * @param pTextMetric - pointer to Hex metric structure.
 */
void CHexView::GetTextMetrics(PTEXTMETRIC pTextMetric)
{
	HDC hdc = GetDC(m_hwnd);
	HFONT hOldFont = g_pResManager->m_hFixedFont ? SelectFont(hdc, g_pResManager->m_hFixedFont) : NULL;
	::GetTextMetrics(hdc, pTextMetric);
	if (hOldFont)
		SelectFont(hdc, hOldFont);
	ReleaseDC(m_hwnd, hdc);
}

/**
 * @param nScrollBarType - scrollbar type.
 * @param nScrollCode - scroll code.
 */
void CHexView::ScrollHexView(int nScrollBarType, int nScrollCode)
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

	TEXTMETRIC tmetr;
	GetTextMetrics(&tmetr);
	int nLineOffset = nScrollBarType == SB_HORZ ? tmetr.tmAveCharWidth : 1;

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
			nVertOffset = (nOldPos - sinfo.nPos) * tmetr.tmHeight;
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
LRESULT CALLBACK CHexView::HexViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int zDelta, zTotal, nScrollCode, nScrollBarType;
	LONG lWindowStyle;

	CHexView* _this  = (CHexView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
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
				_this->DrawHexView(hdc, &ps.rcPaint);
				EndPaint(hwnd, &ps);
			}
		}
		else
			_this->DrawHexView(hdc, NULL);
		return 0;
	case WM_PRINTCLIENT:
		hdc = (HDC)wParam;
		_this->DrawHexView(hdc, NULL);
		return 0;
	case WM_SIZE:
		_this->ResizeHexView(FALSE);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			_this->ScrollHexView(SB_HORZ, SB_LINELEFT);
			break;
		case VK_UP:
			_this->ScrollHexView(SB_VERT, SB_LINEUP);
			break;
		case VK_RIGHT:
			_this->ScrollHexView(SB_HORZ, SB_LINERIGHT);
			break;
		case VK_DOWN:
			_this->ScrollHexView(SB_VERT, SB_LINEDOWN);
			break;
		case VK_HOME:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollHexView(SB_VERT, SB_TOP);
			else
				_this->ScrollHexView(SB_HORZ, SB_LEFT);
			break;
		case VK_END:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollHexView(SB_VERT, SB_BOTTOM);
			else
				_this->ScrollHexView(SB_HORZ, SB_RIGHT);
			break;
		case VK_PRIOR:
			_this->ScrollHexView(SB_VERT, SB_PAGEUP);
			break;
		case VK_NEXT:
			_this->ScrollHexView(SB_VERT, SB_PAGEDOWN);
			break;
		}
		return 0;
	case WM_HSCROLL:
		_this->ScrollHexView(SB_HORZ, LOWORD(wParam));
		return 0;
	case WM_VSCROLL:
		_this->ScrollHexView(SB_VERT, LOWORD(wParam));
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
			_this->ScrollHexView(nScrollBarType, nScrollCode);
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
 * @param hwnd - hex view window handle.
 */
void CHexView::Attach(HWND hwnd)
{
	_ASSERTE(hwnd != NULL);
	_ASSERTE(m_hwnd == NULL);
	_ASSERTE(g_pResManager != NULL);

	if (! SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_nWheelLines, 0))
		m_nWheelLines = g_dwDefaultNumWheels;

	m_hwnd = hwnd;
	m_pfnOldHexViewWndProc = SubclassWindow(hwnd, HexViewWndProc);
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	// Preserve original window styles that could be modified by SetScrollInfo().
	m_lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
	ResizeHexView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CHexView::Detach(void)
{
	if (m_pfnOldHexViewWndProc)
	{
		SubclassWindow(m_hwnd, m_pfnOldHexViewWndProc);
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
 * @param hFile - file handle.
 */
void CHexView::SetFile(HANDLE hFile)
{
	_ASSERTE(hFile != INVALID_HANDLE_VALUE);
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
	_ASSERTE(m_hwnd != NULL);
	m_hFile = hFile;
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	LoadCache();
	ResizeHexView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CHexView::ResetFile(void)
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_dwFirstCachedLine = m_dwFileSize = 0;
	delete[] m_pbCache;
	m_pbCache = NULL;
}

void CHexView::LoadCache(void)
{
	_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
	m_dwFileSize = GetFileSize(m_hFile, NULL);
	m_dwFirstCachedLine = 0;
	DWORD dwCacheSize = min(m_dwFileSize, CACHE_SIZE);
	m_pbCache = new BYTE[dwCacheSize];
	if (m_pbCache == NULL)
	{
		ResetFile();
		goto end;
	}
	DWORD dwNumRead = 0;
	ReadFile(m_hFile, m_pbCache, dwCacheSize, &dwNumRead, NULL);

end:
	ResizeHexView(TRUE);
}

/**
 * @param dwCachedLineNum - index of cached line.
 */
void CHexView::CacheLine(DWORD dwCachedLineNum)
{
	DWORD dwCacheSize = min(m_dwFileSize, CACHE_SIZE);
	DWORD dwNumCachedLines = dwCacheSize / LINE_WIDTH;
	if (dwCacheSize % LINE_WIDTH)
		++dwNumCachedLines;
	if (dwCachedLineNum >= m_dwFirstCachedLine && dwCachedLineNum < m_dwFirstCachedLine + dwNumCachedLines)
		return;

	DWORD dwFirstCachedLine = dwCachedLineNum > NUMBER_OF_CACHED_LINES / 2 ? dwCachedLineNum - NUMBER_OF_CACHED_LINES / 2 : 0;
	DWORD dwFirstMappedLine, dwNumMappedLines, dwFirstLoadedLine, dwNumLoadedLines, dwFromMemOffset, dwToMemOffset, dwLoadOffset;
	if (m_dwFirstCachedLine <= dwFirstCachedLine && dwFirstCachedLine < m_dwFirstCachedLine + dwNumCachedLines)
	{
		dwFirstMappedLine = dwFirstCachedLine;
		dwNumMappedLines = dwNumCachedLines - (dwFirstCachedLine - m_dwFirstCachedLine);
		dwFirstLoadedLine = dwFirstMappedLine + dwNumMappedLines;
		dwNumLoadedLines = dwNumCachedLines - dwNumMappedLines;
		dwFromMemOffset = (dwFirstMappedLine - m_dwFirstCachedLine) * LINE_WIDTH;
		dwToMemOffset = 0;
		dwLoadOffset = dwNumMappedLines * LINE_WIDTH;
	}
	else if (dwFirstCachedLine <= m_dwFirstCachedLine && m_dwFirstCachedLine < dwFirstCachedLine + dwNumCachedLines)
	{
		dwFirstMappedLine = m_dwFirstCachedLine;
		dwNumMappedLines = dwNumCachedLines - (m_dwFirstCachedLine - dwFirstCachedLine);
		dwFirstLoadedLine = dwFirstCachedLine;
		dwNumLoadedLines = dwNumCachedLines - dwNumMappedLines;
		dwFromMemOffset = (dwFirstMappedLine - m_dwFirstCachedLine) * LINE_WIDTH;
		dwToMemOffset = dwNumLoadedLines * LINE_WIDTH;
		dwLoadOffset = 0;
	}
	else
	{
		dwFirstMappedLine = dwNumMappedLines = dwFromMemOffset = dwToMemOffset = 0;
		dwFirstLoadedLine = dwFirstCachedLine;
		dwNumLoadedLines = dwNumCachedLines;
		dwLoadOffset = 0;
	}

	_ASSERTE(m_pbCache != NULL);
	if (dwNumMappedLines > 0)
		MoveMemory(m_pbCache + dwToMemOffset, m_pbCache + dwFromMemOffset, dwNumMappedLines * LINE_WIDTH);

	if (dwNumLoadedLines > 0)
	{
		SetFilePointer(m_hFile, dwFirstLoadedLine * LINE_WIDTH, NULL, FILE_BEGIN);
		DWORD dwNumRead = 0;
		ReadFile(m_hFile, m_pbCache + dwLoadOffset, dwNumLoadedLines * LINE_WIDTH, &dwNumRead, NULL);
	}

	m_dwFirstCachedLine = dwFirstCachedLine;
}
