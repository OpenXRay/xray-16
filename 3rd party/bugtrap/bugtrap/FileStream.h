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

#pragma once

#include "Stream.h"

/// File stream class.
class CFileStream : public CStream
{
public:
	/// Initialize the object.
	explicit CFileStream(int nBufferSize = 1024);
	/// Initialize the object.
	CFileStream(PCTSTR pszFileName, DWORD dwCreationDisposition, DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE, DWORD dwShareMode = 0, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL, int nBufferSize = 1024);
	/// Initialize the object.
	CFileStream(HANDLE hFile, int nBufferSize = 1024);
	/// Destroy the object.
	virtual ~CFileStream(void);
	/// Get list of supported features.
	virtual unsigned GetFeatures(void) const;
	/// Open a file.
	bool Open(PCTSTR pszFileName, DWORD dwCreationDisposition, DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE, DWORD dwShareMode = 0, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
	/// Return true if stream is open.
	virtual bool IsOpen(void) const;
	/// Closes the stream.
	virtual void Close(void);
	/// Get stream name.
	virtual bool GetName(PTSTR pszName, int nNameSize) const;
	/// Get last IO error code.
	virtual long GetLastError(void) const;
	/// Return number of bytes in the stream.
	virtual int GetLength(void) const;
	/// Get current position.
	virtual int GetPosition(void) const;
	/// Set current position.
	virtual int SetPosition(int nOffset, int nMoveMethod);
	/// Return true if end of stream has been reached.
	virtual bool IsEndOfStream(void) const;
	/// Read one byte from the stream.
	virtual bool ReadByte(unsigned char& bValue);
	/// Read array of bytes from the stream.
	virtual int ReadBytes(unsigned char* arrBytes, int nCount);
	/// Move the end-of-file position for the specified stream to the current position of the stream pointer.
	virtual int SetLength(int nLength);
	/// Write one byte to the stream.
	virtual bool WriteByte(unsigned char bValue);
	/// Write array of bytes to the stream.
	virtual int WriteBytes(const unsigned char* arrBytes, int nCount);
	/// Set buffer size.
	void SetBufferSize(int nBufferSize);
	/// Get buffer size.
	int GetBufferSize(void) const;

private:
	/// Object can't be copied.
	CFileStream(const CFileStream& rStream);
	/// Object can't be copied.
	CFileStream& operator=(const CFileStream& rStream);
	/// Initialize member variables.
	void InitVars(int nBufferSize);
	/// Initialize input/output buffer.
	void InitBuffer(int nBufferSize);
	/// Reset file handle.
	void ResetFile(LONG lLastError);
	/// Clear buffer contents.
	void ResetBuffer(void);
	/// Synchronize buffer with current file position.
	bool SynchronizeBuffer(void);
	/// Flush buffer to the disk.
	bool FlushBuffer(void);

	/// Type of data in a buffer.
	enum BUFFER_TYPE
	{
		/// Buffer is empty.
		BT_EMPTY,
		/// Buffer keeps input data.
		BT_READ,
		/// Buffer keeps output data.
		BT_WRITE
	};

	/// File name.
	TCHAR m_szFileName[MAX_PATH];
	/// File handle.
	HANDLE m_hFile;
	/// Last error code.
	mutable LONG m_lLastError;
	/// End of file state.
	bool m_bEndOfFile;
	/// Input/output buffer.
	PBYTE m_pBuffer;
	/// Buffer size.
	int m_nBufferSize;
	/// Number of bytes stored in a buffer.
	int m_nBufferLength;
	/// Buffer read/write position.
	int m_nBufferPos;
	/// Type of data in a buffer.
	BUFFER_TYPE m_eBufferType;
};

/**
 * @param nBufferSize - buffer size.
 */
inline CFileStream::CFileStream(int nBufferSize)
{
	InitVars(nBufferSize);
}

/**
 * @param pszFileName - file name.
 * @param dwCreationDisposition - an action to take on files that exist and do not exist.
 * @param dwDesiredAccess - the access to the object, which can be read, write, or both.
 * @param dwShareMode - the sharing mode of an object, which can be read, write, both, or none.
 * @param dwFlagsAndAttributes - the file attributes and flags.
 * @param nBufferSize - buffer size.
 */
inline CFileStream::CFileStream(PCTSTR pszFileName, DWORD dwCreationDisposition, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes, int nBufferSize)
{
	InitVars(nBufferSize);
	Open(pszFileName, dwCreationDisposition, dwDesiredAccess, dwShareMode, dwFlagsAndAttributes);
}

/**
 * @param hFile - existing file handle.
 * @param nBufferSize - buffer size.
 */
inline CFileStream::CFileStream(HANDLE hFile, int nBufferSize)
{
	_ASSERTE(hFile != INVALID_HANDLE_VALUE);
	InitVars(nBufferSize);
	m_hFile = hFile;
}

inline CFileStream::~CFileStream(void)
{
	Close();
	delete[] m_pBuffer;
}

/**
 * @param nBufferSize - buffer size.
 */
inline void CFileStream::InitVars(int nBufferSize)
{
	ResetFile(NOERROR);
	InitBuffer(nBufferSize);
}

/**
 * @return bit mask of supported features.
 */
inline unsigned CFileStream::GetFeatures(void) const
{
	return (SF_ISOPEN | SF_CLOSE | SF_GETNAME | SF_GETLASTERROR | SF_GETLENGTH | SF_SETLENGTH | SF_GETPOSITION | SF_SETPOSITION);
}

/**
 * @return true if stream is open.
 */
inline bool CFileStream::IsOpen(void) const
{
	return (m_hFile != INVALID_HANDLE_VALUE);
}

/**
 * @return latest IO error code.
 */
inline long CFileStream::GetLastError(void) const
{
	return m_lLastError;
}

/**
 * @return size of buffer.
 */
inline int CFileStream::GetBufferSize(void) const
{
	return m_nBufferSize;
}

/**
 * @return true if end of stream has been reached.
 */
inline bool CFileStream::IsEndOfStream(void) const
{
	return (m_bEndOfFile && (m_eBufferType != BT_READ || m_nBufferPos == m_nBufferLength));
}

/**
 * @param bValue - output byte value.
 * @return true if byte has been successfully retrieved.
 */
inline bool CFileStream::ReadByte(unsigned char& bValue)
{
	return (ReadBytes(&bValue, sizeof(bValue)) == sizeof(bValue));
}

/**
 * @param bValue - input byte value.
 * @return true if byte has been successfully retrieved.
 */
inline bool CFileStream::WriteByte(unsigned char bValue)
{
	return (WriteBytes(&bValue, sizeof(bValue)) == sizeof(bValue));
}
