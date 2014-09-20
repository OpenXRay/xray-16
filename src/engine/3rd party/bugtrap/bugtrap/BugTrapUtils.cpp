/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: BugTrap utilities.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "BugTrapUtils.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUtils Internal BugTrap Utilities
 * @{
 */

// Windows processing.

/**
 * @brief Center window according to the desktop or another window.
 * @param hwnd - window handle to center.
 * @param hwndCenter - relative window handle or zero for desktop.
 */
void CenterWindow(HWND hwnd, HWND hwndCenter)
{
	// Determine owner window to center against.
	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	if (hwndCenter == NULL)
	{
		if(dwStyle & WS_CHILD)
			hwndCenter = GetParent(hwnd);
		else
			hwndCenter = GetWindow(hwnd, GW_OWNER);
	}

	// Get coordinates of the window relative to its parent.
	RECT rcDlg;
	GetWindowRect(hwnd, &rcDlg);
	RECT rcArea;
	RECT rcCenter;
	HWND hwndParent;
	if ((dwStyle & WS_CHILD) == 0)
	{
		// Don't center against invisible or minimized windows.
		if (hwndCenter != NULL)
		{
			DWORD dwStyleCenter = GetWindowLong(hwndCenter, GWL_STYLE);
			if (! (dwStyleCenter & WS_VISIBLE) || (dwStyleCenter & WS_MINIMIZE))
				hwndCenter = NULL;
		}

		// Center within screen coordinates.
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
		if(hwndCenter == NULL)
			rcCenter = rcArea;
		else
			GetWindowRect(hwndCenter, &rcCenter);
	}
	else
	{
		// Center within parent client coordinates.
		hwndParent = GetParent(hwnd);
		GetClientRect(hwndParent, &rcArea);
		GetClientRect(hwndCenter, &rcCenter);
		MapWindowPoints(hwndCenter, hwndParent, (POINT*)&rcCenter, 2);
	}

	int nDlgWidth = rcDlg.right - rcDlg.left;
	int nDlgHeight = rcDlg.bottom - rcDlg.top;

	// Find dialog's upper left based on rcCenter.
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - nDlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - nDlgHeight / 2;

	// If the dialog is outside the screen, move it inside.
	if (xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if (xLeft + nDlgWidth > rcArea.right)
		xLeft = rcArea.right - nDlgWidth;

	if (yTop < rcArea.top)
		yTop = rcArea.top;
	else if (yTop + nDlgHeight > rcArea.bottom)
		yTop = rcArea.bottom - nDlgHeight;

	// Map screen coordinates to child coordinates.
	SetWindowPos(hwnd, NULL, xLeft, yTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 * @brief Display wait message on the screen.
 * @param hwnd - window handle.
 */
void DisplayWaitBanner(HWND hwnd)
{
	_ASSERTE(g_pResManager != NULL);
	TCHAR szMessage[100];
	LoadString(g_hInstance, IDS_PLEASE_WAIT, szMessage, countof(szMessage));
	RECT rect;
	GetClientRect(hwnd, &rect);
	HDC hdc = GetDC(hwnd);
	FillRect(hdc, &rect, g_pResManager->m_hbrWindowBrush);
	HFONT hOldFont = SelectFont(hdc, g_pResManager->m_hDialogFont);
	DrawText(hdc, szMessage, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX);
	SelectFont(hdc, hOldFont);
	ReleaseDC(hwnd, hdc);
}

/**
 * @brief Validate scrollbar metrics.
 * @param pSInfo - pointer to scrollbar info.
 */
void ValidateScrollInfo(SCROLLINFO* pSInfo)
{
	_ASSERTE(pSInfo->nMax >= 0 && pSInfo->nPage >= 0);
	if ((UINT)pSInfo->nMax + 1 <= pSInfo->nPage)
		pSInfo->nMax = pSInfo->nPage = 0;
	if (pSInfo->nPos < 0)
	{
		pSInfo->nPos = 0;
	}
	else
	{
		int nMaxScrollPos = pSInfo->nMax - (pSInfo->nPage > 0 ? pSInfo->nPage - 1 : 0);
		if (pSInfo->nPos > nMaxScrollPos)
			pSInfo->nPos = nMaxScrollPos;
	}
}

// Folders processing.

/**
 * @brief Delete folder and recursively removes its contents.
 * @param pszFolder - folder name.
 * @return true if operation was completed successfully.
 */
BOOL DeleteFolder(PCTSTR pszFolder)
{
	TCHAR szFindFileTemplate[MAX_PATH];
	PathCombine(szFindFileTemplate, pszFolder, _T("*"));

	WIN32_FIND_DATA FindData;
	HANDLE hFindFile = FindFirstFile(szFindFileTemplate, &FindData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		BOOL bMore = TRUE;
		while (bMore)
		{
			TCHAR szFileName[MAX_PATH];
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (_tcscmp(FindData.cFileName, _T(".")) != 0 && _tcscmp(FindData.cFileName, _T("..")) != 0)
				{
					PathCombine(szFileName, pszFolder, FindData.cFileName);
					if (! DeleteFolder(szFileName))
						break;
				}
			}
			else
			{
				PathCombine(szFileName, pszFolder, FindData.cFileName);
				if (! DeleteFile(szFileName))
					break;
			}
			bMore = FindNextFile(hFindFile, &FindData);
		}
		FindClose(hFindFile);
	}
	return RemoveDirectory(pszFolder);
}

/**
 * @brief Creates folder even if parent folder doesn't exist.
 * @param pszFolder - folder name.
 * @return true if operation was completed successfully.
 */
BOOL CreateFolder(PCTSTR pszFolder)
{
	PCTSTR pszOldSegment = PathSkipRoot(pszFolder);
	_ASSERTE(pszOldSegment != NULL);
	if (pszOldSegment == NULL)
		return FALSE;
	TCHAR szFolderPath[MAX_PATH];
	_tcsncpy_s(szFolderPath, countof(szFolderPath), pszFolder, pszOldSegment - pszFolder);
	while (*pszOldSegment)
	{
		PCTSTR pszSegment = PathFindNextComponent(pszOldSegment);
		_tcsncat_s(szFolderPath, countof(szFolderPath), pszOldSegment, pszSegment - pszOldSegment);
		if (! CreateDirectory(szFolderPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			return FALSE;
		pszOldSegment = pszSegment;
	}
	return TRUE;
}

/**
 * @brief Creates parent folder for specified file.
 * @param pszFileName - file name.
 * @return true if operation was completed successfully.
 */
BOOL CreateParentFolder(PCTSTR pszFileName)
{
	TCHAR szFolderPath[MAX_PATH];
	_tcscpy_s(szFolderPath, countof(szFolderPath), pszFileName);
	PathRemoveFileSpec(szFolderPath);
	return CreateFolder(szFolderPath);
}

/**
 * @brief Generate temporary folder name and creates it.
 * @param pszTempPath - generated temporary path name.
 * @param dwTempPathSize - size of path buffer in characters.
 * @return true if operation was completed successfully.
 */
BOOL CreateTempFolder(PTSTR pszTempPath, DWORD dwTempPathSize)
{
	TCHAR szSysTempPath[MAX_PATH];
	GetTempPath(countof(szSysTempPath), szSysTempPath);
	PathRemoveBackslash(szSysTempPath);
	DWORD dwTicks = GetTickCount();
	for (;;)
	{
		_stprintf_s(pszTempPath, dwTempPathSize, _T("%s\\TEMP%lu"), szSysTempPath, dwTicks);
		if (CreateDirectory(pszTempPath, NULL))
			return TRUE;
		if (GetLastError() != ERROR_ALREADY_EXISTS)
			return FALSE;
		++dwTicks;
	}
}

/**
 * @brief Get canonical application name (all non-alphanumeric characters are stripped).
 * @param pszAppName - buffer for resulting application name.
 * @param dwBufferSize - size of file name buffer.
 * @param bAllowSpaces - true if spaces are allowed.
 * @return length of the name.
 */
DWORD GetCanonicalAppName(PTSTR pszAppName, DWORD dwBufferSize, BOOL bAllowSpaces)
{
	if (dwBufferSize == 0)
		return 0;
	if (*g_szAppName != _T('\0'))
	{
		--dwBufferSize;
		DWORD dwStrLength = _tcslen(g_szAppName);
		if (dwStrLength > dwBufferSize)
			dwStrLength = dwBufferSize;
		DWORD dwSrcPos = 0, dwDstPos = 0;
		WORD wCharMask = C1_ALPHA | C1_DIGIT;
		if (bAllowSpaces)
			wCharMask |= C1_SPACE;
		while (dwSrcPos < dwStrLength)
		{
			WORD arrCharType[2];
#ifdef _UNICODE
			int nCharSize = GetUnicodeCharSize((const BYTE*)(g_szAppName + dwSrcPos)) / sizeof(WCHAR);
			GetStringTypeW(CT_CTYPE1, g_szAppName + dwSrcPos, nCharSize, arrCharType);
#else
			int nCharSize = IsDBCSLeadByte(g_szAppName[dwSrcPos]) ? 2 : 1;
			GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, g_szAppName + dwSrcPos, nCharSize, arrCharType);
#endif
			if (*arrCharType & wCharMask)
			{
				pszAppName[dwDstPos++] = g_szAppName[dwSrcPos++];
				if (nCharSize > 1)
					pszAppName[dwDstPos++] = g_szAppName[dwSrcPos++];
			}
			else
			{
				/*if (*arrCharType & C1_SPACE)
					pszAppName[dwDstPos++] = _T('_');*/
				dwSrcPos += nCharSize;
			}
		}
		pszAppName[dwDstPos] = _T('\0');
		return dwDstPos;
	}
	else
	{
		TCHAR szAppFileName[MAX_PATH];
		if (! GetModuleFileName(NULL, szAppFileName, countof(szAppFileName)))
			return FALSE;
		PTSTR pszFileName = PathFindFileName(szAppFileName);
		PathRemoveExtension(pszFileName);
		_tcscpy_s(pszAppName, dwBufferSize, pszFileName);
		return _tcslen(pszAppName);
	}
}

/**
 * @brief Create complete log file name.
 * @param pszCompleteLogFileName - output complete file name.
 * @param pszLogFileName - base log file name.
 * @param pszDefFileExtension - default extension.
 * @return true if log file name was created and false otherwise.
 */
BOOL GetCompleteLogFileName(PTSTR pszCompleteLogFileName, PCTSTR pszLogFileName, PCTSTR pszDefFileExtension)
{
	if (pszLogFileName && *pszLogFileName && PathIsRoot(pszLogFileName))
	{
		_tcscpy_s(pszCompleteLogFileName, MAX_PATH, pszLogFileName);
		return TRUE;
	}
	TCHAR szAppDataPath[MAX_PATH];
	if (! SHGetSpecialFolderPath(NULL, szAppDataPath, CSIDL_APPDATA, TRUE))
		return FALSE;
	TCHAR szAppName[MAX_PATH];
	if (! GetCanonicalAppName(szAppName, countof(szAppName), TRUE))
		return FALSE;
	TCHAR szAppFileName[MAX_PATH];
	if (! GetModuleFileName(NULL, szAppFileName, countof(szAppFileName)))
		return FALSE;
	PTSTR pszFileName = PathFindFileName(szAppFileName);
	PathRemoveExtension(pszFileName);
	PTSTR pszAppName = *szAppName ? szAppName : pszFileName;
	PathCombine(pszCompleteLogFileName, szAppDataPath, pszAppName);
	if (pszLogFileName == NULL || *pszLogFileName == _T('\0'))
	{
		if (pszDefFileExtension == NULL || *pszDefFileExtension != _T('.'))
			return FALSE;
		PathAppend(pszCompleteLogFileName, pszFileName);
		PathAddExtension(pszCompleteLogFileName, pszDefFileExtension);
	}
	else
		PathAppend(pszCompleteLogFileName, pszLogFileName);
	return TRUE;
}

// List controls utilities.

/**
 * @param hwndList - list view control.
 */

void CListViewOrder::InitList(HWND hwndList)
{
	m_iColumnNumber = -1;
	m_bAscending = TRUE;
	_ASSERTE(g_pResManager != NULL);
	HWND hwndHeader = ListView_GetHeader(hwndList);
	Header_SetImageList(hwndHeader, g_pResManager->m_hSortArrows);
}

/**
 * @param hwndList - list view control.
 * @param iImage - image index.
 */
void CListViewOrder::SetSortImage(HWND hwndList, int iImage)
{
	_ASSERTE(m_iColumnNumber >= 0);
	HWND hwndHeader = ListView_GetHeader(hwndList);
	HDITEM hdi;
	ZeroMemory(&hdi, sizeof(hdi));
	if (iImage >= 0)
	{
		hdi.mask = HDI_FORMAT | HDI_IMAGE;
		hdi.iImage = iImage;
		hdi.fmt = HDF_LEFT | HDF_STRING | HDF_IMAGE;
	}
	else
	{
		hdi.mask = HDI_FORMAT;
		hdi.fmt = HDF_LEFT | HDF_STRING;
	}
	Header_SetItem(hwndHeader, m_iColumnNumber, &hdi);
}

/**
 * @param hwndList - list view control.
 */
void CListViewOrder::ClearSortParams(HWND hwndList)
{
	SetSortImage(hwndList, -1);
	m_iColumnNumber = -1;
	m_bAscending = TRUE;
}

/**
 * @param hwndList - list view control.
 * @param iColumnNumber - active column number.
 * @param bAscending - ascending/desceinding sort order.
 */
void CListViewOrder::SetSortParams(HWND hwndList, int iColumnNumber, BOOL bAscending)
{
	if (m_iColumnNumber != iColumnNumber &&
		m_iColumnNumber >= 0)
	{
		SetSortImage(hwndList, -1);
	}
	_ASSERTE(iColumnNumber >= 0);
	m_iColumnNumber = iColumnNumber;
	m_bAscending = bAscending;
	SetSortImage(hwndList, bAscending ? 0 : 1);
}


/**
 * @param hwndList - list view control.
 * @param iColumnNumber - active column number.
 */
void CListViewOrder::ToggleSortParams(HWND hwndList, int iColumnNumber)
{
	_ASSERTE(iColumnNumber >= 0);
	if (m_iColumnNumber == iColumnNumber)
	{
		m_bAscending = ! m_bAscending;
		SetSortImage(hwndList, m_bAscending ? 0 : 1);
	}
	else
		SetSortParams(hwndList, iColumnNumber, TRUE);
}

/**
 * @brief Compare two list view items.
 * @param lParam1 - 1st item index.
 * @param lParam2 - 2nd item index.
 * @param lParamSort - sort argument.
 * @return comparison result.
 */
int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LISTVIEW_SORT_PARAMS* pLVSortParams = (LISTVIEW_SORT_PARAMS*)lParamSort;
	TCHAR szItem1Text[256];
	ListView_GetItemText(pLVSortParams->hwndList, lParam1, pLVSortParams->iColumnNumber, szItem1Text, countof(szItem1Text));
	TCHAR szItem2Text[256];
	ListView_GetItemText(pLVSortParams->hwndList, lParam2, pLVSortParams->iColumnNumber, szItem2Text, countof(szItem2Text));
	int iResult;
	switch (pLVSortParams->eCompareType)
	{
	case LISTVIEW_SORT_PARAMS::ICT_STRING:
		iResult = _tcscmp(szItem1Text, szItem2Text);
		break;
	case LISTVIEW_SORT_PARAMS::ICT_INTEGER:
		{
			int iItemValue1 = _ttoi(szItem1Text);
			int iItemValue2 = _ttoi(szItem2Text);
			iResult = Comparator(iItemValue1, iItemValue2);
		}
		break;
	case LISTVIEW_SORT_PARAMS::ICT_HEXADECIMAL:
		{
			unsigned uItemValue1 = _tcstoul(szItem1Text, NULL, 16);
			unsigned uItemValue2 = _tcstoul(szItem2Text, NULL, 16);
			iResult = Comparator(uItemValue1, uItemValue2);
		}
		break;
	default:
		iResult = 0;
		break;
	}
	return (pLVSortParams->bAscending ? iResult : -iResult);
}

/** @} */
