/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Custom resources manager.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

/**
 * @addtogroup ResManager Custom resource manager.
 * @{
 */

/// Custom resources manager.
class CResManager
{
public:
	/// Initialize custom resources.
	explicit CResManager(HWND hwndParent);
	/// Free custom resources.
	~CResManager(void);

	/// Fixed font handle.
	HFONT m_hFixedFont;
	/// Underlined font handle.
	HFONT m_hUnderlinedFont;
	/// Dialog font handle.
	HFONT m_hDialogFont;
	/// Hand cursor.
	HCURSOR m_hHandCursor;
	/// Arrow cursor.
	HCURSOR m_hArrowCursor;
	/// Arrow + hour-glass cursor.
	HCURSOR m_hAppStartingCursor;
	/// Double-pointed arrow pointing north and south.
	HCURSOR m_hUpDownCursor;
	/// Double-pointed arrow pointing west and east.
	HCURSOR m_hLeftRightCursor;
	/// Hour-glass cursor.
	HCURSOR m_hWaitCursor;
	/// I-beam cursor.
	HCURSOR m_hIBeamCursor;
	/// Window brush handle.
	HBRUSH m_hbrWindowBrush;
	/// Control light brush handle.
	HBRUSH m_hbrControlLight;
	/// Button-face brush handle.
	HBRUSH m_hbrButtonFaceBrush;
	/// Background brush handle.
	HBRUSH m_hbrAppWorkspace;
	/// Big application icon.
	HICON m_hBigAppIcon;
	/// Small application icon.
	HICON m_hSmallAppIcon;
	/// Transparent dialog icon.
	HBITMAP m_hDialogIcon;
	/// Check-mark bitmap.
	HBITMAP m_hCheckMark;
	/// Sort arrows bitmap.
	HIMAGELIST m_hSortArrows;

private:
	/// Protects the class from being accidentally copied.
	CResManager(const CResManager& rResManager);
	/// Protects the class from being accidentally copied.
	CResManager& operator=(const CResManager& rResManager);
};

/** @} */
