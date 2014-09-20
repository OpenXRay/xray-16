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

#include "StdAfx.h"
#include "resource.h"
#include "BugTrap.h"
#include "BugTrapUtils.h"
#include "BugTrapUI.h"
#include "WaitDlg.h"
#include "WaitCursor.h"
#include "MainDlg.h"
#include "SimpleDlg.h"
#include "SendMailDlg.h"
#include "Globals.h"
#include "Encoding.h"
#include "MemStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @addtogroup BugTrapUI BugTrap Graphical User Interface
 * @{
 */

/**
 * @brief Get default subject for bug report.
 * @param pszSubject - pointer to subject buffer.
 * @param dwSubjectSize - size of subject buffer.
 */
void GetDefaultMailSubject(PTSTR pszSubject, DWORD dwSubjectSize)
{
	if (*g_szAppName)
		_stprintf_s(pszSubject, dwSubjectSize, _T("\"%s\" Error Report"), g_szAppName);
	else
		_tcscpy_s(pszSubject, dwSubjectSize, _T("Error Report"));
}

/**
 * @brief Get default mail-to URL for bug report.
 * @param pszURLString - pointer to URL buffer.
 * @param dwURLSize - size of URL buffer.
 */
void GetDefaultMailURL(PTSTR pszURLString, DWORD dwURLSize)
{
	if (*g_szAppName)
		_stprintf_s(pszURLString, dwURLSize, _T("mailto:%s?subject=%%22%s%%22%%20Error%%20Report"), g_szSupportEMail, g_szAppName);
	else
		_stprintf_s(pszURLString, dwURLSize, _T("mailto:%s?subject=Error%%20Report"), g_szSupportEMail);
}

/**
 * @brief Send e-mail message through Simple MPI facilities.
 * @param hwndParent - parent window handle (valid if user interface is enabled).
 * @param pszSubject - subject text.
 * @param pszMessage - message text.
 * @return true for successfully completed operation.
 */
BOOL SendEMail(HWND hwndParent, PCTSTR pszSubject, PCTSTR pszMessage)
{
	if (*g_szSupportEMail == _T('\0'))
		return FALSE;
	CWaitCursor wait;
	if (g_eActivityType == BTA_SHOWUI)
		wait.BeginWait();
	if (g_pMapiSession == NULL)
	{
		g_pMapiSession = new CMapiSession;
		if (g_pMapiSession == NULL)
			return FALSE;
	}
	if (! g_pMapiSession->LoggedOn())
	{
		if (*g_szMailProfile)
		{
			if (! g_pMapiSession->Logon(g_szMailProfile, g_szMailPassword))
				return FALSE;
		}
		else if (g_eActivityType == BTA_SHOWUI)
		{
			wait.EndWait();
			if (! g_pMapiSession->Logon(hwndParent))
				return FALSE;
			wait.BeginWait();
		}
		else
		{
			if (! g_pMapiSession->Logon())
				return FALSE;
		}
	}

	CMapiMessage message;
	message.GetTo().AddItem(g_szSupportEMail);
	message.SetSubject(pszSubject);
	message.SetBody(pszMessage);
	if (g_dwFlags & BTF_ATTACHREPORT)
	{
		message.GetAttachments().AddItem(g_szInternalReportFilePath);
		message.GetAttachmentTitles().AddItem(PathFindFileName(g_szInternalReportFilePath));
	}
	BOOL bShowMessageEditor = g_eActivityType == BTA_SHOWUI && (g_dwFlags & BTF_EDITMAIL) == 0;
	return g_pMapiSession->Send(message, bShowMessageEditor, hwndParent);
}

/**
 * @brief Send bug report through e-mail either using custom or system dialogs.
 * @param hwndParent - parent window handle.
 */
void SendReport(HWND hwndParent)
{
	if (*g_szSupportEMail)
	{
		BOOL bResult;
		if (g_eActivityType == BTA_SHOWUI && (g_dwFlags & BTF_EDITMAIL) != 0)
		{
			bResult = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SEND_MAIL_DLG), hwndParent, SendMailDlgProc) == IDOK;
		}
		else if (g_dwFlags & BTF_ATTACHREPORT)
		{
			TCHAR szSubject[MAX_PATH];
			GetDefaultMailSubject(szSubject, countof(szSubject));
			bResult = SendEMail(hwndParent, szSubject, NULL);
		}
		else
		{
			CWaitCursor wait;
			if (g_eActivityType == BTA_SHOWUI)
				wait.BeginWait();
			TCHAR szURLString[MAX_PATH];
			GetDefaultMailURL(szURLString, countof(szURLString));
			bResult = ShellExecute(NULL, _T("open"), szURLString, NULL, NULL, SW_SHOWDEFAULT) == ERROR_SUCCESS;
		}
		if (g_eActivityType == BTA_SHOWUI)
		{
			TCHAR szProjectName[32], szMessageText[128];
			LoadString(g_hInstance, IDS_BUGTRAP_NAME, szProjectName, countof(szProjectName));
			if (bResult)
			{
				SetForegroundWindow(hwndParent);
				LoadString(g_hInstance, IDS_STATUS_REPORTSENT, szMessageText, countof(szMessageText));
				::MessageBox(hwndParent, szMessageText, szProjectName, MB_ICONINFORMATION | MB_OK);
				EndDialog(hwndParent, FALSE);
			}
			else if ((g_dwFlags & BTF_EDITMAIL) == 0)
			{
				SetForegroundWindow(hwndParent);
				LoadString(g_hInstance, IDS_ERROR_TRANSFERFAILED, szMessageText, countof(szMessageText));
				::MessageBox(hwndParent, szMessageText, szProjectName, MB_ICONERROR | MB_OK);
			}
		}
	}
}

/// Protocol message type.
enum MESSAGE_TYPE
{
	/// Compound message that includes project info and report data.
	MT_COMPOUND_MESSAGE = 1
};

/// Compound message flags.
enum COMPOUND_MESSAGE_FLAGS
{
	/// No flags specified.
	CMF_NONE = 0x00
};

/**
 * @param hwndSink - sink window handle.
 */
CTransferThreadParams::CTransferThreadParams(HWND hwndSink)
{
	m_hCancellationEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // non-signalled manual reset event
	m_hCompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // non-signalled manual reset event
	m_hwndSink = hwndSink;
	m_dwErrorCode = ERROR_SUCCESS;
	m_pszMessageBuffer = NULL;
	m_eContext = IC_UNDEFINED;
	m_dwCallbackResult = ERROR_SUCCESS;
}

CTransferThreadParams::~CTransferThreadParams(void)
{
	if (m_pszMessageBuffer)
		LocalFree(m_pszMessageBuffer);
	CloseHandle(m_hCancellationEvent);
	CloseHandle(m_hCompletionEvent);
}

/**
 * @param dwErrorCode - error code.
 * @param pszMessageBuffer - error message.
 */
void CTransferThreadParams::SetErrorCode(DWORD dwErrorCode, PTSTR pszMessageBuffer)
{
	if (m_pszMessageBuffer)
		LocalFree(m_pszMessageBuffer);
	_ASSERTE(dwErrorCode != ERROR_SUCCESS);
	m_dwErrorCode = dwErrorCode;
	m_pszMessageBuffer = pszMessageBuffer;
}

/**
 * @param dwErrorCode - error code.
 */
void CTransferThreadParams::SetErrorCode(DWORD dwErrorCode)
{
	PTSTR pszMessageBuffer = NULL;
	if (dwErrorCode != ERROR_SUCCESS)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		              NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		              (PTSTR)&pszMessageBuffer, 0, NULL);
		if (pszMessageBuffer)
			TrimSpaces(pszMessageBuffer);
	}
	SetErrorCode(dwErrorCode, pszMessageBuffer);
}

/**
 * @brief Wait until pending operation is either complete or canceled.
 * @param pTransferThreadParams - thread parameters.
 * @return error code.
 */
static DWORD WaitForPendingSocketRequest(CTransferThreadParams* pTransferThreadParams)
{
	HANDLE hCancelationEvent = pTransferThreadParams->GetCancellationEvent();
	HANDLE hCompletionEvent = pTransferThreadParams->GetCompletionEvent();
	HANDLE arrEvents[] = { hCompletionEvent, hCancelationEvent };

	DWORD dwErrorCode = ERROR_SUCCESS;
	DWORD dwTemp = WSAGetLastError();
	if (dwTemp != WSA_IO_PENDING && dwTemp != WSAEWOULDBLOCK)
		dwErrorCode = dwTemp;
	else if (WaitForMultipleObjects(countof(arrEvents), arrEvents, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		dwErrorCode = WSA_OPERATION_ABORTED;
	return dwErrorCode;
}

/**
 * @brief Submit bug report through low-level network protocol to BugTrap server.
 * @param pszHostName - server host name.
 * @param pTransferThreadParams - thread parameters.
 * @return error code.
 */
static DWORD WSASubmitReport(PCTSTR pszHostName, CTransferThreadParams* pTransferThreadParams)
{
	typedef BOOL (WINAPI *PFCancelIo)(HANDLE hFile); /// Type definition of pointer to CancelIO() function.
	HMODULE hKernelDll = GetModuleHandle(_T("KERNEL32.DLL"));
	PFCancelIo FCancelIo = hKernelDll ? (PFCancelIo)GetProcAddress(hKernelDll, "CancelIo") : NULL;
	HWND hwndSink = pTransferThreadParams->GetSinkWnd();

	DWORD dwErrorCode = ERROR_SUCCESS;
	HANDLE hFile = CreateFile(g_szInternalReportFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		const int nAppNameSize = sizeof(DWORD) + countof(g_szAppName) * sizeof(DWORD);
		const int nAppVersionSize = sizeof(DWORD) + countof(g_szAppVersion) * sizeof(DWORD);
		const int nReportFileExtensionSize = sizeof(DWORD) + 8 * sizeof(DWORD);
		const int nNotificationEMailSize = sizeof(DWORD) + countof(g_szNotificationEMail) * sizeof(DWORD);
		const int nMaxHeaderSize =
						sizeof(DWORD) +            // Protocol signature
						sizeof(DWORD) +            // Data size
						sizeof(BYTE) +             // Message type
						sizeof(DWORD) +            // Message flags
						nAppNameSize +             // Application name
						nAppVersionSize +          // Application version
						nReportFileExtensionSize + // Report file extension
						nNotificationEMailSize +   // Notification e-mail
						sizeof(BYTE);              // Report data type

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		int nBufferSize = max(nMaxHeaderSize, min(dwFileSize, g_dwMaxBufferSize));
		PBYTE pBuffer = new BYTE[nBufferSize];
		if (pBuffer)
		{
			WSADATA wd;
			dwErrorCode = WSAStartup(MAKEWORD(2, 0), &wd);
			if (dwErrorCode == ERROR_SUCCESS)
			{
				PCSTR pszHostNameA;
#ifdef _UNICODE
				CHAR szHostNameA[MAX_PATH];
				WideCharToMultiByte(CP_ACP, 0, pszHostName, -1, szHostNameA, countof(szHostNameA), NULL, NULL);
				pszHostNameA = szHostNameA;
#else
				pszHostNameA = pszHostName;
#endif

				SOCKADDR_IN sockAddr;
				ZeroMemory(&sockAddr, sizeof(sockAddr));
				if (*pszHostNameA >= '0' && *pszHostNameA <= '9')
					sockAddr.sin_addr.s_addr = inet_addr(pszHostNameA);
				else
				{
					hostent* pHostEnt = gethostbyname(pszHostNameA);
					if (pHostEnt != NULL)
						CopyMemory(&sockAddr.sin_addr, pHostEnt->h_addr, pHostEnt->h_length);
					else
						sockAddr.sin_addr.s_addr = INADDR_NONE;
				}
				if (sockAddr.sin_addr.s_addr != INADDR_NONE)
				{
					sockAddr.sin_family = AF_INET;
					sockAddr.sin_port = htons(g_nSupportPort);
					SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
					if (sock != INVALID_SOCKET)
					{
						CMemStream MemStream(256);
						CUTF8EncStream EncStream(&MemStream);
						BOOL bConnected = FALSE;
						HANDLE hCompletionEvent = pTransferThreadParams->GetCompletionEvent();
						// Implicitly put socket in non-blocking mode.
						if (WSAEventSelect(sock, hCompletionEvent, FD_CONNECT) == SOCKET_ERROR)
						{
							dwErrorCode = WSAGetLastError();
							goto end; // Internal error.
						}
						if (hwndSink)
							PostMessage(hwndSink, UM_CONNECTINGTOSERVER, 0, 0);
						if (connect(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
						{
							dwErrorCode = WaitForPendingSocketRequest(pTransferThreadParams);
							if (dwErrorCode != ERROR_SUCCESS)
								goto end;
							WSANETWORKEVENTS nev;
							if (WSAEnumNetworkEvents(sock, hCompletionEvent, &nev) == SOCKET_ERROR)
							{
								dwErrorCode = WSAGetLastError();
								goto end; // Internal error.
							}
							_ASSERTE(nev.lNetworkEvents & FD_CONNECT);
							dwErrorCode = nev.iErrorCode[FD_CONNECT_BIT];
							if (dwErrorCode != ERROR_SUCCESS)
								goto end; // Connection can't be established.
						}

						bConnected = TRUE;
						if (hwndSink)
							PostMessage(hwndSink, UM_SENDINGREPORT, 0, 0);

						// Cancel association and selection of network events.
						if (WSAEventSelect(sock, hCompletionEvent, 0) == SOCKET_ERROR)
						{
							dwErrorCode = WSAGetLastError();
							goto end; // Internal error.
						}

						int nHeaderPosition = 0;

						// Protocol signature.
						*(PDWORD)(pBuffer + nHeaderPosition) = g_dwProtocolSignature;
						nHeaderPosition += sizeof(DWORD);

						// Data size placeholder.
						*(PDWORD)(pBuffer + nHeaderPosition) = 0;
						nHeaderPosition += sizeof(DWORD);

						// Message type.
						*(PBYTE)(pBuffer + nHeaderPosition) = MT_COMPOUND_MESSAGE;
						nHeaderPosition += sizeof(BYTE);

						// Message flags.
						*(PDWORD)(pBuffer + nHeaderPosition) = CMF_NONE;
						nHeaderPosition += sizeof(DWORD);

						// Application name.
						if (! WriteBinaryString(EncStream, g_szAppName, pBuffer, nHeaderPosition, nBufferSize))
						{
							dwErrorCode = ERROR_INTERNAL_ERROR;
							goto end; // Internal error.
						}

						// Application version.
						if (! WriteBinaryString(EncStream, g_szAppVersion, pBuffer, nHeaderPosition, nBufferSize))
						{
							dwErrorCode = ERROR_INTERNAL_ERROR;
							goto end; // Internal error.
						}

						// Report file extension.
						PCTSTR pszReportFileExtension = CSymEngine::GetReportFileExtension();
						if (! WriteBinaryString(EncStream, pszReportFileExtension, pBuffer, nHeaderPosition, nBufferSize))
						{
							dwErrorCode = ERROR_INTERNAL_ERROR;
							goto end; // Internal error.
						}

						// Notification e-mail.
						if (! WriteBinaryString(EncStream, g_szNotificationEMail, pBuffer, nHeaderPosition, nBufferSize))
						{
							dwErrorCode = ERROR_INTERNAL_ERROR;
							goto end; // Internal error.
						}

						_ASSERTE(nHeaderPosition <= nBufferSize);
						// Store real data size.
						((PDWORD)pBuffer)[1] = nHeaderPosition + dwFileSize;

						WSAOVERLAPPED ov;
						ZeroMemory(&ov, sizeof(ov));
						ov.hEvent = hCompletionEvent;

						DWORD dwPacketSize = nHeaderPosition;
						for (;;)
						{
							WSABUF buf;
							buf.buf = (char*)pBuffer;
							buf.len = dwPacketSize;
							while (buf.len > 0)
							{
								DWORD dwProcessedNumber = 0;
								if (WSASend(sock, &buf, 1, &dwProcessedNumber, 0, &ov, NULL) == SOCKET_ERROR)
								{
									dwErrorCode = WaitForPendingSocketRequest(pTransferThreadParams);
									if (dwErrorCode != ERROR_SUCCESS)
										goto end;
									DWORD dwFlags;
									if (! WSAGetOverlappedResult(sock, &ov, &dwProcessedNumber, FALSE, &dwFlags))
									{
										dwErrorCode = WSAGetLastError();
										goto end; // Internal error.
									}
								}
								buf.buf += dwProcessedNumber;
								buf.len -= dwProcessedNumber;
							}
							if (! ReadFile(hFile, pBuffer, nBufferSize, &dwPacketSize, NULL) || dwPacketSize == 0)
							{
								if (hwndSink)
									PostMessage(hwndSink, UM_CHECKINGERRORSTATUS, 0, 0);
								goto end; // End of file.
							}
						}
end:
						if (dwErrorCode == ERROR_SUCCESS)
						{
							// Initiate graceful shutdown.
							shutdown(sock, SD_BOTH);
						}
						else
						{
							if (bConnected && FCancelIo)
							{
								// Cancel pending send operation.
								FCancelIo((HANDLE)sock);
							}
							// Initiate abortive shutdown.
							LINGER linger;
							linger.l_onoff = TRUE;
							linger.l_linger = 0;
							setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
						}
						closesocket(sock);
					}
				}
				WSACleanup();
			}
			delete[] pBuffer;
		}
		else
			dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
		CloseHandle(hFile);
	}
	else
		dwErrorCode = GetLastError();
	if (dwErrorCode != ERROR_SUCCESS)
		pTransferThreadParams->SetErrorCode(dwErrorCode);
	return dwErrorCode;
}

/**
 * Set error state and find system error description.
 * @param pTransferThreadParams - thread parameters.
 * @param dwErrorCode - error code.
 */
static void SetInternetErrorMessage(CTransferThreadParams* pTransferThreadParams, DWORD dwErrorCode)
{
	HANDLE hWinInetDll = GetModuleHandle(_T("wininet.dll"));
	PTSTR pszMessageBuffer = NULL;
	DWORD dwBaseMessageLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
	                                          hWinInetDll, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
	                                          (PTSTR)&pszMessageBuffer, 0, NULL);
	if (pszMessageBuffer == NULL)
	{
		pTransferThreadParams->SetErrorCode(dwErrorCode);
		return;
	}
	TrimSpaces(pszMessageBuffer);
	if (dwErrorCode == ERROR_INTERNET_EXTENDED_ERROR)
	{
		DWORD dwInetError, dwAdditionalMessageLength;
		if (InternetGetLastResponseInfo(&dwInetError, NULL, &dwAdditionalMessageLength))
		{
			++dwAdditionalMessageLength;
			const DWORD dwMessageDividerSize = 128;
			PTSTR pszNewMessageBuffer = (PTSTR)LocalReAlloc(pszMessageBuffer, (dwBaseMessageLength + dwMessageDividerSize + dwAdditionalMessageLength) * sizeof(TCHAR), LMEM_MOVEABLE);
			if (pszNewMessageBuffer != NULL)
			{
				pszMessageBuffer = pszNewMessageBuffer;
				pszNewMessageBuffer += dwBaseMessageLength;
				pszNewMessageBuffer += LoadString(g_hInstance, IDS_ADDITIONAL_INFORMATION, pszNewMessageBuffer, dwMessageDividerSize);
				InternetGetLastResponseInfo(&dwInetError, pszNewMessageBuffer, &dwAdditionalMessageLength);
				TrimSpaces(pszNewMessageBuffer);
			}
		}
	}
	pTransferThreadParams->SetErrorCode(dwErrorCode, pszMessageBuffer);
}

#ifdef _DEBUG

/**
 * Print Internet status debugging information.
 * @param dwInternetStatus - status code that indicates why the callback function is being called.
 * @param pvStatusInformation - pointer to additional status information.
 * @param dwStatusInformationLength - size of the data pointed to by @a pvStatusInformation.
 */
static void PrintInternetStatus(DWORD dwInternetStatus, PVOID pvStatusInformation, DWORD dwStatusInformationLength)
{
#define CONVERT_INTERNET_STATUS_TO_STRING(status) \
	case INTERNET_STATUS_##status: pszInternetStatus = _T(#status); break
#define CONVERT_INTERNET_STATE_TO_STRING(state) \
	case INTERNET_STATE_##state: pszInternetState = _T(#state); break

	PCTSTR pszInternetStatus, pszInternetState;
	TCHAR szInternetStatusBuffer[128], szInternetStateBuffer[128];

	switch (dwInternetStatus)
	{
	CONVERT_INTERNET_STATUS_TO_STRING(RESOLVING_NAME);
	CONVERT_INTERNET_STATUS_TO_STRING(NAME_RESOLVED);
	CONVERT_INTERNET_STATUS_TO_STRING(CONNECTING_TO_SERVER);
	CONVERT_INTERNET_STATUS_TO_STRING(CONNECTED_TO_SERVER);
	CONVERT_INTERNET_STATUS_TO_STRING(SENDING_REQUEST);
	CONVERT_INTERNET_STATUS_TO_STRING(REQUEST_SENT);
	CONVERT_INTERNET_STATUS_TO_STRING(RECEIVING_RESPONSE);
	CONVERT_INTERNET_STATUS_TO_STRING(RESPONSE_RECEIVED);
	CONVERT_INTERNET_STATUS_TO_STRING(CTL_RESPONSE_RECEIVED);
	CONVERT_INTERNET_STATUS_TO_STRING(PREFETCH);
	CONVERT_INTERNET_STATUS_TO_STRING(CLOSING_CONNECTION);
	CONVERT_INTERNET_STATUS_TO_STRING(CONNECTION_CLOSED);
	CONVERT_INTERNET_STATUS_TO_STRING(HANDLE_CREATED);
	CONVERT_INTERNET_STATUS_TO_STRING(HANDLE_CLOSING);
	CONVERT_INTERNET_STATUS_TO_STRING(DETECTING_PROXY);
	CONVERT_INTERNET_STATUS_TO_STRING(REQUEST_COMPLETE);
	CONVERT_INTERNET_STATUS_TO_STRING(REDIRECT);
	CONVERT_INTERNET_STATUS_TO_STRING(INTERMEDIATE_RESPONSE);
	CONVERT_INTERNET_STATUS_TO_STRING(USER_INPUT_REQUIRED);
	CONVERT_INTERNET_STATUS_TO_STRING(STATE_CHANGE);
	CONVERT_INTERNET_STATUS_TO_STRING(COOKIE_SENT);
	CONVERT_INTERNET_STATUS_TO_STRING(COOKIE_RECEIVED);
	CONVERT_INTERNET_STATUS_TO_STRING(PRIVACY_IMPACTED);
	CONVERT_INTERNET_STATUS_TO_STRING(P3P_HEADER);
	CONVERT_INTERNET_STATUS_TO_STRING(P3P_POLICYREF);
	CONVERT_INTERNET_STATUS_TO_STRING(COOKIE_HISTORY);
	default:
		_stprintf_s(szInternetStatusBuffer, countof(szInternetStatusBuffer), _T("0x%08lX"), dwInternetStatus);
		pszInternetStatus = szInternetStatusBuffer;
	}

	OutputDebugString(_T("INTERNET STATUS: "));
	OutputDebugString(pszInternetStatus);
	if (dwInternetStatus == INTERNET_STATUS_STATE_CHANGE)
	{
		_ASSERTE(dwStatusInformationLength == sizeof(DWORD));
		switch (*(PDWORD)pvStatusInformation)
		{
		CONVERT_INTERNET_STATE_TO_STRING(CONNECTED);
		CONVERT_INTERNET_STATE_TO_STRING(DISCONNECTED);
		CONVERT_INTERNET_STATE_TO_STRING(DISCONNECTED_BY_USER);
		CONVERT_INTERNET_STATE_TO_STRING(IDLE);
		CONVERT_INTERNET_STATE_TO_STRING(BUSY);
		default:
			_stprintf_s(szInternetStateBuffer, countof(szInternetStateBuffer), _T("0x%08lX"), *(PDWORD)pvStatusInformation);
			pszInternetState = szInternetStateBuffer;
		}
		OutputDebugString(_T(", INTERNET STATE: "));
		OutputDebugString(pszInternetState);
	}
	OutputDebugString(_T("\n"));

#undef CONVERT_INTERNET_STATUS_TO_STRING
#undef CONVERT_INTERNET_STATE_TO_STRING
}

#endif

/**
 * Application-defined status callback function.
 * @param hSession - handle for which the callback function is being called.
 * @param dwContext - pointer to a variable that specifies the application-defined context value associated with @a hSession.
 * @param dwInternetStatus - status code that indicates why the callback function is being called.
 * @param pvStatusInformation - pointer to additional status information.
 * @param dwStatusInformationLength - size of the data pointed to by @a pvStatusInformation.
 */
static void CALLBACK InternetStatusCallback(HINTERNET hSession, DWORD_PTR dwContext, DWORD dwInternetStatus, PVOID pvStatusInformation, DWORD dwStatusInformationLength)
{
	hSession; dwInternetStatus; pvStatusInformation; dwStatusInformationLength;
	CTransferThreadParams* pTransferThreadParams = (CTransferThreadParams*)dwContext;

#ifdef _DEBUG
	PrintInternetStatus(dwInternetStatus, pvStatusInformation, dwStatusInformationLength);
#endif

	HANDLE hCompletionEvent = pTransferThreadParams->GetCompletionEvent();
	switch (pTransferThreadParams->GetContext())
	{
	case CTransferThreadParams::IC_SENDREQUEST:
	case CTransferThreadParams::IC_WRITEFILE:
	case CTransferThreadParams::IC_ENDREQUEST:
	case CTransferThreadParams::IC_RECEIVINGRESPONSE:
		if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
		{
			_ASSERTE(dwStatusInformationLength == sizeof(INTERNET_ASYNC_RESULT));
			pTransferThreadParams->SetCallbackResult(((LPINTERNET_ASYNC_RESULT)pvStatusInformation)->dwError);
			SetEvent(hCompletionEvent);
		}
		else if (dwInternetStatus == INTERNET_STATUS_STATE_CHANGE)
		{
			pTransferThreadParams->SetContext(CTransferThreadParams::IC_ERROR);
			SetEvent(hCompletionEvent);
		}
		break;
	case CTransferThreadParams::IC_INTERNETCONNECT:
	case CTransferThreadParams::IC_OPENREQUEST:
	case CTransferThreadParams::IC_CLOSEHANDLE:
		break;
	default:
		_ASSERT(FALSE);
	}
}

/**
 * @brief Wait until pending operation is either complete or canceled.
 * @param pTransferThreadParams - thread parameters.
 * @return error code.
 */
static DWORD WaitForPendingInternetRequest(CTransferThreadParams* pTransferThreadParams)
{
	HANDLE hCancelationEvent = pTransferThreadParams->GetCancellationEvent();
	HANDLE hCompletionEvent = pTransferThreadParams->GetCompletionEvent();
	HANDLE arrEvents[] = { hCompletionEvent, hCancelationEvent };

	DWORD dwErrorCode = ERROR_SUCCESS;
	DWORD dwTemp = GetLastError();
	if (dwTemp != ERROR_IO_PENDING)
	{
		dwErrorCode = dwTemp;
		SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
	}
	else if (WaitForMultipleObjects(countof(arrEvents), arrEvents, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
	{
		dwErrorCode = ERROR_OPERATION_ABORTED;
		pTransferThreadParams->SetErrorCode(dwErrorCode);
	}
	else if (pTransferThreadParams->GetContext() == CTransferThreadParams::IC_ERROR)
	{
		dwErrorCode = ERROR_REQUEST_ABORTED;
	}
	else
	{
		dwErrorCode = pTransferThreadParams->GetCallbackResult();
		if (dwErrorCode != ERROR_SUCCESS)
			SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
	}

	return dwErrorCode;
}

/**
 * @brief Submit bug report through HTTP protocol to BugTrap server.
 * @param pszSupportUrl - support URL address.
 * @param pTransferThreadParams - thread parameters.
 * @return error code.
 */
static DWORD HTTPSubmitReport(PCTSTR pszSupportUrl, CTransferThreadParams* pTransferThreadParams)
{
#define MESSAGE_HEADER                    _T("Content-Type: multipart/form-data; boundary=") _T(SECTION_BOUNDARY)

#define SECTION_BOUNDARY                  "BUGTRAP-7A1D6378-1294-491B-996C-37D4FF91D184"

#define TEXT_SECTION_HEADER(name)         "--" SECTION_BOUNDARY "\r\n" \
                                          "Content-Disposition: form-data; name=\"" name "\"\r\n" \
                                          "Content-Type: text/plain; charset=\"utf-8\"\r\n" \
										  "Content-Transfer-Encoding: 8bit\r\n" \
                                          "\r\n"

#define FILE_SECTION_HEADER(name, file)   "--" SECTION_BOUNDARY "\r\n" \
                                          "Content-Disposition: form-data; name=\"" name "\"; filename=\"" file "\"\r\n" \
                                          "Content-Type: application/octet-stream\r\n" \
										  "Content-Transfer-Encoding: binary\r\n" \
                                          "\r\n"

#define SECTION_TRAILER                   "\r\n"

#define MESSAGE_TRAILER                   SECTION_TRAILER "--" SECTION_BOUNDARY "--\r\n\r\n"

	HWND hwndSink = pTransferThreadParams->GetSinkWnd();
	// Call dial-up dialog box to initiate a connection to the Internet
	// or check if a connection already exists.
	DWORD dwErrorCode = InternetAttemptConnect(0);
	if (dwErrorCode == ERROR_SUCCESS)
	{
		HANDLE hFile = CreateFile(g_szInternalReportFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwFileSize = GetFileSize(hFile, NULL);
			DWORD nBufferSize = min(dwFileSize, g_dwMaxBufferSize);
			PBYTE pBuffer = new BYTE[nBufferSize];
			if (pBuffer != NULL)
			{
				TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH],
				      szPassword[INTERNET_MAX_PASSWORD_LENGTH],
				      szHostName[INTERNET_MAX_HOST_NAME_LENGTH],
				      szUrlPath[INTERNET_MAX_PATH_LENGTH];
				URL_COMPONENTS urlComponents;
				ZeroMemory(&urlComponents, sizeof(urlComponents));
				urlComponents.dwStructSize = sizeof(urlComponents);
				urlComponents.lpszHostName = szHostName;
				urlComponents.dwHostNameLength = countof(szHostName);
				urlComponents.lpszUserName = szUserName;
				urlComponents.dwUserNameLength = countof(szUserName);
				urlComponents.lpszPassword = szPassword;
				urlComponents.dwPasswordLength = countof(szPassword);
				urlComponents.lpszUrlPath = szUrlPath;
				urlComponents.dwUrlPathLength = countof(szUrlPath);
				if (InternetCrackUrl(pszSupportUrl, 0, ICU_DECODE | ICU_ESCAPE, &urlComponents))
				{
					if (urlComponents.nScheme == INTERNET_SCHEME_HTTP)
					{
#ifdef _DEBUG
						OutputDebugString(_T("InternetOpen()\n"));
#endif
						HINTERNET hSession = InternetOpen(BUGTRAP_TITLE, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
						if (hSession != NULL)
						{
							InternetSetStatusCallback(hSession, InternetStatusCallback);
							INTERNET_PORT nPort = urlComponents.nPort ? urlComponents.nPort : INTERNET_DEFAULT_HTTP_PORT;
							PCTSTR pszUserName = *szUserName ? szUserName : NULL;
							PCTSTR pszPassword = *szPassword ? szPassword : NULL;

							pTransferThreadParams->SetContext(CTransferThreadParams::IC_INTERNETCONNECT);
#ifdef _DEBUG
							OutputDebugString(_T("InternetConnect()\n"));
#endif
							HINTERNET hConnection = InternetConnect(hSession, szHostName, nPort, pszUserName, pszPassword, INTERNET_SERVICE_HTTP, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_UI, (DWORD_PTR)pTransferThreadParams);
							if (hConnection)
							{
								pTransferThreadParams->SetContext(CTransferThreadParams::IC_OPENREQUEST);
#ifdef _DEBUG
								OutputDebugString(_T("HttpOpenRequest()\n"));
#endif
								HINTERNET hRequest = HttpOpenRequest(hConnection, _T("POST"), szUrlPath, NULL, NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_UI, (DWORD_PTR)pTransferThreadParams);
								if (hRequest)
								{
									CMemStream MemStream(4 * 1024);
									CUTF8EncStream EncStream(&MemStream);
									CHAR szTemp[16];

									EncStream.WriteAscii(TEXT_SECTION_HEADER("protocolSignature"));
									EncStream.WriteBytes((const BYTE*)&g_dwProtocolSignature, sizeof(g_dwProtocolSignature));
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("messageType"));
									_itoa_s(MT_COMPOUND_MESSAGE, szTemp, countof(szTemp), 10);
									EncStream.WriteAscii(szTemp);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("messageFlags"));
									_itoa_s(CMF_NONE, szTemp, countof(szTemp), 10);
									EncStream.WriteAscii(szTemp);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("appName"));
									EncStream.WriteUTF8Bin(g_szAppName);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("appVersion"));
									EncStream.WriteUTF8Bin(g_szAppVersion);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("reportFileExtension"));
									PCTSTR pszReportFileExtension = CSymEngine::GetReportFileExtension();
									EncStream.WriteUTF8Bin(pszReportFileExtension);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(TEXT_SECTION_HEADER("notificationEMail"));
									EncStream.WriteUTF8Bin(g_szNotificationEMail);
									EncStream.WriteAscii(SECTION_TRAILER);

									EncStream.WriteAscii(FILE_SECTION_HEADER("reportData", "report.dat"));

									static const TCHAR szHeader[] = MESSAGE_HEADER;
									const DWORD dwHeaderLength = countof(szHeader) - 1;
									static const CHAR szTrailer[] = MESSAGE_TRAILER;
									const DWORD dwTrailerLength = countof(szTrailer) - 1;

									const BYTE* pFormData = MemStream.GetBuffer();
									if (pFormData != NULL)
									{
										DWORD dwFormDataLength = MemStream.GetLength();
										_ASSERTE(dwFormDataLength > 0);

										INTERNET_BUFFERS InetBuf;
										ZeroMemory(&InetBuf, sizeof(InetBuf));
										InetBuf.dwStructSize = sizeof(InetBuf);
										InetBuf.dwHeadersTotal = dwHeaderLength;
										InetBuf.dwHeadersLength = dwHeaderLength;
										InetBuf.lpcszHeader = szHeader;
										InetBuf.dwBufferTotal = dwFormDataLength + dwFileSize + dwTrailerLength;

										HANDLE hCompletionEvent = pTransferThreadParams->GetCompletionEvent();
										if (hwndSink)
											PostMessage(hwndSink, UM_CONNECTINGTOSERVER, 0, 0);
										pTransferThreadParams->SetContext(CTransferThreadParams::IC_SENDREQUEST);
										ResetEvent(hCompletionEvent);
#ifdef _DEBUG
										OutputDebugString(_T("HttpSendRequestEx()\n"));
#endif
										if (! HttpSendRequestEx(hRequest, &InetBuf, NULL, 0, 456))
											dwErrorCode = WaitForPendingInternetRequest(pTransferThreadParams);

										if (dwErrorCode == ERROR_SUCCESS)
										{
											if (hwndSink)
												PostMessage(hwndSink, UM_SENDINGREPORT, 0, 0);
											pTransferThreadParams->SetContext(CTransferThreadParams::IC_WRITEFILE);
											const BYTE* pBytes = pFormData;
											DWORD dwNumBytes = dwFormDataLength;
											BOOL bDataEnd = FALSE;
											for (;;)
											{
												ResetEvent(hCompletionEvent);
#ifdef _DEBUG
												OutputDebugString(_T("InternetWriteFile()\n"));
#endif
												while (dwNumBytes > 0)
												{
													DWORD dwBytesWritten = 0;
													if (! InternetWriteFile(hRequest, pBytes, dwNumBytes, &dwBytesWritten))
													{
														dwErrorCode = WaitForPendingInternetRequest(pTransferThreadParams);
														if (dwErrorCode != ERROR_SUCCESS)
														{
															bDataEnd = TRUE;
															break;
														}
													}
													pBytes += dwBytesWritten;
													dwNumBytes -= dwBytesWritten;
												}
												if (bDataEnd)
													break;
												DWORD dwBytesRead = 0;
												if (! ReadFile(hFile, pBuffer, nBufferSize, &dwBytesRead, NULL) || dwBytesRead == 0)
												{
													bDataEnd = TRUE;
													pBytes = (const BYTE*)szTrailer;
													dwNumBytes = dwTrailerLength;
												}
												else
												{
													pBytes = pBuffer;
													dwNumBytes = dwBytesRead;
												}
											}

											if (dwErrorCode == ERROR_SUCCESS)
											{
												if (hwndSink)
													PostMessage(hwndSink, UM_CHECKINGERRORSTATUS, 0, 0);
												pTransferThreadParams->SetContext(CTransferThreadParams::IC_ENDREQUEST);
												ResetEvent(hCompletionEvent);
#ifdef _DEBUG
												OutputDebugString(_T("HttpEndRequest()\n"));
#endif
												if (! HttpEndRequest(hRequest, NULL, 0, NULL))
													dwErrorCode = WaitForPendingInternetRequest(pTransferThreadParams);
#ifdef _DEBUG
												if (dwErrorCode == ERROR_SUCCESS)
												{
													OutputDebugString(_T("WEB RESPONSE:\n========================================\n"));
													pTransferThreadParams->SetContext(CTransferThreadParams::IC_RECEIVINGRESPONSE);
													for (;;)
													{
														CHAR arrBuffer[1024];
														DWORD dwBytesRead = 0;
														if (! InternetReadFile(hRequest, arrBuffer, sizeof(arrBuffer) - 1, &dwBytesRead))
														{
															dwErrorCode = GetLastError();
															if (dwErrorCode != ERROR_IO_PENDING)
																break;
															else
															{
																ResetEvent(hCompletionEvent);
																dwErrorCode = WaitForPendingInternetRequest(pTransferThreadParams);
																if (dwErrorCode != ERROR_SUCCESS)
																	break;
															}
														}
														if (dwBytesRead == 0)
															break;
														arrBuffer[dwBytesRead] = '\0';
														OutputDebugStringA(arrBuffer);
													}
													OutputDebugString(_T("\n========================================\n"));
												}
#endif
											}
										}
									}
									else
									{
										dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
										pTransferThreadParams->SetErrorCode(dwErrorCode);
									}
									pTransferThreadParams->SetContext(CTransferThreadParams::IC_CLOSEHANDLE);
									InternetCloseHandle(hRequest);
								}
								else
								{
									dwErrorCode = GetLastError(); // HttpOpenRequest() failed
									SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
								}
								pTransferThreadParams->SetContext(CTransferThreadParams::IC_CLOSEHANDLE);
								InternetCloseHandle(hConnection);
							}
							else
							{
								dwErrorCode = GetLastError(); // InternetConnect() failed
								SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
							}
							pTransferThreadParams->SetContext(CTransferThreadParams::IC_CLOSEHANDLE);
							InternetCloseHandle(hSession);
						}
						else
						{
							dwErrorCode = GetLastError(); // InternetOpen() failed
							SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
						}
					}
					else
					{
						dwErrorCode = ERROR_BAD_NET_NAME;
						pTransferThreadParams->SetErrorCode(dwErrorCode);
					}
				}
				else
				{
					dwErrorCode = GetLastError(); // InternetCrackUrl() failed
					SetInternetErrorMessage(pTransferThreadParams, dwErrorCode);
				}
				delete[] pBuffer;
			}
			else
			{
				dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
				pTransferThreadParams->SetErrorCode(dwErrorCode);
			}
			CloseHandle(hFile);
		}
		else
		{
			dwErrorCode = GetLastError(); // CreateFile() failed
			pTransferThreadParams->SetErrorCode(dwErrorCode);
		}
	}
	else
		SetInternetErrorMessage(pTransferThreadParams, dwErrorCode); // InternetAttemptConnect() failed

	return dwErrorCode;

#undef MESSAGE_HEADER
#undef SECTION_BOUNDARY
#undef TEXT_SECTION_HEADER
#undef SECTION_TRAILER
#undef MESSAGE_TRAILER
}

/**
 * @brief Thread function responsible for sending report to the server.
 * @param pParam - thread parameter (pointer to CTransferThreadParams).
 * @return error code.
 */
static UINT CALLBACK TransferThreadProc(PVOID pParam)
{
	__try
	{
		CTransferThreadParams* pTransferThreadParams = (CTransferThreadParams*)pParam;
		_ASSERTE(pTransferThreadParams != NULL);
		DWORD dwErrorCode = ERROR_SUCCESS;
		if (*g_szSupportHost == _T('\0'))
			dwErrorCode = ERROR_BAD_NETPATH;
		if (dwErrorCode != ERROR_SUCCESS)
			pTransferThreadParams->SetErrorCode(dwErrorCode);
		if (dwErrorCode == ERROR_SUCCESS)
		{
			static const TCHAR szHttpPrefix[] = _T("http://");
			static const int nHttpPrefixLength = countof(szHttpPrefix) - 1;
			if (_tcsnicmp(g_szSupportHost, szHttpPrefix, nHttpPrefixLength) == 0)
				dwErrorCode = HTTPSubmitReport(g_szSupportHost, pTransferThreadParams);
			else
				dwErrorCode = WSASubmitReport(g_szSupportHost, pTransferThreadParams);
		}
		pTransferThreadParams->PostCompletionMessage();
		return dwErrorCode;
	}
	__except (InternalFilter(GetExceptionInformation()))
	{
		return ERROR_INTERNAL_ERROR;
	}
}

/**
 * @brief Start transfer thread.
 * @param pTransferThreadParams - thread parameters.
 */
HANDLE StartTransferThread(CTransferThreadParams* pTransferThreadParams)
{
	return (HANDLE)_beginthreadex(NULL, 0, TransferThreadProc, pTransferThreadParams, 0, NULL);
}

/**
 * Close transfer thread handle and free used memory.
 * @param hTransferThread - thread handle.
 */
void CloseTransferThread(HANDLE hTransferThread)
{
	if (hTransferThread)
	{
		WaitForSingleObject(hTransferThread, INFINITE);
		CloseHandle(hTransferThread);
	}
}

/**
 * @brief Submit bug report through network protocol.
 * @param hwndParent - parent window handle (valid if user interface is enabled).
 */
void SubmitReport(HWND hwndParent)
{
	if (*g_szSupportHost && g_nSupportPort)
	{
		if (g_eActivityType == BTA_SHOWUI)
		{
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_TRANSFERPROGRESS_DLG), hwndParent, TransferProgressDlgProc);
			EndDialog(hwndParent, FALSE);
		}
		else
		{
			CTransferThreadParams TransferThreadParams;
			HANDLE hTransferThread = StartTransferThread(&TransferThreadParams);
			CloseTransferThread(hTransferThread);
		}
	}
}

/**
 * @brief Initialize introduction message.
 * @param hwnd - parent window handle.
 * @param hlURL - hyper link control.
 */
void InitIntro(HWND hwnd, CHyperLink& hlURL)
{
	HWND hwndCtl;

	if (g_pResManager->m_hDialogIcon)
	{
		HWND hwndDialogIcon = GetDlgItem(hwnd, IDC_DIALOG_ICON);
		SendMessage(hwndDialogIcon, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_pResManager->m_hDialogIcon);
	}

	if (*g_szAppName)
		SetWindowText(hwnd, g_szAppName);

	if (! g_strFirstIntroMesage.IsEmpty())
	{
		hwndCtl = GetDlgItem(hwnd, IDC_INTRO1);
		SetWindowText(hwndCtl, g_strFirstIntroMesage);
	}

	if (! g_strSecondIntroMesage.IsEmpty())
	{
		hwndCtl = GetDlgItem(hwnd, IDC_INTRO2);
		SetWindowText(hwndCtl, g_strSecondIntroMesage);
	}

	if (*g_szSupportURL)
	{
		hwndCtl = GetDlgItem(hwnd, IDC_URL);
		SetWindowText(hwndCtl, g_szSupportURL);
		hlURL.SetLinkURL(g_szSupportURL);
		hlURL.Attach(hwndCtl);
	}
	else
	{
		hwndCtl = GetDlgItem(hwnd, IDC_URL);
		ShowWindow(hwndCtl, SW_HIDE);
		hwndCtl = GetDlgItem(hwnd, IDC_URL_PREFIX);
		ShowWindow(hwndCtl, SW_HIDE);
	}

	_ASSERTE(g_pResManager != NULL);
	if (g_pResManager->m_hBigAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_pResManager->m_hBigAppIcon);
	if (g_pResManager->m_hSmallAppIcon)
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_pResManager->m_hSmallAppIcon);

	CenterWindow(hwnd, GetParent(hwnd));
}

/**
 * @brief Initialize About menu item.
 * @param hwnd - parent window handle.
 */
void InitAbout(HWND hwnd)
{
	// IDM_ABOUTBOX must be in the system command range.
	_ASSERTE((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	_ASSERTE(IDM_ABOUTBOX < 0xF000);

	HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
	if (hSysMenu != NULL)
	{
		TCHAR szAboutMenu[64];
		LoadString(g_hInstance, IDS_ABOUT_ITEM, szAboutMenu, countof(szAboutMenu));
		AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hSysMenu, MF_STRING, IDM_ABOUTBOX, szAboutMenu);
	}
}

/**
 * @brief Perform BugTrap action (show dialog, submit report, etc.).
 */
static void ExecuteHandlerAction(void)
{
	TCHAR szReportFileName[MAX_PATH];
	g_pSymEngine->GetReportFileName(szReportFileName, countof(szReportFileName));
	if (g_eActivityType == BTA_SAVEREPORT)
	{
		// Use user defined report path.
		_tcscpy_s(g_szInternalReportFilePath, countof(g_szInternalReportFilePath), BT_GetReportFilePath());
		CreateFolder(g_szInternalReportFilePath);
		PathAppend(g_szInternalReportFilePath, szReportFileName);
		// Store report files to user supplied location.
		g_pSymEngine->WriteReport(g_szInternalReportFilePath, g_pEnumProc);
		// Clear global variables.
		*g_szInternalReportFilePath = _T('\0');
	}
	else
	{
		BOOL bResult;
		{
			CWaitDialog wait;
			if (g_eActivityType == BTA_SHOWUI)
				wait.BeginWait(NULL);
			// Generate full report file name.
			GetTempPath(countof(g_szInternalReportFolder), g_szInternalReportFolder);
			PathCombine(g_szInternalReportFilePath, g_szInternalReportFolder, szReportFileName);
			if (g_dwFlags & BTF_DETAILEDMODE)
			{
				// Generate report files in temporary location.
				CreateTempFolder(g_szInternalReportFolder, countof(g_szInternalReportFolder));
				bResult = g_pSymEngine->WriteReportFiles(g_szInternalReportFolder, g_pEnumProc) &&
				          g_pSymEngine->ArchiveReportFiles(g_szInternalReportFolder, g_szInternalReportFilePath);
			}
			else
			{
				// Create single log file in system temporary folder.
				bResult = g_pSymEngine->WriteLog(g_szInternalReportFilePath, g_pEnumProc);
			}
		}
		if (bResult)
		{
			// Process remaining actions.
			switch (g_eActivityType)
			{
			case BTA_MAILREPORT:
				if (*g_szSupportEMail)
				{
					TCHAR szSubject[MAX_PATH];
					GetDefaultMailSubject(szSubject, countof(szSubject));
					SendEMail(NULL, szSubject, NULL);
				}
				break;
			case BTA_SENDREPORT:
				if (*g_szSupportHost && g_nSupportPort)
					SubmitReport(NULL);
				break;
			case BTA_SHOWUI:
				if (g_pResManager)
				{
					if (g_dwFlags & BTF_SHOWADVANCEDUI)
						DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MAIN_DLG), NULL, MainDlgProc);
					else if (DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_SIMPLE_DLG), NULL, SimpleDlgProc) == TRUE)
						DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_MAIN_DLG), NULL, MainDlgProc);
				}
				break;
			}
		}
		// Remove temporarily generated report files.
		if (g_dwFlags & BTF_DETAILEDMODE)
		{
			DeleteFolder(g_szInternalReportFolder);
			*g_szInternalReportFolder = _T('\0');
		}
		DeleteFile(g_szInternalReportFilePath);
		*g_szInternalReportFilePath = _T('\0');
	}
}

/**
 * @brief Thread function responsible for processing BugTrap actions in background.
 * @param pParam - thread parameter.
 * @return error code.
 */
static UINT CALLBACK HandlerThreadProc(PVOID pParam)
{
	pParam;
	__try
	{
		ExecuteHandlerAction();
	}
	__except (InternalFilter(GetExceptionInformation()))
	{
	}
	return 0;
}

/**
 * @brief Get application window.
 * @return application window handle.
 */
static HWND GetAppWindow(void)
{
	HWND hwndParent = GetActiveWindow();
	if (hwndParent == NULL)
		hwndParent = GetForegroundWindow();
	if (hwndParent != NULL)
	{
		DWORD dwProcessID = 0;
		GetWindowThreadProcessId(hwndParent, &dwProcessID);
		if (dwProcessID == GetCurrentProcessId())
		{
			HWND hwndNext;
			for (;;)
			{
				hwndNext = GetParent(hwndParent);
				if (hwndNext != NULL)
					hwndParent = hwndNext;
				else
					break;
			}
		}
		else
			hwndParent = NULL;
	}
	return hwndParent;
}

/**
 * @brief Hide application window.
 * @param hwndParent - application window handle.
 */
static bool is_main_thread( HWND hwndParent )
{
	DWORD dwProcessID = 0;
	return ( GetWindowThreadProcessId( hwndParent, &dwProcessID ) == GetCurrentThreadId() );
}

static void HideAppWindow(HWND hwndParent)
{

	if (hwndParent != NULL) {
		__try {
			if( is_main_thread( hwndParent ) )
			{
				ShowWindow(hwndParent, SW_HIDE );//SW_FORCEMINIMIZE //SW_HIDE
			}
			else
			{
				// так у нас из второго потока  в полноэкранном режиме вывести диалог и не получилость
				// закрываем процесс чтобы не зависнуть - в xrDebugNew теперь дамп сохраняется всегда
				TerminateProcess	(GetCurrentProcess(),1);
/*
				HANDLE h = OpenThread( THREAD_ALL_ACCESS, FALSE, GetWindowThreadProcessId( hwndParent, NULL ) );
				TerminateThread( h, DWORD(-1) );
				CloseHandle( h );
				//TerminateThread GetCurrentThread()
				//THREAD_ALL_ACCESS
				ShowWindow( hwndParent, SW_FORCEMINIMIZE );
				// ShowWindow(hwnd, nCmdShow);
				 //UpdateWindow(hwnd);
				//_endthreadex //HANDLE GetCurrentThread() //HANDLE GetCurrentThreadId() //AttachThreadInput //GetWindowThreadProcessId
				//TerminateThread DuplicateHandle
*/
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			// ignore any exception in broken app...
		}
	}

}

/**
 * @brief Perform BugTrap action (show dialog, submit report, etc.).
 */

void StartHandlerThread(void)
{
	_ASSERTE(g_pSymEngine != NULL && g_pEnumProc != NULL);
	if (g_pSymEngine == NULL || g_pEnumProc == NULL)
		return;
	// Execute BugTrap action.
	HWND hwndParent;
	if (g_eActivityType == BTA_SHOWUI)
	{
		hwndParent = GetAppWindow();
		//pass NULL to prevent calling SendMessage to main window and resulting hanging secondary threads calls
		g_pResManager = new CResManager( NULL );
		//_endthreadex //HANDLE GetCurrentThread() //HANDLE GetCurrentThreadId() //AttachThreadInput //GetWindowThreadProcessId
		HideAppWindow( hwndParent );

	}
	else
		hwndParent = NULL;
	HANDLE hHandlerThread = (HANDLE)_beginthreadex(NULL, 0, HandlerThreadProc, NULL, 0, NULL);
	if (hHandlerThread != NULL)
	{
		WaitForSingleObject(hHandlerThread, INFINITE);
		CloseHandle(hHandlerThread);
	}
}

/**
 * Internal unhandled exception handler.
 * @param pExceptionPointers - pointer to the exception information.
 * @return determines how the exception is handled.
 */
LONG InternalFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
	__try
	{
		SetUnhandledExceptionFilter(NULL);
		static const TCHAR szDbgHelDll[] = _T("DBGHELP.DLL");
		TCHAR szDbgHelpPath[MAX_PATH];
		GetModuleFileName(g_hInstance, szDbgHelpPath, countof(szDbgHelpPath));
		PathRemoveFileSpec(szDbgHelpPath);
		PathAppend(szDbgHelpPath, szDbgHelDll);
		HINSTANCE hDbgHelpDll = LoadLibrary(szDbgHelpPath);
		if (hDbgHelpDll == NULL)
			hDbgHelpDll = LoadLibrary(szDbgHelDll);
		if (hDbgHelpDll != NULL)
		{
			PFMiniDumpWriteDump FMiniDumpWriteDump = (PFMiniDumpWriteDump)GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
			if (FMiniDumpWriteDump != NULL)
			{
				TCHAR szDumpFileName[MAX_PATH];
				GetTempPath(countof(szDumpFileName), szDumpFileName);
				PathAppend(szDumpFileName, _T("BugTrap-") VERSION_STRING _T(".dmp"));
				HANDLE hFile = CreateFile(szDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					HANDLE hProcess = GetCurrentProcess();
					DWORD dwProcessID = GetCurrentProcessId();
					MINIDUMP_EXCEPTION_INFORMATION ExInfo;
					ExInfo.ThreadId = GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionPointers;
					ExInfo.ClientPointers = TRUE;
					BOOL bResult = FMiniDumpWriteDump(hProcess, dwProcessID, hFile, MiniDumpWithDataSegs, &ExInfo, NULL, NULL);
					CloseHandle(hFile);
					if (! bResult)
						DeleteFile(szDumpFileName);
				}
			}
			FreeLibrary(hDbgHelpDll);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// any possible nested exception is not handled
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

/** @} */
