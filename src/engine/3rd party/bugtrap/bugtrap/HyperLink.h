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

#pragma once

/// Hyper-link control class.
class CHyperLink
{
public:
	/// Initialize the object.
	CHyperLink(void);
	/// Get link URL.
	PCTSTR GetLinkURL(void) const;
	/// Set link URL.
	void SetLinkURL(PCTSTR pszLinkURL);
	/// Attach hyper-link object to window handle.
	void Attach(HWND hwnd);
	/// Detach hyper-link object to window handle.
	void Detach(void);

private:
	/// Protect the class from being accidentally copied.
	CHyperLink(const CHyperLink& rHyperLink);
	/// Protect the class from being accidentally copied.
	CHyperLink& operator=(const CHyperLink& rHyperLink);
	/// Window procedure of hyper-link window.
	static LRESULT CALLBACK HyperLinkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Calculates hyper-link size.
	void GetHyperLinkSize(SIZE& size) const;
	/// Calculates hyper-link rectangle.
	void GetHyperLinkRect(RECT& rect) const;
	/// Check if point appears within hyper-link text.
	BOOL HitTest(const POINT& point) const;
	/// Check if point appears within hyper-link text.
	BOOL HitTest(int x, int y) const;
	/// Draw window client area.
	void DrawHyperLink(HDC hdc) const;
	/// Do hyper-link action.
	void DoAction(void) const;

	/// Blue color.
	static const COLORREF m_rgbBlueColor;
	/// Red color.
	static const COLORREF m_rgbRedColor;

	/// Window handle.
	HWND m_hwnd;
	/// Old static window procedure.
	WNDPROC m_pfnOldHyperLinkWndProc;
	/// Link URL.
	TCHAR m_szLinkURL[MAX_PATH];
	/// Current hyper-link color.
	COLORREF m_rgbCurrentColor;
	/// Previous default button ID.
	int m_nPrevDefButtonID;
};

/**
 * @return current link URL.
 */
inline PCTSTR CHyperLink::GetLinkURL(void) const
{
	return m_szLinkURL;
}
