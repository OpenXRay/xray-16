/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text encoding/decoding.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "Encoding.h"
#include "MemStream.h"
#include "BugTrapUtils.h"
#include "TextFormat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/// Maximal 7-bit ASCII value.
#define ASCII_MAX              0x007F
/// Non-ASCII character value.
#define NON_ASCII_CHAR         0x80
/// Maximal UTF-8 2-byte sequence (32 * 64 = 2048).
#define UTF8_2_MAX             0x07FF
/// Maximal UTF-8 3-byte sequence (16 * 64 * 64 = 2048).
#define UTF8_3_MAX             0xFFFF
/// Prefix of UTF-8 2-byte sequence.
#define UTF8_1ST_OF_2          0xC0    // 110xxxxx
/// Prefix of UTF-8 3-byte sequence.
#define UTF8_1ST_OF_3          0xE0    // 1110xxxx
/// Prefix of UTF-8 4-byte sequence.
#define UTF8_1ST_OF_4          0xF0    // 11110xxx
/// Prefix of UTF-8 bytes from sequence.
#define UTF8_TRAIL             0x80    // 10xxxxxx

/// Single object instance.
CUTF8Decoder CUTF8Decoder::m_instance;
/// Single object instance.
CUTF16LeDecoder CUTF16LeDecoder::m_instance;
/// Single object instance.
CUTF16BeDecoder CUTF16BeDecoder::m_instance;
/// Single object instance.
CAnsiDecoder CAnsiDecoder::m_instance;

/**
 * @param pchValue - pointer to the character normally stored in the string.
 * @param nCharSize - upon return keeps character size.
 * @return Unicode character value.
 */
static DWORD GetUnicodeValue(const TCHAR* pchValue, int& nCharSize)
{
#ifndef _UNICODE
	WCHAR arrUnicodeChar[2];
	arrUnicodeChar[0] = arrUnicodeChar[1] = 0;
	nCharSize = IsDBCSLeadByte(*pchValue) ? 2 : 1;
	MultiByteToWideChar(CP_ACP, 0, pchValue, nCharSize, arrUnicodeChar, countof(arrUnicodeChar));
#else
	const WCHAR* arrUnicodeChar = pchValue;
#endif
	DWORD dwUnicodeChar;
	if ((HIGH_SURROGATE_START <= arrUnicodeChar[0] && arrUnicodeChar[0] <= HIGH_SURROGATE_END) &&
		(LOW_SURROGATE_START <= arrUnicodeChar[1] && arrUnicodeChar[1] <= LOW_SURROGATE_END))
	{
		dwUnicodeChar = ((arrUnicodeChar[0] - HIGH_SURROGATE_START) << 10) + (arrUnicodeChar[1] - LOW_SURROGATE_START) + 0x10000;
#ifdef _UNICODE
		nCharSize = 2;
#endif
	}
	else
	{
#ifndef _UNICODE
		_ASSERTE(arrUnicodeChar[1] == 0);
#endif
		dwUnicodeChar = arrUnicodeChar[0];
#ifdef _UNICODE
		nCharSize = 1;
#endif
	}
	return dwUnicodeChar;
}

/**
 * @param dwUnicodeChar - Unicode character value.
 * @return true if data has been successfully written.
 */
bool CUTF8EncStream::WriteUTF8Bin(DWORD dwUnicodeChar)
{
	_ASSERTE(m_pOutputStream != NULL);
	if (dwUnicodeChar <= ASCII_MAX)
	{
		// 7-bit ASCII data is stored as-is
		if (! m_pOutputStream->WriteByte((BYTE)dwUnicodeChar))
			return false;
	}
	else if (dwUnicodeChar <= UTF8_2_MAX)
	{
		// Use upper 5 bits in first byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_1ST_OF_2 | ((dwUnicodeChar >> 6) & 0x1F))))
			return false;
		// Use lower 6 bits in second byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | (dwUnicodeChar & 0x3F))))
			return false;
	}
	else if (dwUnicodeChar <= UTF8_3_MAX)
	{
		// Use upper 4 bits in first byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_1ST_OF_3 | ((dwUnicodeChar >> 12) & 0x0F))))
			return false;
		// Use middle 6 bits in second byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | ((dwUnicodeChar >> 6) & 0x3F))))
			return false;
		// Use lower 6 bits in third byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | (dwUnicodeChar & 0x3F))))
			return false;
	}
	else
	{
		// Use upper 3 bits in first byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_1ST_OF_4 | ((dwUnicodeChar >> 18) & 0x07))))
			return false;
		// Use middle 6 bits in second byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | ((dwUnicodeChar >> 12) & 0x3F))))
			return false;
		// Use middle 6 bits in third byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | ((dwUnicodeChar >> 6) & 0x3F))))
			return false;
		// Use lower 6 bits in fourth byte
		if (! m_pOutputStream->WriteByte((BYTE)(UTF8_TRAIL | (dwUnicodeChar & 0x3F))))
			return false;
	}
	return true;
}

/**
 * @param dwUnicodeChar - Unicode character value.
 * @return true if data has been successfully written.
 */
bool CUTF8EncStream::WriteUTF8Hex(DWORD dwUnicodeChar)
{
	_ASSERTE(m_pOutputStream != NULL);
	CHAR szHexValue[16];
	int nLength;
	if (dwUnicodeChar <= ASCII_MAX)
	{
		if (! m_pOutputStream->WriteByte((BYTE)dwUnicodeChar))
			return false;
	}
	else if (dwUnicodeChar <= UTF8_3_MAX)
	{
		nLength = sprintf_s(szHexValue, countof(szHexValue), "&#x%04lX;", dwUnicodeChar);
		if (! m_pOutputStream->WriteBytes((const BYTE*)szHexValue, nLength))
			return false;
	}
	else
	{
		nLength = sprintf_s(szHexValue, countof(szHexValue), "&#x%06lX;", dwUnicodeChar);
		if (! m_pOutputStream->WriteBytes((const BYTE*)szHexValue, nLength))
			return false;
	}
	return true;
}

/**
 * @param pszString - string to be written.
 * @return true if data has been written.
 */
bool CUTF8EncStream::WriteUTF8Bin(PCTSTR pszString)
{
	int nPosition = 0;
	while (pszString[nPosition] != _T('\0'))
	{
		int nCharSize;
		if (! WriteUTF8Bin(pszString + nPosition, nCharSize))
			return false;
		nPosition += nCharSize;
	}
	return true;
}

/**
 * @param pszString - string to be written.
 * @return true if data has been written.
 */
bool CUTF8EncStream::WriteUTF8Hex(PCTSTR pszString)
{
	int nPosition = 0;
	while (pszString[nPosition] != _T('\0'))
	{
		int nCharSize;
		if (! WriteUTF8Hex(pszString + nPosition, nCharSize))
			return false;
		nPosition += nCharSize;
	}
	return true;
}

/**
 * @param pchValue - pointer to the character normally stored in the string.
 * @param nCharSize - upon return keeps character size.
 * @return true if data has been written.
 */
bool CUTF8EncStream::WriteUTF8Bin(const TCHAR* pchValue, int& nCharSize)
{
	DWORD dwUnicodeChar = GetUnicodeValue(pchValue, nCharSize);
	return WriteUTF8Bin(dwUnicodeChar);
}

/**
 * @param pchValue - pointer to the character normally stored in the string.
 * @param nCharSize - upon return keeps number of bytes in character.
 * @return true if data has been written.
 */
bool CUTF8EncStream::WriteUTF8Hex(const TCHAR* pchValue, int& nCharSize)
{
	DWORD dwUnicodeChar = GetUnicodeValue(pchValue, nCharSize);
	return WriteUTF8Hex(dwUnicodeChar);
}

/**
 * @brief Write a 32-bit integer in a compressed format to the buffer.
 * @param nValue - integer value.
 * @param pBuffer - pointer to the buffer.
 * @param nPosition - position in the buffer.
 * @param nBufferSize - size of output buffer.
 * @return true if operation has been completed successfully.
 */
bool Write7BitEncodedInt(int nValue, PBYTE pBuffer, int& nPosition, int nBufferSize)
{
	int nOldPosition = nPosition;
	while (nValue >= 0x80)
	{
		if (nPosition >= nBufferSize)
			goto error;
		pBuffer[nPosition++] = (BYTE)(nValue | 0x80);
		nValue >>= 7;
	}
	if (nPosition < nBufferSize)
	{
		pBuffer[nPosition++] = (BYTE)nValue;
		return true;
	}

error:
	nPosition = nOldPosition;
	return false;
}

/**
 * @brief Read a 32-bit integer in a compressed format to the buffer.
 * @param pBuffer - pointer to the buffer.
 * @param nPosition - position in the buffer.
 * @param nBufferSize - size of output buffer.
 * @return decoded integer value.
 */
int Read7BitEncodedInt(PBYTE pBuffer, int& nPosition, int nBufferSize)
{
	int nValue = 0, nShift = 0;
	for (;;)
	{
		if (nPosition >= nBufferSize)
			break;
		BYTE bTempValue = pBuffer[nPosition++];
		nValue |= (bTempValue & 0x7F) << (nShift & 0x1F);
		nShift += 7;
		if ((bTempValue & 0x80) == 0)
			break;
	}
	return nValue;
}

/**
 * @brief Write length prefixed Unicode string to the buffer.
 * @param rEncStream - shared encoder.
 * @param pszString - string value.
 * @param pBuffer - pointer to the buffer.
 * @param nPosition - position in the buffer.
 * @param nBufferSize - size of output buffer.
 * @return true if operation has been completed successfully.
 */
bool WriteBinaryString(CUTF8EncStream& rEncStream, PCTSTR pszString, PBYTE pBuffer, int& nPosition, int nBufferSize)
{
	CStream* pStream = rEncStream.GetStream();
	_ASSERTE(pStream != NULL);
	if (pStream == NULL)
		return false;
	unsigned uStreamFeatures = pStream->GetFeatures();
	_ASSERTE((uStreamFeatures & (CStream::SF_GETLENGTH | CStream::SF_SETPOSITION)) == (CStream::SF_GETLENGTH | CStream::SF_SETPOSITION));
	if ((uStreamFeatures & (CStream::SF_GETLENGTH | CStream::SF_SETPOSITION)) != (CStream::SF_GETLENGTH | CStream::SF_SETPOSITION))
		return false;
	int nOldPosition = nPosition;
	rEncStream.Reset();
	rEncStream.WriteUTF8Bin(pszString);
	pStream->SetPosition(0, FILE_BEGIN);
	int nLength = pStream->GetLength();
	_ASSERTE(nLength >= 0);
	if (! Write7BitEncodedInt(nLength, pBuffer, nPosition, nBufferSize))
		return false;
	if (nBufferSize - nPosition >= nLength)
	{
		int nNumRead = pStream->ReadBytes(pBuffer + nPosition, nLength);
		_ASSERTE(nNumRead == nLength);
		if (nNumRead == nLength)
		{
			nPosition += nLength;
			return true;
		}
	}
	nPosition = nOldPosition;
	return false;
}

/**
 * @brief Write length prefixed Unicode string to the buffer.
 * @param pszString - string value.
 * @param pBuffer - pointer to the buffer.
 * @param nPosition - position in the buffer.
 * @param nBufferSize - size of output buffer.
 * @return true if operation has been completed successfully.
 */
bool WriteBinaryString(PCTSTR pszString, PBYTE pBuffer, int& nPosition, int nBufferSize)
{
	CMemStream MemStream;
	CUTF8EncStream EncStream(&MemStream);
	return WriteBinaryString(EncStream, pszString, pBuffer, nPosition, nBufferSize);
}

/**
 * @brief Decode UTF-8 character to normal character.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param arrChar - output buffer for the character.
 * @param nCharSize - number of symbols in one character.
 * @return number of bytes in one character.
 */
int UTF8DecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
{
	nCharSize = -1;
	if (nNumBytes <= 0)
		return 0;
	BYTE bValue = pBytes[0];
	DWORD dwUnicodeChar;
	int nNumBytesInChar;
	if ((bValue & UTF8_1ST_OF_4) == UTF8_1ST_OF_4)
	{
		if (4 > nNumBytes)
			return 0;
		nNumBytesInChar = 4;
		dwUnicodeChar = ((DWORD)pBytes[0] & ~UTF8_1ST_OF_4 & 0x07) << 18;
		dwUnicodeChar |= ((DWORD)pBytes[1] & ~UTF8_TRAIL & 0x3F) << 12;
		dwUnicodeChar |= ((DWORD)pBytes[2] & ~UTF8_TRAIL & 0x3F) << 6;
		dwUnicodeChar |= (DWORD)pBytes[3] & ~UTF8_TRAIL;
	}
	else if ((bValue & UTF8_1ST_OF_3) == UTF8_1ST_OF_3)
	{
		if (3 > nNumBytes)
			return 0;
		nNumBytesInChar = 3;
		dwUnicodeChar = ((DWORD)pBytes[0] & ~UTF8_1ST_OF_3 & 0x0F) << 12;
		dwUnicodeChar |= ((DWORD)pBytes[1] & ~UTF8_TRAIL & 0x3F) << 6;
		dwUnicodeChar |= (DWORD)pBytes[2] & ~UTF8_TRAIL;
	}
	else if ((bValue & UTF8_1ST_OF_2) == UTF8_1ST_OF_2)
	{
		if (2 > nNumBytes)
			return 0;
		nNumBytesInChar = 2;
		dwUnicodeChar = ((DWORD)pBytes[0] & ~UTF8_1ST_OF_2 & 0x1F) << 6;
		dwUnicodeChar |= (DWORD)pBytes[1] & ~UTF8_TRAIL;
	}
	else if ((bValue & NON_ASCII_CHAR) == 0)
	{
		if (1 > nNumBytes)
			return 0;
		nNumBytesInChar = 1;
		dwUnicodeChar = (DWORD)pBytes[0];
	}
	else
		return 0;
	const WCHAR* arrUnicodeChar = (const WCHAR*)&dwUnicodeChar;
#ifdef _UNICODE
	if ((HIGH_SURROGATE_START <= arrUnicodeChar[0] && arrUnicodeChar[0] <= HIGH_SURROGATE_END) &&
		(LOW_SURROGATE_START <= arrUnicodeChar[1] && arrUnicodeChar[1] <= LOW_SURROGATE_END))
	{
		nCharSize = 2;
		arrChar[0] = arrUnicodeChar[0];
		arrChar[1] = arrUnicodeChar[1];
	}
	else
	{
		nCharSize = 1;
		arrChar[0] = arrUnicodeChar[0];
		_ASSERTE(arrUnicodeChar[1] == 0);
	}
#else
	int nWordsInChar;
	if ((HIGH_SURROGATE_START <= arrUnicodeChar[0] && arrUnicodeChar[0] <= HIGH_SURROGATE_END) &&
		(LOW_SURROGATE_START <= arrUnicodeChar[1] && arrUnicodeChar[1] <= LOW_SURROGATE_END))
	{
		nWordsInChar = 2;
	}
	else
	{
		nWordsInChar = 1;
	}
	nCharSize = WideCharToMultiByte(CP_ACP, 0, arrUnicodeChar, nWordsInChar, arrChar, countof(arrChar), NULL, NULL);
#endif
	return nNumBytesInChar;
}

/**
 * @brief Decode UTF-16 character to normal character.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param arrChar - output buffer for the character.
 * @param nCharSize - number of symbols in one character.
 * @return number of bytes in one character.
 */
int UTF16BeDecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
{
	if (UTF16BeToLeChar(pBytes, nNumBytes) < 0)
	{
		nCharSize = -1;
		return 0;
	}
	return UTF16LeDecodeChar(pBytes, nNumBytes, arrChar, nCharSize);
}

/**
 * @brief Decode UTF-16 character to normal character.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param arrChar - output buffer for the character.
 * @param nCharSize - number of symbols in one character.
 * @return number of bytes in one character.
 */
int UTF16LeDecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
{
	int nNumBytesInChar = GetUnicodeCharSize(pBytes);
	if (nNumBytes >= nNumBytesInChar)
	{
#ifdef _UNICODE
		_ASSERTE(nNumBytesInChar == sizeof(WCHAR) || nNumBytesInChar == 2 * sizeof(WCHAR));
		arrChar[0] = *(const WCHAR*)pBytes;
		if (nNumBytesInChar == 2 * sizeof(WCHAR))
			arrChar[1] = *((const WCHAR*)pBytes + 1);
		nCharSize = nNumBytesInChar / sizeof(WCHAR);
#else
		nCharSize = WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)pBytes, nNumBytesInChar, arrChar, countof(arrChar), NULL, NULL) / sizeof(WCHAR);
		if (nCharSize <= 0)
			nNumBytesInChar = 0;
#endif
	}
	else
		nCharSize = -1;
	return nNumBytesInChar;
}

/**
 * @brief Decode Ansi character to normal character.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param arrChar - output buffer for the character.
 * @param nCharSize - number of symbols in one character.
 * @return number of bytes in one character.
 */
int AnsiDecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
{
	int nNumBytesInChar = IsDBCSLeadByte(*pBytes) ? 2 : 1;
	if (nNumBytes >= nNumBytesInChar)
	{
#ifdef _UNICODE
		nCharSize = MultiByteToWideChar(CP_ACP, 0, (const CHAR*)pBytes, nNumBytesInChar, arrChar, countof(arrChar));
		if (nCharSize <= 0)
			nNumBytesInChar = 0;
#else
		arrChar[0] = *(const CHAR*)pBytes;
		if (nNumBytesInChar == 2)
			arrChar[1] = *((const CHAR*)pBytes + 1);
		nCharSize = nNumBytesInChar;
#endif
	}
	else
		nCharSize = -1;
	return nNumBytesInChar;
}

/**
 * @brief Decode UTF-8 string to normal text.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param pszString - decoded string.
 * @param nBufferSize - size of output buffer (in characters).
 * @return number of characters in resulting string including null-terminator.
 */
int UTF8DecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
{
	if (! nBufferSize)
		return 0;
	bool bAddNullTerminator;
	if (nNumBytes < 0)
	{
		nNumBytes = strlen((PCSTR)pBytes);
		--nBufferSize;
		bAddNullTerminator = true;
	}
	else
		bAddNullTerminator = false;
	int nBytePos = 0, nCharPos = 0;
	while (nBytePos < nNumBytes)
	{
		TCHAR arrChar[2];
		int nCharSize;
		int nNumBytesInChar = UTF8DecodeChar(pBytes + nBytePos, nNumBytes - nBytePos, arrChar, nCharSize);
		if (nNumBytesInChar <= 0 || nCharPos + nCharSize > nBufferSize)
			break;
		nBytePos += nNumBytesInChar;
		pszString[nCharPos++] = arrChar[0];
		if (nCharSize > 1)
			pszString[nCharPos++] = arrChar[1];
	}
	if (bAddNullTerminator)
		pszString[nCharPos++] = _T('\0');
	return nCharPos;
}

/**
 * @brief Decode UTF-16 string to normal text.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param pszString - decoded string.
 * @param nBufferSize - size of output buffer (in characters).
 * @return number of characters in resulting string including null-terminator.
 */
int UTF16BeDecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
{
	if (UTF16BeToLeString(pBytes, nNumBytes) == 0)
		return 0;
	return UTF8DecodeString(pBytes, nNumBytes, pszString, nBufferSize);
}

/**
 * @brief Decode UTF-16 string to normal text.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param pszString - decoded string.
 * @param nBufferSize - size of output buffer (in characters).
 * @return number of characters in resulting string including null-terminator.
 */
int UTF16LeDecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
{
#ifdef _UNICODE
	if (nNumBytes < 0)
		nNumBytes = (wcslen((const WCHAR*)pBytes) + 1) * sizeof(WCHAR);
	if (nNumBytes > nBufferSize)
		nNumBytes = nBufferSize;
	CopyMemory(pszString, pBytes, nNumBytes);
	return nNumBytes;
#else
	int nResult = WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)pBytes, nNumBytes / sizeof(WCHAR), pszString, nBufferSize, NULL, NULL);
	return (nResult >= 0 ? nResult : 0);
#endif
}

/**
 * @brief Decode Ansi string to normal text.
 * @param pBytes - array of bytes.
 * @param nNumBytes - number of bytes in the array.
 * @param pszString - decoded string.
 * @param nBufferSize - size of output buffer (in characters).
 * @return number of characters in resulting string including null-terminator.
 */
int AnsiDecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
{
#ifdef _UNICODE
	int nResult = MultiByteToWideChar(CP_ACP, 0, (const CHAR*)pBytes, nNumBytes, pszString, nBufferSize);
	return (nResult >= 0 ? nResult : 0);
#else
	if (nNumBytes < 0)
		nNumBytes = strlen((const CHAR*)pBytes) + 1;
	if (nNumBytes > nBufferSize)
		nNumBytes = nBufferSize;
	CopyMemory(pszString, pBytes, nNumBytes);
	return nNumBytes;
#endif
}

/**
 * @brief Get number of bytes representing one character.
 * @param pBytes - array of bytes.
 * @return number of bytes in one character.
 */
int GetUTF8CharSize(const BYTE* pBytes)
{
	BYTE bValue = *pBytes;
	if ((bValue & UTF8_1ST_OF_4) == UTF8_1ST_OF_4)
		return 4;
	else if ((bValue & UTF8_1ST_OF_3) == UTF8_1ST_OF_3)
		return 3;
	else if ((bValue & UTF8_1ST_OF_2) == UTF8_1ST_OF_2)
		return 2;
	else
		return 1;
}

/**
 * @brief Used to check if next character starts surrogate pair.
 * @param pBytes - array of bytes.
 * @return true if next character is high surrogate character.
 */
bool IsUnicodeLeadChar(const BYTE* pBytes)
{
	WCHAR chValue = *((const WCHAR*)pBytes);
	return (HIGH_SURROGATE_START <= chValue && chValue <= HIGH_SURROGATE_END);
}

/**
 * @brief Get number of bytes representing one character.
 * @param pBytes - array of bytes.
 * @return number of bytes in one character.
 */
int GetUnicodeCharSize(const BYTE* pBytes)
{
	WCHAR chValue1 = ((const WCHAR*)pBytes)[0];
	if (HIGH_SURROGATE_START <= chValue1 && chValue1 <= HIGH_SURROGATE_END)
	{
		WCHAR chValue2 = ((const WCHAR*)pBytes)[1];
		if (LOW_SURROGATE_START <= chValue2 && chValue2 <= LOW_SURROGATE_END)
			return (2 * sizeof(WCHAR));
	}
	return (sizeof(WCHAR));
}

/**
 * @brief Convert UTF-16 value from big-endian to low-endian format.
 * @param pBytes - array of bytes.
 * @param nNumBytes - buffer size in bytes.
 * @return number of bytes in one character.
 */
int UTF16BeToLeChar(BYTE* pBytes, int nNumBytes)
{
	if (nNumBytes < 1)
		return -1;
	WCHAR chValue1 = SWAP16(((WCHAR*)pBytes)[0]);
	if (LOW_SURROGATE_START <= chValue1 && chValue1 <= LOW_SURROGATE_END)
	{
		if (nNumBytes < 2)
			return -1;
		WCHAR chValue2 = SWAP16(((WCHAR*)pBytes)[1]);
		if (HIGH_SURROGATE_START <= chValue2 && chValue2 <= HIGH_SURROGATE_END)
		{
			((WCHAR*)pBytes)[0] = chValue2;
			((WCHAR*)pBytes)[1] = chValue1;
			return (2 * sizeof(WCHAR));
		}
	}
	*(WCHAR*)pBytes = chValue1;
	return (sizeof(WCHAR));
}

/**
 * @brief Convert UTF-16 value from big-endian to low-endian format.
 * @param pBytes - array of bytes.
 * @param nNumBytes - buffer size in bytes.
 * @return number of processed bytes.
 */
int UTF16BeToLeString(BYTE* pBytes, int nNumBytes)
{
	int nBytePos = 0;
	while (nBytePos < nNumBytes)
	{
		int nResult = UTF16BeToLeChar(pBytes + nBytePos, nNumBytes - nBytePos);
		if (nResult < 0)
			break;
		nBytePos += nResult;
	}
	return nBytePos;
}

/**
 * @param pchValue - pointer to the character normally stored in the string.
 * @param nCharSize - upon return keeps character size.
 * @return size of the character after UTF-8 encoding.
 */
int GetCharSizeInUTF8(const TCHAR* pchValue, int& nCharSize)
{
	DWORD dwUnicodeChar = GetUnicodeValue(pchValue, nCharSize);
	if (dwUnicodeChar <= ASCII_MAX)
		return 1;
	else if (dwUnicodeChar <= UTF8_2_MAX)
		return 2;
	else if (dwUnicodeChar <= UTF8_3_MAX)
		return 3;
	else
		return 4;
}

/**
 * @param pszString - input string that needs to be encoded in UTF-8 format.
 * @return size of the character encoded in UTF-8 format.
 */
int GetStringSizeInUTF8(PCTSTR pszString)
{
	_ASSERTE(pszString != NULL);
	int nUTF8StringSize = 0;
	while (*pszString)
	{
		int nCharSize;
		nUTF8StringSize += GetCharSizeInUTF8(pszString, nCharSize);
		pszString += nCharSize;
	}
	return nUTF8StringSize;
}

/**
 * @param eEncoding - text encoding.
 * @return pointer to corresponding decoder object.
 */
CBaseDecoder* CBaseDecoder::GetDecoder(TEXT_ENCODING eEncoding)
{
	switch (eEncoding)
	{
	case TXTENC_UTF8:
		return &CUTF8Decoder::GetInstance();
	case TXTENC_UTF16LE:
		return &CUTF16LeDecoder::GetInstance();
	case TXTENC_UTF16BE:
		return &CUTF16BeDecoder::GetInstance();
	case TXTENC_ANSI:
		return &CAnsiDecoder::GetInstance();
	default:
		return NULL;
	}
}

/**
 * @param pszEncoding - text encoding.
 * @return pointer to corresponding decoder object.
 */
CBaseDecoder* CBaseDecoder::GetDecoder(PCTSTR pszEncoding)
{
	if (_tcsicmp(pszEncoding, _T("UTF-8")) == 0)
		return &CUTF8Decoder::GetInstance();
	else if (_tcsicmp(pszEncoding, _T("UTF-16")) == 0)
		return &CUTF16LeDecoder::GetInstance();
	else
		return &CAnsiDecoder::GetInstance();
}

/**
 * @param pInputStream - input stream.
 */
void CDecInputStream::SetInputStream(CInputStream* pInputStream)
{
	m_pInputStream = pInputStream;
	m_nInputBufferPos = 0;
	m_nInputBufferLength = 0;
	m_bEndOfFile = false;
}

/**
 * @param nNumBytes - number of bytes to read.
 * @return number of bytes available in a buffer.
 */
int CDecInputStream::FillBuffer(int nNumBytes)
{
	_ASSERTE(m_pInputStream != NULL);
	int nBytesLeft = m_nInputBufferLength - m_nInputBufferPos;
	if (nBytesLeft < nNumBytes && ! m_bEndOfFile)
	{
		MoveMemory(m_arrInputBuffer, m_arrInputBuffer + m_nInputBufferPos, nBytesLeft);
		m_nInputBufferPos = 0;
		int nFreeSize = sizeof(m_arrInputBuffer) - nBytesLeft;
		int nBytesRead = m_pInputStream->ReadBytes(m_arrInputBuffer + nBytesLeft, nFreeSize);
		if (nBytesRead < 0)
			return -1;
		m_nInputBufferLength = nBytesLeft + nBytesRead;
		m_bEndOfFile = nBytesRead < nFreeSize;
		nBytesLeft = m_nInputBufferLength;
	}
	return nBytesLeft;
}

/**
 * @param arrChar - character data.
 * @return number of characters in one symbol.
 */
int CDecInputStream::ReadChar(TCHAR arrChar[2])
{
	// one character in UTF-16 encoding may require up to 4 bytes
	// one character in UTF-8 encoding may require up to 4 bytes
	// one character in ANSI encoding may require up to 2 bytes
	// make sure at least one character can be read from the buffer
	// (i.e. at least 4 bytes should be available)
	int nBytesLeft = FillBuffer(4);
	if (nBytesLeft <= 0)
		return nBytesLeft;
	int nNumBytesInChar, nCharSize;
	_ASSERTE(m_pDecoder != NULL);
	nNumBytesInChar = m_pDecoder->DecodeChar(m_arrInputBuffer + m_nInputBufferPos, nBytesLeft, arrChar, nCharSize);
	_ASSERTE(nNumBytesInChar >= 0);
	if (nCharSize > 0)
		m_nInputBufferPos += nNumBytesInChar;
	else
		++m_nInputBufferPos;
	return nCharSize;
}

/**
 * @param eEncoding - stream encoding.
 * @return true if stream encoding has been read from the stream.
 */
bool CDecInputStream::ReadPreamble(TEXT_ENCODING& eEncoding)
{
	// make sure at least 4 bytes are available in a buffer;
	// this should be enough for any preamble.
	int nBytesLeft = FillBuffer(4);
	if (nBytesLeft <= 0)
		return false;
	PBYTE pBytes = m_arrInputBuffer + m_nInputBufferPos;
	if (nBytesLeft >= sizeof(g_arrUTF16LEPreamble) &&
		memcmp(pBytes, g_arrUTF16LEPreamble, sizeof(g_arrUTF16LEPreamble)) == 0)
	{
		eEncoding = TXTENC_UTF16LE;
		m_nInputBufferPos += sizeof(g_arrUTF16LEPreamble);
		return true;
	}
	else if (nBytesLeft >= sizeof(g_arrUTF16BEPreamble) &&
		memcmp(pBytes, g_arrUTF16BEPreamble, sizeof(g_arrUTF16BEPreamble)) == 0)
	{
		eEncoding = TXTENC_UTF16BE;
		m_nInputBufferPos += sizeof(g_arrUTF16BEPreamble);
		return true;
	}
	else if (nBytesLeft >= sizeof(g_arrUTF8Preamble) &&
		memcmp(pBytes, g_arrUTF8Preamble, sizeof(g_arrUTF8Preamble)) == 0)
	{
		eEncoding = TXTENC_UTF8;
		m_nInputBufferPos += sizeof(g_arrUTF8Preamble);
		return true;
	}
	return false;
}

/**
 * @param arrChar - character data.
 * @return number of characters in one symbol.
 */
int CStrInputStream::ReadChar(TCHAR arrChar[2])
{
	_ASSERTE(m_pStrStream != NULL);
	int nLength = m_pStrStream->GetLength();
	if (m_nPosition >= nLength)
		return 0;
	arrChar[0] = ((PCTSTR)m_pStrStream)[m_nPosition++];
#ifdef _UNICODE
	if (IsUnicodeLeadChar((const BYTE*)arrChar))
#else
	if (IsDBCSLeadByte(*arrChar))
#endif
	{
		if (m_nPosition >= nLength)
		{
			_ASSERT(FALSE);
			return -1;
		}
		arrChar[1] = ((PCTSTR)m_pStrStream)[m_nPosition++];
		return 2;
	}
	return 1;
}

/**
 * @return true if byte order mark has been read from the stream.
 */
bool CCharInputStream::CheckEncoding(void)
{
	TEXT_ENCODING eEncoding;
	if (! ReadPreamble(eEncoding))
		return false;
	return SetDecoder(CBaseDecoder::GetDecoder(eEncoding));
}

/**
 * @param eDefaultEncoding - default encoding.
 * @return true if byte order mark has been read from the stream.
 */
bool CCharInputStream::CheckEncoding(TEXT_ENCODING eDefaultEncoding)
{
	TEXT_ENCODING eEncoding;
	bool bResult = ReadPreamble(eEncoding);
	if (! bResult)
		eEncoding = eDefaultEncoding;
	return SetDecoder(CBaseDecoder::GetDecoder(eEncoding));
}
