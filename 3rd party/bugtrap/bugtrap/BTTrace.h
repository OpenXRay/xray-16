/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: C++ wrapper for tracing functions.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#ifndef _BTTRACE_H_
#define _BTTRACE_H_

#pragma once

#ifndef _BUGTRAP_H_
 #error Include BugTrap.h first
#endif // _BUGTRAP_H_

#ifndef __cplusplus
 #error C++ compiler is required
#endif // __cplusplus

/// C++ wrapper for tracing API.
class BTTrace {
public:
	/// Initialize the object.
	BTTrace(void) {
		Detach();
	}

	/// Initialize the object.
	BTTrace(INT_PTR iHandle) {
		m_eDefaultLogLevel = BTLL_INFO;
		m_iHandle = iHandle;
	}

	/// Initialize the object.
	BTTrace(LPCTSTR pszLogFileName, BUGTRAP_LOGFORMAT eLogFormat) {
		m_eDefaultLogLevel = BTLL_INFO;
		m_iHandle = BT_OpenLogFile(pszLogFileName, eLogFormat);
	}

	/// Destroy the object.
	~BTTrace(void) {
		Close();
	}

	/// Open log file.
	INT_PTR Open(LPCTSTR pszLogFileName, BUGTRAP_LOGFORMAT eLogFormat) {
		Close();
		return (m_iHandle = BT_OpenLogFile(pszLogFileName, eLogFormat));
	}

	/// Attach log file handle.
	void Attach(INT_PTR iHandle) {
		m_iHandle = iHandle;
	}

	/// Detach log file handle.
	void Detach(void) {
		m_eDefaultLogLevel = BTLL_INFO;
		m_iHandle = NULL;
	}

	/// Close log file.
	void Close(void) {
		if (m_iHandle != NULL) {
			BT_CloseLogFile(m_iHandle);
			Detach();
		}
	}

	/// Get default log level.
	BUGTRAP_LOGLEVEL GetDefaultLogLevel(void) const {
		return m_eDefaultLogLevel;
	}

	/// Set default log level.
	void SetDefaultLogLevel(BUGTRAP_LOGLEVEL eDefaultLogLevel) {
		m_eDefaultLogLevel = eDefaultLogLevel;
	}

	/// Flush contents of the log file.
	void Flush(void) const {
		BT_FlushLogFile(m_iHandle);
	}

	/// Get custom log file handle.
	INT_PTR GetHandle(void) const {
		return m_iHandle;
	}

	/// Get custom log file name.
	LPCTSTR GetFileName(void) const {
		return BT_GetLogFileName(m_iHandle);
	}

	/// Get custom log file size in records.
	DWORD GetLogSizeInEntries(void) const {
		return BT_GetLogSizeInEntries(m_iHandle);
	}

	/// Set custom log file size in records.
	void SetLogSizeInEntries(DWORD dwSize) const {
		BT_SetLogSizeInEntries(m_iHandle, dwSize);
	}

	/// Get custom log file size in bytes.
	DWORD GetLogSizeInBytes(void) const {
		return BT_GetLogSizeInBytes(m_iHandle);
	}

	/// Set custom log file size in bytes.
	void SetLogSizeInBytes(DWORD dwSize) const {
		BT_SetLogSizeInBytes(m_iHandle, dwSize);
	}

	/// Return current set of log flags.
	DWORD GetLogFlags(void) const {
		return BT_GetLogFlags(m_iHandle);
	}

	/// Set new set of log flags.
	void SetLogFlags(DWORD dwLogFlags) const {
		BT_SetLogFlags(m_iHandle, dwLogFlags);
	}

	/// Return minimal log level accepted by tracing functions.
	BUGTRAP_LOGLEVEL GetLogLevel(void) const {
		return BT_GetLogLevel(m_iHandle);
	}

	/// Set minimal log level accepted by tracing functions.
	void SetLogLevel(BUGTRAP_LOGLEVEL eLogLevel) const {
		BT_SetLogLevel(m_iHandle, eLogLevel);
	}

	/// Get log echo mode.
	DWORD GetLogEchoMode(void) const {
		return BT_GetLogEchoMode(m_iHandle);
	}

	/// Set log echo mode.
	void SetLogEchoMode(DWORD dwLogEchoMode) const {
		BT_SetLogEchoMode(m_iHandle, dwLogEchoMode);
	}

	/// Clear log file.
	void Clear(void) const {
		BT_ClearLog(m_iHandle);
	}

	/// Insert entry into the beginning of custom log file.
	void InsertF(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, ...) const {
		va_list argList;
		va_start(argList, pszFormat);
		BT_InsLogEntryV(m_iHandle, eLogLevel, pszFormat, argList);
		va_end(argList);
	}

	/// Insert entry into the beginning of custom log file.
	void InsertF(LPCTSTR pszFormat, ...) const {
		va_list argList;
		va_start(argList, pszFormat);
		BT_InsLogEntryV(m_iHandle, m_eDefaultLogLevel, pszFormat, argList);
		va_end(argList);
	}

	/// Insert entry into the beginning of custom log file.
	void InsertV(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, va_list argList) const {
		BT_InsLogEntryV(m_iHandle, eLogLevel, pszFormat, argList);
	}

	/// Insert entry into the beginning of custom log file.
	void InsertV(LPCTSTR pszFormat, va_list argList) const {
		BT_InsLogEntryV(m_iHandle, m_eDefaultLogLevel, pszFormat, argList);
	}

	/// Insert entry into the beginning of custom log file.
	void Insert(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszEntry) const {
		BT_InsLogEntry(m_iHandle, eLogLevel, pszEntry);
	}

	/// Insert entry into the beginning of custom log file.
	void Insert(LPCTSTR pszEntry) const {
		BT_InsLogEntry(m_iHandle, m_eDefaultLogLevel, pszEntry);
	}

	/// Append entry to the end of custom log file.
	void AppendF(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, ...) const {
		va_list argList;
		va_start(argList, pszFormat);
		BT_AppLogEntryV(m_iHandle, eLogLevel, pszFormat, argList);
		va_end(argList);
	}

	/// Append entry to the end of custom log file.
	void AppendF(LPCTSTR pszFormat, ...) const {
		va_list argList;
		va_start(argList, pszFormat);
		BT_AppLogEntryV(m_iHandle, m_eDefaultLogLevel, pszFormat, argList);
		va_end(argList);
	}

	/// Append entry to the end of custom log file.
	void AppendV(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, va_list argList) const {
		BT_AppLogEntryV(m_iHandle, eLogLevel, pszFormat, argList);
	}

	/// Append entry to the end of custom log file.
	void AppendV(LPCTSTR pszFormat, va_list argList) const {
		BT_AppLogEntryV(m_iHandle, m_eDefaultLogLevel, pszFormat, argList);
	}

	/// Append entry to the end of custom log file.
	void Append(BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszEntry) const {
		BT_AppLogEntry(m_iHandle, eLogLevel, pszEntry);
	}

	/// Append entry to the end of custom log file.
	void Append(LPCTSTR pszEntry) const {
		BT_AppLogEntry(m_iHandle, m_eDefaultLogLevel, pszEntry);
	}

private:
	/// Prevent object from being accidentally copied.
	BTTrace(const BTTrace& rTrace);
	/// Prevent object from being accidentally copied.
	BTTrace& operator=(const BTTrace& rTrace);

	/// Log file handle.
	INT_PTR m_iHandle;
	/// Default log level.
	BUGTRAP_LOGLEVEL m_eDefaultLogLevel;
};

#endif // _BTTRACE_H_
