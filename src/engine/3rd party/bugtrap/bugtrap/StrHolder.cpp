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

#include "StdAfx.h"
#include "StrStream.h"
#include "StrHolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/// Empty string data.
CStrHolder::CStringData CStrHolder::m_sdEmptyData = { 0, 0, _T("") };

void CStrHolder::Release(void)
{
	if (m_pData != &m_sdEmptyData)
	{
		if (m_pData->m_nUsageCount == 1)
			delete[] (PBYTE)m_pData;
		else
			--m_pData->m_nUsageCount;
	}
}

/**
 * @param pszStrData - another string data.
 */
void CStrHolder::InitData(PCSTR pszStrData)
{
	if (pszStrData && *pszStrData)
	{
#ifdef _UNICODE
		int nSize = MultiByteToWideChar(CP_ACP, 0, pszStrData, -1, NULL, 0);
		m_pData = (CStringData*)new BYTE[sizeof(CStringData) + sizeof(WCHAR) * nSize];
		if (! m_pData)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		MultiByteToWideChar(CP_ACP, 0, pszStrData, -1, m_pData->m_szData, nSize);
		m_pData->m_nLength = nSize - 1;
		m_pData->m_nUsageCount = 1;
#else
		int nLength = strlen(pszStrData);
		int nSize = nLength + 1;
		m_pData = (CStringData*)new BYTE[sizeof(CStringData) + sizeof(CHAR) * nSize];
		if (! m_pData)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		strcpy_s(m_pData->m_szData, nSize, pszStrData);
		m_pData->m_nLength = nLength;
		m_pData->m_nUsageCount = 1;
#endif
	}
	else
		m_pData = &m_sdEmptyData;
}

/**
 * @param pszStrData - another string data.
 */
void CStrHolder::InitData(PCWSTR pszStrData)
{
	if (pszStrData && *pszStrData)
	{
#ifdef _UNICODE
		int nLength = wcslen(pszStrData);
		int nSize = nLength + 1;
		m_pData = (CStringData*)new BYTE[sizeof(CStringData) + sizeof(WCHAR) * nSize];
		if (! m_pData)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		wcscpy_s(m_pData->m_szData, nSize, pszStrData);
		m_pData->m_nLength = nLength;
		m_pData->m_nUsageCount = 1;
#else
		int nSize = WideCharToMultiByte(CP_ACP, 0, pszStrData, -1, NULL, 0, NULL, NULL);
		m_pData = (CStringData*)new BYTE[sizeof(CStringData) + sizeof(CHAR) * nSize];
		if (! m_pData)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		WideCharToMultiByte(CP_ACP, 0, pszStrData, -1, m_pData->m_szData, nSize, NULL, NULL);
		m_pData->m_nLength = nSize - 1;
		m_pData->m_nUsageCount = 1;
#endif
	}
	else
		m_pData = &m_sdEmptyData;
}

/**
 * @param rStrHolder - another string object.
 */
void CStrHolder::InitData(const CStrHolder& rStrHolder)
{
	m_pData = rStrHolder.m_pData;
	if (m_pData != &m_sdEmptyData)
		++m_pData->m_nUsageCount;
}

/**
 * @param rStrStream - another string object.
 */
void CStrHolder::InitData(const CStrStream& rStrStream)
{
	if (! rStrStream.IsEmpty())
	{
		int nLength = rStrStream.GetLength();
		int nSize = nLength + 1;
		m_pData = (CStringData*)new BYTE[sizeof(CStringData) + sizeof(WCHAR) * nSize];
		if (! m_pData)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		_tcscpy_s(m_pData->m_szData, nSize, (PCTSTR)rStrStream);
		m_pData->m_nLength = nLength;
		m_pData->m_nUsageCount = 1;
	}
	else
		m_pData = &m_sdEmptyData;
}

/**
 * @param rStrHolder - another string object.
 */
void CStrHolder::CopyData(const CStrHolder& rStrHolder)
{
	if (this != &rStrHolder)
	{
		Release();
		InitData(rStrHolder);
	}
}
