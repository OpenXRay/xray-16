/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Custom resources manager.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "resource.h"
#include "ResManager.h"
#include "Globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FIXED_FONT_HEIGHT             90
#define DIALOG_FONT_HEIGHT            60

/**
 * @addtogroup ResManager Custom resource manager.
 * @{
 */

/**
 * @param hwndParent - parent window handle.
 */
CResManager::CResManager(HWND hwndParent)
{
	ZeroMemory(this, sizeof(*this));

	HDC hDisplayDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	if (hDisplayDC == NULL)
		return;
	LONG lLogPixelsY = GetDeviceCaps(hDisplayDC, LOGPIXELSY);

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -MulDiv(FIXED_FONT_HEIGHT, lLogPixelsY, 720);
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	_tcscpy_s(lf.lfFaceName, countof(lf.lfFaceName), _T("Courier"));
	m_hFixedFont = CreateFontIndirect(&lf);

	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -MulDiv(DIALOG_FONT_HEIGHT, lLogPixelsY, 720);
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	lf.lfUnderline = TRUE;
	_tcscpy_s(lf.lfFaceName, countof(lf.lfFaceName), _T("MS Sans Serif"));
	m_hUnderlinedFont = CreateFontIndirect(&lf);

	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -MulDiv(DIALOG_FONT_HEIGHT, lLogPixelsY, 720);
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	_tcscpy_s(lf.lfFaceName, countof(lf.lfFaceName), _T("MS Sans Serif"));
	m_hDialogFont = CreateFontIndirect(&lf);

	m_hHandCursor = LoadCursor(NULL, IDC_HAND);
	if (! m_hHandCursor)
	{
		// Thanks to Paul DiLascia.
		TCHAR szWinHelpPath[MAX_PATH];
		GetWindowsDirectory(szWinHelpPath, countof(szWinHelpPath));
		PathAppend(szWinHelpPath, _T("winhlp32.exe"));
		HMODULE hWinHelp = LoadLibrary(szWinHelpPath);
		if (hWinHelp)
		{
			HCURSOR hHandCursor = LoadCursor(hWinHelp, MAKEINTRESOURCE(106));
			if (hHandCursor)
				m_hHandCursor = CopyCursor(hHandCursor);
			FreeLibrary(hWinHelp);
		}
	}
	m_hArrowCursor = LoadCursor(NULL, IDC_ARROW);
	m_hAppStartingCursor = LoadCursor(NULL, IDC_APPSTARTING);
	m_hWaitCursor = LoadCursor(NULL, IDC_WAIT);
	m_hUpDownCursor = LoadCursor(NULL, IDC_SIZENS);
	m_hLeftRightCursor = LoadCursor(NULL, IDC_SIZEWE);
	m_hIBeamCursor = LoadCursor(NULL, IDC_IBEAM);

	m_hbrWindowBrush = GetSysColorBrush(COLOR_WINDOW);
	m_hbrButtonFaceBrush = GetSysColorBrush(COLOR_BTNFACE);
	m_hbrControlLight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
	m_hbrAppWorkspace = GetSysColorBrush(COLOR_APPWORKSPACE);

	if (hwndParent != NULL)
	{ 
		//SendMessage was commented out to prevent hanging secondary threads calls
		__try {
			//m_hBigAppIcon = (HICON)SendMessage(hwndParent, WM_GETICON, ICON_BIG, 0l);
			//if (m_hBigAppIcon == NULL)
				m_hBigAppIcon = (HICON)GetClassLong(hwndParent, GCL_HICON);
			//m_hSmallAppIcon = (HICON)SendMessage(hwndParent, WM_GETICON, ICON_SMALL, 0l);
			//if (m_hSmallAppIcon == NULL)
				m_hSmallAppIcon = (HICON)GetClassLong(hwndParent, GCL_HICONSM);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			// ignore any exception in broken app...
			m_hBigAppIcon = NULL;
			m_hSmallAppIcon = NULL;
		}
	}
	int nCXSmallIcon = GetSystemMetrics(SM_CXSMICON);
	int nCYSmallIcon = GetSystemMetrics(SM_CYSMICON);
	if (m_hSmallAppIcon == NULL)
		m_hSmallAppIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_BUG), IMAGE_ICON, nCXSmallIcon, nCYSmallIcon, LR_DEFAULTCOLOR);
	int nCXIcon = GetSystemMetrics(SM_CXICON);
	int nCYIcon = GetSystemMetrics(SM_CYICON);
	if (m_hBigAppIcon == NULL)
		m_hBigAppIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_BUG), IMAGE_ICON, nCXIcon, nCYIcon, LR_DEFAULTCOLOR);
	if (m_hBigAppIcon != NULL)
	{
		m_hDialogIcon = CreateCompatibleBitmap(hDisplayDC, nCXIcon, nCYIcon);
		if (m_hDialogIcon != NULL)
		{
			HDC hMemDC = CreateCompatibleDC(hDisplayDC);
			if (hMemDC != NULL)
			{
				HBITMAP hbmpSafeBitmap = SelectBitmap(hMemDC, m_hDialogIcon);
				HBRUSH hbrSafeBrush = SelectBrush(hMemDC, m_hbrWindowBrush);
				PatBlt(hMemDC, 0, 0, nCXIcon, nCYIcon, PATCOPY);
				DrawIconEx(hMemDC, 0, 0, m_hBigAppIcon, nCXIcon, nCYIcon, 0, NULL, DI_NORMAL);
				SelectBrush(hMemDC, hbrSafeBrush);
				SelectBitmap(hMemDC, hbmpSafeBitmap);
				DeleteDC(hMemDC);
			}
		}
	}
	m_hCheckMark = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_CHECKMARK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	m_hSortArrows = ImageList_LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SORTARROWS), 7, 0, RGB(0xFF, 0x00, 0xFF), IMAGE_BITMAP, LR_LOADMAP3DCOLORS);

	DeleteDC(hDisplayDC);
}

CResManager::~CResManager(void)
{
	if (m_hFixedFont)
		DeleteFont(m_hFixedFont);
	if (m_hUnderlinedFont)
		DeleteFont(m_hUnderlinedFont);
	if (m_hDialogFont)
		DeleteFont(m_hDialogFont);
/*
	// System color brushes are owned by the system and must not be destroyed.
	if (m_hbrWindowBrush)
		DeleteBrush(m_hbrWindowBrush);
	if (m_hbrButtonFaceBrush)
		DeleteBrush(m_hbrButtonFaceBrush);
	if (m_hbrControlLight)
		DeleteBrush(m_hbrControlLight);
*/
	if (m_hBigAppIcon)
		DestroyIcon(m_hBigAppIcon);
	if (m_hSmallAppIcon)
		DestroyIcon(m_hSmallAppIcon);
	if (m_hDialogIcon)
		DeleteBitmap(m_hDialogIcon);
	if (m_hCheckMark)
		DeleteBitmap(m_hCheckMark);
	if (m_hSortArrows)
		ImageList_Destroy(m_hSortArrows);
}

/** @} */
