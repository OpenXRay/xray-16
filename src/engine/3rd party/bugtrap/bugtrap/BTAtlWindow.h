/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: This class provides better error handling for ATL/WTL windows.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#ifndef _BTATLWINDOW_H_
#define _BTATLWINDOW_H_

#pragma once

#ifndef _BUGTRAP_H_
 #error Include BugTrap.h first
#endif // _BUGTRAP_H_

#ifndef __cplusplus
 #error C++ compiler is required
#endif // __cplusplus

#ifndef __ATLWIN_H__
 #error This class cannot be used in non ATL applications
#endif // __ATLWIN_H__

namespace ATL
{

#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
 #define _BTWND_INITIALIZER_ : m_pfnFilter(&BT_SehFilter), m_pfnBaseWndProc(BASE_CLASS::GetWindowProc()), BASE_CLASS
#else
 #define _BTWND_INITIALIZER_ : m_pfnBaseWndProc(BASE_CLASS::GetWindowProc()), BASE_CLASS
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_

/// This class adds error handling to ATL/WTL windows.
/// Only one form of exception handling is permitted per function,
/// therefore this class uses two functions to catch C++ and Windows errors.
template <class BASE_CLASS>
class BTWindow : public BASE_CLASS {
protected:
	/// Object initialization (0 parameters).
	BTWindow(void) _BTWND_INITIALIZER_ () { }
	/// Object initialization (1 parameter).
	template <typename T1>
	explicit BTWindow(T1 param1) _BTWND_INITIALIZER_ (param1) { }
	/// Object initialization (2 parameters)
	template <typename T1, typename T2>
	BTWindow(T1 param1, T2 param2) _BTWND_INITIALIZER_ (param1, param2) { }
	/// Object initialization (3 parameters).
	template <typename T1, typename T2, typename T3>
	BTWindow(T1 param1, T2 param2, T3 param3) _BTWND_INITIALIZER_ (param1, param2, param3) { }
	/// Object initialization (4 parameters).
	template <typename T1, typename T2, typename T3, typename T4>
	BTWindow(T1 param1, T2 param2, T3 param3, T4 param4) _BTWND_INITIALIZER_ (param1, param2, param3, param4) { }
	/// Object initialization (5 parameters).
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	BTWindow(T1 param1, T2 param2, T3 param3, T4 param4, T5 param5) _BTWND_INITIALIZER_ (param1, param2, param3, param4, param5) { }
	/// This window procedure uses SEH to intercept all unhandled exceptions.
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Overridden to return address of custom window procedure.
	virtual WNDPROC GetWindowProc(void);

private:
#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
	/// This window procedure intercepts C++ exceptions.
	LRESULT PrivWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Exception filter.
	LONG (CALLBACK * m_pfnFilter)(PEXCEPTION_POINTERS pExceptionPointers);
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_
	/// Cached address of base window procedure.
	WNDPROC m_pfnBaseWndProc;
};

#undef _BTWND_INITIALIZER_

/**
 * @return address of window procedure.
 */
template <class BASE_CLASS>
inline WNDPROC BTWindow<BASE_CLASS>::GetWindowProc()
{
	return &WindowProc;
}

#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
/**
 * @param hWnd - window handle.
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
LRESULT BTWindow<BASE_CLASS>::PrivWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	try {
		return (*m_pfnBaseWndProc)(hWnd, uMsg, wParam, lParam);
	}
#ifndef _ATL_NO_EXCEPTIONS
	catch (CAtlException& rException) {
		BT_SetUserMessageFromCode(rException.m_hr);
		// an exception will be caught by SEH handler
		throw;
	}
#endif // _ATL_NO_EXCEPTIONS
#ifdef _EXCEPTION_
	catch (exception& rException) {
		// extract error message
		const CHAR* pszErrorMessageA = rException.what();
		if (pszErrorMessageA != NULL && *pszErrorMessageA != '\0') {
#ifdef _UNICODE
			DWORD dwErrorMessageSizeW = MultiByteToWideChar(CP_ACP, 0, pszErrorMessageA, -1, NULL, 0);
			// alloca() cannot be used in catch block
			WCHAR* pszErrorMessageW = (WCHAR*)malloc(dwErrorMessageSizeW * sizeof(WCHAR));
			if (pszErrorMessageW != NULL) {
				MultiByteToWideChar(CP_ACP, 0, pszErrorMessageA, -1, pszErrorMessageW, dwErrorMessageSizeW);
				BT_SetUserMessage(pszErrorMessageW);
				free(pszErrorMessageW);
			}
#else
			BT_SetUserMessage(pszErrorMessageA);
#endif // ! _UNICODE
		}
		m_pfnFilter = &BT_CppFilter;
		// an exception will be caught by SEH handler
		throw;
	}
#endif // _EXCEPTION_
}
#endif // ! _ATL_NO_EXCEPTIONS || _EXCEPTION_

/**
 * @param hWnd - window handle.
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
LRESULT BTWindow<BASE_CLASS>::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// This cast requires direct inheritance from CWindowImplBase, i.e.
	// CWindowImplBase descendant should be declared first in the list of base classes.
	// More generic solution would require additional template arguments.
	BTWindow* pThis = (BTWindow*)hWnd;
#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
	__try {
		return pThis->PrivWindowProc(hWnd, uMsg, wParam, lParam);
	} __except ((*pThis->m_pfnFilter)(GetExceptionInformation())) {
		pThis->m_pfnFilter = &BT_SehFilter;
		return 0;
	}
#else
	__try {
		return (*pThis->m_pfnBaseWndProc)(hWnd, uMsg, wParam, lParam);
	} __except (BT_SehFilter(GetExceptionInformation())) {
		return 0;
	}
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_
}

#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
 #define _BTDLG_INITIALIZER_ : m_pfnFilter(&BT_SehFilter), m_pfnBaseDlgProc(BASE_CLASS::GetDialogProc()), BASE_CLASS
#else
 #define _BTDLG_INITIALIZER_ : m_pfnBaseDlgProc(BASE_CLASS::GetDialogProc()), BASE_CLASS
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_

/// This class adds error handling to ATL/WTL dialogs.
/// Only one form of exception handling is permitted per function,
/// therefore this class uses two functions to catch C++ and Windows errors.
template <class BASE_CLASS>
class BTDialog : public BASE_CLASS {
protected:
	/// Object initialization (0 parameters).
	BTDialog(void) _BTDLG_INITIALIZER_ () { }
	/// Object initialization (1 parameter).
	template <typename T1>
	explicit BTDialog(T1 param1) _BTDLG_INITIALIZER_ (param1) { }
	/// Object initialization (2 parameters)
	template <typename T1, typename T2>
	BTDialog(T1 param1, T2 param2) _BTDLG_INITIALIZER_ (param1, param2) { }
	/// Object initialization (3 parameters).
	template <typename T1, typename T2, typename T3>
	BTDialog(T1 param1, T2 param2, T3 param3) _BTDLG_INITIALIZER_ (param1, param2, param3) { }
	/// Object initialization (4 parameters).
	template <typename T1, typename T2, typename T3, typename T4>
	BTDialog(T1 param1, T2 param2, T3 param3, T4 param4) _BTDLG_INITIALIZER_ (param1, param2, param3, param4) { }
	/// Object initialization (5 parameters).
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	BTDialog(T1 param1, T2 param2, T3 param3, T4 param4, T5 param5) _BTDLG_INITIALIZER_ (param1, param2, param3, param4, param5) { }
	/// This dialog procedure uses SEH to intercept all unhandled exceptions.
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Overridden to return address of custom dialog procedure.
	virtual DLGPROC GetDialogProc(void);

private:
#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
	/// This dialog procedure intercepts C++ exceptions.
	INT_PTR PrivDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Exception filter.
	LONG (CALLBACK * m_pfnFilter)(PEXCEPTION_POINTERS pExceptionPointers);
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_
	/// Cached address of base dialog procedure.
	DLGPROC m_pfnBaseDlgProc;
};

#undef _BTDLG_INITIALIZER_

/**
 * @return address of dialog procedure.
 */
template <class BASE_CLASS>
inline DLGPROC BTDialog<BASE_CLASS>::GetDialogProc()
{
	return &DialogProc;
}

#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
/**
 * @param hWnd - window handle.
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
INT_PTR BTDialog<BASE_CLASS>::PrivDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	try {
		return (*m_pfnBaseDlgProc)(hWnd, uMsg, wParam, lParam);
	}
#ifndef _ATL_NO_EXCEPTIONS
	catch (CAtlException& rException) {
		BT_SetUserMessageFromCode(rException.m_hr);
		// an exception will be caught by SEH handler
		throw;
	}
#endif // _ATL_NO_EXCEPTIONS
#ifdef _EXCEPTION_
	catch (exception& rException) {
		// extract error message
		const CHAR* pszErrorMessageA = rException.what();
		if (pszErrorMessageA != NULL && *pszErrorMessageA != '\0') {
#ifdef _UNICODE
			DWORD dwErrorMessageSizeW = MultiByteToWideChar(CP_ACP, 0, pszErrorMessageA, -1, NULL, 0);
			// alloca() cannot be used in catch block
			WCHAR* pszErrorMessageW = (WCHAR*)malloc(dwErrorMessageSizeW * sizeof(WCHAR));
			if (pszErrorMessageW != NULL) {
				MultiByteToWideChar(CP_ACP, 0, pszErrorMessageA, -1, pszErrorMessageW, dwErrorMessageSizeW);
				BT_SetUserMessage(pszErrorMessageW);
				free(pszErrorMessageW);
			}
#else
			BT_SetUserMessage(pszErrorMessageA);
#endif // ! _UNICODE
		}
		m_pfnFilter = &BT_CppFilter;
		// an exception will be caught by SEH handler
		throw;
	}
#endif // _EXCEPTION_
}
#endif // ! _ATL_NO_EXCEPTIONS || _EXCEPTION_

/**
 * @param hWnd - window handle.
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
INT_PTR BTDialog<BASE_CLASS>::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// This cast requires direct inheritance from CDialogImplBase, i.e.
	// CDialogImplBase descendant should be declared first in the list of base classes.
	// More generic solution would require additional template arguments.
	BTDialog* pThis = (BTDialog*)hWnd;
#if ! defined _ATL_NO_EXCEPTIONS || defined _EXCEPTION_
	__try {
		return pThis->PrivDialogProc(hWnd, uMsg, wParam, lParam);
	} __except ((*pThis->m_pfnFilter)(GetExceptionInformation())) {
		pThis->m_pfnFilter = &BT_SehFilter;
		return 0;
	}
#else
	__try {
		return (*pThis->m_pfnBaseDlgProc)(hWnd, uMsg, wParam, lParam);
	} __except (BT_SehFilter(GetExceptionInformation())) {
		return 0;
	}
#endif // _ATL_NO_EXCEPTIONS && ! _EXCEPTION_
}

}

#ifndef BT_DO_NOT_USE_DEFAULT_NAMESPACES
 using namespace ATL;
#endif // BT_DO_NOT_USE_DEFAULT_NAMESPACES

#endif // _BTATLWINDOW_H_
