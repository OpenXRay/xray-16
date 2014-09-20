/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Layout manager class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

/// Left-side alignment.
const int ALIGN_LEFT    = 0;
/// Top-side alignment.
const int ALIGN_TOP     = 0;
/// Center-side alignment.
const int ALIGN_CENTER  = 1;
/// Right-side alignment.
const int ALIGN_RIGHT   = 2;
/// Bottom-side alignment.
const int ALIGN_BOTTOM  = 2;
/// Alignment range.
const int ALIGN_RANGE   = 2;

/// Layout info block.
struct LAYOUT_INFO
{
	/// Initialize the object.
	LAYOUT_INFO(int nCtlID, int nRatioX1, int nRatioY1, int nRatioX2, int nRatioY2);

	/// Initialize the object.
	LAYOUT_INFO(void);

	/// Control ID.
	int m_nCtlID;
	/// Left coordinate ratio.
	int m_nRatioX1;
	/// Top coordinate ratio.
	int m_nRatioY1;
	/// Right coordinate ratio.
	int m_nRatioX2;
	/// Bottom coordinate ratio.
	int m_nRatioY2;
	/// Original control coordinates.
	RECT m_rcOriginal;
};

/// Window layout manager class.
class CLayoutManager
{
private:
	/// Minimal acceptable window size.
	POINT m_ptMinWindowSize;
	/// Minimal window client size.
	POINT m_ptMinClientSize;
	/// Parent window handle.
	HWND m_hwndParent;
	/// Size-box window handle.
	HWND m_hwndSizeBox;
	/// Array of control layout information.
	LAYOUT_INFO* m_arrLayout;
	/// Number of items in control layouts array.
	int m_nItemCount;

	/// Protect the class from being accidentally copied.
	CLayoutManager(const CLayoutManager& rLayoutManager);
	/// Protect the class from being accidentally copied.
	CLayoutManager& operator=(const CLayoutManager& rLayoutManager);

public:
	/// Initialize the object.
	CLayoutManager(void);
	/// Get minimal window size.
	const POINT& GetMinTrackSize(void) const;
	/// Initialize layout info blocks for child windows.
	void InitLayout(HWND hwndParent, LAYOUT_INFO arrLayout[], int nItemCount, bool bAddSizeBox = true);
	/// Apply layout information to the window.
	void ApplyLayout(void);
};

inline CLayoutManager::CLayoutManager(void)
{
	ZeroMemory(this, sizeof(*this));
}

/**
 * @return minimum window size.
 */
inline const POINT& CLayoutManager::GetMinTrackSize(void) const
{
	return m_ptMinWindowSize;
}
