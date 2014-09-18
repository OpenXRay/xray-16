/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: String (in-memory) stream class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

class CStrHolder;

/// Dynamic string builder.
class CStrStream
{
public:
	/// Initialize the object.
	CStrStream(void);
	/// Initialize the object.
	explicit CStrStream(int nSize);
	/// Makes a copy of string data.
	CStrStream(PCSTR pszStrData);
	/// Makes a copy of string data.
	CStrStream(PCWSTR pszStrData);
	/// Makes a copy of string data.
	CStrStream(const CStrHolder& rStrHolder);
	/// Makes a copy of string data.
	CStrStream(const CStrStream& rStrStream);
	/// Destroy the object.
	~CStrStream(void);
	/// Get string data.
	operator PCTSTR(void) const;
	/// Get string length.
	int GetLength(void) const;
	/// Return true if string is empty.
	bool IsEmpty(void) const;
	/// Get string character.
	TCHAR operator[](int nPosition) const;
	/// Get/set string character.
	TCHAR& operator[](int nPosition);
	/// Get string character.
	TCHAR GetAt(int nPosition) const;
	/// Set string character.
	void SetAt(int nPosition, TCHAR chData);
	/// Extract substring from the stream.
	void Substring(CStrHolder& rStrHolder, int nPosition, int nLength = -1) const;
	/// Extract substring from the stream.
	void Substring(CStrStream& rStrStream, int nPosition, int nLength = -1) const;
	/// Insert single character to the stream.
	void Insert(TCHAR chData, int nPosition);
	/// Insert substring to the stream.
	void Insert(PCTSTR pszSubstring, int nPosition);
	/// Insert substring to the stream.
	void Insert(const CStrHolder& rStrHolder, int nPosition);
	/// Insert substring to the stream.
	void Insert(const CStrStream& rStrStream, int nPosition);
	/// Delete substring from the string.
	void Delete(int nPosition, int nLength = -1);
	/// Trim a string.
	void Trim(void);
	/// Makes a copy of string data.
	CStrStream& operator=(CHAR chData);
	/// Makes a copy of string data.
	CStrStream& operator=(WCHAR chData);
	/// Makes a copy of string data.
	CStrStream& operator=(PCSTR pszStrData);
	/// Makes a copy of string data.
	CStrStream& operator=(PCWSTR pszStrData);
	/// Makes a copy of string data.
	CStrStream& operator=(const CStrHolder& rStrHolder);
	/// Makes a copy of string data.
	CStrStream& operator=(const CStrStream& rStrStream);
	/// Appends data to stream.
	CStrStream& operator<<(CHAR chData);
	/// Appends data to stream.
	CStrStream& operator<<(WCHAR chData);
	/// Appends data to stream.
	CStrStream& operator<<(PCSTR pszStrData);
	/// Appends data to stream.
	CStrStream& operator<<(PCWSTR pszStrData);
	/// Appends data to stream.
	CStrStream& operator<<(const CStrHolder& rStrHolder);
	/// Appends data to stream.
	CStrStream& operator<<(const CStrStream& rStrStream);
	/// Detach string data from the object.
	PTSTR Detach(void);
	/// Reset stream contents.
	void Reset(void);
	/// Free stream object memory.
	void Free(void);
	/// Object comparison.
	friend bool operator==(const CStrStream& rStrStream1, const CStrStream& rStrStream2);
	/// Object comparison.
	friend bool operator!=(const CStrStream& rStrStream1, const CStrStream& rStrStream2);
	/// Object comparison.
	friend bool operator<(const CStrStream& rStrStream1, const CStrStream& rStrStream2);
	/// Object comparison.
	friend bool operator<=(const CStrStream& rStrStream1, const CStrStream& rStrStream2);
	/// Object comparison.
	friend bool operator>(const CStrStream& rStrStream1, const CStrStream& rStrStream2);
	/// Object comparison.
	friend bool operator>=(const CStrStream& rStrStream1, const CStrStream& rStrStream2);

private:
	/// Clear internal data.
	void InitBuffer(void);
	/// Initialize new string.
	void InitBuffer(int nSize);
	/// Reallocates string buffer if necessary.
	void EnsureSize(int nSize, bool bAdaptiveGrowth);
	/// Initialize new string.
	void InitData(PCTSTR pszStrData, int nLength);
	/// Copy string into the existing buffer.
	void CopyData(PCTSTR pszStrData, int nLength);
	/// Append string to the existing data.
	void AppendData(PCTSTR pszStrData, int nLength);
#ifdef _UNICODE
	/// Initialize new string.
	void InitData(PCSTR pszStrData);
	/// Copy string into the existing buffer.
	void CopyData(PCSTR pszStrData);
	/// Append string to the existing data.
	void AppendData(PCSTR pszStrData);
#else
	/// Initialize new string.
	void InitData(PCWSTR pszStrData);
	/// Copy string into existing buffer.
	void CopyData(PCWSTR pszStrData);
	/// Append string to existing data.
	void AppendData(PCWSTR pszStrData);
#endif
	/// Insert substring to the stream.
	void Insert(PCTSTR pszSubstring, int nPosition, int nLength);
	/// Validate string position.
	void ValidateIndex(int nPosition) const;
	/// Validate string position and length.
	void ValidateIndex(int nPosition, int& nLength) const;

	/// Stream data data.
	PTSTR m_pszData;
	/// String length.
	int m_nLength;
	/// Buffer size.
	int m_nSize;
};

inline CStrStream::CStrStream(void)
{
	InitBuffer();
}

/**
 * @param nSize - initial buffer size.
 */
inline CStrStream::CStrStream(int nSize)
{
	InitBuffer(nSize);
}

inline CStrStream::~CStrStream(void)
{
	delete[] m_pszData;
}

/**
 * @return stream length.
 */
inline int CStrStream::GetLength(void) const
{
	return m_nLength;
}

/**
 * @return true if string is empty.
 */
inline bool CStrStream::IsEmpty(void) const
{
	return (m_nLength == 0);
}

/**
 * @return pointer to stream buffer.
 */
inline CStrStream::operator PCTSTR(void) const
{
	return (m_pszData ? m_pszData : _T(""));
}

/**
 * @param pszStrData - string data to copy.
 */
inline CStrStream::CStrStream(PCSTR pszStrData)
{
#ifdef _UNICODE
	InitData(pszStrData);
#else
	InitData(pszStrData, strlen(pszStrData));
#endif
}

/**
 * @param pszStrData - string data to copy.
 */
inline CStrStream::CStrStream(PCWSTR pszStrData)
{
#ifdef _UNICODE
	InitData(pszStrData, wcslen(pszStrData));
#else
	InitData(pszStrData);
#endif
}

/**
 * @param rStrStream - another stream object.
 */
inline CStrStream::CStrStream(const CStrStream& rStrStream)
{
	InitData(rStrStream.m_pszData, rStrStream.m_nLength);
}

/**
 * @param chData - character data to copy.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(CHAR chData)
{
#ifdef _UNICODE
	WCHAR chTemp;
	MultiByteToWideChar(CP_ACP, 0, &chData, 1, &chTemp, 1);
	CopyData(&chTemp, 1);
#else
	CopyData(&chData, 1);
#endif
	return *this;
}

/**
 * @param chData - character data to copy.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(WCHAR chData)
{
#ifdef _UNICODE
	CopyData(&chData, 1);
#else
	CHAR chTemp;
	WideCharToMultiByte(CP_ACP, 0, &chData, 1, &chTemp, 1, NULL, NULL);
	CopyData(&chTemp, 1);
#endif
	return *this;
}
/**
 * @param pszStrData - string data to copy.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(PCSTR pszStrData)
{
#ifdef _UNICODE
	CopyData(pszStrData);
#else
	CopyData(pszStrData, strlen(pszStrData));
#endif
	return *this;
}

/**
 * @param pszStrData - string data to copy.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(PCWSTR pszStrData)
{
#ifdef _UNICODE
	CopyData(pszStrData, wcslen(pszStrData));
#else
	CopyData(pszStrData);
#endif
	return *this;
}

/**
 * @param rStrStream - another stream object.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(const CStrStream& rStrStream)
{
	if (this != &rStrStream)
		CopyData(rStrStream.m_pszData, rStrStream.m_nLength);
	return *this;
}

/**
 * @param chData - character data to append.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(CHAR chData)
{
#ifdef _UNICODE
	WCHAR chTemp;
	MultiByteToWideChar(CP_ACP, 0, &chData, 1, &chTemp, 1);
	AppendData(&chTemp, 1);
#else
	AppendData(&chData, 1);
#endif
	return *this;
}

/**
 * @param chData - character data to append.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(WCHAR chData)
{
#ifdef _UNICODE
	AppendData(&chData, 1);
#else
	CHAR chTemp;
	WideCharToMultiByte(CP_ACP, 0, &chData, 1, &chTemp, 1, NULL, NULL);
	AppendData(&chTemp, 1);
#endif
	return *this;
}

/**
 * @param pszStrData - string data to append.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(PCSTR pszStrData)
{
#ifdef _UNICODE
	AppendData(pszStrData);
#else
	AppendData(pszStrData, strlen(pszStrData));
#endif
	return *this;
}

/**
 * @param pszStrData - string data to append.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(PCWSTR pszStrData)
{
#ifdef _UNICODE
	AppendData(pszStrData, wcslen(pszStrData));
#else
	AppendData(pszStrData);
#endif
	return *this;
}

/**
 * @param rStrStream - another stream object.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(const CStrStream& rStrStream)
{
	AppendData(rStrStream.m_pszData, rStrStream.m_nLength);
	return *this;
}

/**
 * @param nPosition - character position within the string.
 * @return character value.
 */
inline TCHAR CStrStream::GetAt(int nPosition) const
{
	ValidateIndex(nPosition);
	return m_pszData[nPosition];
}

/**
 * @param nPosition - character position within the string.
 * @return character value.
 */
inline TCHAR CStrStream::operator[](int nPosition) const
{
	ValidateIndex(nPosition);
	return m_pszData[nPosition];
}

/**
 * @param nPosition - character position within the string.
 * @param chData - character value.
 */
inline void CStrStream::SetAt(int nPosition, TCHAR chData)
{
	ValidateIndex(nPosition);
	m_pszData[nPosition] = chData;
}

/**
 * @param nPosition - character position within the string.
 * @return reference to character value.
 */
inline TCHAR& CStrStream::operator[](int nPosition)
{
	ValidateIndex(nPosition);
	return m_pszData[nPosition];
}

/**
 * @param rStrStream - string buffer that receives the data.
 * @param nPosition - start position.
 * @param nLength - length of the extracted string.
 */
inline void CStrStream::Substring(CStrStream& rStrStream, int nPosition, int nLength) const
{
	ValidateIndex(nPosition, nLength);
	rStrStream.CopyData(m_pszData + nPosition, nLength);
}

/**
 * @param nPosition - start position.
 */
inline void CStrStream::ValidateIndex(int nPosition) const
{
	_ASSERTE(nPosition >= 0 && nPosition < m_nLength);
	if (nPosition < 0 || nPosition >= m_nLength)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
}

/**
 * @param chData - character value.
 * @param nPosition - start position.
 */
inline void CStrStream::Insert(TCHAR chData, int nPosition)
{
	Insert(&chData, nPosition, 1);
}

/**
 * @param pszSubstring - inserted string buffer.
 * @param nPosition - start position.
 */
inline void CStrStream::Insert(PCTSTR pszSubstring, int nPosition)
{
	Insert(pszSubstring, nPosition, _tcslen(pszSubstring));
}

/**
 * @param rStrStream - inserted string buffer.
 * @param nPosition - start position.
 */
inline void CStrStream::Insert(const CStrStream& rStrStream, int nPosition)
{
	Insert(rStrStream.m_pszData, nPosition, rStrStream.m_nLength);
}

#include "StrHolder.h"

/**
 * @param rStrHolder - string data to copy.
 */
inline CStrStream::CStrStream(const CStrHolder& rStrHolder)
{
	InitData((PCTSTR)rStrHolder, rStrHolder.GetLength());
}

/**
 * @param rStrHolder - string data to copy.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator=(const CStrHolder& rStrHolder)
{
	CopyData((PCTSTR)rStrHolder, rStrHolder.GetLength());
	return *this;
}

/**
 * @param rStrHolder - string data to append.
 * @return reference to itself.
 */
inline CStrStream& CStrStream::operator<<(const CStrHolder& rStrHolder)
{
	AppendData((PCTSTR)rStrHolder, rStrHolder.GetLength());
	return *this;
}

/**
 * @param rStrHolder - inserted string buffer.
 * @param nPosition - start position.
 */
inline void CStrStream::Insert(const CStrHolder& rStrHolder, int nPosition)
{
	Insert((PCTSTR)rStrHolder, nPosition, rStrHolder.GetLength());
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator==(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) == 0);
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator!=(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) != 0);
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator<(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) < 0);
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator<=(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) <= 0);
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator>(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) > 0);
}

/**
 * @param rStrStream1 - 1st object.
 * @param rStrStream2 - 2nd object.
 * @return comparison result.
 */
inline bool operator>=(const CStrStream& rStrStream1, const CStrStream& rStrStream2)
{
	return (_tcscmp(rStrStream1.m_pszData, rStrStream2.m_pszData) >= 0);
}
