/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Custom log file description.
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
#include "StrStream.h"

/**
 * @brief Custom log file description.
 */
class CLogFile
{
public:
	/// Entry mode.
	enum ENTRY_MODE
	{
		EM_APPEND,
		EM_INSERT
	};

	/// Initialize the object.
	CLogFile(DWORD dwInitialLogSizeInBytes);
	/// Destroy the object.
	virtual ~CLogFile(void);
	/// Get custom log file name.
	PCTSTR GetLogFileName(void);
	/// Set custom log file name.
	void SetLogFileName(PCTSTR pszLogFileName);
	/// Get number of records in custom log file.
	DWORD GetLogSizeInEntries(void) const;
	/// Set number of records in custom log file.
	void SetLogSizeInEntries(DWORD dwLogSizeInEntries);
	/// Get maximum log file size in bytes.
	DWORD GetLogSizeInBytes(void) const;
	/// Set maximum log file size in bytes.
	void SetLogSizeInBytes(DWORD dwLogSizeInBytes);
	/// Return true if time stamp is added to every log entry.
	DWORD GetLogFlags(void) const;
	/// Set true if time stamp is added to every log entry.
	void SetLogFlags(DWORD dwLogFlags);
	/// Return minimal log level accepted by tracing functions.
	BUGTRAP_LOGLEVEL GetLogLevel(void) const;
	/// Set minimal log level accepted by tracing functions.
	void SetLogLevel(BUGTRAP_LOGLEVEL eLogLevel);
	/// Get echo mode.
	DWORD GetLogEchoMode(void);
	/// Set echo mode.
	void SetLogEchoMode(DWORD dwLogEchoMode);
	/// Take possession of the log file.
	void CaptureObject(void);
	/// Release possession of the log file.
	void ReleaseObject(void);
	/// Load Entries into memory.
	virtual BOOL LoadEntries(void) = 0;
	/// Save entries into disk.
	virtual BOOL SaveEntries(bool bCrash) = 0;
	/// Free log entries.
	void FreeEntries(void);
	/// Add new log entry.
	virtual void WriteLogEntry(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszEntry) = 0;
	/// Add new log entry.
	void WriteLogEntryF(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszFormat, ...);
	/// Add new log entry.
	void WriteLogEntryV(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszFormat, va_list argList);
	/// Get number of entries in a log.
	DWORD GetNumEntries(void) const;
	/// Get number of bytes in a log.
	DWORD GetNumBytes(void) const;

protected:
	/// Log entry data.
	struct CLogEntry
	{
		/// Pointer to the previous log entry.
		CLogEntry* m_pPrevEntry;
		/// Pointer to the next log entry.
		CLogEntry* m_pNextEntry;
		/// Buffer size in bytes.
		DWORD m_dwSize;
	};

	/// Get default log file extension.
	virtual PCTSTR GetLogFileExtension(void) const = 0;
	/// Get last entry text.
	PCTSTR GetEntryText(void) const;
	/// Get pointer to the first log entry.
	CLogEntry* GetFirstEntry(void) const;
	/// Get pointer to the last log entry.
	CLogEntry* GetLastEntry(void) const;
	/// Add log entry to the head.
	void AddToHead(CLogEntry* pLogEntry);
	/// Add log entry to the tail.
	void AddToTail(CLogEntry* pLogEntry);
	/// Remove the entry from the head.
	void DeleteHead(void);
	/// Remove the entry from the tail.
	void DeleteTail(void);
	/// Remove extra head entries.
	void FreeHead(void);
	/// Remove extra tail entries.
	void FreeTail(void);
	/// Get log level prefix.
	static PCTSTR GetLogLevelPrefix(BUGTRAP_LOGLEVEL eLogLevel);
	/// Get time statistics.
	static void GetTimeStatistics(const SYSTEMTIME* pSystemTime, PTSTR pszTimeStatistics, DWORD dwTimeStatisticsSize);
	/// Fill entry text.
	void FillEntryText(BUGTRAP_LOGLEVEL eLogLevel, const SYSTEMTIME* pSystemTime, PCTSTR pszEntry);
	/// Write text to console.
	void WriteTextToConsole(HANDLE hConsole);
	/// Write text to debug console.
	void WriteTextToDebugConsole(void);
	/// Get output console handle.
	HANDLE GetConsoleHandle(void) const;

private:
	/// Protects the class from being accidentally copied.
	CLogFile(const CLogFile& rLogFile);
	/// Protects the class from being accidentally copied.
	CLogFile& operator=(const CLogFile& rLogFile);
	/// Set log file name to default value.
	BOOL CompleteLogFileName(PCTSTR pszLogFileName);
	/// Return size of pre-allocated format buffer.
	DWORD GetFormatBufferSize(void);
	/// Allocate format buffer.
	PTCHAR GetFormatBuffer(DWORD dwFormatBufferSize);
	/// Fill buffer with formatted string.
	PTSTR FormatBufferV(PCTSTR pszFormat, va_list argList);
	/// Fill buffer with formatted string.
	PTSTR FormatBufferF(PCTSTR pszFormat, ...);

	/// Custom log file name.
	TCHAR m_szLogFileName[MAX_PATH];
	/// Number of entries kept in memory.
	DWORD m_dwNumEntries;
	/// Size of log in bytes.
	DWORD m_dwNumBytes;
	/// Initial log size in bytes.
	DWORD m_dwInitialLogSizeInBytes;
	/// Maximum number of records in custom log file.
	DWORD m_dwLogSizeInEntries;
	/// Maximum log file size in bytes.
	DWORD m_dwLogSizeInBytes;
	/// True if date and time stamp is added to every log entry.
	DWORD m_dwLogFlags;
	/// Echo mode.
	DWORD m_dwLogEchoMode;
	/// Minimal log level accepted by tracing functions.
	BUGTRAP_LOGLEVEL m_eLogLevel;
	/// Synchronization object.
	CRITICAL_SECTION m_csLogFile;
	/// Pointer to the first log entry.
	CLogEntry* m_pFirstEntry;
	/// Pointer to the last log entry.
	CLogEntry* m_pLastEntry;
	/// Pre-allocated buffer for log entry text.
	CStrStream m_StrStream;
#ifndef _UNICODE
	/// Pre-allocated buffer for encoded console message.
	PCHAR m_pchConsoleBufferA;
	/// Size of console buffer.
	DWORD m_dwConsoleBufferSizeA;
	/// Pre-allocated buffer for encoded console message.
	PWCHAR m_pchConsoleBufferW;
	/// Size of console buffer.
	DWORD m_dwConsoleBufferSizeW;
#endif
	/// Format buffer.
	PTCHAR m_pchFormatBuffer;
	/// Size of format buffer.
	DWORD m_dwFormatBufferSize;
};

/**
 *	@return maximum number of records in log file.
 */
inline DWORD CLogFile::GetLogSizeInEntries(void) const
{
	return m_dwLogSizeInEntries;
}

/**
 *	@param dwLogSizeInEntries - maximum number of records in log file.
 */
inline void CLogFile::SetLogSizeInEntries(DWORD dwLogSizeInEntries)
{
	m_dwLogSizeInEntries = dwLogSizeInEntries;
}

/**
 * @return maximum log file size in bytes.
 */
inline DWORD CLogFile::GetLogSizeInBytes(void) const
{
	return m_dwLogSizeInBytes;
}

/**
 * @param dwLogSizeInBytes - maximum log file size in bytes.
 */
inline void CLogFile::SetLogSizeInBytes(DWORD dwLogSizeInBytes)
{
	m_dwLogSizeInBytes = dwLogSizeInBytes;
}


inline void CLogFile::CaptureObject(void)
{
	EnterCriticalSection(&m_csLogFile);
}

inline void CLogFile::ReleaseObject(void)
{
	LeaveCriticalSection(&m_csLogFile);
}

/**
 * @return current set of log flags.
 */
inline DWORD CLogFile::GetLogFlags(void) const
{
	return m_dwLogFlags;
}

/**
 * @param dwLogFlags - set of log flags.
 */
inline void CLogFile::SetLogFlags(DWORD dwLogFlags)
{
	m_dwLogFlags = dwLogFlags;
}


/**
 * @return minimal log level accepted by tracing functions.
 */
inline BUGTRAP_LOGLEVEL CLogFile::GetLogLevel(void) const
{
	return m_eLogLevel;
}

/**
 * @param eLogLevel - minimal logl level accepted by tracing functions.
 */
inline void CLogFile::SetLogLevel(BUGTRAP_LOGLEVEL eLogLevel)
{
	m_eLogLevel = eLogLevel;
}

/**
 * @return current echo mode.
 */
inline DWORD CLogFile::GetLogEchoMode(void)
{
	return m_dwLogEchoMode;
}

/**
 * @param dwLogEchoMode - new echo mode.
 */
inline void CLogFile::SetLogEchoMode(DWORD dwLogEchoMode)
{
	m_dwLogEchoMode = dwLogEchoMode;
}

inline void CLogFile::WriteTextToDebugConsole(void)
{
	OutputDebugString(m_StrStream);
}

/**
 * @return last entry text.
 */
inline PCTSTR CLogFile::GetEntryText(void) const
{
	return (PCTSTR)m_StrStream;
}

/**
 * @return pointer to the first log entry.
 */
inline CLogFile::CLogEntry* CLogFile::GetFirstEntry(void) const
{
	return m_pFirstEntry;
}

/**
 * @return pointer to the last log entry.
 */
inline CLogFile::CLogEntry* CLogFile::GetLastEntry(void) const
{
	return m_pLastEntry;
}

/**
 * @param pszLogFileName - log file name.
 * @return true if log file name was created and false otherwise.
 */
inline BOOL CLogFile::CompleteLogFileName(PCTSTR pszLogFileName)
{
	return GetCompleteLogFileName(m_szLogFileName, pszLogFileName, GetLogFileExtension());
}

/**
 * @param pszLogFileName - pointer to custom log file name.
 */
inline void CLogFile::SetLogFileName(PCTSTR pszLogFileName)
{
	CompleteLogFileName(pszLogFileName);
}

/**
 * @return number of entries in a log.
 */
inline DWORD CLogFile::GetNumEntries(void) const
{
	return m_dwNumEntries;
}

/**
 * @return number of entries in a log.
 */
inline DWORD CLogFile::GetNumBytes(void) const
{
	return m_dwNumBytes;
}
