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

#include "StdAfx.h"
#include "XmlLogFile.h"
#include "FileStream.h"
#include "XmlReader.h"
#include "XmlWriter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CXmlLogFile::CStrLogRecord::CStrLogRecord(void) :
	m_strLogLevel(8),
	m_strTimeStatistics(32),
	m_strEntryText(1024)
{
}

void CXmlLogFile::CStrLogRecord::Reset(void)
{
	m_strLogLevel.Reset();
	m_strTimeStatistics.Reset();
	m_strEntryText.Reset();
}

void CXmlLogFile::CPtrLogRecord::Reset(void)
{
	m_pszLogLevel = _T("");
	m_pszTimeStatistics = _T("");
	m_pszEntryText = _T("");
}

/**
 * @return true if the log was loaded successfully.
 */
BOOL CXmlLogFile::LoadEntries(void)
{
#ifdef _DEBUG
	DWORD dwStartTime = GetTickCount();
#endif
	BOOL bResult = FALSE;
	PCTSTR pszLogFileName = GetLogFileName();
	CFileStream FileStream;
	if (FileStream.Open(pszLogFileName, OPEN_EXISTING, GENERIC_READ, FILE_SHARE_READ))
	{
		int nFileSize = FileStream.GetLength();
		if (nFileSize == 0)
		{
			// ignore empty files
			return TRUE;
		}
		CXmlReader XmlReader(&FileStream);
		CXmlReader::CXmlNode XmlNode;
		int iResult = XmlReader.GotoNextElement(_T("log"), XmlNode, CXmlReader::XGF_ALLOW_ELEMENT_END);
		if (iResult < 0)
			goto end;
		if (iResult > 0 && XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN)
		{
			CStrLogRecord LogRecord;
			for (;;)
			{
				iResult = XmlReader.GotoNextElement(_T("entry"), XmlNode, CXmlReader::XGF_ALLOW_ELEMENT_END);
				if (iResult <= 0)
					goto end;
				if (XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_END)
					break;
				_ASSERTE(XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN);
				LogRecord.Reset();

				iResult = XmlReader.GotoNextElement(_T("level"), XmlNode, 0);
				if (iResult <= 0)
					goto end;
				_ASSERTE(
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN ||
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT);
				// enforce constraint: log level is required field
				if (XmlNode.GetNodeType() != CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN)
					goto end;
				iResult = XmlReader.GotoText(XmlNode, 0);
				if (iResult <= 0)
					goto end;
				_ASSERTE(XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_TEXT);
				LogRecord.SetLogLevel(XmlNode.GetNodeValue());
				iResult = XmlReader.GotoNextElementEnd(XmlNode, 0);
				if (iResult <= 0)
					goto end;

				iResult = XmlReader.GotoNextElement(_T("time"), XmlNode, 0);
				if (iResult <= 0)
					goto end;
				_ASSERTE(
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN ||
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT);
				// enforce constraint: time statistics is required field
				if (XmlNode.GetNodeType() != CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN)
					goto end;
				iResult = XmlReader.GotoText(XmlNode, 0);
				if (iResult <= 0)
					goto end;
				_ASSERTE(XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_TEXT);
				LogRecord.SetTimeStatistics(XmlNode.GetNodeValue());
				iResult = XmlReader.GotoNextElementEnd(XmlNode, 0);
				if (iResult <= 0)
					goto end;

				iResult = XmlReader.GotoNextElement(_T("text"), XmlNode, 0);
				if (iResult <= 0)
					goto end;
				_ASSERTE(
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN ||
					XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT);
				// entry text is optional field
				if (XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_BEGIN)
				{
					iResult = XmlReader.GotoText(XmlNode, CXmlReader::XGF_ALLOW_ELEMENT_END);
					if (iResult <= 0)
						goto end;
					_ASSERTE(
						XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_TEXT ||
						XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_ELEMENT_END);
					if (XmlNode.GetNodeType() == CXmlReader::CXmlNode::XNT_TEXT)
					{
						LogRecord.SetEntryText(XmlNode.GetNodeValue());
						iResult = XmlReader.GotoNextElementEnd(XmlNode, 0);
						if (iResult <= 0)
							goto end;
					}
				}

				if (! AddToTail(LogRecord))
					goto end;
				iResult = XmlReader.GotoNextElementEnd(XmlNode, 0);
				if (iResult <= 0)
					goto end;
			}
		}
		bResult = TRUE;
end:
		if (! bResult)
			FreeEntries();
	}
	else
	{
		DWORD dwLastError = FileStream.GetLastError();
		if (dwLastError == ERROR_FILE_NOT_FOUND ||
			dwLastError == ERROR_PATH_NOT_FOUND ||
			GetFileAttributes(pszLogFileName) == INVALID_FILE_ATTRIBUTES)
		{
			bResult = TRUE; // ignore missing files
		}
	}
#ifdef _DEBUG
	DWORD dwEndTime = GetTickCount();
	TCHAR szMessage[128];
	_stprintf_s(szMessage, countof(szMessage), _T("CXmlLogFile::LoadEntries(): %lu entries, %lu bytes, %lu milliseconds\r\n"), GetNumEntries(), GetNumBytes(), dwEndTime - dwStartTime);
	OutputDebugString(szMessage);
#endif
	return bResult;
}

/**
 * @param bCrash - true if crash has occurred.
 * @return true if the log was saved successfully.
 */
BOOL CXmlLogFile::SaveEntries(bool /*bCrash*/)
{
#ifdef _DEBUG
	DWORD dwStartTime = GetTickCount();
#endif
	PCTSTR pszLogFileName = GetLogFileName();
	CFileStream FileStream(1024);
	if (! FileStream.Open(pszLogFileName, CREATE_ALWAYS, GENERIC_WRITE))
		return FALSE;
	CXmlWriter XmlWriter(&FileStream);
	XmlWriter.SetIndentation(_T(' '), 2);
	XmlWriter.WriteStartDocument();
	XmlWriter.WriteStartElement(_T("log")); // <log>
	CLogEntry* pLogEntry = GetFirstEntry();
	while (pLogEntry)
	{
		CXmlLogEntry* pXmlLogEntry = (CXmlLogEntry*)pLogEntry;
		PTCHAR pchPointer = pXmlLogEntry->m_pchData;
		_ASSERTE(pchPointer != NULL);
		PCTSTR pszLogLevel = pchPointer;
		pchPointer += _tcslen(pchPointer) + 1;
		PCTSTR pszTimeStatistics = pchPointer;
		pchPointer += _tcslen(pchPointer) + 1;
		PCTSTR pszEntry = pchPointer;
		//pchPointer += _tcslen(pchPointer) + 1;
		XmlWriter.WriteStartElement(_T("entry")); // <entry>
		 XmlWriter.WriteElementString(_T("level"), pszLogLevel); // <level>...</level>
		 XmlWriter.WriteElementString(_T("time"), pszTimeStatistics); // <time>...</time>
		 XmlWriter.WriteElementString(_T("text"), pszEntry); // <text>...</text>
		XmlWriter.WriteEndElement(); // </entry>
		pLogEntry = pLogEntry->m_pNextEntry;
	}
	XmlWriter.WriteEndElement(); // </log>
	XmlWriter.WriteEndDocument();
#ifdef _DEBUG
	DWORD dwEndTime = GetTickCount();
	TCHAR szMessage[128];
	_stprintf_s(szMessage, countof(szMessage), _T("CXmlLogFile::SaveEntries(): %lu entries, %lu bytes, %lu milliseconds\r\n"), GetNumEntries(), GetNumBytes(), dwEndTime - dwStartTime);
	OutputDebugString(szMessage);
#endif
	return TRUE;
}

/**
 * @param rLogRecord - reference the log record.
 * @return pointer to the new entry.
 */
CXmlLogFile::CXmlLogEntry* CXmlLogFile::AllocLogEntry(const CBaseLogRecord& rLogRecord)
{
	PCTSTR pszLogLevel = rLogRecord.GetLogLevel();
	DWORD dwLogLevelSize = rLogRecord.GetLogLevelLength() + 1;
	PCTSTR pszTimeStatistics = rLogRecord.GetTimeStatistics();
	DWORD dwTimeStatisticsSize = rLogRecord.GetTimeStatisticsLength() + 1;
	PCTSTR pszEntryText = rLogRecord.GetEntryText();
	DWORD dwEntryTextSize = rLogRecord.GetEntryTextLength() + 1;
	DWORD dwLogRecordSize = dwLogLevelSize + dwTimeStatisticsSize + dwEntryTextSize + 1;
	CXmlLogEntry* pLogEntry = (CXmlLogEntry*)new BYTE[sizeof(CXmlLogEntry) + dwLogRecordSize * sizeof(TCHAR)];
	if (pLogEntry)
	{
		PTCHAR pchPointer = pLogEntry->m_pchData;
		_ASSERTE(pchPointer != NULL);

		_tcscpy_s(pchPointer, dwLogLevelSize, pszLogLevel);
		pchPointer += dwLogLevelSize;
		dwLogLevelSize = GetStringSizeInUTF8(pszLogLevel);

		_tcscpy_s(pchPointer, dwTimeStatisticsSize, pszTimeStatistics);
		pchPointer += dwTimeStatisticsSize;
		dwTimeStatisticsSize = GetStringSizeInUTF8(pszTimeStatistics);

		_tcscpy_s(pchPointer, dwEntryTextSize, pszEntryText);
		pchPointer += dwEntryTextSize;
		dwEntryTextSize = GetStringSizeInUTF8(pszEntryText);

		*pchPointer = _T('\0');
		pLogEntry->m_dwSize = dwLogLevelSize + dwTimeStatisticsSize + dwEntryTextSize +
								sizeof("  <entry>\r\n"
									   "    <level></level>\r\n"
									   "    <time></time>\r\n"
									   "    <text></text>\r\n"
									   "  </entry>\r\n") - 1;
	}
	return pLogEntry;
}

/**
* @param rLogRecord - reference the log record.
 * @return true if entry was added.
 */
BOOL CXmlLogFile::AddToHead(const CBaseLogRecord& rLogRecord)
{
	CXmlLogEntry* pLogEntry = AllocLogEntry(rLogRecord);
	if (pLogEntry)
	{
		CLogFile::AddToHead(pLogEntry);
		return TRUE;
	}
	else
		return FALSE;
}

/**
* @param rLogRecord - reference the log record.
 * @return true if entry was added.
 */
BOOL CXmlLogFile::AddToTail(const CBaseLogRecord& rLogRecord)
{
	CXmlLogEntry* pLogEntry = AllocLogEntry(rLogRecord);
	if (pLogEntry)
	{
		CLogFile::AddToTail(pLogEntry);
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param rcsConsoleAccess - provides synchronous access to the console.
 * @param pszEntry - log entry text.
 */
void CXmlLogFile::WriteLogEntry(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszEntry)
{
	BUGTRAP_LOGLEVEL eLogFileLevel = GetLogLevel();
	if (eLogLevel <= eLogFileLevel)
	{
		DWORD dwLogEchoMode = GetLogEchoMode();
		HANDLE hConsole = GetConsoleHandle();
		BOOL bLogEcho = (dwLogEchoMode & BTLE_DBGOUT) != 0;
		BOOL bConsoleOutput = hConsole || bLogEcho;
		if (bConsoleOutput)
			EnterCriticalSection(&rcsConsoleAccess);
		SYSTEMTIME st;
		GetLocalTime(&st);
		if (hConsole || bLogEcho)
		{
			FillEntryText(eLogLevel, &st, pszEntry);
			if (hConsole)
				WriteTextToConsole(hConsole);
			if (bLogEcho)
				WriteTextToDebugConsole();
		}
		PCTSTR pszLogLevelPrefix = GetLogLevelPrefix(eLogLevel);
		TCHAR szTimeStatistics[32];
		GetTimeStatistics(&st, szTimeStatistics, countof(szTimeStatistics));
		CPtrLogRecord LogRecord;
		LogRecord.SetLogLevel(pszLogLevelPrefix);
		LogRecord.SetTimeStatistics(szTimeStatistics);
		LogRecord.SetEntryText(pszEntry);
		switch (eEntryMode)
		{
		case EM_APPEND:
			AddToTail(LogRecord);
			break;
		case EM_INSERT:
			AddToHead(LogRecord);
			break;
		default:
			_ASSERT(FALSE);
		}
		if (bConsoleOutput)
			LeaveCriticalSection(&rcsConsoleAccess);
	}
}
