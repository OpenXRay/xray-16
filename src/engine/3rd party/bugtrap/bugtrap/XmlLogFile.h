/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: XML log file.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "LogFile.h"

/**
 * @brief XML log file.
 */
class CXmlLogFile : public CLogFile
{
public:
	/// Initialize the object.
	CXmlLogFile(void);
	/// Load entries into memory.
	virtual BOOL LoadEntries(void);
	/// Save entries into disk.
	virtual BOOL SaveEntries(bool bCrash);
	/// Add new log entry.
	virtual void WriteLogEntry(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszEntry);

private:
	/// Log entry data.
	struct CXmlLogEntry : public CLogFile::CLogEntry
	{
#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
		/// Data buffer.
		TCHAR m_pchData[0];
#pragma warning(pop)
	};

	/// Base log record.
	class CBaseLogRecord
	{
	public:
		virtual ~CBaseLogRecord(void) { }
		/// Reset log entry.
		virtual void Reset(void) = 0;
		/// Set log level.
		virtual void SetLogLevel(PCTSTR pszLogLevel) = 0;
		/// Get log level.
		virtual PCTSTR GetLogLevel(void) const = 0;
		/// Get log level length.
		virtual int GetLogLevelLength(void) const = 0;
		/// Set time statistics.
		virtual void SetTimeStatistics(PCTSTR pszTimeStatistics) = 0;
		/// Get time statistics.
		virtual PCTSTR GetTimeStatistics(void) const = 0;
		/// Get time statistics length.
		virtual int GetTimeStatisticsLength(void) const = 0;
		/// Set entry text.
		virtual void SetEntryText(PCTSTR pszEntryText) = 0;
		/// Get entry text.
		virtual PCTSTR GetEntryText(void) const = 0;
		/// Get entry text length.
		virtual int GetEntryTextLength(void) const = 0;
	};

	/// Log record that keeps strings.
	class CStrLogRecord : public CBaseLogRecord
	{
	public:
		/// Object initializer.
		CStrLogRecord(void);
		/// Reset log entry.
		virtual void Reset(void);
		/// Set log level.
		virtual void SetLogLevel(PCTSTR pszLogLevel)
		{ m_strLogLevel = pszLogLevel; }
		/// Get log level.
		virtual PCTSTR GetLogLevel(void) const
		{ return m_strLogLevel; }
		/// Get log level length.
		virtual int GetLogLevelLength(void) const
		{ return m_strLogLevel.GetLength(); }
		/// Set time statistics.
		virtual void SetTimeStatistics(PCTSTR pszTimeStatistics)
		{ m_strTimeStatistics = pszTimeStatistics; }
		/// Get time statistics.
		virtual PCTSTR GetTimeStatistics(void) const
		{ return m_strTimeStatistics; }
		/// Get time statistics length.
		virtual int GetTimeStatisticsLength(void) const
		{ return m_strTimeStatistics.GetLength(); }
		/// Set entry text.
		virtual void SetEntryText(PCTSTR pszEntryText)
		{ m_strEntryText = pszEntryText; }
		/// Get entry text.
		virtual PCTSTR GetEntryText(void) const
		{ return m_strEntryText; }
		/// Get entry text length.
		virtual int GetEntryTextLength(void) const
		{ return m_strEntryText.GetLength(); }

	private:
		/// Log level.
		CStrStream m_strLogLevel;
		/// time statistics.
		CStrStream m_strTimeStatistics;
		/// Log entry text.
		CStrStream m_strEntryText;
	};

	/// Log record that keeps pointers to strings.
	class CPtrLogRecord : public CBaseLogRecord
	{
	public:
		/// Object initializer.
		CPtrLogRecord(void)
		{ CPtrLogRecord::Reset(); }
		/// Reset log entry.
		virtual void Reset(void);
		/// Set log level.
		virtual void SetLogLevel(PCTSTR pszLogLevel)
		{ m_pszLogLevel = pszLogLevel; }
		/// Get log level.
		virtual PCTSTR GetLogLevel(void) const
		{ return m_pszLogLevel; }
		/// Get log level length.
		virtual int GetLogLevelLength(void) const
		{ return _tcslen(m_pszLogLevel); }
		/// Set time statistics.
		virtual void SetTimeStatistics(PCTSTR pszTimeStatistics)
		{ m_pszTimeStatistics = pszTimeStatistics; }
		/// Get time statistics.
		virtual PCTSTR GetTimeStatistics(void) const
		{ return m_pszTimeStatistics; }
		/// Get time statistics length.
		virtual int GetTimeStatisticsLength(void) const
		{ return _tcslen(m_pszTimeStatistics); }
		/// Set entry text.
		virtual void SetEntryText(PCTSTR pszEntryText)
		{ m_pszEntryText = pszEntryText; }
		/// Get entry text.
		virtual PCTSTR GetEntryText(void) const
		{ return m_pszEntryText; }
		/// Get entry text length.
		virtual int GetEntryTextLength(void) const
		{ return _tcslen(m_pszEntryText); }

	private:
		/// Log level.
		PCTSTR m_pszLogLevel;
		/// time statistics.
		PCTSTR m_pszTimeStatistics;
		/// Log entry text.
		PCTSTR m_pszEntryText;
	};

	/// Protects the class from being accidentally copied.
	CXmlLogFile(const CXmlLogFile& rLogFile);
	/// Protects the class from being accidentally copied.
	CXmlLogFile& operator=(const CXmlLogFile& rLogFile);
	/// Get default log file extension.
	virtual PCTSTR GetLogFileExtension(void) const;
	/// Allocate log entry.
	CXmlLogEntry* AllocLogEntry(const CBaseLogRecord& rLogRecord);
	/// Add log entry to the head.
	BOOL AddToHead(const CBaseLogRecord& rLogRecord);
	/// Add log entry to the tail.
	BOOL AddToTail(const CBaseLogRecord& rLogRecord);
};

inline CXmlLogFile::CXmlLogFile(void) :
	CLogFile(sizeof("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
					"<log>\r\n"
					"</log>") - 1)
{
}

/**
 * @return log file extension.
 */
inline PCTSTR CXmlLogFile::GetLogFileExtension(void) const
{
	return _T(".xml");
}
