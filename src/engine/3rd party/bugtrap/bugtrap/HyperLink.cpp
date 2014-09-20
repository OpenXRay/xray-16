/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Hyper-link control class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "HyperLink.h"
#include "BugTrapUtils.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const COLORREF CHyperLink::m_rgbBlueColor = RGB(0x00, 0x00, 0xDD);
const COLORREF CHyperLink::m_rgbRedColor = RGB(0xDD, 0x00, 0x00);

CHyperLink::CHyperLink(void)
{
	m_rgbCurrentColor = m_rgbBlueColor;
	*m_szLinkURL = _T('\0');
	m_hwnd = NULL;
	m_pfnOldHyperLinkWndProc = NULL;
	m_nPrevDefButtonID = -1;
}

/**
 * @param pszLinkURL - new link URL.
 */
void CHyperLink::SetLinkURL(PCTSTR pszLinkURL)
{
	if (pszLinkURL)
		_tcscpy_s(m_szLinkURL, countof(m_szLinkURL), pszLinkURL);
	else
		*m_szLinkURL = _T('\0');
}

/**
 * @param point - point coordinates.
 * @return true if point appears in boundaries of hyper-link text.
 */
BOOL CHyperLink::HitTest(const POINT& point) const
{
	RECT rect;
	GetHyperLinkRect(rect);
	return PtInRect(&rect, point);
}

/**
 * @param x - point x-coordinate.
 * @param y - point y-coordinate.
 * @return true if point appears in boundaries of hyper-link text.
 */
BOOL CHyperLink::HitTest(int x, int y) const
{
	POINT point;
	point.x = x;
	point.y = y;
	return HitTest(point);
}

/**
 * @param hdc - drawing context.
 */
void CHyperLink::DrawHyperLink(HDC hdc) const
{
	TCHAR szLinkText[MAX_PATH];
	GetWindowText(m_hwnd, szLinkText, countof(szLinkText));
	SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
	_ASSERTE(g_pResManager != NULL);
	HFONT hOldFont = g_pResManager->m_hUnderlinedFont ? SelectFont(hdc, g_pResManager->m_hUnderlinedFont) : NULL;
	HWND hwndParent = GetParent(m_hwnd);
	HBRUSH hBrush = hwndParent ? FORWARD_WM_CTLCOLORSTATIC(hwndParent, hdc, m_hwnd, SendMessage) : NULL;
	if (! hBrush)
		hBrush = g_pResManager->m_hbrButtonFaceBrush;
	if (hBrush)
	{
		RECT rcClient;
		GetClientRect(m_hwnd, &rcClient);
		FillRect(hdc, &rcClient, hBrush);
	}
	SetTextColor(hdc, m_rgbCurrentColor);
	TextOut(hdc, 1, 1, szLinkText, _tcslen(szLinkText));
	SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
	if (hOldFont)
		SelectFont(hdc, hOldFont);
	if (GetFocus() == m_hwnd)
	{
		RECT rect;
		GetHyperLinkRect(rect);
		DrawFocusRect(hdc, &rect);
	}
}

void CHyperLink::DoAction(void) const
{
	if (*m_szLinkURL)
	{
		_ASSERTE(g_pResManager != NULL);
		if (g_pResManager->m_hAppStartingCursor)
			SetCursor(g_pResManager->m_hAppStartingCursor);
		ShellExecute(NULL, _T("open"), m_szLinkURL, NULL, NULL, SW_SHOWDEFAULT);
	}
	else
	{
		HWND hwndParent = GetParent(m_hwnd);
		if (hwndParent)
		{
			int nCtrlID = GetDlgCtrlID(m_hwnd);
			FORWARD_WM_COMMAND(hwndParent, nCtrlID, m_hwnd, BN_CLICKED, PostMessage);
		}
	}
}

/**
 * @param hwnd - window handle.
 * @param uMsg - message identifier.
 * @param wParam - first message parameter.
 * @param lParam - second message parameter.
 */
LRESULT CALLBACK CHyperLink::HyperLinkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int x, y, nCtrlID;
	POINT point;
	HCURSOR hCursor;
	COLORREF rgbNewColor;
	PAINTSTRUCT ps;
	HWND hwndParent, hwndCtrl;
	LRESULT lResult;
	DWORD dwResult;
	HDC hdc;

	CHyperLink* _this  = (CHyperLink*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	_ASSERTE(_this != NULL);
	switch(uMsg)
	{
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_SETCURSOR:
		_ASSERTE(g_pResManager != NULL);
		hCursor = g_pResManager->m_hArrowCursor;
		if (g_pResManager->m_hHandCursor)
		{
			dwResult = GetMessagePos();
			point.x = GET_X_LPARAM(dwResult);
			point.y = GET_Y_LPARAM(dwResult);
			ScreenToClient(hwnd, &point);
			if (_this->HitTest(point))
				hCursor = g_pResManager->m_hHandCursor;
		}
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
			{
				_this->m_rgbCurrentColor = m_rgbRedColor;
				SetCapture(hwnd);
				if (GetFocus() == hwnd)
					InvalidateRect(hwnd, NULL, FALSE);
				else
					SetFocus(hwnd);
			}
		}
		return 0;
	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd)
		{
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			rgbNewColor = _this->HitTest(x, y) ? m_rgbRedColor : m_rgbBlueColor;
			if (_this->m_rgbCurrentColor != rgbNewColor)
			{
				_this->m_rgbCurrentColor = rgbNewColor;
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		return 0;
	case WM_LBUTTONUP:
		if (GetCapture() == hwnd)
		{
			ReleaseCapture();
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			if (_this->HitTest(x, y))
				_this->DoAction();
		}
		return 0;
	case WM_SETFOCUS:
		hwndParent = GetParent(hwnd);
		if (hwndParent)
		{
			nCtrlID = GetDlgCtrlID(hwnd);
			if (_this->m_nPrevDefButtonID < 0)
			{
				lResult = SendMessage(hwndParent, DM_GETDEFID, 0, 0);
				_this->m_nPrevDefButtonID = HIWORD(lResult) == DC_HASDEFID && LOWORD(lResult) != nCtrlID ? LOWORD(lResult) : -1;
			}
			SendMessage(hwndParent, DM_SETDEFID, nCtrlID, 0);
			if (_this->m_nPrevDefButtonID > 0)
			{
				// Sending a DM_SETDEFID message to change the default button will not always
				// remove the default state border from the first push button. In these cases,
				// the application should send a BM_SETSTYLE message to change the first push
				// button border style.
				hwndCtrl = GetDlgItem(hwndParent, _this->m_nPrevDefButtonID);
				if (SendMessage(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
					SendMessage(hwndCtrl, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
			}
		}
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	case WM_KILLFOCUS:
		hwndParent = GetParent(hwnd);
		if (hwndParent && wParam && IsChild(hwndParent, (HWND)wParam))
		{
			if (! (SendMessage((HWND)wParam, WM_GETDLGCODE, 0, 0) & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)))
			{
				if (_this->m_nPrevDefButtonID > 0)
				{
					nCtrlID = _this->m_nPrevDefButtonID;
					_this->m_nPrevDefButtonID = -1;
				}
				else
					nCtrlID = IDOK;
				hwndCtrl = GetDlgItem(hwndParent, nCtrlID);
				if (hwndCtrl && IsWindowEnabled(hwndCtrl))
					SendMessage(hwndParent, DM_SETDEFID, nCtrlID, 0);
			}
			else
			{
				_this->m_nPrevDefButtonID = -1;
				nCtrlID = GetDlgCtrlID((HWND)wParam);
				SendMessage(hwndParent, DM_SETDEFID, nCtrlID, 0);
			}
		}
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	case WM_CAPTURECHANGED:
		_this->m_rgbCurrentColor = m_rgbBlueColor;
		InvalidateRect(hwnd, NULL, FALSE);
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
				_this->DrawHyperLink(hdc);
				EndPaint(hwnd, &ps);
			}
		}
		else
			_this->DrawHyperLink(hdc);
		return 0;
	case WM_PRINTCLIENT:
		hdc = (HDC)wParam;
		_this->DrawHyperLink(hdc);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
			_this->DoAction();
			break;
		case VK_TAB:
			hwndParent = GetParent(hwnd);
			if (hwndParent)
			{
				BOOL bPrevCtrl = GetKeyState(VK_SHIFT) < 0;
				FORWARD_WM_NEXTDLGCTL(hwndParent, bPrevCtrl, FALSE, PostMessage);
			}
			break;
		case VK_LEFT:
		case VK_UP:
			hwndParent = GetParent(hwnd);
			if (hwndParent)
				FORWARD_WM_NEXTDLGCTL(hwndParent, TRUE, FALSE, PostMessage);
			break;
		case VK_RIGHT:
		case VK_DOWN:
			hwndParent = GetParent(hwnd);
			if (hwndParent)
				FORWARD_WM_NEXTDLGCTL(hwndParent, FALSE, FALSE, PostMessage);
			break;
		case VK_ESCAPE:
			hwndParent = GetParent(hwnd);
			if (hwndParent)
			{
				HWND hwndCtl = GetDlgItem(hwndParent, IDCANCEL);
				if (! hwndCtl || IsWindowEnabled(hwndCtl))
					FORWARD_WM_COMMAND(hwndParent, IDCANCEL, hwndCtl, BN_CLICKED, PostMessage);
			}
			break;
		}
		return 0;
	case WM_GETDLGCODE:
		return ((GetFocus() == hwnd ? DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON) | DLGC_WANTALLKEYS);
	case BM_SETSTYLE:
		return 0;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

/**
 * @param hwnd - hyper-link window handle.
 */
void CHyperLink::Attach(HWND hwnd)
{
	_ASSERTE(hwnd != NULL);
	_ASSERTE(m_hwnd == NULL);
	_ASSERTE(g_pResManager != NULL);
	m_hwnd = hwnd;
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	m_pfnOldHyperLinkWndProc = SubclassWindow(hwnd, HyperLinkWndProc);
	InvalidateRect(hwnd, NULL, TRUE);
}

void CHyperLink::Detach(void)
{
	if (m_pfnOldHyperLinkWndProc)
	{
		SubclassWindow(m_hwnd, m_pfnOldHyperLinkWndProc);
		SetWindowLongPtr(m_hwnd, GWL_USERDATA, NULL);
		InvalidateRect(m_hwnd, NULL, TRUE);
		m_pfnOldHyperLinkWndProc = NULL;
		m_hwnd = NULL;
	}
}

/**
 * @param size - hyper-link size.
 */
void CHyperLink::GetHyperLinkSize(SIZE& size) const
{
	HDC hdc = GetDC(m_hwnd);
	_ASSERTE(g_pResManager != NULL);
	HFONT hOldFont = NULL;
	if (g_pResManager->m_hUnderlinedFont)
		hOldFont = SelectFont(hdc, g_pResManager->m_hUnderlinedFont);
	TCHAR szLinkText[MAX_PATH];
	GetWindowText(m_hwnd, szLinkText, countof(szLinkText));
	GetTextExtentPoint32(hdc, szLinkText, _tcslen(szLinkText), &size);
	if (hOldFont)
		SelectFont(hdc, hOldFont);
	ReleaseDC(m_hwnd, hdc);
}

/**
 * @param rect - hyper-link rectangle.
 */
void CHyperLink::GetHyperLinkRect(RECT& rect) const
{
	SIZE size;
	GetHyperLinkSize(size);
	rect.left = rect.top = 0;
	rect.right = size.cx + 3;
	rect.bottom = size.cy + 3;
}
