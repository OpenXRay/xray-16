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

#pragma once

/// Hex view control class.
class CHexView
{
public:
	/// Initialize the object.
	CHexView(void);
	/// De-initialize the object.
	~CHexView(void);
	/// Attach hex view object to window handle.
	void Attach(HWND hwnd);
	/// Detach hex view object to window handle.
	void Detach(void);
	/// Set file information.
	void SetFile(HANDLE hFile);
	/// Reset file information.
	void ResetFile(void);
	/// Get file handle.
	HANDLE GetFileHandle(void) const;

	enum
	{
		/// Number of symbols in one line.
		LINE_WIDTH = 16,
		/// Number of cached lines.
		NUMBER_OF_CACHED_LINES = 2000,
		/// Size of cache.
		CACHE_SIZE = LINE_WIDTH * NUMBER_OF_CACHED_LINES
	};

private:
	/// Protect the class from being accidentally copied.
	CHexView(const CHexView& rHexView);
	/// Protect the class from being accidentally copied.
	CHexView& operator=(const CHexView& rHexView);
	/// Window procedure of hex view window.
	static LRESULT CALLBACK HexViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draw window client area.
	void DrawHexView(HDC hdc, RECT* prcPaint);
	/// Resize hex view client area.
	void ResizeHexView(BOOL bIgnoreScrollPos);
	/// Scroll hex view client area.
	void ScrollHexView(int nScrollBarType, int nScrollCode);
	/// Initialize variables.
	void InitVars(void);
	/// Load cache from file.
	void LoadCache(void);
	/// Load line to the cache.
	void CacheLine(DWORD dw—achedLineNum);
	/// Get current font metrics.
	void GetTextMetrics(PTEXTMETRIC pTextMetric);

	/// File cache.
	PBYTE m_pbCache;
	/// File size.
	DWORD m_dwFileSize;
	/// File handle.
	HANDLE m_hFile;
	/// Window handle.
	HWND m_hwnd;
	/// Old static window procedure.
	WNDPROC m_pfnOldHexViewWndProc;
	/// Number of first cache lined.
	DWORD m_dwFirstCachedLine;
	/// Old window style.
	LONG m_lOldStyle;
	/// Number of wheel lines.
	int m_nWheelLines;
};

inline CHexView::CHexView(void) : m_pbCache(NULL)
{
	InitVars();
}

inline CHexView::~CHexView(void)
{
	ResetFile();
}

/**
 * @return file handle.
 */
inline HANDLE CHexView::GetFileHandle(void) const
{
	return m_hFile;
}
