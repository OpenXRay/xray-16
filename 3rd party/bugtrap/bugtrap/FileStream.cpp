/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: File stream class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "FileStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CHECK_FILE_HANDLE(eResult) \
	_ASSERTE(m_hFile != INVALID_HANDLE_VALUE); \
	if (m_hFile == INVALID_HANDLE_VALUE) \
	{ \
		m_lLastError = ERROR_INVALID_HANDLE_STATE; \
		return eResult; \
	}

#define CHECK_LAST_ERROR(eResult) \
	m_lLastError = GetLastError(); \
	if (m_lLastError != NOERROR) \
		return eResult;

/**
 * @param lLastError - last error code.
 */
void CFileStream::ResetFile(LONG lLastError)
{
	m_hFile = INVALID_HANDLE_VALUE;
	*m_szFileName = _T('\0');
	m_bEndOfFile = false;
	m_lLastError = lLastError;
}

/**
 * @param nBufferSize - buffer size.
 */
void CFileStream::InitBuffer(int nBufferSize)
{
	ResetBuffer();
	m_nBufferSize = nBufferSize;
	if (nBufferSize > 0)
	{
		m_pBuffer = new BYTE[nBufferSize];
		if (m_pBuffer == NULL)
		{
			m_nBufferSize = 0;
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		}
	}
	else
		m_pBuffer = NULL;
}

/**
 * @param pszFileName - file name.
 * @param dwCreationDisposition - an action to take on files that exist and do not exist.
 * @param dwDesiredAccess - the access to the object, which can be read, write, or both.
 * @param dwShareMode - the sharing mode of an object, which can be read, write, both, or none.
 * @param dwFlagsAndAttributes - the file attributes and flags.
 */
bool CFileStream::Open(PCTSTR pszFileName, DWORD dwCreationDisposition, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes)
{
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		m_lLastError = ERROR_INVALID_HANDLE_STATE;
		return false;
	}
	m_hFile = CreateFile(pszFileName, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	m_lLastError = GetLastError();
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		m_bEndOfFile = false;
		_tcscpy_s(m_szFileName, countof(m_szFileName), pszFileName);
		return true;
	}
	return false;
}

void CFileStream::Close(void)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		FlushBuffer();
		CloseHandle(m_hFile);
		ResetFile(GetLastError());
	}
}

/**
 * @param pszName - stream name buffer.
 * @param nNameSize - size of stream name buffer.
 * @return true if name was retrieved.
 */
bool CFileStream::GetName(PTSTR pszName, int nNameSize) const
{
	_tcscpy_s(pszName, nNameSize, m_szFileName);
	m_lLastError = NOERROR;
	return true;
}

/**
 * @return number of bytes in the stream.
 */
int CFileStream::GetLength(void) const
{
	CHECK_FILE_HANDLE(-1);
	DWORD dwFileSize = GetFileSize(m_hFile, NULL);
	CHECK_LAST_ERROR(-1);
	_ASSERTE((int)dwFileSize >= 0); // check 2GB limit
	return (int)dwFileSize;
}

/**
 * @param nOffset - offset from start point.
 * @param nMoveMethod - start point.
 * @return new position value.
 */
int CFileStream::SetPosition(int nOffset, int nMoveMethod)
{
	CHECK_FILE_HANDLE(-1);
	m_bEndOfFile = false;
	int nDeltaPos = 0;
	if (m_eBufferType == BT_READ)
	{
		if (nMoveMethod == FILE_CURRENT)
			nDeltaPos = m_nBufferLength - m_nBufferPos;
		ResetBuffer();
	}
	else
	{
		if (! FlushBuffer())
			return -1;
	}
	DWORD dwFilePos = SetFilePointer(m_hFile, nOffset - nDeltaPos, NULL, nMoveMethod);
	CHECK_LAST_ERROR(-1);
	_ASSERTE((int)dwFilePos >= 0); // check 2GB limit
	return (int)dwFilePos;
}

/**
 * @return current stream position.
 */
int CFileStream::GetPosition(void) const
{
	CHECK_FILE_HANDLE(-1);
	DWORD dwFilePos = SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
	CHECK_LAST_ERROR(-1);
	_ASSERTE((int)dwFilePos >= 0); // check 2GB limit
	int nDeltaPos = m_nBufferLength - m_nBufferPos;
	return ((int)dwFilePos - nDeltaPos);
}

/**
 * @param nLength - new stream length.
 * @return new file length.
 */
int CFileStream::SetLength(int nLength)
{
	CHECK_FILE_HANDLE(-1);
	int nDeltaPos;
	if (m_eBufferType == BT_READ)
	{
		nDeltaPos = m_nBufferLength - m_nBufferPos;
		ResetBuffer();
	}
	else
	{
		nDeltaPos = 0;
		if (! FlushBuffer())
			return -1;
	}
	m_bEndOfFile = false;
	DWORD dwFilePos = SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
	CHECK_LAST_ERROR(-1);
	dwFilePos -= nDeltaPos;
	_ASSERTE((int)dwFilePos >= 0); // check 2GB limit
	SetFilePointer(m_hFile, nLength, NULL, FILE_BEGIN);
	CHECK_LAST_ERROR(-1);
	SetEndOfFile(m_hFile);
	CHECK_LAST_ERROR(-1);
	if ((int)dwFilePos > nLength)
		dwFilePos = nLength;
	SetFilePointer(m_hFile, dwFilePos, NULL, FILE_BEGIN);
	CHECK_LAST_ERROR(-1);
	return nLength;
}

/**
 * @param arrBytes - array of bytes.
 * @param nCount - size of the array.
 * @return number of retrieved bytes.
 */
int CFileStream::ReadBytes(unsigned char* arrBytes, int nCount)
{
	CHECK_FILE_HANDLE(-1);
	if (m_pBuffer != NULL)
	{
		if (! FlushBuffer())
			return -1;
		int nTotalLength = 0;
		for (;;)
		{
			int nNumBytes = m_nBufferLength - m_nBufferPos;
			if (nNumBytes > nCount)
				nNumBytes = nCount;
			if (nNumBytes > 0)
			{
				CopyMemory(arrBytes, m_pBuffer + m_nBufferPos, nNumBytes);
				arrBytes += nNumBytes;
				nTotalLength += nNumBytes;
				nCount -= nNumBytes;
				m_nBufferPos += nNumBytes;
			}
			if (nCount == 0 || m_bEndOfFile)
				break;
			_ASSERTE(m_nBufferPos == m_nBufferLength);
			DWORD dwNumRead = 0;
			ReadFile(m_hFile, m_pBuffer, m_nBufferSize, &dwNumRead, NULL);
			m_lLastError = GetLastError();
			m_nBufferPos = 0;
			m_nBufferLength = (int)dwNumRead;
			m_eBufferType = m_nBufferLength > 0 ? BT_READ : BT_EMPTY;
			m_bEndOfFile = m_nBufferLength < m_nBufferSize;
			if (m_lLastError != NOERROR)
				break;
		}
		return nTotalLength;
	}
	else
	{
		DWORD dwNumRead = 0;
		ReadFile(m_hFile, arrBytes, nCount, &dwNumRead, NULL);
		m_lLastError = GetLastError();
		return (int)dwNumRead;
	}
}

/**
 * @param arrBytes - array of bytes.
 * @param nCount - size of the array.
 * @return number of written bytes.
 */
int CFileStream::WriteBytes(const unsigned char* arrBytes, int nCount)
{
	CHECK_FILE_HANDLE(-1);
	if (m_pBuffer != NULL)
	{
		if (! SynchronizeBuffer())
			return -1;
		int nTotalLength = 0;
		for (;;)
		{
			int nNumBytes = m_nBufferSize - m_nBufferLength;
			if (nNumBytes > nCount)
				nNumBytes = nCount;
			if (nNumBytes > 0)
			{
				m_eBufferType = BT_WRITE;
				CopyMemory(m_pBuffer + m_nBufferLength, arrBytes, nNumBytes);
				arrBytes += nNumBytes;
				nTotalLength += nNumBytes;
				nCount -= nNumBytes;
				m_nBufferLength += nNumBytes;
			}
			if (nCount == 0)
				break;
			_ASSERTE(m_nBufferLength == m_nBufferSize);
			DWORD dwNumWritten = 0;
			WriteFile(m_hFile, m_pBuffer, m_nBufferSize, &dwNumWritten, NULL);
			m_lLastError = GetLastError();
			ResetBuffer();
			if (m_lLastError != NOERROR)
				break;
		}
		return nTotalLength;
	}
	else
	{
		DWORD dwNumWritten = 0;
		WriteFile(m_hFile, arrBytes, nCount, &dwNumWritten, NULL);
		m_lLastError = GetLastError();
		return (int)dwNumWritten;
	}
}

/**
 * @param nBufferSize - buffer size.
 */
void CFileStream::SetBufferSize(int nBufferSize)
{
	if (m_nBufferSize != nBufferSize)
	{
		if (! FlushBuffer())
			return;
		delete[] m_pBuffer;
		InitBuffer(nBufferSize);
	}
}

void CFileStream::ResetBuffer(void)
{
	m_eBufferType = BT_EMPTY;
	m_nBufferPos = 0;
	m_nBufferLength = 0;
}

/**
 * @return true if file pointer has been synchronized.
 */
bool CFileStream::SynchronizeBuffer(void)
{
	if (m_eBufferType == BT_READ)
	{
		_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
		if (m_nBufferPos < m_nBufferLength)
		{
			SetFilePointer(m_hFile, m_nBufferPos - m_nBufferLength, NULL, FILE_CURRENT);
			CHECK_LAST_ERROR(false);
		}
		ResetBuffer();
	}
	return true;
}

/**
 * @return true if buffer has been be flushed.
 */
bool CFileStream::FlushBuffer(void)
{
	if (m_eBufferType == BT_WRITE)
	{
		_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
		if (m_nBufferLength > 0)
		{
			DWORD dwNumWritten = 0;
			WriteFile(m_hFile, m_pBuffer, m_nBufferLength, &dwNumWritten, NULL);
			CHECK_LAST_ERROR(false);
		}
		ResetBuffer();
	}
	return true;
}
