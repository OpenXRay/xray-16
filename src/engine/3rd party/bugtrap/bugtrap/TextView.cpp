/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text view control class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "TextView.h"
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

/// Ellipsis string.
static const TCHAR g_szEllipsis[] = _T("...");
/// Length of ellipsis string.
static const DWORD g_dwEllipsisLength = countof(g_szEllipsis) - 1;
/// Default number of wheel lines.
static const DWORD g_dwDefaultNumWheels = 3;

void CTextView::InitVars(void)
{
	m_hwnd = NULL;
	m_pfnOldTextViewWndProc = NULL;
	m_lOldStyle = 0;
	m_nWheelLines = g_dwDefaultNumWheels;
	ResetFile();
}

/**
 * @param hdc - drawing context.
 * @param prcPaint - the rectangle in which the painting is requested.
 */
void CTextView::DrawTextView(HDC hdc, RECT* prcPaint)
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

	DWORD dwNumLines = m_arrLines.GetCount();
	DWORD dwTopLineNum = GetScrollPos(m_hwnd, SB_VERT);
	DWORD dwTopVisLineNum = dwTopLineNum + prcPaint->top / tmetr.tmHeight;

	if (dwTopVisLineNum < dwNumLines)
	{
		int nHorPos = tmetr.tmAveCharWidth - GetScrollPos(m_hwnd, SB_HORZ);
		int nVertPos = prcPaint->top - prcPaint->top % tmetr.tmHeight;
		DWORD dwNumVisLines = prcPaint->bottom / tmetr.tmHeight;
		if (prcPaint->bottom % tmetr.tmHeight)
			++dwNumVisLines;
		DWORD dwBottomVisLineNum = dwTopLineNum + dwNumVisLines - 1;
		if (dwBottomVisLineNum >= dwNumLines)
			dwBottomVisLineNum = dwNumLines - 1;

		for (DWORD dwLineNum = dwTopVisLineNum; dwLineNum <= dwBottomVisLineNum; ++dwLineNum)
		{
			CacheLine(dwLineNum);
			const CLineInfo& rLineInfo = m_arrLines[(int)dwLineNum];
			int nTextWidth = LOWORD(TabbedTextOut(hdcMem, nHorPos, nVertPos, m_pTextCache + rLineInfo.m_dwTextStart, rLineInfo.m_dwLength, 0, NULL, -nHorPos));
			if (rLineInfo.m_bTruncated)
				TextOut(hdcMem, nHorPos + nTextWidth, nVertPos, g_szEllipsis, g_dwEllipsisLength);
			nVertPos += tmetr.tmHeight;
		}
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
void CTextView::ResizeTextView(BOOL bIgnoreScrollPos)
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

	sinfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sinfo.nMin = 0;
	if (sinfo.nMax < m_nMaxLineWidth - 1)
		sinfo.nMax = m_nMaxLineWidth - 1;
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
	int nNumLines = m_arrLines.GetCount();
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
 * @param pTextMetric - pointer to text metric structure.
 */
void CTextView::GetTextMetrics(PTEXTMETRIC pTextMetric)
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
void CTextView::ScrollTextView(int nScrollBarType, int nScrollCode)
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
LRESULT CALLBACK CTextView::TextViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int zDelta, zTotal, nScrollCode, nScrollBarType;
	LONG lWindowStyle;

	CTextView* _this  = (CTextView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
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
				_this->DrawTextView(hdc, &ps.rcPaint);
				EndPaint(hwnd, &ps);
			}
		}
		else
			_this->DrawTextView(hdc, NULL);
		return 0;
	case WM_PRINTCLIENT:
		hdc = (HDC)wParam;
		_this->DrawTextView(hdc, NULL);
		return 0;
	case WM_SIZE:
		_this->ResizeTextView(FALSE);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			_this->ScrollTextView(SB_HORZ, SB_LINELEFT);
			break;
		case VK_UP:
			_this->ScrollTextView(SB_VERT, SB_LINEUP);
			break;
		case VK_RIGHT:
			_this->ScrollTextView(SB_HORZ, SB_LINERIGHT);
			break;
		case VK_DOWN:
			_this->ScrollTextView(SB_VERT, SB_LINEDOWN);
			break;
		case VK_HOME:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollTextView(SB_VERT, SB_TOP);
			else
				_this->ScrollTextView(SB_HORZ, SB_LEFT);
			break;
		case VK_END:
			if (GetKeyState(VK_CONTROL) < 0)
				_this->ScrollTextView(SB_VERT, SB_BOTTOM);
			else
				_this->ScrollTextView(SB_HORZ, SB_RIGHT);
			break;
		case VK_PRIOR:
			_this->ScrollTextView(SB_VERT, SB_PAGEUP);
			break;
		case VK_NEXT:
			_this->ScrollTextView(SB_VERT, SB_PAGEDOWN);
			break;
		}
		return 0;
	case WM_HSCROLL:
		_this->ScrollTextView(SB_HORZ, LOWORD(wParam));
		return 0;
	case WM_VSCROLL:
		_this->ScrollTextView(SB_VERT, LOWORD(wParam));
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
			_this->ScrollTextView(nScrollBarType, nScrollCode);
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
 * @param hwnd - text view window handle.
 */
void CTextView::Attach(HWND hwnd)
{
	_ASSERTE(hwnd != NULL);
	_ASSERTE(m_hwnd == NULL);
	_ASSERTE(g_pResManager != NULL);

	if (! SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_nWheelLines, 0))
		m_nWheelLines = g_dwDefaultNumWheels;

	m_hwnd = hwnd;
	m_pfnOldTextViewWndProc = SubclassWindow(hwnd, TextViewWndProc);
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	// Preserve original window styles that could be modified by SetScrollInfo().
	m_lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
	ResizeTextView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CTextView::Detach(void)
{
	if (m_pfnOldTextViewWndProc)
	{
		SubclassWindow(m_hwnd, m_pfnOldTextViewWndProc);
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
 * @param eEncoding - text encoding.
 * @param dwSignatureSize - size of file signature.
 */
void CTextView::SetFile(HANDLE hFile, TEXT_ENCODING eEncoding, DWORD dwSignatureSize)
{
	_ASSERTE(hFile != INVALID_HANDLE_VALUE);
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
	_ASSERTE(m_hwnd != NULL);
	m_hFile = hFile;
	m_pDecoder = CBaseDecoder::GetDecoder(eEncoding);
	_ASSERTE(m_pDecoder != NULL);
	SetFilePointer(hFile, dwSignatureSize, NULL, FILE_BEGIN);
	CountLines();
	LoadCache();
	ResizeTextView(TRUE);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void CTextView::ResetFile(void)
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_pDecoder = NULL;
	m_arrLines.DeleteAll(true);
	m_dwFirstCachedLine = m_dwNumCachedLines = 0;
	delete[] m_pTextCache;
	m_pTextCache = NULL;
	m_dwLineBufferSize = 0;
	m_nMaxLineWidth = 0;
	delete[] m_pLineBuffer;
	m_pLineBuffer = NULL;
}

void CTextView::CountLines(void)
{
	_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
	m_arrLines.DeleteAll(false);

	DWORD dwFileSize = GetFileSize(m_hFile, NULL);
	DWORD dwFilePos = SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
	dwFileSize -= dwFilePos;
	DWORD dwBufferSize = min(dwFileSize, g_dwMaxBufferSize);
	PBYTE pFileBuffer = new BYTE[dwBufferSize];
	if (pFileBuffer == NULL)
		return;

	m_arrLines.EnsureSize(dwFileSize / APROX_NUMBER_OF_SYMBOLS, false); // approximate number of lines
	DWORD dwBufferPos = 0, dwNumSymbols = 0, dwNumChars = 0, dwLineStartPos = dwFilePos, dwLineEndPos = dwFilePos;
	TCHAR chPrevLineEnd = _T('\0');
	m_dwLineBufferSize = 0;

	for (;;)
	{
		DWORD dwNumRead = 0, dwFreeSize = dwBufferSize - dwBufferPos;
		if (! ReadFile(m_hFile, pFileBuffer + dwBufferPos, dwFreeSize, &dwNumRead, NULL))
			break;
		BOOL bEndOfFile = dwNumRead < dwFreeSize;
		dwNumRead += dwBufferPos;
		dwBufferPos = 0;
		if (dwNumRead == 0)
			break;
		for (;;)
		{
			DWORD dwBytesLeft = dwNumRead - dwBufferPos;
			// one character in UTF-16 encoding may require up to 4 bytes
			// one character in UTF-8 encoding may require up to 4 bytes
			// one character in ANSI encoding may require up to 2 bytes
			// make sure at least one character can be read from the buffer
			// (i.e. at least 4 bytes should be available)
			if (dwBytesLeft < 4 && ! bEndOfFile)
			{
				MoveMemory(pFileBuffer, pFileBuffer + dwBufferPos, dwBytesLeft);
				dwBufferPos = dwBytesLeft;
				break;
			}
			BOOL bGotoNewLine, bSkipLineEnd, bLineEndChar;
			if (dwBytesLeft > 0)
			{
				TCHAR arrChar[2];
				int nNumBytesInChar, nCharSize;
				nNumBytesInChar = m_pDecoder->DecodeChar(pFileBuffer + dwBufferPos, dwBytesLeft, arrChar, nCharSize);
				if (nCharSize <= 0)
				{
					++dwBufferPos;
					++dwFilePos;
					++dwLineEndPos;
					continue;
				}
				else
				{
					dwBufferPos += nNumBytesInChar;
					dwFilePos += nNumBytesInChar;
				}
				if (nCharSize == 1 && (arrChar[0] == _T('\r') || arrChar[0] == _T('\n')))
				{
					bGotoNewLine = bLineEndChar = TRUE;
					if (chPrevLineEnd != _T('\0') && chPrevLineEnd != arrChar[0])
					{
						chPrevLineEnd = _T('\0');
						bSkipLineEnd = TRUE;
					}
					else
					{
						chPrevLineEnd = arrChar[0];
						bSkipLineEnd = FALSE;
					}
				}
				else
				{
					++dwNumSymbols;
					dwNumChars += nCharSize;
					bGotoNewLine = dwNumSymbols == MAX_NUMBER_OF_SYMBOLS;
					chPrevLineEnd = _T('\0');
					bSkipLineEnd = bLineEndChar = FALSE;
					dwLineEndPos = dwFilePos;
				}
			}
			else
			{
				bGotoNewLine = TRUE;
				bSkipLineEnd = FALSE;
				bLineEndChar = TRUE;
			}
			if (bGotoNewLine)
			{
				if (! bSkipLineEnd)
				{
					CLineInfo LineInfo;
					LineInfo.m_dwLineStart = dwLineStartPos;
					LineInfo.m_dwLength = dwNumChars;
					LineInfo.m_dwSize = dwLineEndPos - dwLineStartPos;
					LineInfo.m_bTruncated = ! bLineEndChar;
					m_arrLines.AddItem(LineInfo);
					if (m_dwLineBufferSize < LineInfo.m_dwSize)
						m_dwLineBufferSize = LineInfo.m_dwSize;
					if (dwBytesLeft == 0)
						goto end;
				}
				dwLineStartPos = dwLineEndPos = dwFilePos;
				dwNumSymbols = 0;
				dwNumChars = 0;
			}
		}
	}
end:
	delete[] pFileBuffer;
	// reserve space for two line end characters in order to retrieve contiguous lines using ReadFile() sequentially
	// (for Unicode encoding this requires 4 bytes)
	m_dwLineBufferSize += 4;
	// add some extra space to the buffer to let several lines to be loaded
	if (m_dwLineBufferSize < LINE_BUFFER_SIZE)
		m_dwLineBufferSize = LINE_BUFFER_SIZE;
	m_pLineBuffer = new BYTE[m_dwLineBufferSize];
	if (m_pLineBuffer == NULL)
		ResetFile();
}

void CTextView::LoadCache(void)
{
	_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
	m_dwFirstCachedLine = m_dwNumCachedLines = 0;
	m_nMaxLineWidth = 0;
	DWORD dwNumLines = m_arrLines.GetCount();
	if (dwNumLines > 0)
	{
		HDC hdc = GetDC(m_hwnd);
		HFONT hOldFont = g_pResManager->m_hFixedFont ? SelectFont(hdc, g_pResManager->m_hFixedFont) : NULL;
		TEXTMETRIC tmetr;
		::GetTextMetrics(hdc, &tmetr);
		int nExtraWidth = tmetr.tmAveCharWidth + LOWORD(GetTabbedTextExtent(hdc, g_szEllipsis, g_dwEllipsisLength, 0, NULL));

		if (m_pTextCache == NULL)
		{
			_ASSERTE(MAX_NUMBER_OF_SYMBOLS <= TEXT_CACHE_SIZE);
			m_pTextCache = new TCHAR[TEXT_CACHE_SIZE];
			if (m_pTextCache == NULL)
			{
				ResetFile();
				goto end;
			}
		}
		const CLineInfo& rLineInfo = m_arrLines[0];
		SetFilePointer(m_hFile, rLineInfo.m_dwLineStart, NULL, FILE_BEGIN);
		DWORD dwLineBufferPos = 0, dwTextCachePos = 0;
		for (;;)
		{
			DWORD dwNumRead = 0;
			if (! ReadFile(m_hFile, m_pLineBuffer + dwLineBufferPos, m_dwLineBufferSize - dwLineBufferPos, &dwNumRead, NULL))
				goto end;
			dwNumRead += dwLineBufferPos;
			dwLineBufferPos = 0;
			if (dwNumRead == 0)
				break;
			for (;;)
			{
				DWORD dwBytesLeft = dwNumRead - dwLineBufferPos;
				if (dwBytesLeft == 0)
				{
					dwLineBufferPos = 0;
					break;
				}
				CLineInfo& rLineInfo = m_arrLines[(int)m_dwNumCachedLines];
				if (dwTextCachePos + rLineInfo.m_dwLength > TEXT_CACHE_SIZE)
					goto end;
				DWORD dwLineSize;
				if (m_dwNumCachedLines + 1 < dwNumLines)
				{
					const CLineInfo& rNextLineInfo = m_arrLines[(int)(m_dwNumCachedLines + 1)];
					dwLineSize = rNextLineInfo.m_dwLineStart - rLineInfo.m_dwLineStart; // line size includes line end
				}
				else
					dwLineSize = rLineInfo.m_dwSize;
				if (dwLineSize > dwBytesLeft)
				{
					MoveMemory(m_pLineBuffer, m_pLineBuffer + dwLineBufferPos, dwBytesLeft);
					dwLineBufferPos = dwBytesLeft;
					break;
				}
				m_pDecoder->DecodeString(m_pLineBuffer + dwLineBufferPos, rLineInfo.m_dwSize, m_pTextCache + dwTextCachePos, rLineInfo.m_dwLength);
				int nLineWidth = LOWORD(GetTabbedTextExtent(hdc, m_pTextCache + dwTextCachePos, rLineInfo.m_dwLength, 0, NULL));
				if (m_nMaxLineWidth < nLineWidth)
					m_nMaxLineWidth = nLineWidth;

				rLineInfo.m_dwTextStart = dwTextCachePos;
				dwTextCachePos += rLineInfo.m_dwLength;
				dwLineBufferPos += dwLineSize;
				++m_dwNumCachedLines;
			}
		}
end:
		m_nMaxLineWidth += nExtraWidth;
		ResizeTextView(TRUE);
		if (hOldFont)
			SelectFont(hdc, hOldFont);
		ReleaseDC(m_hwnd, hdc);
	}
}

/**
 * @param dwCachedLineNum - index of cached line.
 */
void CTextView::CacheLine(DWORD dwCachedLineNum)
{
	if (dwCachedLineNum >= m_dwFirstCachedLine && dwCachedLineNum < m_dwFirstCachedLine + m_dwNumCachedLines)
		return;

	HDC hdc = GetDC(m_hwnd);
	HFONT hOldFont = g_pResManager->m_hFixedFont ? SelectFont(hdc, g_pResManager->m_hFixedFont) : NULL;
	TEXTMETRIC tmetr;
	::GetTextMetrics(hdc, &tmetr);
	int nExtraWidth = tmetr.tmAveCharWidth + LOWORD(GetTabbedTextExtent(hdc, g_szEllipsis, g_dwEllipsisLength, 0, NULL));
	int nMaxLineWidth = 0;

	DWORD dwNumLines = m_arrLines.GetCount();
	_ASSERT(dwCachedLineNum < dwNumLines);
	// Put new line into the middle of cache
	const CLineInfo& rLineInfo = m_arrLines[(int)dwCachedLineNum];
	DWORD dwFirstCachedLine = dwCachedLineNum,
		  dwLastCachedLine = dwCachedLineNum,
		  dwTotalSize = rLineInfo.m_dwLength;
	BOOL bCanMoveFirstCachedLine = TRUE, bCanMoveLastCachedLine = TRUE;
	do
	{
		if (bCanMoveFirstCachedLine)
		{
			BOOL bLineNumberChanged = FALSE;
			if (dwFirstCachedLine > 0)
			{
				const CLineInfo& rLineInfo = m_arrLines[(int)(dwFirstCachedLine - 1)];
				if (dwTotalSize + rLineInfo.m_dwLength <= TEXT_CACHE_SIZE)
				{
					dwTotalSize += rLineInfo.m_dwLength;
					--dwFirstCachedLine;
					bLineNumberChanged = TRUE;
				}
			}
			bCanMoveFirstCachedLine = bLineNumberChanged;
		}
		if (bCanMoveLastCachedLine)
		{
			BOOL bLineNumberChanged = FALSE;
			if (dwLastCachedLine + 1 < dwNumLines)
			{
				const CLineInfo& rLineInfo = m_arrLines[(int)(dwLastCachedLine + 1)];
				if (dwTotalSize + rLineInfo.m_dwLength <= TEXT_CACHE_SIZE)
				{
					dwTotalSize += rLineInfo.m_dwLength;
					++dwLastCachedLine;
					bLineNumberChanged = TRUE;
				}
			}
			bCanMoveLastCachedLine = bLineNumberChanged;
		}
	}
	while (bCanMoveFirstCachedLine || bCanMoveLastCachedLine);

	DWORD dwNumCachedLines = dwLastCachedLine - dwFirstCachedLine + 1;
	DWORD dwFirstMappedLine, dwLastMappedLine, dwNumMappedLines, dwFirstLoadedLine, dwLastLoadedLine, dwNumLoadedLines, dwFromMemOffset, dwToMemOffset, dwLoadOffset, dwMemSize;
	if (m_dwFirstCachedLine <= dwFirstCachedLine && dwFirstCachedLine < m_dwFirstCachedLine + m_dwNumCachedLines)
	{
		dwFirstMappedLine = dwFirstCachedLine;
		dwNumMappedLines = m_dwNumCachedLines - (dwFirstCachedLine - m_dwFirstCachedLine);
		dwLastMappedLine = dwFirstMappedLine + dwNumMappedLines - 1;

		dwFirstLoadedLine = dwFirstMappedLine + dwNumMappedLines;
		dwNumLoadedLines = dwNumCachedLines - dwNumMappedLines;
		dwLastLoadedLine = dwFirstLoadedLine + dwNumLoadedLines - 1;

		const CLineInfo& rLineInfo1 = m_arrLines[(int)dwFirstMappedLine];
		const CLineInfo& rLineInfo2 = m_arrLines[(int)dwLastMappedLine];
		dwFromMemOffset = rLineInfo1.m_dwTextStart;
		dwToMemOffset = 0;
		dwLoadOffset = dwMemSize = rLineInfo2.m_dwTextStart + rLineInfo2.m_dwLength - rLineInfo1.m_dwTextStart;
	}
	else if (dwFirstCachedLine <= m_dwFirstCachedLine && m_dwFirstCachedLine < dwFirstCachedLine + dwNumCachedLines)
	{
		dwFirstMappedLine = m_dwFirstCachedLine;
		dwNumMappedLines = dwNumCachedLines - (m_dwFirstCachedLine - dwFirstCachedLine);
		dwLastMappedLine = dwFirstMappedLine + dwNumMappedLines - 1;

		dwFirstLoadedLine = dwFirstCachedLine;
		dwNumLoadedLines = dwNumCachedLines - dwNumMappedLines;
		dwLastLoadedLine = dwFirstLoadedLine + dwNumLoadedLines - 1;

		const CLineInfo& rLineInfo1 = m_arrLines[(int)dwFirstMappedLine];
		const CLineInfo& rLineInfo2 = m_arrLines[(int)dwLastMappedLine];
		dwFromMemOffset = rLineInfo1.m_dwTextStart;
		dwToMemOffset = 0;
		dwMemSize = rLineInfo2.m_dwTextStart + rLineInfo2.m_dwLength - rLineInfo1.m_dwTextStart;
		for (DWORD dwLineNum = dwFirstLoadedLine; dwLineNum <= dwLastLoadedLine; ++dwLineNum)
		{
			const CLineInfo& rLineInfo = m_arrLines[(int)dwLineNum];
			dwToMemOffset += rLineInfo.m_dwLength;
		}
		dwLoadOffset = 0;
	}
	else
	{
		dwFirstMappedLine = dwLastMappedLine = dwNumMappedLines = dwFromMemOffset = dwToMemOffset = dwMemSize = 0;

		dwFirstLoadedLine = dwFirstCachedLine;
		dwNumLoadedLines = dwNumCachedLines;
		dwLastLoadedLine = dwFirstLoadedLine + dwNumLoadedLines - 1;
		dwLoadOffset = 0;
	}

	_ASSERTE(m_pTextCache != NULL);
	if (dwNumMappedLines > 0)
	{
		MoveMemory(m_pTextCache + dwToMemOffset, m_pTextCache + dwFromMemOffset, dwMemSize * sizeof(TCHAR));
		for (DWORD dwLineNum = dwFirstMappedLine; dwLineNum <= dwLastMappedLine; ++dwLineNum)
		{
			CLineInfo& rLineInfo = m_arrLines[(int)dwLineNum];
			rLineInfo.m_dwTextStart = dwToMemOffset;
			dwToMemOffset += rLineInfo.m_dwLength;
		}
	}

	if (dwNumLoadedLines > 0)
	{
		DWORD dwLineBufferPos = 0, dwLineNum = dwFirstLoadedLine;
		const CLineInfo& rLineInfo = m_arrLines[(int)dwLineNum];
		SetFilePointer(m_hFile, rLineInfo.m_dwLineStart, NULL, FILE_BEGIN);
		while (dwLineNum <= dwLastLoadedLine)
		{
			DWORD dwNumRead = 0;
			if (! ReadFile(m_hFile, m_pLineBuffer + dwLineBufferPos, m_dwLineBufferSize - dwLineBufferPos, &dwNumRead, NULL))
				return;
			dwNumRead += dwLineBufferPos;
			dwLineBufferPos = 0;
			if (dwNumRead == 0)
				break;
			while (dwLineNum <= dwLastLoadedLine)
			{
				CLineInfo& rLineInfo = m_arrLines[(int)dwLineNum];
				DWORD dwLineSize;
				if (dwLineNum < dwLastLoadedLine)
				{
					const CLineInfo& rNextLineInfo = m_arrLines[(int)(dwLineNum + 1)];
					dwLineSize = rNextLineInfo.m_dwLineStart - rLineInfo.m_dwLineStart; // line size including line end
				}
				else
					dwLineSize = rLineInfo.m_dwSize;
				DWORD dwBytesLeft = dwNumRead - dwLineBufferPos;
				if (dwLineSize > dwBytesLeft)
				{
					MoveMemory(m_pLineBuffer, m_pLineBuffer + dwLineBufferPos, dwBytesLeft);
					dwLineBufferPos = dwBytesLeft;
					break;
				}
				m_pDecoder->DecodeString(m_pLineBuffer + dwLineBufferPos, rLineInfo.m_dwSize, m_pTextCache + dwLoadOffset, rLineInfo.m_dwLength);
				int nLineWidth = LOWORD(GetTabbedTextExtent(hdc, m_pTextCache + dwLoadOffset, rLineInfo.m_dwLength, 0, NULL));
				if (nMaxLineWidth < nLineWidth)
					nMaxLineWidth = nLineWidth;

				rLineInfo.m_dwTextStart = dwLoadOffset;
				dwLoadOffset += rLineInfo.m_dwLength;
				dwLineBufferPos += dwLineSize;
				++dwLineNum;
			}
		}
	}

	m_dwFirstCachedLine = dwFirstCachedLine;
	m_dwNumCachedLines = dwNumCachedLines;
	nMaxLineWidth += nExtraWidth;
	if (m_nMaxLineWidth < nMaxLineWidth)
	{
		m_nMaxLineWidth = nMaxLineWidth;
		SCROLLINFO sinfo;
		ZeroMemory(&sinfo, sizeof(sinfo));
		sinfo.cbSize = sizeof(sinfo);
		sinfo.fMask = SIF_RANGE;
		sinfo.nMin = 0;
		sinfo.nMax = nMaxLineWidth - 1;
		SetScrollInfo(m_hwnd, SB_HORZ, &sinfo, TRUE);
	}
	if (hOldFont)
		SelectFont(hdc, hOldFont);
	ReleaseDC(m_hwnd, hdc);
}
