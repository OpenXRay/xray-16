/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Dynamic string holder.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

class CStrStream;

/// Dynamic string holder.
class CStrHolder
{
public:
	/// Initialize the object.
	CStrHolder(void);
	/// Destroy the object.
	~CStrHolder(void);
	/// Makes a copy of string data.
	CStrHolder(PCSTR pszStrData);
	/// Makes a copy of string data.
	CStrHolder(PCWSTR pszStrData);
	/// Makes a copy of string data.
	CStrHolder(const CStrHolder& rStrHolder);
	/// Makes a copy of string data.
	CStrHolder(const CStrStream& rStrStream);
	/// Makes a copy of string data.
	CStrHolder& operator=(PCSTR pszStrData);
	/// Makes a copy of string data.
	CStrHolder& operator=(PCWSTR pszStrData);
	/// Makes a copy of string data.
	CStrHolder& operator=(const CStrHolder& rStrHolder);
	/// Makes a copy of string data.
	CStrHolder& operator=(const CStrStream& rStrStream);
	/// Get string data.
	operator PCTSTR(void) const;
	/// Free string data.
	void Free(void);
	/// Get string length.
	int GetLength(void) const;
	/// Return true if string is empty.
	BOOL IsEmpty(void) const;
	/// Get string character.
	TCHAR GetAt(int nPosition) const;
	/// Get string character.
	TCHAR operator[](int nPosition) const;
	/// Object comparison.
	friend bool operator==(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);
	/// Object comparison.
	friend bool operator!=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);
	/// Object comparison.
	friend bool operator<(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);
	/// Object comparison.
	friend bool operator<=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);
	/// Object comparison.
	friend bool operator>(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);
	/// Object comparison.
	friend bool operator>=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2);

private:
	/// Release string data.
	void Release(void);
	/// Initialize sting data.
	void InitData(PCSTR pszStrData);
	/// Initialize sting data.
	void InitData(PCWSTR pszStrData);
	/// Initialize sting data.
	void InitData(const CStrHolder& rStrHolder);
	/// Initialize sting data.
	void InitData(const CStrStream& rStrStream);
	/// Copy string into existing holder.
	void CopyData(PCSTR pszStrData);
	/// Copy string into existing holder.
	void CopyData(PCWSTR pszStrData);
	/// Copy string into existing holder.
	void CopyData(const CStrHolder& rStrHolder);
	/// Copy string into existing holder.
	void CopyData(const CStrStream& rStrStream);

	/// Shared string data.
	struct CStringData
	{
		/// Usage counter.
		int m_nUsageCount;
		/// String length.
		int m_nLength;
#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
		/// Pointer to the string.
		TCHAR m_szData[0];
#pragma warning(pop)
	};

	/// Pointer to string data.
	CStringData* m_pData;
	/// Empty string data.
	static CStringData m_sdEmptyData;
};

inline CStrHolder::CStrHolder(void)
{
	m_pData = &m_sdEmptyData;
}

inline CStrHolder::~CStrHolder(void)
{
	Release();
}

/**
 * @return string data.
 */
inline CStrHolder::operator PCTSTR(void) const
{
	_ASSERTE(m_pData != NULL);
	return m_pData->m_szData;
}

/**
 * @return string length.
 */
inline int CStrHolder::GetLength(void) const
{
	_ASSERTE(m_pData != NULL);
	return m_pData->m_nLength;
}

/**
 * @return true if string is empty.
 */
inline BOOL CStrHolder::IsEmpty(void) const
{
	_ASSERTE(m_pData != NULL);
	return (m_pData->m_nLength == 0);
}

/**
 * @param pszStrData - another string data.
 */
inline CStrHolder::CStrHolder(PCSTR pszStrData)
{
	InitData(pszStrData);
}

/**
 * @param pszStrData - another string data.
 */
inline CStrHolder::CStrHolder(PCWSTR pszStrData)
{
	InitData(pszStrData);
}

/**
 * @param rStrHolder - another string data.
 */
inline CStrHolder::CStrHolder(const CStrHolder& rStrHolder)
{
	InitData(rStrHolder);
}

/**
 * @param rStrStream - another string data.
 */
inline CStrHolder::CStrHolder(const CStrStream& rStrStream)
{
	InitData(rStrStream);
}

/**
 * @param pszStrData - another string data.
 * @return reference to itself.
 */
inline CStrHolder& CStrHolder::operator=(PCSTR pszStrData)
{
	CopyData(pszStrData);
	return *this;
}

/**
 * @param pszStrData - another string data.
 * @return reference to itself.
 */
inline CStrHolder& CStrHolder::operator=(PCWSTR pszStrData)
{
	CopyData(pszStrData);
	return *this;
}

/**
 * @param rStrHolder - another string data.
 * @return reference to itself.
 */
inline CStrHolder& CStrHolder::operator=(const CStrHolder& rStrHolder)
{
	if (this != &rStrHolder)
		CopyData(rStrHolder);
	return *this;
}

/**
 * @param rStrStream - another string data.
 * @return reference to itself.
 */
inline CStrHolder& CStrHolder::operator=(const CStrStream& rStrStream)
{
	CopyData(rStrStream);
	return *this;
}

inline void CStrHolder::Free(void)
{
	Release();
	m_pData = &m_sdEmptyData;
}

/**
 * @param pszStrData - another string data.
 */
inline void CStrHolder::CopyData(PCSTR pszStrData)
{
	Release();
	InitData(pszStrData);
}

/**
 * @param pszStrData - another string data.
 */
inline void CStrHolder::CopyData(PCWSTR pszStrData)
{
	Release();
	InitData(pszStrData);
}

/**
 * @param rStrStream - another string object.
 */
inline void CStrHolder::CopyData(const CStrStream& rStrStream)
{
	Release();
	InitData(rStrStream);
}

/**
 * @param nPosition - character position within the string.
 * @return character value.
 */
inline TCHAR CStrHolder::GetAt(int nPosition) const
{
	_ASSERTE(nPosition < m_pData->m_nLength);
	if (nPosition >= m_pData->m_nLength)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
	return m_pData->m_szData[nPosition];
}

/**
 * @param nPosition - character position within the string.
 * @return character value.
 */
inline TCHAR CStrHolder::operator[](int nPosition) const
{
	_ASSERTE(nPosition < m_pData->m_nLength);
	if (nPosition >= m_pData->m_nLength)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
	return m_pData->m_szData[nPosition];
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator==(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) == 0);
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator!=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) != 0);
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator<(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) < 0);
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator<=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) <= 0);
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator>(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) > 0);
}

/**
 * @param rStrHolder1 - 1st object.
 * @param rStrHolder2 - 2nd object.
 * @return comparison result.
 */
inline bool operator>=(const CStrHolder& rStrHolder1, const CStrHolder& rStrHolder2)
{
	return (_tcscmp(rStrHolder1.m_pData->m_szData, rStrHolder2.m_pData->m_szData) >= 0);
}
