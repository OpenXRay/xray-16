/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Wait dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Wait dialog class.
class CWaitDialog
{
public:
	/// Initialize wait dialog.
	CWaitDialog(void);
	/// Initialize wait dialog and wait message.
	explicit CWaitDialog(HWND hwndParent);
	/// Destroy the object.
	~CWaitDialog(void);
	/// Shows wait dialog.
	void BeginWait(HWND hwndParent);
	/// Closes wait dialog.
	void EndWait(void);

private:
	/// Protect the class from being accidentally copied.
	CWaitDialog(const CWaitDialog& rWaitDialog);
	/// Protect the class from being accidentally copied.
	CWaitDialog& operator=(const CWaitDialog& rWaitDialog);
	/// Initialize member variables.
	void InitVars(void);

	/// Handle of previous cursor.
	HCURSOR m_hOldCursor;
	/// Handle of wait message window.
	HWND m_hwndWait;
};

inline CWaitDialog::CWaitDialog(void)
{
	InitVars();
}

inline CWaitDialog::~CWaitDialog(void)
{
	EndWait();
}

inline void CWaitDialog::InitVars(void)
{
	m_hOldCursor = NULL;
	m_hwndWait = NULL;
}

/** @} */
