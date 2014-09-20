/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: This class provides better error handling for MFC windows.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#ifndef _BTMFCWINDOW_H_
#define _BTMFCWINDOW_H_

#pragma once

#ifndef _BUGTRAP_H_
 #error Include BugTrap.h first
#endif // _BUGTRAP_H_

#ifndef __cplusplus
 #error C++ compiler is required
#endif // __cplusplus

#ifndef __AFX_H__
 #error This class cannot be used in non MFC applications
#endif // __AFX_H__

namespace MFC
{

#define _BTWND_INITIALIZER_ : m_pfnFilter(&BT_SehFilter), BASE_CLASS

/// This class substitutes default MFC error handling.
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
	virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	/// This window procedure intercepts MFC exceptions.
	LRESULT PrivWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Exception filter.
	LONG (CALLBACK * m_pfnFilter)(PEXCEPTION_POINTERS pExceptionPointers);
};

#undef _BTWND_INITIALIZER_

/**
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
LRESULT BTWindow<BASE_CLASS>::PrivWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	try {
		return BASE_CLASS::WindowProc(uMsg, wParam, lParam);
	} catch (CException* pException) {
		ASSERT(pException->IsKindOf(RUNTIME_CLASS(CException)));
		// extract error message
		TCHAR szErrorMessage[512];
		if (pException->GetErrorMessage(szErrorMessage, sizeof(szErrorMessage) / sizeof(TCHAR)))
			BT_SetUserMessage(szErrorMessage);
		pException->Delete();
		m_pfnFilter = &BT_CppFilter;
		// an exception will be caught by SEH handler
		throw;
	}
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

/**
 * @param uMsg - specifies the Windows message to be processed.
 * @param wParam - provides additional information used in processing the message.
 * @param lParam - provides additional information used in processing the message.
 * @return the return value depends on the message.
 */
template <class BASE_CLASS>
LRESULT BTWindow<BASE_CLASS>::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	__try {
		return PrivWindowProc(uMsg, wParam, lParam);
	} __except ((*m_pfnFilter)(GetExceptionInformation())) {
		m_pfnFilter = &BT_SehFilter;
		return 0;
	}
}

}

#ifndef BT_DO_NOT_USE_DEFAULT_NAMESPACES
 using namespace MFC;
#endif // BT_DO_NOT_USE_DEFAULT_NAMESPACES

#endif // _BTMFCWINDOW_H_
