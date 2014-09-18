/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Common BugTrap UI routines.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "HyperLink.h"
#include "TransferProgressDlg.h"

/// Transfer thread parameters.
class CTransferThreadParams
{
public:
	/// Current context.
	enum INTERNET_CONTEXT
	{
		/// Unexpected error.
		IC_ERROR              = -1,
		/// Undefined context.
		IC_UNDEFINED          = 0,
		/// InternetConnect() call.
		IC_INTERNETCONNECT    = +1,
		/// HttpOpenRequest() call.
		IC_OPENREQUEST        = +2,
		/// HttpSendRequestEx() call.
		IC_SENDREQUEST        = +3,
		/// InternetWriteFile() call.
		IC_WRITEFILE          = +4,
		/// HttpEndRequest() call.
		IC_ENDREQUEST         = +5,
		/// Receiving response.
		IC_RECEIVINGRESPONSE  = +6,
		/// InternetCloseHandle() call.
		IC_CLOSEHANDLE        = +7
	};


	/// Initialize the object.
	CTransferThreadParams(HWND hwndSink = NULL);
	/// Destroy the object.
	~CTransferThreadParams(void);
	/// Get sink window handle.
	HWND GetSinkWnd(void) const;
	/// Set sink window handle.
	void SetSinkWnd(HWND hwndSink);
	/// Get current context.
	INTERNET_CONTEXT GetContext(void) const;
	/// Set current context.
	void SetContext(INTERNET_CONTEXT eContext);
	/// Get callback result.
	DWORD GetCallbackResult(void) const;
	/// Set callback result.
	void SetCallbackResult(DWORD dwCallbackResult);
	/// Get cancellation event handle.
	HANDLE GetCancellationEvent(void) const;
	/// Get completion event handle.
	HANDLE GetCompletionEvent(void) const;
	/// Set error code and error message to the default.
	void SetErrorCode(DWORD dwErrorCode);
	/// Set error code and error message to the default.
	void SetErrorCode(DWORD dwErrorCode, PTSTR pszMessageBuffer);
	/// Get error code.
	DWORD GetErrorCode(void) const;
	/// Get error message.
	PCTSTR GetErrorMessage(void) const;
	/// Post completion message to the sink window.
	void PostCompletionMessage(void);

private:
	/// Protect the class from being accidentally copied.
	CTransferThreadParams(const CTransferThreadParams& rParams);
	/// Protect the class from being accidentally copied.
	CTransferThreadParams& operator=(const CTransferThreadParams& rParams);

	/// Event handle used to cancel pending IO operation.
	HANDLE m_hCancellationEvent;
	/// Event handle used inform waiting thread about completion.
	HANDLE m_hCompletionEvent;
	/// Window handler of sink event.
	HWND m_hwndSink;
	/// Error message text.
	PTSTR m_pszMessageBuffer;
	/// Error code.
	DWORD m_dwErrorCode;
	/// Current context.
	INTERNET_CONTEXT m_eContext;
	/// Callback result.
	DWORD m_dwCallbackResult;

};

/**
 * @return current context.
 */
inline CTransferThreadParams::INTERNET_CONTEXT CTransferThreadParams::GetContext(void) const
{
	return m_eContext;
}

/**
 * @param eContext - current context.
 */
inline void CTransferThreadParams::SetContext(CTransferThreadParams::INTERNET_CONTEXT eContext)
{
	m_eContext = eContext;
}

/**
 * @return callback result.
 */
inline DWORD CTransferThreadParams::GetCallbackResult(void) const
{
	return m_dwCallbackResult;
}

/**
 * @param dwCallbackResult - callback result.
 */
inline void CTransferThreadParams::SetCallbackResult(DWORD dwCallbackResult)
{
	m_dwCallbackResult = dwCallbackResult;
}

/**
 * @return sink window handle.
 */
inline HWND CTransferThreadParams::GetSinkWnd(void) const
{
	return m_hwndSink;
}

/**
 * @param hwndSink - sink window handle.
 */
inline void CTransferThreadParams::SetSinkWnd(HWND hwndSink)
{
	m_hwndSink = hwndSink;
}

/**
 * @return cancellation event handle.
 */
inline HANDLE CTransferThreadParams::GetCancellationEvent(void) const
{
	return m_hCancellationEvent;
}

/**
 * @return completion event handle.
 */
inline HANDLE CTransferThreadParams::GetCompletionEvent(void) const
{
	return m_hCompletionEvent;
}

/**
 * @return error code.
 */
inline DWORD CTransferThreadParams::GetErrorCode(void) const
{
	return m_dwErrorCode;
}

/**
 * @return error message.
 */
inline PCTSTR CTransferThreadParams::GetErrorMessage(void) const
{
	return m_pszMessageBuffer;
}

inline void CTransferThreadParams::PostCompletionMessage(void)
{
	if (m_hwndSink)
		PostMessage(m_hwndSink, UM_TRANSFERCOMPLETE, m_dwErrorCode, 0);
}

HANDLE StartTransferThread(CTransferThreadParams* pTransferThreadParams);
void CloseTransferThread(HANDLE hTransferThread);

#define IDM_ABOUTBOX 0x0010

void GetDefaultMailSubject(PTSTR pszSubject, DWORD dwSubjectSize);
void GetDefaultMailURL(PTSTR pszURLString, DWORD dwURLSize);
BOOL SendEMail(HWND hwndParent, PCTSTR pszSubject, PCTSTR pszMessage);
void SendReport(HWND hwndParent);
void SubmitReport(HWND hwndParent);
void InitIntro(HWND hwnd, CHyperLink& hlURL);
void InitAbout(HWND hwnd);
void StartHandlerThread(void);
LONG InternalFilter(PEXCEPTION_POINTERS pExceptionPointers);
