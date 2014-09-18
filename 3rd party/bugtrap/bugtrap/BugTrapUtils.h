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

#pragma once

// Generic functions.

/**
 * @brief Compare two values.
 * @param val1 - 1st value.
 * @param val2 - 2nd value.
 * @return comparison result.
 */
template <typename TYPE>
inline int Comparator(const TYPE& val1, const TYPE& val2)
{
	if (val1 < val2)
		return -1;
	else if (val2 < val1)
		return +1;
	else
		return 0;
}

/**
 * @brief Swap bytes in 16-bit value.
 * @param value - integer value.
 * @return swapped value.
 */
inline WORD SWAP16(WORD value)
{
	return (
		(((WORD)(value) & 0xFF00) >> 8) |
		(((WORD)(value) & 0x00FF) << 8)
		);
}

/**
 * @brief Swap bytes in 32-bit value.
 * @param value - integer value.
 * @return swapped value.
 */
inline DWORD SWAP32(DWORD value)
{
	return (
		(((DWORD)(value) & 0xFF000000) >> 24) |
		(((DWORD)(value) & 0x00FF0000) >> 8) |
		(((DWORD)(value) & 0x000000FF) << 24) |
		(((DWORD)(value) & 0x0000FF00) << 8)
		);
}

// Windows processing.
void CenterWindow(HWND hwnd, HWND hwndCenter = NULL);
void DisplayWaitBanner(HWND hwnd);
void ValidateScrollInfo(SCROLLINFO* pSInfo);

// Folders processing.
BOOL DeleteFolder(PCTSTR pszFolder);
BOOL CreateFolder(PCTSTR pszFolder);
BOOL CreateParentFolder(PCTSTR pszFileName);
BOOL CreateTempFolder(PTSTR pszTempPath, DWORD dwTempPathSize);
DWORD GetCanonicalAppName(PTSTR pszAppName, DWORD dwBufferSize, BOOL bAllowSpaces);
BOOL GetCompleteLogFileName(PTSTR pszCompleteLogFileName, PCTSTR pszLogFileName, PCTSTR pszDefFileExtension);

// Strings processing.
#define TrimSpaces(str)   StrTrim(str, _T(" \t\r\n"))
#define TrimSpacesA(str)  StrTrimA(str, " \t\r\n")
#define TrimSpacesW(str)  StrTrimW(str, L" \t\r\n")

#define IsSpace(ch)       ((ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')
#define IsAlpha(ch)       (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))
#define IsNumber(ch)      ((ch) >= '0' && (ch) <= '9')
#define IsAlphaNum(ch)    (IsAlpha(ch) || IsNumber(ch))
#define IsQuotation(ch)   ((ch) == '\"' || (ch) == '\'')

// List controls utilities.

/// List view sort parameters.
struct LISTVIEW_SORT_PARAMS
{
	/// Comparison type.
	enum ITEM_COMPARE_TYPE
	{
		/// String comparison.
		ICT_STRING,
		/// Integer comparison.
		ICT_INTEGER,
		/// Hexadecimal comparison.
		ICT_HEXADECIMAL
	};

	/// List control window handle.
	HWND hwndList;
	/// Sorted column number.
	int iColumnNumber;
	/// Type of item comparison.
	ITEM_COMPARE_TYPE eCompareType;
	/// Ascending/desceinding sort order.
	BOOL bAscending;
};

/// Store list view order information.
class CListViewOrder
{
public:
	/// Initialize the object.
	CListViewOrder(void);
	/// Initialize list view control.
	void InitList(HWND hwndList);
	/// Get active column number.
	int GetActiveColumn(void) const;
	/// Get ascending/desceinding sort order.
	BOOL GetSortOrder(void) const;
	/// Clear sort parameters.
	void ClearSortParams(HWND hwndList);
	/// Set sort parameters.
	void SetSortParams(HWND hwndList, int iColumnNumber, BOOL bAscending);
	/// Toggle active sort parameters.
	void ToggleSortParams(HWND hwndList, int iColumnNumber);

private:
	/// Set sort marker of the list.
	void SetSortImage(HWND hwndList, int iImage);
	/// Active column number.
	int m_iColumnNumber;
	/// Ascending/desceinding sort order.
	BOOL m_bAscending;
};

inline CListViewOrder::CListViewOrder(void)
{
	m_iColumnNumber = -1;
	m_bAscending = TRUE;
}

/**
 * @return active column number.
 */
inline int CListViewOrder::GetActiveColumn(void) const
{
	return m_iColumnNumber;
}

/**
 * @return ascending/descending sort order.
 */
inline BOOL CListViewOrder::GetSortOrder(void) const
{
	return m_bAscending;
}

int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
