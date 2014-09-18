/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text format analyzer.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "TextFormat.h"
#include "BugTrapUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Detect file format and encoding.
 * @param pszFileName - file name.
 * @param hFile - file handle.
 * @param bTextFile - true if file contains text and false for binary files.
 * @param eEncoding - text encoding.
 * @return sizeof of encoding signature.
 */
DWORD DetectFileFormat(PCTSTR pszFileName, HANDLE hFile, BOOL& bTextFile, TEXT_ENCODING& eEncoding)
{
	_ASSERTE(hFile != INVALID_HANDLE_VALUE);
	BYTE arrBuffer[100];
	DWORD dwNumRead = 0;
	if (! ReadFile(hFile, arrBuffer, sizeof(arrBuffer), &dwNumRead, NULL))
	{
		bTextFile = FALSE;
		eEncoding = TXTENC_ANSI;
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		return 0;
	}
	static const TCHAR szXmlFileExt[] = _T(".xml");
	const DWORD dwXmlFileExtLength = countof(szXmlFileExt) - 1;
	DWORD dwFileNameLength = _tcslen(pszFileName);
	BOOL bIsXmlFile = dwFileNameLength >= dwXmlFileExtLength && _tcsicmp(pszFileName + dwFileNameLength - dwXmlFileExtLength, szXmlFileExt) == 0;
	bTextFile = TRUE;
	DWORD dwStartPos, dwCharSize, dwOffsetFromStart;
	if (dwNumRead >= sizeof(g_arrUTF16LEPreamble) && memcmp(arrBuffer, g_arrUTF16LEPreamble, sizeof(g_arrUTF16LEPreamble)) == 0)
	{
		eEncoding = TXTENC_UTF16LE;
		dwStartPos = sizeof(g_arrUTF16LEPreamble);
		dwCharSize = sizeof(WCHAR);
		dwOffsetFromStart = 0;
	}
	else if (dwNumRead >= sizeof(g_arrUTF16BEPreamble) && memcmp(arrBuffer, g_arrUTF16BEPreamble, sizeof(g_arrUTF16BEPreamble)) == 0)
	{
		eEncoding = TXTENC_UTF16BE;
		dwStartPos = sizeof(g_arrUTF16BEPreamble);
		dwCharSize = sizeof(WCHAR);
		dwOffsetFromStart = 1;
	}
	else if (dwNumRead >= sizeof(g_arrUTF8Preamble) && memcmp(arrBuffer, g_arrUTF8Preamble, sizeof(g_arrUTF8Preamble)) == 0)
	{
		eEncoding = TXTENC_UTF8;
		dwStartPos = sizeof(g_arrUTF8Preamble);
		dwCharSize = sizeof(CHAR);
		dwOffsetFromStart = 0;
	}
	else if (bIsXmlFile)
	{
		static const CHAR szXmlProcessingInstruction[] = "<?xml ";
		const DWORD dwXmlProcessingInstructionLength = sizeof(szXmlProcessingInstruction) - 1;
		static const CHAR szEncodingAttribute[] = "encoding";
		const DWORD dwEncodingAttributeLength = sizeof(szEncodingAttribute) - 1;
		static const CHAR szUTF8Encoding[] = "UTF-8";
		const DWORD dwUTF8EncodingLength = sizeof(szUTF8Encoding) - 1;

		eEncoding = TXTENC_UTF8; // default XML encoding
		dwStartPos = dwOffsetFromStart = 0;
		dwCharSize = sizeof(CHAR);
		if (dwNumRead >= dwXmlProcessingInstructionLength && _memicmp(arrBuffer, szXmlProcessingInstruction, dwXmlProcessingInstructionLength) == 0)
		{
			DWORD dwBytePos = dwXmlProcessingInstructionLength;
			while (dwBytePos < dwNumRead)
			{
				while (dwBytePos < dwNumRead && IsSpace(arrBuffer[dwBytePos]))
					++dwBytePos;
				if (dwBytePos >= dwNumRead || arrBuffer[dwBytePos] == '?')
					break;
				DWORD dwWordStart = dwBytePos;
				while (dwBytePos < dwNumRead && IsAlpha(arrBuffer[dwBytePos]))
					++dwBytePos;
				DWORD dwWordLength = dwBytePos - dwWordStart;
				if (dwWordLength == 0)
					break;
				while (dwBytePos < dwNumRead && IsSpace(arrBuffer[dwBytePos]))
					++dwBytePos;
				if (dwBytePos < dwNumRead && arrBuffer[dwBytePos] == '=')
					++dwBytePos;
				else
					break;
				while (dwBytePos < dwNumRead && IsSpace(arrBuffer[dwBytePos]))
					++dwBytePos;
				if (dwBytePos < dwNumRead && IsQuotation(arrBuffer[dwBytePos]))
					++dwBytePos;
				else
					break;
				if (dwBytePos < dwNumRead)
				{
					DWORD dwValueStart = dwBytePos;
					while (dwBytePos < dwNumRead && ! IsQuotation(arrBuffer[dwBytePos]))
						++dwBytePos;
					DWORD dwValueLength = dwBytePos - dwValueStart;
					if (dwBytePos < dwNumRead)
						++dwBytePos;
					if (dwWordLength == dwEncodingAttributeLength && _memicmp(arrBuffer + dwWordStart, szEncodingAttribute, dwWordLength) == 0)
					{
						if (dwValueLength != dwUTF8EncodingLength || _memicmp(arrBuffer + dwValueStart, szUTF8Encoding, dwUTF8EncodingLength) != 0)
							eEncoding = TXTENC_ANSI;
						break;
					}
				}
			}
		}
	}
	else
	{
		eEncoding = TXTENC_ANSI;
		dwStartPos = dwOffsetFromStart = 0;
		dwCharSize = sizeof(CHAR);
	}
	for (DWORD dwBytePos = dwStartPos + dwOffsetFromStart; dwBytePos < dwNumRead; dwBytePos += dwCharSize)
	{
		if (arrBuffer[dwBytePos] == 0)
		{
			bTextFile = FALSE;
			eEncoding = TXTENC_ANSI;
			dwStartPos = 0;
			break;
		}
	}
	return dwStartPos;
}
