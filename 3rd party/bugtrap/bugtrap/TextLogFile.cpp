/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text log file.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "TextLogFile.h"
#include "TextFormat.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @return true if the log was loaded successfully.
 */
BOOL CTextLogFile::LoadEntries(void)
{
#ifdef _DEBUG
	DWORD dwStartTime = GetTickCount();
#endif
	BOOL bResult = FALSE;
	PCTSTR pszLogFileName = GetLogFileName();
	HANDLE hFile = CreateFile(pszLogFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize == 0)
		{
			// ignore empty files
			CloseHandle(hFile);
			return TRUE;
		}
		DWORD dwBufferSize = min(dwFileSize, g_dwMaxBufferSize);
		PBYTE pFileBuffer = new BYTE[dwBufferSize];
		if (pFileBuffer)
		{
			BYTE arrUTF8Preamble[sizeof(g_arrUTF8Preamble)];
			DWORD dwWritten = 0;
			if (ReadFile(hFile, arrUTF8Preamble, sizeof(arrUTF8Preamble), &dwWritten, NULL) &&
				dwWritten == sizeof(arrUTF8Preamble) && memcmp(arrUTF8Preamble, g_arrUTF8Preamble, sizeof(arrUTF8Preamble)) == 0)
			{
				bResult = TRUE;
				DWORD dwCurrentPos = 0, dwLineStart = 0;
				for (;;)
				{
					dwWritten = 0;
					DWORD dwFreeSize = dwBufferSize - dwCurrentPos;
					if (! ReadFile(hFile, pFileBuffer + dwCurrentPos, dwFreeSize, &dwWritten, NULL))
						goto end;
					BOOL bEndOfFile = dwWritten < dwFreeSize;
					dwWritten += dwCurrentPos;
					dwCurrentPos = 0;
					if (dwWritten == 0)
						goto end;
					for (;;)
					{
						if (dwCurrentPos >= dwWritten && ! bEndOfFile)
						{
							if (dwLineStart > 0)
							{
								dwCurrentPos -= dwLineStart;
								MoveMemory(pFileBuffer, pFileBuffer + dwLineStart, dwCurrentPos);
								dwLineStart = 0;
							}
							else
							{
								dwBufferSize *= 2;
								PBYTE pNewFileBuffer = new BYTE[dwBufferSize];
								if (! pNewFileBuffer)
								{
									bResult = FALSE;
									goto end;
								}
								CopyMemory(pNewFileBuffer, pFileBuffer, dwCurrentPos);
								delete[] pFileBuffer;
								pFileBuffer = pNewFileBuffer;
							}
							break;
						}
						if (dwCurrentPos < dwWritten ? pFileBuffer[dwCurrentPos] == '\r' || pFileBuffer[dwCurrentPos] == '\n' : bEndOfFile)
						{
							if (dwLineStart < dwCurrentPos && ! AddToTail(pFileBuffer + dwLineStart, dwCurrentPos - dwLineStart, true))
							{
								bResult = FALSE;
								goto end;
							}
							if (dwCurrentPos == dwWritten) // bEndOfFile == TRUE, see condition above
							{
								bResult = TRUE;
								goto end;
							}
							dwLineStart = dwCurrentPos + 1;
						}
						++dwCurrentPos;
					}
				}
			}
end:
			if (! bResult)
				FreeEntries();
			delete[] pFileBuffer;
		}
		CloseHandle(hFile);
	}
	else
	{
		DWORD dwLastError = GetLastError();
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
	_stprintf_s(szMessage, countof(szMessage), _T("CTextLogFile::LoadEntries(): %lu entries, %lu bytes, %lu milliseconds\r\n"), GetNumEntries(), GetNumBytes(), dwEndTime - dwStartTime);
	OutputDebugString(szMessage);
#endif
	return bResult;
}

/**
 * @param bCrash - true if crash has occurred.
 * @return true if the log was saved successfully.
 */
BOOL CTextLogFile::SaveEntries(bool /*bCrash*/)
{
#ifdef _DEBUG
	DWORD dwStartTime = GetTickCount();
#endif
	PCTSTR pszLogFileName = GetLogFileName();
	HANDLE hFile = CreateFile(pszLogFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD dwWritten;
	WriteFile(hFile, g_arrUTF8Preamble, sizeof(g_arrUTF8Preamble), &dwWritten, NULL);
	CLogEntry* pLogEntry = GetFirstEntry();
	while (pLogEntry)
	{
		CTextLogEntry* pTextLogEntry = (CTextLogEntry*)pLogEntry;
		WriteFile(hFile, pTextLogEntry->m_pbData, pTextLogEntry->m_dwSize, &dwWritten, NULL);
		pLogEntry = pLogEntry->m_pNextEntry;
	}
#ifdef _DEBUG
	DWORD dwEndTime = GetTickCount();
	TCHAR szMessage[128];
	_stprintf_s(szMessage, countof(szMessage), _T("CTextLogFile::SaveEntries(): %lu entries, %lu bytes, %lu milliseconds\r\n"), GetNumEntries(), GetNumBytes(), dwEndTime - dwStartTime);
	OutputDebugString(szMessage);
#endif
	CloseHandle(hFile);
	return TRUE;
}

/**
 * @param pbData - entry data.
 * @param dwSize - data size.
 * @param bAddCrLf - true if CR/LF must be added.
 * @return pointer to the new entry.
 */
CTextLogFile::CTextLogEntry* CTextLogFile::AllocLogEntry(const BYTE* pbData, DWORD dwSize, bool bAddCrLf)
{
	DWORD dwFullSize = bAddCrLf ? dwSize + 2 : dwSize;
	CTextLogEntry* pLogEntry = (CTextLogEntry*)new BYTE[sizeof(CTextLogEntry) + dwFullSize];
	if (pLogEntry)
	{
		pLogEntry->m_dwSize = dwFullSize;
		PBYTE pDstData = pLogEntry->m_pbData;
		CopyMemory(pDstData, pbData, dwSize);
		if (bAddCrLf)
		{
			pDstData[dwSize++] = _T('\r');
			pDstData[dwSize++] = _T('\n');
		}
	}
	return pLogEntry;
}

/**
 * @param pbData - entry data.
 * @param dwSize - data size.
 * @param bAddCrLf - true if CR/LF must be added.
 * @return true if entry was added.
 */
BOOL CTextLogFile::AddToHead(const BYTE* pbData, DWORD dwSize, bool bAddCrLf)
{
	CTextLogEntry* pLogEntry = AllocLogEntry(pbData, dwSize, bAddCrLf);
	if (pLogEntry)
	{
		CLogFile::AddToHead(pLogEntry);
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * @param pbData - entry data.
 * @param dwSize - data size.
 * @param bAddCrLf - true if CR/LF must be added.
 * @return true if entry was added.
 */
BOOL CTextLogFile::AddToTail(const BYTE* pbData, DWORD dwSize, bool bAddCrLf)
{
	CTextLogEntry* pLogEntry = AllocLogEntry(pbData, dwSize, bAddCrLf);
	if (pLogEntry)
	{
		CLogFile::AddToTail(pLogEntry);
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * @param bAddCrLf - true if CR/LF must be added.
 * @return true if entry was added.
 */
BOOL CTextLogFile::AddToHead(bool bAddCrLf)
{
	const BYTE* pBuffer = m_MemStream.GetBuffer();
	if (pBuffer == NULL)
		return FALSE;
	DWORD dwLength = m_MemStream.GetLength();
	_ASSERTE(dwLength > 0);
	return AddToHead(pBuffer, dwLength, bAddCrLf);
}

/**
 * @param bAddCrLf - true if CR/LF must be added.
 * @return true if entry was added.
 */
BOOL CTextLogFile::AddToTail(bool bAddCrLf)
{
	const BYTE* pBuffer = m_MemStream.GetBuffer();
	if (pBuffer == NULL)
		return FALSE;
	DWORD dwLength = m_MemStream.GetLength();
	_ASSERTE(dwLength > 0);
	return AddToTail(pBuffer, dwLength, bAddCrLf);
}

/**
 * @param eLogLevel - log level number.
 * @param eEntryMode - entry mode.
 * @param rcsConsoleAccess - provides synchronous access to the console.
 * @param pszEntry - log entry text.
 */
void CTextLogFile::WriteLogEntry(BUGTRAP_LOGLEVEL eLogLevel, ENTRY_MODE eEntryMode, CRITICAL_SECTION& rcsConsoleAccess, PCTSTR pszEntry)
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
		FillEntryText(eLogLevel, &st, pszEntry);
		if (hConsole)
			WriteTextToConsole(hConsole);
		if (bLogEcho)
			WriteTextToDebugConsole();
		EncodeEntryText();
		switch (eEntryMode)
		{
		case EM_APPEND:
			AddToTail(false);
			break;
		case EM_INSERT:
			AddToHead(false);
			break;
		default:
			_ASSERT(FALSE);
		}
		if (bConsoleOutput)
			LeaveCriticalSection(&rcsConsoleAccess);
	}
}
