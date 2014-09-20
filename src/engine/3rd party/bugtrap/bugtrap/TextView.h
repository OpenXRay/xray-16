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

#pragma once

#include "Array.h"
#include "Encoding.h"

/// Text view control class.
class CTextView
{
public:
	/// Initialize the object.
	CTextView(void);
	/// De-initialize the object.
	~CTextView(void);
	/// Attach text view object to window handle.
	void Attach(HWND hwnd);
	/// Detach text view object to window handle.
	void Detach(void);
	/// Set file information.
	void SetFile(HANDLE hFile, TEXT_ENCODING eEncoding, DWORD dwSignatureSize);
	/// Reset file information.
	void ResetFile(void);
	/// Get file handle.
	HANDLE GetFileHandle(void) const;
	/// Get file encoding.
	TEXT_ENCODING GetEncoding(void) const;

	enum
	{
		/// Maximum number of symbols in the line.
		MAX_NUMBER_OF_SYMBOLS = 300,
		/// Approximated average number of symbols in the line.
		APROX_NUMBER_OF_SYMBOLS = 30,
		/// Size of text buffer in characters.
		TEXT_CACHE_SIZE = 40000,
		/// Size of line buffer in bytes.
		LINE_BUFFER_SIZE = 40000
	};

private:
	/// Protect the class from being accidentally copied.
	CTextView(const CTextView& rTextView);
	/// Protect the class from being accidentally copied.
	CTextView& operator=(const CTextView& rTextView);
	/// Window procedure of text view window.
	static LRESULT CALLBACK TextViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draw window client area.
	void DrawTextView(HDC hdc, RECT* prcPaint);
	/// Resize text view client area.
	void ResizeTextView(BOOL bIgnoreScrollPos);
	/// Scroll text view client area.
	void ScrollTextView(int nScrollBarType, int nScrollCode);
	/// Initialize variables.
	void InitVars(void);
	/// Load line numbers info.
	void CountLines(void);
	/// Load cache from file.
	void LoadCache(void);
	/// Load line to the cache.
	void CacheLine(DWORD dw—achedLineNum);
	/// Get current font metrics.
	void GetTextMetrics(PTEXTMETRIC pTextMetric);

	/// Line information.
	struct CLineInfo
	{
		/// Line offset in file.
		DWORD m_dwLineStart;
		/// Text offset in cache.
		DWORD m_dwTextStart;
		/// Line length in characters.
		DWORD m_dwLength;
		/// Line length in bytes.
		DWORD m_dwSize;
		/// True if line was truncated.
		BOOL m_bTruncated;
	};

	/// Line numbers info.
	CArray<CLineInfo> m_arrLines;
	/// File handle.
	HANDLE m_hFile;
	/// Text decoder.
	CBaseDecoder* m_pDecoder;
	/// Window handle.
	HWND m_hwnd;
	/// Old static window procedure.
	WNDPROC m_pfnOldTextViewWndProc;
	/// Number of first cache lined.
	DWORD m_dwFirstCachedLine;
	/// Number of lines stored in cache.
	DWORD m_dwNumCachedLines;
	/// Pointer to the cache.
	PTCHAR m_pTextCache;
	/// Size of line buffer in bytes.
	DWORD m_dwLineBufferSize;
	/// Pointer to the line buffer.
	PBYTE m_pLineBuffer;
	/// Old window style.
	LONG m_lOldStyle;
	/// Maximum line width.
	int m_nMaxLineWidth;
	/// Number of wheel lines.
	int m_nWheelLines;
};

inline CTextView::CTextView(void) : m_arrLines(0), m_pTextCache(NULL), m_pLineBuffer(NULL)
{
	InitVars();
}

inline CTextView::~CTextView(void)
{
	ResetFile();
}

/**
 * @return file handle.
 */
inline HANDLE CTextView::GetFileHandle(void) const
{
	return m_hFile;
}

/**
 * @return text encoding.
 */
inline TEXT_ENCODING CTextView::GetEncoding(void) const
{
	return (m_pDecoder ? m_pDecoder->GetEncoding() : TXTENC_ANSI);
}
