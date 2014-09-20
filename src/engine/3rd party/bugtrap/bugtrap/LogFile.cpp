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

#include "StdAfx.h"
#include "LogFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @param dwInitialLogSizeInBytes - initial log file size.
 */
CLogFile::CLogFile(DWORD dwInitialLogSizeInBytes) : m_StrStream(1024)
{
	*m_szLogFileName = _T('\0');
	m_dwLogSizeInEntries = MAXDWORD;
	m_dwLogSizeInBytes = MAXDWORD;
	m_dwNumEntries = 0;
	m_dwInitialLogSizeInBytes = dwInitialLogSizeInBytes;
	m_dwNumBytes = dwInitialLogSizeInBytes;
	m_dwLogEchoMode = BTLE_NONE;
	m_dwLogFlags = BTLF_NONE;
	m_eLogLevel = BTLL_ALL;
	m_pFirstEntry = NULL;
	m_pLastEntry = NULL;
#ifndef _UNICODE
	m_pchConsoleBufferA = NULL;
	m_dwConsoleBufferSizeA = 0;
	m_pchConsoleBufferW = NULL;
	m_dwConsoleBufferSizeW = 0;
#endif
	m_pchFormatBuffer = NULL;
	m_dwFormatBufferSize = 0;
	InitializeCriticalSection(&m_csLogFile);
}

CLogFile::~CLogFile(void)
{
#ifndef _UNICODE
	delete[] m_pchConsoleBufferA;
	delete[] m_pchConsoleBufferW;
#endif
	delete[] m_pchFormatBuffer;
	FreeEntries();
	DeleteCriticalSection(&m_csLogFile);
}

/**
 * @param pLogEntry - new log entry.
 */
void CLogFile::AddToHead(CLogEntry* pLogEntry)
{
	_ASSERTE(pLogEntry != NULL);
	pLogEntry->m_pPrevEntry = NULL;
	pLogEntry->m_pNextEntry = m_pFirstEntry;
	if (m_pFirstEntry)
		m_pFirstEntry->m_pPrevEntry = pLogEntry;
	else
		m_pLastEntry = pLogEntry;
	m_pFirstEntry = pLogEntry;
	m_dwNumBytes += pLogEntry->m_dwSize;
	++m_dwNumEntries;
	FreeTail();
}

/**
 * @param pLogEntry - new log entry.
 */
void CLogFile::AddToTail(CLogEntry* pLogEntry)
{
	_ASSERTE(pLogEntry != NULL);
	pLogEntry->m_pPrevEntry = m_pLastEntry;
	pLogEntry->m_pNextEntry = NULL;
	if (m_pLastEntry)
		m_pLastEntry->m_pNextEntry = pLogEntry;
	else
		m_pFirstEntry = pLogEntry;
	m_pLastEntry = pLogEntry;
	m_dwNumBytes += pLogEntry->m_dwSize;
	++m_dwNumEntries;
	FreeHead();
}

void CLogFile::DeleteHead(void)
{
	_ASSERTE(m_pFirstEntry != NULL);
	CLogEntry* pLogEntry = m_pFirstEntry;
	m_pFirstEntry = pLogEntry->m_pNextEntry;
	if (m_pFirstEntry)
		m_pFirstEntry->m_pPrevEntry = NULL;
	else
		m_pLastEntry = NULL;
	m_dwNumBytes -= pLogEntry->m_dwSize;
	--m_dwNumEntries;
	delete[] (PBYTE)pLogEntry;
}

void CLogFile::DeleteTail(void)
{
	_ASSERTE(m_pLastEntry != NULL);
	CLogEntry* pLogEntry = m_pLastEntry;
	m_pLastEntry = pLogEntry->m_pPrevEntry;
	if (m_pLastEntry)
		m_pLastEntry->m_pNextEntry = NULL;
	else
		m_pFirstEntry = NULL;
	m_dwNumBytes -= pLogEntry->m_dwSize;
	--m_dwNumEntries;
	delete[] (PBYTE)pLogEntry;
}

void CLogFile::FreeHead(void)
{
	if (m_dwLogSizeInEntries != MAXDWORD)
	{
		while (m_dwNumEntries > m_dwLogSizeInEntries && m_dwNumEntries > 0)
			DeleteHead();
	}
	if (m_dwLogSizeInBytes != MAXDWORD)
	{
		while (m_dwNumBytes > m_dwLogSizeInBytes && m_dwNumEntries > 0)
			DeleteHead();
	}
}

void CLogFile::FreeTail(void)
{
	if (m_dwLogSizeInEntries != MAXDWORD)
	{
		while (m_dwNumEntries > m_dwLogSizeInEntries && m_dwNumEntries > 0)
			DeleteTail();
	}
	if (m_dwLogSizeInBytes != MAXDWORD)
	{
		while (m_dwNumBytes > m_dwLogSizeInBytes && m_dwNumEntries > 0)
			DeleteTail();
	}
}

void CLogFile::FreeEntries(void)
{
	while (m_pFirstEntry)
	{
		CLogEntry* pNextEntry = m_pFirstEntry->m_pNextEntry;
		delete[] (PBYTE)m_pFirstEntry;
		m_pFirstEntry = pNextEntry;
	}
	m_pLastEntry = NULL;
	m_dwNumEntries = 0;
	m_dwNumBytes = m_dwInitialLogSizeInBytes;
}

/**
 * @return pointer to custom log file name.
 */
PCTSTR CLogFile::GetLogFileName(void)
{
	if (*m_szLogFileName == _T('\0'))
		CompleteLogFileName(NULL);
	return m_szLogFileName;
}

/**
 * @param eLogLevel - log level number.
 * @return log level prefix.
 */
PCTSTR CLogFile::GetLogLevelPrefix(BUGTRAP_LOGLEVEL eLogLevel)
{
	switch (eLogLevel)
	{
	case BTLL_ERROR:   return _T("ERROR");
	case BTLL_WARNING: return _T("WARNING");
	case BTLL_INFO:    return _T("INFO");
	default:           return NULL;
	}
}

/**
 * @return output console handle.
 */
HANDLE CLogFile::GetConsoleHandle(void) const
{
	HANDLE hConsole;
	if (m_dwLogEchoMode & BTLE_STDOUT)
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	else if (m_dwLogEchoMode & BTLE_STDERR)
		hConsole = GetStdHandle(STD_ERROR_HANDLE);
	else
		hConsole = NULL;
	return hConsole;
}

/**
 * @param hConsole - console handle.
 */
void CLogFile::WriteTextToConsole(HANDLE hConsole)
{
	DWORD dwTextLength, dwWritten;
	PCTSTR pszText = m_StrStream;
#ifndef _UNICODE
	UINT uConsoleCP = GetConsoleOutputCP();
	if (uConsoleCP != CP_ACP)
	{
		DWORD dwTextSizeW = MultiByteToWideChar(CP_ACP, 0, pszText, -1, NULL, 0);
		if (m_dwConsoleBufferSizeW < dwTextSizeW)
		{
			dwTextSizeW *= 2;
			delete[] m_pchConsoleBufferW;
			m_pchConsoleBufferW = new WCHAR[dwTextSizeW];
			if (m_pchConsoleBufferW == NULL)
			{
				m_dwConsoleBufferSizeW = 0;
				return;
			}
			m_dwConsoleBufferSizeW = dwTextSizeW;
		}
		MultiByteToWideChar(CP_ACP, 0, pszText, -1, m_pchConsoleBufferW, dwTextSizeW);
		DWORD dwTextSizeA = WideCharToMultiByte(uConsoleCP, 0, m_pchConsoleBufferW, -1, NULL, 0, NULL, NULL);
		if (m_dwConsoleBufferSizeA < dwTextSizeA)
		{
			dwTextSizeA *= 2;
			delete[] m_pchConsoleBufferA;
			m_pchConsoleBufferA = new CHAR[dwTextSizeA];
			if (m_pchConsoleBufferA == NULL)
			{
				m_dwConsoleBufferSizeA = 0;
				return;
			}
			m_dwConsoleBufferSizeA = dwTextSizeA;
		}
		dwTextLength = WideCharToMultiByte(uConsoleCP, 0, m_pchConsoleBufferW, -1, m_pchConsoleBufferA, dwTextSizeA, NULL, NULL);
		WriteConsoleA(hConsole, m_pchConsoleBufferA, dwTextLength - 1, &dwWritten, NULL);
	}
	else
	{
		dwTextLength = m_StrStream.GetLength();
		WriteConsoleA(hConsole, pszText, dwTextLength, &dwWritten, NULL);
	}
#else
	dwTextLength = m_StrStream.GetLength();
	WriteConsole(hConsole, pszText, dwTextLength, &dwWritten, NULL);
#endif
}

/**
 * @param pSystemTime - pointer to system time.
 * @param pszTimeStatistics - time statistics buffer.
 * @param dwTimeStatisticsSize - size of time statistics buffer.
 */
void CLogFile::GetTimeStatistics(const SYSTEMTIME* pSystemTime, PTSTR pszTimeStatistics, DWORD dwTimeStatisticsSize)
{
	_stprintf_s(pszTimeStatistics, dwTimeStatisticsSize,
	            _T("%04d/%02d/%02d %02d:%02d:%02d"),
	            pSystemTime->wYear, pSystemTime->wMonth, pSystemTime->wDay, pSystemTime->wHour, pSystemTime->wMinute, pSystemTime->wSecond);
}

/**
 * @param pSystemTime - pointer to system time.
 * @param eLogLevel - log level number.
 * @param pszEntry - log entry text.
 */
void CLogFile::FillEntryText(BUGTRAP_LOGLEVEL eLogLevel, const SYSTEMTIME* pSystemTime, PCTSTR pszEntry)
{
	_ASSERTE(pszEntry != NULL);
	m_StrStream.Reset();
	if (m_dwLogFlags & BTLF_SHOWTIMESTAMP)
	{
		TCHAR szTimeStatistics[32];
		GetTimeStatistics(pSystemTime, szTimeStatistics, countof(szTimeStatistics));
		m_StrStream << _T('[') << szTimeStatistics << _T("] ");
	}
	if (m_dwLogFlags & BTLF_SHOWLOGLEVEL)
	{
		PCTSTR pszLogLevelPrefix = GetLogLevelPrefix(eLogLevel);
		if (pszLogLevelPrefix)
			m_StrStream << pszLogLevelPrefix << _T(": ");
	}
	m_StrStream << pszEntry << _T("\r\n");
}

/**
 * @param dwFormatBufferSize - requested size of the buffer.
 * @return format buffer.
 */
PTCHAR CLogFile::GetFormatBuffer(DWORD dwFormatBufferSize)
{
	if (m_dwFormatBufferSize < dwFormatBufferSize)
	{
		delete[] m_pchFormatBuffer;
		m_pchFormatBuffer = new TCHAR[dwFormatBufferSize];
		m_dwFormatBufferSize = m_pchFormatBuffer != NULL ? dwFormatBufferSize : 0;
	}
	return m_pchFormatBuffer;
}

/**
 * @return format buffer size.
 */
DWORD CLogFile::GetFormatBufferSize(void)
{
	if (m_dwFormatBufferSize == 0)
	{
		// Allocate initial buffer.
		GetFormatBuffer(512);
	}
	return m_dwFormatBufferSize;
}

/**
 * @param pszFormat - format expression.
 * @param argList - variable argument list.
 * @return formatted output.
 */
PTSTR CLogFile::FormatBufferV(PCTSTR pszFormat, va_list argList)
{
	DWORD dwFormatBufferSize = GetFormatBufferSize();
	_ASSERTE(dwFormatBufferSize > 0);
	for (;;)
	{
		PTCHAR pchFormatBuffer = GetFormatBuffer(dwFormatBufferSize);
		if (pchFormatBuffer == NULL)
			return NULL;
		int iResult = _vsntprintf_s(pchFormatBuffer, dwFormatBufferSize, _TRUNCATE, pszFormat, argList);
		if (iResult < 0)
			dwFormatBufferSize *= 2;
		else
			return pchFormatBuffer;
	}
}

/**
 * @param pszFormat - format expression.
 * @param argList - variable argument list.
 * @return formatted output.
 */
PTSTR CLogFile::FormatBufferF(PCTSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	PTSTR pszFormatBuffer = FormatBufferV(pszFormat, argList);
	va_end(argList);
	return pszFormatBuffer;
}

/**
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param rcsConsoleAccess - provides synchronous access to the console.
 * @param pszFormat - format string.
 */
void CLogFile::WriteLogEntryF(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	WriteLogEntryV(eLogLevel, eEntryMode, rcsConsoleAccess, pszFormat, argList);
	va_end(argList);
}

/**
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param rcsConsoleAccess - provides synchronous access to the console.
 * @param pszFormat - format string.
 * @param argList - variable argument list.
 */
void CLogFile::WriteLogEntryV(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszFormat, va_list argList)
{
	PTSTR pszFormatBuffer = FormatBufferV(pszFormat, argList);
	_ASSERTE(pszFormatBuffer != NULL);
	if (pszFormatBuffer != NULL)
		WriteLogEntry(eLogLevel, eEntryMode, rcsConsoleAccess, pszFormatBuffer);
}
