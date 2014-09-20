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

#pragma once

/// Image view control class.
class CImageView
{
public:
	/// Initialize the object.
	CImageView(void);
	/// De-initialize the object.
	~CImageView(void);
	/// Attach image view object to window handle.
	void Attach(HWND hwnd);
	/// Detach image view object to window handle.
	void Detach(void);
	/// Set image information.
	void SetImage(HBITMAP hBitmap);
	/// Reset image information.
	void ResetImage(void);
	/// Get image handle.
	HBITMAP GetImageHandle(void) const;
	/// Fit image to actual window size.
	void FitImage(void);
	/// Reset image size.
	void ResetSize(void);
	/// Zoom in image.
	void ZoomIn(void);
	/// Zoom out image.
	void ZoomOut(void);

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
	CImageView(const CImageView& rImageView);
	/// Protect the class from being accidentally copied.
	CImageView& operator=(const CImageView& rImageView);
	/// Window procedure of image view window.
	static LRESULT CALLBACK ImageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draw window client area.
	void DrawImageView(HDC hdc, RECT* prcPaint);
	/// Resize image view client area.
	void ResizeImageView(BOOL bIgnoreScrollPos);
	/// Scroll image view client area.
	void ScrollImageView(int nScrollBarType, int nScrollCode);
	/// Resize image.
	void ResizeImage(void);
	/// Initialize variables.
	void InitVars(void);
	/// Get client rectangle with no scrollbars.
	void GetBiggestClientRect(RECT* prcClient);

	/// Initial image handle.
	HBITMAP m_hBitmap;
	/// Initial bitmap size.
	SIZE m_szBitmapSize;
	/// Image handle with adjusted size.
	HBITMAP m_hAdjustedBitmap;
	/// Adjusted bitmap size.
	SIZE m_szAjustedBitmapSize;
	/// Window handle.
	HWND m_hwnd;
	/// Old static window procedure.
	WNDPROC m_pfnOldImageViewWndProc;
	/// Number of first cache lined.
	DWORD m_dwFirstCachedLine;
	/// Old window style.
	LONG m_lOldStyle;
	/// Number of wheel lines.
	int m_nWheelLines;
};

inline CImageView::CImageView(void) : m_hAdjustedBitmap(NULL)
{
	InitVars();
}

inline CImageView::~CImageView(void)
{
	ResetImage();
}

/**
 * @return image handle.
 */
inline HBITMAP CImageView::GetImageHandle(void) const
{
	return m_hBitmap;
}
