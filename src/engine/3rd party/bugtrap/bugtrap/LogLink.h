/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Log file descriptor.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "BugTrap.h"
#include "BugTrapUtils.h"

/// Log file descriptor.
class CLogLink
{
public:
	/// Initialize the object.
	CLogLink(void);
	/// Initialize the object.
	CLogLink(PCTSTR pszLogFileName);
	/// Destroy the object.
	virtual ~CLogLink(void);
	/// Get log type.
	virtual BUGTRAP_LOGTYPE GetLogType(void) const;
	/// Save log link entries.
	virtual void SaveEntries(bool bCrash);
	/// Get custom log file name.
	PCTSTR GetLogFileName(void) const;
	/// Set custom log file name.
	void SetLogFileName(PCTSTR pszLogFileName);
	/// Object comparison.
	friend bool operator==(const CLogLink& rLogLink1, const CLogLink& rLogLink2);
	/// Object comparison.
	friend bool operator!=(const CLogLink& rLogLink1, const CLogLink& rLogLink2);

protected:
	/// Custom log file name.
	TCHAR m_szLogFileName[MAX_PATH];
};

inline CLogLink::CLogLink(void)
{
	*m_szLogFileName = _T('\0');
}

/**
 * @param pszLogFileName - pointer to custom log file name.
 */
inline CLogLink::CLogLink(PCTSTR pszLogFileName)
{
	GetCompleteLogFileName(m_szLogFileName, pszLogFileName, NULL);
}

inline CLogLink::~CLogLink(void)
{
}

/**
 * @return pointer to custom log file name.
 */
inline PCTSTR CLogLink::GetLogFileName(void) const
{
	return m_szLogFileName;
}

/**
 * @param pszLogFileName - pointer to custom log file name.
 */
inline void CLogLink::SetLogFileName(PCTSTR pszLogFileName)
{
	GetCompleteLogFileName(m_szLogFileName, pszLogFileName, NULL);
}

/**
 * @return type of the log.
 */
inline BUGTRAP_LOGTYPE CLogLink::GetLogType(void) const
{
	return BTLT_LOGFILE;
}

/**
 * @param bCrash - true if crash has occurred.
 */
inline void CLogLink::SaveEntries(bool /*bCrash*/)
{
}

/// Reg file descriptor.
class CRegLink : public CLogLink
{
public:
	/// Object constructor.
	CRegLink(void);
	/// Object constructor.
	CRegLink(PCTSTR pszLogFileName);
	/// Object constructor.
	CRegLink(PCTSTR pszLogFileName, PCTSTR pszRegKey);
	/// Get log type.
	virtual BUGTRAP_LOGTYPE GetLogType(void) const;
	/// Save log link entries.
	virtual void SaveEntries(bool bCrash);
	/// Get registry key path.
	PCTSTR GetRegKey(void) const;
	/// Set registry key path.
	void SetRegKey(PCTSTR pszRegKey);

protected:
	/// Registry key path.
	TCHAR m_szRegKey[MAX_PATH];
};

inline CRegLink::CRegLink(void)
{
	*m_szRegKey = _T('\0');
}

/**
 * @param pszLogFileName - pointer to custom log file name.
 */
inline CRegLink::CRegLink(PCTSTR pszLogFileName) : CLogLink(pszLogFileName)
{
	*m_szRegKey = _T('\0');
}

/**
 * @param pszLogFileName - pointer to custom log file name.
 * @param pszRegKey - registry key path
 */
inline CRegLink::CRegLink(PCTSTR pszLogFileName, PCTSTR pszRegKey) : CLogLink(pszLogFileName)
{
	_tcscpy_s(m_szRegKey, countof(m_szRegKey), pszRegKey);
}

/**
 * @return type of the log.
 */
inline BUGTRAP_LOGTYPE CRegLink::GetLogType(void) const
{
	return BTLT_REGEXPORT;
}

/**
 * @param bCrash - true if crash has occurred.
 */
inline void CRegLink::SaveEntries(bool bCrash)
{
	if (bCrash)
		BT_ExportRegistryKey(m_szLogFileName, m_szRegKey);
}

/**
 * @param pszRegKey - registry key path
 */
inline void CRegLink::SetRegKey(PCTSTR pszRegKey)
{
	_tcscpy_s(m_szRegKey, countof(m_szRegKey), pszRegKey);
}

/**
 * @return registry key path.
 */
inline PCTSTR CRegLink::GetRegKey(void) const
{
	return m_szRegKey;
}

/**
 * @param rLogLink1 - 1st object.
 * @param rLogLink2 - 2nd object.
 * @return comparison result.
 */
inline bool operator==(const CLogLink& rLogLink1, const CLogLink& rLogLink2)
{
	return (_tcsicmp(rLogLink1.m_szLogFileName, rLogLink2.m_szLogFileName) == 0);
}

/**
 * @param rLogLink1 - 1st object.
 * @param rLogLink2 - 2nd object.
 * @return comparison result.
 */
inline bool operator!=(const CLogLink& rLogLink1, const CLogLink& rLogLink2)
{
	return (_tcsicmp(rLogLink1.m_szLogFileName, rLogLink2.m_szLogFileName) != 0);
}
