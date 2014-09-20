/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Preview dialog.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "PreviewDlg.h"
#include "BugTrapUtils.h"
#include "ResManager.h"
#include "LayoutManager.h"
#include "Splitter.h"
#include "TextFormat.h"
#include "TextView.h"
#include "HexView.h"
#include "ImageView.h"
#include "Globals.h"
#include "WaitCursor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IDC_ZOOMIN         101
#define IDC_ZOOMOUT        102
#define IDC_RESETSIZE      103
#define IDC_FITIMAGE       104

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/// Control layouts for Preview dialog.
static LAYOUT_INFO g_arrPreviewLayout[] =
{
	LAYOUT_INFO(IDC_SPLITTER,         ALIGN_LEFT,  ALIGN_TOP,    ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDC_FILEFORMAT_FRAME, ALIGN_LEFT,  ALIGN_BOTTOM, ALIGN_LEFT,  ALIGN_BOTTOM),
	LAYOUT_INFO(IDC_TEXTVIEW,         ALIGN_LEFT,  ALIGN_BOTTOM, ALIGN_LEFT,  ALIGN_BOTTOM),
	LAYOUT_INFO(IDC_HEXVIEW,          ALIGN_LEFT,  ALIGN_BOTTOM, ALIGN_LEFT,  ALIGN_BOTTOM),
	LAYOUT_INFO(IDCANCEL,             ALIGN_RIGHT, ALIGN_BOTTOM, ALIGN_RIGHT, ALIGN_BOTTOM),
	LAYOUT_INFO(IDC_IMAGE_COMMANDS,   ALIGN_LEFT,  ALIGN_BOTTOM, ALIGN_LEFT,  ALIGN_BOTTOM)
};

/// File list column identifiers.
enum FILE_LIST_COLUMN_ID
{
	/// File name.
	CID_FILE_NAME,
	/// File type.
	CID_FILE_TYPE,
	/// File size.
	CID_FILE_SIZE
};

/// File information.
struct FILE_ITEM_INFO
{
	/// File size.
	DWORD dwFileSize;
#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
	/// File name.
	TCHAR szFileName[0];
#pragma warning(pop)
};

/// File view type.
enum FILE_VIEW_TYPE
{
	/// No file view.
	FVT_NONE,
	/// Text view.
	FVT_TEXTVIEW,
	/// Hexadecimal view.
	FVT_HEXVIEW,
	/// Image view.
	FVT_IMAGEVIEW
};

/// Dialog layout manager.
static CLayoutManager g_LayoutMgr;
/// Splitter window.
static CSplitter g_Splitter(CSplitter::SD_VERTICAL, true);
/// File list sort order.
static CListViewOrder g_FilesListOrder;
/// Text view.
static CTextView g_TextView;
/// Hex view.
static CHexView g_HexView;
/// Image view.
static CImageView g_ImageView;
/// Current file view type.
static FILE_VIEW_TYPE g_eFileViewType = FVT_NONE;
/// Current file handle.
static HANDLE g_hFile = INVALID_HANDLE_VALUE;
/// Image handle.
static HBITMAP g_hBitmap = NULL;
/// True if file contains text.
static BOOL g_bTextFile = FALSE;
/// Text encoding.
static TEXT_ENCODING g_eEncoding = TXTENC_ANSI;
/// Size of encoding signature.
static DWORD g_dwSignatureSize = 0;
/// Tool-bar image-list.
static HIMAGELIST g_hImageList = NULL;
/// List of tool-bar buttons.
static const TBBUTTON g_arrButtons[] =
{
	{ 0, IDC_ZOOMIN,     TBSTATE_ENABLED, BTNS_BUTTON },
	{ 1, IDC_ZOOMOUT,    TBSTATE_ENABLED, BTNS_BUTTON },
	{ 2, IDC_RESETSIZE,  TBSTATE_ENABLED, BTNS_BUTTON },
	{ 3, IDC_FITIMAGE,   TBSTATE_ENABLED, BTNS_BUTTON }
};

/**
 * @brief Show or hide file format options.
 * @param hwnd - parent window handle.
 * @param bShow - pass true to show file format options and false otherwise.
 */
static void ShowFileFormatOptions(HWND hwnd, BOOL bShow)
{
	int nCmdShow = bShow ? SW_SHOWNA : SW_HIDE;
	UINT arrFileFormatItems[] =
	{
		IDC_TEXTVIEW, IDC_HEXVIEW, IDC_FILEFORMAT_FRAME
	};
	for (int nFileFormatItem = 0; nFileFormatItem < countof(arrFileFormatItems); ++nFileFormatItem)
	{
		HWND hwndCtl = GetDlgItem(hwnd, arrFileFormatItems[nFileFormatItem]);
		ShowWindow(hwndCtl, nCmdShow);
	}
}

/**
 * @brief Show or hide image commands.
 * @param hwnd - parent window handle.
 * @param bShow - pass true to show image commands and false otherwise.
 */
static void ShowImageCommands(HWND hwnd, BOOL bShow)
{
	HWND hwndCtl = GetDlgItem(hwnd, IDC_IMAGE_COMMANDS);
	int nCmdShow = bShow ? SW_SHOWNA : SW_HIDE;
	ShowWindow(hwndCtl, nCmdShow);
}

/**
 * @brief Set text view mode.
 * @param hwnd - parent window handle.
 */
static void SetTextView(HWND hwnd)
{
	_ASSERTE(g_hFile != INVALID_HANDLE_VALUE);
	g_eFileViewType = FVT_TEXTVIEW;
	HWND hwndFileView = GetDlgItem(hwnd, IDC_FILEVIEW);
	g_TextView.Attach(hwndFileView);
	g_TextView.SetFile(g_hFile, g_eEncoding, g_dwSignatureSize);
}

/**
 * @brief Set hex view mode.
 * @param hwnd - parent window handle.
 */
static void SetHexView(HWND hwnd)
{
	_ASSERTE(g_hFile != INVALID_HANDLE_VALUE);
	g_eFileViewType = FVT_HEXVIEW;
	HWND hwndFileView = GetDlgItem(hwnd, IDC_FILEVIEW);
	g_HexView.Attach(hwndFileView);
	g_HexView.SetFile(g_hFile);
}

/**
 * @brief Set image view mode.
 * @param hwnd - parent window handle.
 */
static void SetImageView(HWND hwnd)
{
	_ASSERTE(g_hBitmap != INVALID_HANDLE_VALUE);
	g_eFileViewType = FVT_IMAGEVIEW;
	HWND hwndFileView = GetDlgItem(hwnd, IDC_FILEVIEW);
	g_ImageView.Attach(hwndFileView);
	g_ImageView.SetImage(g_hBitmap);
}

/**
 * @brief Close previous file view.
 * @param hwnd - parent window handle.
 */
static void CloseFileView(HWND hwnd)
{
	switch (g_eFileViewType)
	{
	case FVT_TEXTVIEW:
		ShowFileFormatOptions(hwnd, FALSE);
		g_TextView.Detach();
		break;
	case FVT_HEXVIEW:
		ShowFileFormatOptions(hwnd, FALSE);
		g_HexView.Detach();
		break;
	case FVT_IMAGEVIEW:
		ShowImageCommands(hwnd, FALSE);
		g_ImageView.Detach();
		break;
	}
	g_eFileViewType = FVT_NONE;
}

/**
 * @brief Close file handle.
 */
static void CloseFileHandle(void)
{
	if (g_hBitmap != NULL)
	{
		DeleteBitmap(g_hBitmap);
		g_hBitmap = NULL;
	}
	if (g_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_hFile);
		g_hFile = INVALID_HANDLE_VALUE;
	}
	g_dwSignatureSize = 0;
}

/**
 * @brief Close file view pane and close file handle.
 * @param hwnd - parent window handle.
 */
inline static void CloseViewPane(HWND hwnd)
{
	CloseFileView(hwnd);
	CloseFileHandle();
}

/**
 * @brief WM_COMMAND handler of Preview dialog.
 * @param hwnd - window handle.
 * @param id - control ID.
 * @param hwndCtl - control handle.
 * @param codeNotify - notification code.
 */
static void PreviewDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	codeNotify; hwndCtl;
	switch (id)
	{
	case IDC_TEXTVIEW:
		if (g_eFileViewType == FVT_HEXVIEW)
		{
			g_HexView.Detach();
			SetTextView(hwnd);
		}
		break;
	case IDC_HEXVIEW:
		if (g_eFileViewType == FVT_TEXTVIEW)
		{
			g_TextView.Detach();
			SetHexView(hwnd);
		}
		break;
	case IDC_FITIMAGE:
		g_ImageView.FitImage();
		break;
	case IDC_RESETSIZE:
		g_ImageView.ResetSize();
		break;
	case IDC_ZOOMIN:
		g_ImageView.ZoomIn();
		break;
	case IDC_ZOOMOUT:
		g_ImageView.ZoomOut();
		break;
	case IDOK:
	case IDCANCEL:
		EndDialog(hwnd, FALSE);
		break;
	}
}

/**
 * @brief WM_SIZE handler of Preview dialog.
 * @param hwnd - window handle.
 * @param state - window state.
 * @param cx - window width.
 * @param cy - window height.
 */
static void PreviewDlg_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	hwnd; cx; cy; state;
	g_LayoutMgr.ApplyLayout();
}

/**
 * @brief WM_GETMINMAXINFO handler of Preview dialog.
 * @param hwnd - window handle.
 * @param pMinMaxInfo - window min-max info.
 */
static void PreviewDlg_OnGetMinMaxInfo(HWND hwnd, PMINMAXINFO pMinMaxInfo)
{
	hwnd;
	pMinMaxInfo->ptMinTrackSize = g_LayoutMgr.GetMinTrackSize();
}

/**
 * @brief Add file item to the list.
 * @param hwndFileList - file list view.
 * @param pszFilePath - file path.
 */
static void AddFileItem(HWND hwndFileList, PCTSTR pszFilePath)
{
	int nItemPos = ListView_GetItemCount(hwndFileList);

	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof(sfi));
	HIMAGELIST hFileImages = (HIMAGELIST)SHGetFileInfo(pszFilePath, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME);
	if (nItemPos == 0)
	{
		_ASSERTE((GetWindowLong(hwndFileList, GWL_STYLE) & LVS_SHAREIMAGELISTS) != 0);
		ListView_SetImageList(hwndFileList, hFileImages, LVSIL_SMALL);
	}

	DWORD dwFileSize;
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile = FindFirstFile(pszFilePath, &FindData);
	TCHAR szFileSize[32];
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		dwFileSize = FindData.nFileSizeLow;
		if (dwFileSize >= 1000000000)
			_stprintf_s(szFileSize, countof(szFileSize), _T("%lu.%lu GB"), dwFileSize / 1000000000, (dwFileSize % 1000000000) / 100000000);
		else if (dwFileSize >= 1000000)
			_stprintf_s(szFileSize, countof(szFileSize), _T("%lu.%lu MB"), dwFileSize / 1000000, (dwFileSize % 1000000) / 100000);
		else if (dwFileSize >= 1000)
			_stprintf_s(szFileSize, countof(szFileSize), _T("%lu.%lu KB"), dwFileSize / 1000, (dwFileSize % 1000) / 100);
		else
			_stprintf_s(szFileSize, countof(szFileSize), _T("%lu B"), dwFileSize);
		FindClose(hFindFile);
	}
	else
	{
		*szFileSize = _T('\0');
		dwFileSize = 0;
	}

	DWORD dwFilePathSize = _tcslen(pszFilePath) + 1;
	FILE_ITEM_INFO* pFileItemInfo = (FILE_ITEM_INFO*)new BYTE[sizeof(FILE_ITEM_INFO) + dwFilePathSize * sizeof(TCHAR)];
	if (pFileItemInfo != NULL)
	{
		pFileItemInfo->dwFileSize = dwFileSize;
		_tcscpy_s(pFileItemInfo->szFileName, dwFilePathSize, pszFilePath);
	}

	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = nItemPos;
	lvi.pszText = PathFindFileName(pszFilePath);
	lvi.iImage = sfi.iIcon;
	lvi.lParam = (LPARAM)pFileItemInfo;
	ListView_InsertItem(hwndFileList, &lvi);
	ListView_SetItemText(hwndFileList, nItemPos, CID_FILE_TYPE, sfi.szTypeName);
	ListView_SetItemText(hwndFileList, nItemPos, CID_FILE_SIZE, szFileSize);
}

/**
 * @brief Compare two list view items.
 * @param lParam1 - 1st item index.
 * @param lParam2 - 2nd item index.
 * @param lParamSort - sort argument.
 * @return comparison result.
 */
static int CALLBACK FileSizeCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LISTVIEW_SORT_PARAMS* pLVSortParams = (LISTVIEW_SORT_PARAMS*)lParamSort;
	FILE_ITEM_INFO* pFileItem1Info = (FILE_ITEM_INFO*)lParam1;
	FILE_ITEM_INFO* pFileItem2Info = (FILE_ITEM_INFO*)lParam2;
	int iResult = Comparator(pFileItem1Info->dwFileSize, pFileItem2Info->dwFileSize);
	return (pLVSortParams->bAscending ? iResult : -iResult);
}

/**
 * Attempt to open file as image.
 * @param hwnd - parent window handle.
 * @param pszFileName - source file name.
 * @return true if file successfully opened and false otherwise.
 */
static BOOL OpenImageFile(HWND hwnd, PCTSTR pszFileName)
{
	_ASSERTE(g_hBitmap == NULL);
	static const TCHAR szBmpFileExt[] = _T(".bmp");
	DWORD dwFileNameLength = _tcslen(pszFileName);
	const DWORD dwBmpFileExtLength = countof(szBmpFileExt) - 1;
	if (dwFileNameLength >= dwBmpFileExtLength && _tcsicmp(pszFileName + dwFileNameLength - dwBmpFileExtLength, szBmpFileExt) == 0 &&
		(g_hBitmap = (HBITMAP)LoadImage(NULL, pszFileName, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_LOADFROMFILE)) != NULL)
	{
		g_eFileViewType = FVT_IMAGEVIEW;
		SetImageView(hwnd);
		ShowImageCommands(hwnd, TRUE);
		return TRUE;
	}
	return FALSE;
}

/**
 * Attempt to open file in text or binary mode.
 * @param hwnd - parent window handle.
 * @param pszFileName - source file name.
 * @return true if file successfully opened and false otherwise.
 */
static BOOL OpenRegularFile(HWND hwnd, PCTSTR pszFileName)
{
	_ASSERTE(g_hFile == INVALID_HANDLE_VALUE);
	g_hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_hFile != INVALID_HANDLE_VALUE)
	{
		g_dwSignatureSize = DetectFileFormat(pszFileName, g_hFile, g_bTextFile, g_eEncoding);
		UINT uCtrlID;
		if (g_bTextFile)
		{
			uCtrlID = IDC_TEXTVIEW;
			SetTextView(hwnd);
		}
		else
		{
			uCtrlID = IDC_HEXVIEW;
			SetHexView(hwnd);
		}
		CheckRadioButton(hwnd, IDC_TEXTVIEW, IDC_HEXVIEW, uCtrlID);
		ShowFileFormatOptions(hwnd, TRUE);
		return TRUE;
	}
	return FALSE;
}

/**
 * @brief WM_NOTIFY handler of Preview dialog.
 * @param hwnd - window handle.
 * @param idCtrl - identifier of the common control sending the message.
 * @param pnmh - pointer to an NMHDR structure that contains the notification code and additional information.
 */
static LRESULT PreviewDlg_OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
	switch (pnmh->code)
	{
	case LVN_ITEMCHANGED:
		{
			if (idCtrl != IDC_FILESLIST)
				break;
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
			if ((pnmv->uNewState & LVIS_SELECTED) != (pnmv->uOldState & LVIS_SELECTED))
			{
				if (pnmv->uNewState & LVIS_SELECTED)
				{
					_ASSERTE(g_hFile == INVALID_HANDLE_VALUE && g_hBitmap == NULL);
					HWND hwndFileList = GetDlgItem(hwnd, IDC_FILESLIST);
					LVITEM lvi;
					ZeroMemory(&lvi, sizeof(lvi));
					lvi.mask = LVIF_PARAM;
					lvi.iItem = pnmv->iItem;
					ListView_GetItem(hwndFileList, &lvi);
					FILE_ITEM_INFO* pFileItemInfo = (FILE_ITEM_INFO*)lvi.lParam;
					if (pFileItemInfo != NULL)
					{
						CWaitCursor wait(true);
						if (! OpenImageFile(hwnd, pFileItemInfo->szFileName))
							OpenRegularFile(hwnd, pFileItemInfo->szFileName);
					}
				}
				else
					CloseViewPane(hwnd);
			}
		}
		break;
	case LVN_COLUMNCLICK:
		{
			if (idCtrl != IDC_FILESLIST)
				break;
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
			HWND hwndFileList = GetDlgItem(hwnd, IDC_FILESLIST);
			g_FilesListOrder.ToggleSortParams(hwndFileList, pnmv->iSubItem);
			LISTVIEW_SORT_PARAMS lvSortParams;
			lvSortParams.hwndList = hwndFileList;
			lvSortParams.iColumnNumber = pnmv->iSubItem;
			lvSortParams.bAscending = g_FilesListOrder.GetSortOrder();
			switch (pnmv->iSubItem)
			{
			case CID_FILE_NAME:
			case CID_FILE_TYPE:
				lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_STRING;
				ListView_SortItemsEx(hwndFileList, &ListViewCompareFunc, (LPARAM)&lvSortParams);
				break;
			case CID_FILE_SIZE:
				lvSortParams.eCompareType = LISTVIEW_SORT_PARAMS::ICT_INTEGER;
				ListView_SortItems(hwndFileList, &FileSizeCompareFunc, (LPARAM)&lvSortParams);
				break;
			}
		}
		break;
	case TTN_GETDISPINFO:
		{
			LPTOOLTIPTEXT ptt = (LPTOOLTIPTEXT)pnmh;
			ptt->hinst = g_hInstance;
			ptt->uFlags |= TTF_DI_SETITEM;
			switch (ptt->hdr.idFrom)
			{
			case IDC_ZOOMIN:
				ptt->lpszText = MAKEINTRESOURCE(IDS_ZOOMIN);
				break;
			case IDC_ZOOMOUT:
				ptt->lpszText = MAKEINTRESOURCE(IDS_ZOOMOUT);
				break;
			case IDC_RESETSIZE:
				ptt->lpszText = MAKEINTRESOURCE(IDS_RESETSIZE);
				break;
			case IDC_FITIMAGE:
				ptt->lpszText = MAKEINTRESOURCE(IDS_FITIMAGE);
				break;
			}
		}
		break;
	}
	return FALSE;
}

/**
 * @brief WM_INITDIALOG handler of Preview dialog.
 * @param hwnd - window handle.
 * @param hwndFocus - system-defined focus window.
 * @param lParam - user-defined parameter.
 * @return true to setup focus to system-defined control.
 */
static BOOL PreviewDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	lParam; hwndFocus;

	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hBigAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_pResManager->m_hBigAppIcon);
	if (g_pResManager->m_hSmallAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_pResManager->m_hSmallAppIcon);
	CenterWindow(hwnd, GetParent(hwnd));

	g_hImageList = ImageList_LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_IMAGETOOLBAR), 16, 0, RGB(0xC0, 0xC0, 0xC0));
	HWND hwndImgToolbar = GetDlgItem(hwnd, IDC_IMAGE_COMMANDS);
	LONG lStyle = GetWindowLong(hwndImgToolbar, GWL_STYLE);
	SetWindowLong(hwndImgToolbar, GWL_STYLE, lStyle | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS);
	SendMessage(hwndImgToolbar, TB_SETIMAGELIST, 0, (LPARAM)g_hImageList);
	SendMessage(hwndImgToolbar, TB_ADDBUTTONS, countof(g_arrButtons), (LPARAM)g_arrButtons);

	HWND hwndFileList = GetDlgItem(hwnd, IDC_FILESLIST);
	ListView_SetExtendedListViewStyle(hwndFileList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	RECT rcList;
	GetClientRect(hwndFileList, &rcList);
	rcList.right -= GetSystemMetrics(SM_CXHSCROLL);

	TCHAR szColumnTitle[64];
	LVCOLUMN lvc;
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = szColumnTitle;

	lvc.cx = rcList.right / 2;
	LoadString(g_hInstance, IDS_COLUMN_FILE, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndFileList, CID_FILE_NAME, &lvc);

	lvc.cx = rcList.right / 4;
	LoadString(g_hInstance, IDS_COLUMN_TYPE, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndFileList, CID_FILE_TYPE, &lvc);

	lvc.cx = rcList.right / 4;
	LoadString(g_hInstance, IDS_COLUMN_SIZE, szColumnTitle, countof(szColumnTitle));
	ListView_InsertColumn(hwndFileList, CID_FILE_SIZE, &lvc);

	g_hFile = INVALID_HANDLE_VALUE;
	g_hBitmap = NULL;
	g_eFileViewType = FVT_NONE;

	if (g_dwFlags & BTF_DETAILEDMODE)
	{
		TCHAR szFindFileTemplate[MAX_PATH];
		PathCombine(szFindFileTemplate, g_szInternalReportFolder, _T("*"));
		WIN32_FIND_DATA FindData;
		HANDLE hFindFile = FindFirstFile(szFindFileTemplate, &FindData);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			BOOL bMore = TRUE;
			while (bMore)
			{
				if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					TCHAR szFilePath[MAX_PATH];
					PathCombine(szFilePath, g_szInternalReportFolder, FindData.cFileName);
					AddFileItem(hwndFileList, szFilePath);
				}
				bMore = FindNextFile(hFindFile, &FindData);
			}
			FindClose(hFindFile);
		}
	}
	else
		AddFileItem(hwndFileList, g_szInternalReportFilePath);

	int nFileCount = g_arrLogLinks.GetCount();
	for (int nFilePos = 0; nFilePos < nFileCount; ++nFilePos)
	{
		CLogLink* pLogLink = g_arrLogLinks[nFilePos];
		_ASSERTE(pLogLink != NULL);
		PCTSTR pszFilePath = pLogLink->GetLogFileName();
		AddFileItem(hwndFileList, pszFilePath);
	}

	HWND hwndSplitter = GetDlgItem(hwnd, IDC_SPLITTER);
	g_Splitter.Attach(hwndSplitter);
	RECT rect;
	GetClientRect(hwndFileList, &rect);
	// Set splitter position according to initial dialog layout.
	g_Splitter.SetSplitterPos(rect.bottom);
	g_Splitter.SetPanel(0, hwndFileList);
	HWND hwndFileView = GetDlgItem(hwnd, IDC_FILEVIEW);
	g_Splitter.SetPanel(1, hwndFileView);

	g_LayoutMgr.InitLayout(hwnd, g_arrPreviewLayout, countof(g_arrPreviewLayout));

	// LVM_SETIMAGELIST resets header control image list
	g_FilesListOrder.InitList(hwndFileList);
	return TRUE;
}

/**
 * @brief WM_DESTROY handler of Preview dialog.
 * @param hwnd - window handle.
 */
static void PreviewDlg_OnDestroy(HWND hwnd)
{
	ImageList_Destroy(g_hImageList);
	g_hImageList = NULL;
	HWND hwndFileList = GetDlgItem(hwnd, IDC_FILESLIST);
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(lvi));
	lvi.mask = LVIF_PARAM;
	int nNumItems = ListView_GetItemCount(hwndFileList);
	for (int nItemNum = 0; nItemNum < nNumItems; ++nItemNum)
	{
		lvi.iItem = nItemNum;
		ListView_GetItem(hwndFileList, &lvi);
		FILE_ITEM_INFO* pFileItemInfo = (FILE_ITEM_INFO*)lvi.lParam;
		delete[] (PBYTE)pFileItemInfo;
	}
	CloseViewPane(hwnd);
	g_Splitter.Detach();
}

/**
 * @brief Dialog procedure of Preview dialog.
 * @param hwndDlg - window handle.
 * @param uMsg - message identifier.
 * @param wParam - 1st message parameter.
 * @param lParam - 2nd message parameter.
 * @return message result.
 */
INT_PTR CALLBACK PreviewDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwndDlg, WM_INITDIALOG, PreviewDlg_OnInitDialog);
	HANDLE_MSG(hwndDlg, WM_COMMAND, PreviewDlg_OnCommand);
	HANDLE_MSG(hwndDlg, WM_SIZE, PreviewDlg_OnSize);
	HANDLE_MSG(hwndDlg, WM_GETMINMAXINFO, PreviewDlg_OnGetMinMaxInfo);
	HANDLE_MSG(hwndDlg, WM_DESTROY, PreviewDlg_OnDestroy);
	HANDLE_MSG(hwndDlg, WM_NOTIFY, PreviewDlg_OnNotify);
	default: return FALSE;
	}
}

/** @} */
