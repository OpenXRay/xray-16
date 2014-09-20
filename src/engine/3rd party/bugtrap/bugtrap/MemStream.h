/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: In-memory stream.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "Stream.h"

/// In-memory stream.
class CMemStream : public CStream
{
public:
	/// Initialize the object.
	CMemStream(void);
	/// Initialize the object.
	explicit CMemStream(int nSize);
	/// Destroy the object.
	virtual ~CMemStream(void);
	/// Copy constructor.
	CMemStream(const CMemStream& rMemStream);
	/// Assignment operator.
	CMemStream& operator=(const CMemStream& rMemStream);
	/// Get list of supported features.
	virtual unsigned GetFeatures(void) const;
	/// Get pointer to the buffer.
	const unsigned char* GetBuffer(void) const;
	/// Return true if end of stream has been reached.
	virtual bool IsEndOfStream(void) const;
	/// Get data length.
	virtual int GetLength(void) const;
	/// Set number of bytes in the stream.
	virtual int SetLength(int nLength);
	/// Get current position.
	virtual int GetPosition(void) const;
	/// Set current position.
	virtual int SetPosition(int nOffset, int nMoveMethod);
	/// Write data to the buffer.
	virtual bool WriteByte(unsigned char bValue);
	/// Write data to the buffer.
	virtual int WriteByte(unsigned char bValue, int nCount);
	/// Write data to the buffer.
	virtual int WriteBytes(const unsigned char* pBytes, int nCount);
	/// Write data to the buffer.
	int WriteStream(CMemStream* pMemStream);
	/// Clear data in the buffer.
	virtual void Close(void);
	/// Read one byte from the stream.
	virtual bool ReadByte(unsigned char& bValue);
	/// Read array of bytes from the stream.
	virtual int ReadBytes(unsigned char* arrBytes, int nNumBytes);

private:
	/// Initialize member variables.
	void InitBuffer(void);
	/// Initialize member variables.
	void InitBuffer(int nSize);
	/// Copy data from one buffer to another.
	void CopyData(const CMemStream& rMemStream);
	/// Ensures that the array has enough space.
	void EnsureSize(int nSize, bool bAdaptiveGrowth);
	/// Set current position.
	void SetPositionPriv(int nOffset, int nStartFrom);

	/// Data length.
	int m_nLength;
	/// Data position.
	int m_nPosition;
	/// Size of allocated buffer.
	int m_nSize;
	/// Pointer to the buffer.
	unsigned char* m_pBuffer;
};

inline CMemStream::CMemStream(void)
{
	InitBuffer();
}

inline CMemStream::~CMemStream(void)
{
	delete[] m_pBuffer;
}

/**
 * @return bit mask of supported features.
 */
inline unsigned CMemStream::GetFeatures(void) const
{
	return (SF_CLOSE | SF_GETLENGTH | SF_SETLENGTH | SF_GETPOSITION | SF_SETPOSITION);
}

/**
 * @return pointer to the buffer.
 */
inline const unsigned char* CMemStream::GetBuffer(void) const
{
	return m_pBuffer;
}

/**
 * @return length of the data.
 */
inline int CMemStream::GetLength(void) const
{
	return m_nLength;
}

/**
 * @return true if end of stream has been reached.
 */
inline bool CMemStream::IsEndOfStream(void) const
{
	return (m_nPosition >= m_nLength);
}

/**
 * @return current stream position.
 */
inline int CMemStream::GetPosition(void) const
{
	return m_nPosition;
}

inline void CMemStream::Close(void)
{
	m_nLength = m_nPosition = 0;
}

/**
 * @param pMemStream - another binary buffer.
 * @return true if data has been written.
 */
inline int CMemStream::WriteStream(CMemStream* pMemStream)
{
	return WriteBytes(pMemStream->m_pBuffer + pMemStream->m_nPosition, pMemStream->m_nLength - pMemStream->m_nPosition);
}

/**
 * @param nSize - initial buffer size.
 */
inline CMemStream::CMemStream(int nSize)
{
	InitBuffer(nSize);
}
