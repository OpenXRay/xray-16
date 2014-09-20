/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Wait cursor.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "ResManager.h"
#include "Globals.h"

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Wait cursor class.
class CWaitCursor
{
public:
	/// Initialize wait cursor.
	CWaitCursor(void);
	/// Initialize and immediately show wait cursor.
	explicit CWaitCursor(bool);
	/// Destroy the object.
	~CWaitCursor(void);
	/// Shows wait cursor.
	void BeginWait(void);
	/// Closes wait cursor.
	void EndWait(void);

private:
	/// Protect the class from being accidentally copied.
	CWaitCursor(const CWaitCursor& rWaitCursor);
	/// Protect the class from being accidentally copied.
	CWaitCursor& operator=(const CWaitCursor& rWaitCursor);

	/// Handle of previous window cursor.
	HCURSOR m_hOldCursor;
};

inline CWaitCursor::CWaitCursor(void)
{
	m_hOldCursor = NULL;
}

/// Initialize and immediately show wait cursor.
inline CWaitCursor::CWaitCursor(bool)
{
	m_hOldCursor = SetCursor(g_pResManager->m_hWaitCursor);
}

inline CWaitCursor::~CWaitCursor(void)
{
	EndWait();
}

inline void CWaitCursor::BeginWait(void)
{
	_ASSERTE(m_hOldCursor == NULL);
	m_hOldCursor = SetCursor(g_pResManager->m_hWaitCursor);
}

/** @} */
