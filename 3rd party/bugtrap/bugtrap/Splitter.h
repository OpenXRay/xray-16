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

#pragma once

/// Splitter control class.
class CSplitter
{
public:
	/// Splitter direction.
	enum SPLITTER_DIRECTION
	{
		/// Vertical splitter.
		SD_VERTICAL   = 0,
		/// Horizontal splitter.
		SD_HORIZONTAL = 1,
	};

	enum
	{
		/// Number of panels.
		NUM_PANELS    = 2
	};

	/// Initialize the object.
	CSplitter(SPLITTER_DIRECTION eDirection = SD_VERTICAL, bool bProportionalMode = false);
	/// Attach splitter object to window handle.
	void Attach(HWND hwnd);
	/// Detach splitter object to window handle.
	void Detach(void);
	/// Get splitter direction.
	SPLITTER_DIRECTION GetDirection(void) const;
	/// Set splitter direction.
	void SetDirection(SPLITTER_DIRECTION eDirection);
	/// Get panel handle.
	HWND GetPanel(int iPanel) const;
	/// Set panel handle.
	BOOL SetPanel(int iPanel, HWND hwndPanel);
	/// Update panels layout.
	void UpdateLayout(void);
	/// Reset splitter position.
	void ResetSplitterPos(void);
	/// Set splitter position.
	void SetSplitterPos(int nSplitterPos);
	/// Get current splitter position.
	int GetSplitterPos(void) const;
	/// Set proportional mode.
	void SetProportionalMode(bool bProportionalMode);
	/// Get proportional mode.
	bool GetProportionalMode(void) const;

private:
	/// Protect the class from being accidentally copied.
	CSplitter(const CSplitter& rSplitter);
	/// Protect the class from being accidentally copied.
	CSplitter& operator=(const CSplitter& rSplitter);
	/// Window procedure of splitter window.
	static LRESULT CALLBACK SplitterWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draw splitter area.
	void DrawSplitter(HDC hdc) const;
	/// Draw splitter bar.
	void DrawSplitterBar(HDC hdc) const;
	/// Draw panel area.
	BOOL DrawPanel(HDC hdc, int iPanel) const;
	/// Check if point appears within splitter bar.
	BOOL HitTest(const POINT& point) const;
	/// Check if point appears within splitter bar.
	BOOL HitTest(int x, int y) const;
	/// Calculates splitter bar rectangle.
	void GetSplitterBarRect(RECT& rect) const;
	/// Calculate panel rectangle based on panel index.
	BOOL CalcPanelRect(int iPanel, RECT& rect, bool bShrinkRect) const;
	/// Resize splitter window and update panels layout.
	void ResizeSplitter(int nSplitterSize);
	/// Clear splitter size and position.
	void ClearSplitterPos(void);
	/// Clear panel windows.
	void ClearPanels(void);
	/// Calculate splitter size.
	int CalcSplitterSize(void) const;

	/// Window handle.
	HWND m_hwnd;
	/// Old window procedure.
	WNDPROC m_pfnOldSplitterWndProc;
	/// Splitter direction.
	SPLITTER_DIRECTION m_eDirection;
	/// Splitter position.
	int m_nSplitterPos;
	/// Ideal slider position.
	int m_nIdealPos;
	/// Keeps ideal window size.
	int m_nSplitterSize;
	/// Proportional mode flag.
	bool m_bProportionalMode;
	/// Panel handles.
	HWND m_arrPanels[NUM_PANELS];
};

/**
 * @return current splitter direction.
 */
inline CSplitter::SPLITTER_DIRECTION CSplitter::GetDirection(void) const
{
	return m_eDirection;
}

/**
 * @param iPanel - panel number.
 * @return panel window handle.
 */
inline HWND CSplitter::GetPanel(int iPanel) const
{
	return (iPanel >= 0 && iPanel < NUM_PANELS ? m_arrPanels[iPanel] : NULL);
}

/**
 * @return current splitter position.
 */
inline int CSplitter::GetSplitterPos(void) const
{
	return m_nSplitterPos;
}

/**
 * @param bProportionalMode - new proportional mode.
 */
inline void CSplitter::SetProportionalMode(bool bProportionalMode)
{
	m_bProportionalMode = bProportionalMode;
}

/**
 * @return active proportional mode.
 */
inline bool CSplitter::GetProportionalMode(void) const
{
	return m_bProportionalMode;
}
