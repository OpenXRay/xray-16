/*
 * This is a part of the BugTrap package.
 * Copyright (c) 1999 by PJ Naughter.
 * All rights reserved.
 *
 * Description: Defines the wrapper class for sending an email using simple MAPI
 * Updated by: Maksim Pyatkovskiy.
 * Note: Based on code developed by PJ Naughter.
 * Downloaded from: http://www.codeproject.com/internet/cmapi.asp
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "CMapi.h"
#include "BugTrapUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMapiSession::CMapiSession(void)
{
	ZeroMemory(this, sizeof(*this));
	Initialise();
}

CMapiSession::~CMapiSession(void)
{
	// Logoff if logged on
	Logoff();
	// Unload the MAPI dll
	Deinitialise();
}

void CMapiSession::Initialise(void)
{
	// First make sure the "WIN.INI" entry for MAPI is present as well
	// as the MAPI32 dll being present on the system
	if (GetProfileInt(_T("MAIL"), _T("MAPI"), 0) != 0)
	{
		// Load up the MAPI dll and get the function pointers we are interested in
		m_hMapi = LoadLibrary(_T("MAPI32.DLL"));
		if (m_hMapi)
		{
			m_pfnMAPILogon = (LPMAPILOGON) GetProcAddress(m_hMapi, "MAPILogon");
			m_pfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress(m_hMapi, "MAPILogoff");
			m_pfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress(m_hMapi, "MAPISendMail");
			m_pfnMAPIResolveName = (LPMAPIRESOLVENAME) GetProcAddress(m_hMapi, "MAPIResolveName");
			m_pfnMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress(m_hMapi, "MAPIFreeBuffer");

			// If any of the functions are not installed then fail the load
			if (m_pfnMAPILogon == NULL ||
				m_pfnMAPILogoff == NULL ||
				m_pfnMAPISendMail == NULL ||
				m_pfnMAPIResolveName == NULL ||
				m_pfnMAPIFreeBuffer == NULL)
				Deinitialise();
		}
	}
}

void CMapiSession::Deinitialise(void)
{
	if (m_hMapi)
	{
		// Unload the MAPI dll and reset the function pointers to NULL
		FreeLibrary(m_hMapi);
		ZeroMemory(this, sizeof(*this));
	}
}

/**
 * @return true if operation has been completed successfully.
 */
BOOL CMapiSession::Logon(void)
{
	if (! MapiInstalled())
		return FALSE;

	// Just in case we are already logged in
	Logoff();

	// First try to acquire a new MAPI session using the supplied settings using the MAPILogon() function
	m_ulLastError = m_pfnMAPILogon(0, NULL, NULL, 0, 0, &m_hSession);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
	return (m_ulLastError == SUCCESS_SUCCESS);
}

/**
 * @param pszProfileName - profile name.
 * @param pszPassword - profile password.
 * @param bNewSession - pass true to open new session instead of attempting to re-use existing shared session.
 * @return true if operation has been completed successfully.
 */
BOOL CMapiSession::Logon(PCTSTR pszProfileName, PCTSTR pszPassword, BOOL bNewSession)
{
	if (! MapiInstalled() || pszProfileName == NULL || *pszProfileName == _T('\0'))
		return FALSE;

	// Just in case we are already logged in
	Logoff();

	// Initialize flags parameter used in the MAPILogon() call
	FLAGS flags = 0;
	if (bNewSession)
		flags |= MAPI_NEW_SESSION;

	// Get the ASCII versions of the profile name and password
	PCSTR pszProfileName2, pszPassword2;
#ifdef _UNICODE
	CHAR szProfileNameA[MAX_PATH], szPasswordA[MAX_PATH];
	if (pszProfileName)
		WideCharToMultiByte(CP_ACP, 0, pszProfileName, -1, szProfileNameA, countof(szProfileNameA), NULL, NULL);
	else
		*szProfileNameA = '\0';
	if (pszPassword)
		WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, szPasswordA, countof(szPasswordA), NULL, NULL);
	else
		*szPasswordA = '\0';
	pszProfileName2 = szProfileNameA;
	pszPassword2 = szPasswordA;
#else
	pszProfileName2 = pszProfileName;
	pszPassword2 = pszPassword;
#endif

	// First try to acquire a new MAPI session using the supplied settings using the MAPILogon() function
	m_ulLastError = m_pfnMAPILogon(0, (PSTR)pszProfileName2, (PSTR)pszPassword2, flags, 0, &m_hSession);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
	return (m_ulLastError == SUCCESS_SUCCESS);
}

/**
 * @param hwndParent - parent window if you want to show the UI.
 * @param bNewSession - pass true to open new session instead of attempting to re-use existing shared session.
 * @return true if operation has been completed successfully.
 */
BOOL CMapiSession::Logon(HWND hwndParent, BOOL bNewSession)
{
	if (! MapiInstalled())
		return FALSE;

	// Just in case we are already logged in
	Logoff();

	// Initialize flags parameter used in the MAPILogon() call
	FLAGS flags = MAPI_LOGON_UI;
	if (bNewSession)
		flags |= MAPI_NEW_SESSION;

	// First try to acquire a new MAPI session using the supplied settings using the MAPILogon() function
	m_ulLastError = m_pfnMAPILogon((ULONG)hwndParent, NULL, NULL, flags, 0, &m_hSession);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
	return (m_ulLastError == SUCCESS_SUCCESS);
}

/**
 * @return true if operation has been completed successfully.
 */
BOOL CMapiSession::Logoff(void)
{
	if (! MapiInstalled())
		return FALSE;

	if (m_hSession)
	{
		// Call the MAPILogoff() function
		m_ulLastError = m_pfnMAPILogoff(m_hSession, 0, 0, 0);
		_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
		m_hSession = NULL;
	}

	return TRUE;
}

/**
 * @param strName - resolved name.
 * @param lpRecip - recipient information.
 * @return true if operation has been complete successfully.
 */
BOOL CMapiSession::Resolve(const CStrHolder& strName, lpMapiRecipDesc& lpRecip)
{
	if (! MapiInstalled() || ! LoggedOn() || strName.IsEmpty())
		return FALSE;

#ifdef _UNICODE
	CHAR szNameA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, strName, -1, szNameA, countof(szNameA), NULL, NULL);
	PCSTR pszName = szNameA;
#else
	PCSTR pszName = strName;
#endif

	m_ulLastError = m_pfnMAPIResolveName(m_hSession, 0, (PSTR)pszName, 0, 0, &lpRecip);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
	return (m_ulLastError == SUCCESS_SUCCESS);
}

/**
 * @param pBuffer - pointer to de-allocated buffer.
 * @return true if operation has been complete successfully.
 */
BOOL CMapiSession::FreeBuffer(PVOID pBuffer)
{
	if (! MapiInstalled())
		return FALSE;
	if (pBuffer == NULL)
		return TRUE;
	m_ulLastError = m_pfnMAPIFreeBuffer(pBuffer);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);
	return (m_ulLastError == SUCCESS_SUCCESS);
}

/**
 * @param ulRecipClass - kind of recipient.
 * @param rRecipDesc - recipient information.
 * @param strName - recipient name.
 */
void CMapiSession::InitRecipient(ULONG ulRecipClass, MapiRecipDesc& rRecipDesc, const CStrHolder& strName)
{
	PCTSTR pszName = strName;
	// Set recipient type
	rRecipDesc.ulRecipClass = ulRecipClass;
	PCTSTR pszBracket1, pszBracket2;
	// John Smith <jsmith@aol.com>
	if ((pszBracket1 = _tcschr(pszName, _T('<'))) != NULL &&
		(pszBracket2 = _tcschr(pszBracket1 + 1, _T('>'))) != NULL)
	{
		// Copy name
		int nLength = pszBracket1 - pszName;
#ifdef _UNICODE
		int nLengthA = WideCharToMultiByte(CP_ACP, 0, pszName, nLength, NULL, 0, NULL, NULL);
		rRecipDesc.lpszName = new CHAR[nLengthA + 1];
#else
		rRecipDesc.lpszName = new CHAR[nLength + 1];
#endif
		if (rRecipDesc.lpszName)
		{
#ifdef _UNICODE
			WideCharToMultiByte(CP_ACP, 0, pszName, nLength, rRecipDesc.lpszName, nLengthA, NULL, NULL);
			rRecipDesc.lpszName[nLengthA] = '\0';
#else
			strcpy_s(rRecipDesc.lpszName, nLength + 1, pszName);
#endif
			TrimSpacesA(rRecipDesc.lpszName);
		}
		// Copy address
		PCTSTR pszAddress = pszBracket1 + 1;
		nLength = pszBracket2 - pszAddress;
#ifdef _UNICODE
		nLengthA = WideCharToMultiByte(CP_ACP, 0, pszAddress, nLength, NULL, 0, NULL, NULL);
		rRecipDesc.lpszAddress = new CHAR[nLengthA + 1];
#else
		rRecipDesc.lpszAddress = new CHAR[nLength + 1];
#endif
		if (rRecipDesc.lpszAddress)
		{
#ifdef _UNICODE
			WideCharToMultiByte(CP_ACP, 0, pszAddress, nLength, rRecipDesc.lpszAddress, nLengthA, NULL, NULL);
			rRecipDesc.lpszAddress[nLengthA] = '\0';
#else
			strcpy_s(rRecipDesc.lpszAddress, nLength + 1, pszAddress);
#endif
			TrimSpacesA(rRecipDesc.lpszAddress);
		}
	}
	else
	{
		int nSize;
		// Try to resolve the name
		lpMapiRecipDesc lpTempRecip = NULL;
		if (_tcschr(pszName, _T('@')) == NULL &&
			Resolve(strName, lpTempRecip))
		{
			// Resolve worked, put the resolved name/address back into the array
			if (lpTempRecip->lpszName)
			{
				nSize = strlen(lpTempRecip->lpszName) + 1;
				rRecipDesc.lpszName = new CHAR[nSize];
				if (rRecipDesc.lpszName)
					strcpy_s(rRecipDesc.lpszName, nSize, lpTempRecip->lpszName);
			}
			if (lpTempRecip->lpszAddress)
			{
				nSize = strlen(lpTempRecip->lpszAddress) + 1;
				rRecipDesc.lpszAddress = new CHAR[nSize];
				if (rRecipDesc.lpszAddress)
					strcpy_s(rRecipDesc.lpszAddress, nSize, lpTempRecip->lpszAddress);
			}
			// Don't forget to free up the memory MAPI allocated for us
			FreeBuffer(lpTempRecip);
		}
		else
		{
#ifdef _UNICODE
			nSize = WideCharToMultiByte(CP_ACP, 0, pszName, -1, NULL, 0, NULL, NULL);
#else
			nSize = strName.GetLength() + 1;
#endif
			rRecipDesc.lpszAddress = new CHAR[nSize];
			if (rRecipDesc.lpszAddress)
			{
#ifdef _UNICODE
				WideCharToMultiByte(CP_ACP, 0, pszName, -1, rRecipDesc.lpszAddress, nSize, NULL, NULL);
#else
				strcpy_s(rRecipDesc.lpszAddress, nSize, pszName);
#endif
			}
		}
	}
}

/**
 * @param ulRecipClass - kind of recipients.
 * @param lpRecips - array of recipient descriptors.
 * @param nRecipIndex - current position of initialized recipient descriptor in the array.
 * @param arrRecipients - recipient names.
 */
void CMapiSession::InitRecipients(ULONG ulRecipClass, lpMapiRecipDesc lpRecips, int& nRecipIndex, const CArray<CStrHolder>& arrRecipients)
{
	int nNumRecipients = arrRecipients.GetCount();
	for (int i = 0; i < nNumRecipients; ++i, ++nRecipIndex)
		InitRecipient(ulRecipClass, lpRecips[nRecipIndex + i], arrRecipients[i]);
}

/**
 * @param rMessage - message object.
 * @param bShowMessageEditor - true if you wan to show message editor.
 * @param hwndParent - parent window for the UI.
 * @param bNewSession - pass true to open new session instead of attempting to re-use existing shared session.
 * @return true if operation has been completed successfully.
 */
BOOL CMapiSession::Send(const CMapiMessage& rMessage, BOOL bShowMessageEditor, HWND hwndParent, BOOL bNewSession)
{
	if (! MapiInstalled() || (bNewSession ? LoggedOn() : ! LoggedOn()))
		return FALSE;

	const CArray<CStrHolder>& arrAttachments = rMessage.GetAttachments();
	int nNumAttachments = arrAttachments.GetCount();
	const CArray<CStrHolder>& arrAttachmentTitles = rMessage.GetAttachmentTitles();
	int nNumAttachmentTitles = arrAttachmentTitles.GetCount();
	if (nNumAttachments != nNumAttachmentTitles && nNumAttachmentTitles != 0)
		return FALSE;

	// Create the MapiMessage structure to match the message parameter send into us
	const CArray<CStrHolder>& arrTo = rMessage.GetTo();
	const CArray<CStrHolder>& arrCC = rMessage.GetCC();
	const CArray<CStrHolder>& arrBCC = rMessage.GetBCC();
	MapiMessage mapiMessage;
	ZeroMemory(&mapiMessage, sizeof(mapiMessage));
	int nRecipCount = arrTo.GetCount() + arrCC.GetCount() + arrBCC.GetCount();

	int nFileIndex, nRecipIndex;
	if (nRecipCount > 0)
	{
		mapiMessage.nRecipCount = nRecipCount;
		// Allocate the recipients array
		mapiMessage.lpRecips = new MapiRecipDesc[nRecipCount];
		if (mapiMessage.lpRecips)
		{
			ZeroMemory(mapiMessage.lpRecips, sizeof(*mapiMessage.lpRecips) * nRecipCount);
			nRecipIndex = 0;

			// Setup the "To" recipients
			InitRecipients(MAPI_TO, mapiMessage.lpRecips, nRecipIndex, arrTo);
			// Setup the "CC" recipients
			InitRecipients(MAPI_CC, mapiMessage.lpRecips, nRecipIndex, arrCC);
			// Setup the "BCC" recipients
			InitRecipients(MAPI_BCC, mapiMessage.lpRecips, nRecipIndex, arrBCC);
		}
	}

	// Setup the sender
	const CStrHolder& strFrom = rMessage.GetFrom();
	if (! strFrom.IsEmpty())
	{
		MapiRecipDesc recipOrigin;
		ZeroMemory(&recipOrigin, sizeof(recipOrigin));
		InitRecipient(MAPI_ORIG, recipOrigin, strFrom);
		mapiMessage.lpOriginator = &recipOrigin;
	}

	// Setup the attachments
	if (nNumAttachments)
	{
		mapiMessage.nFileCount = nNumAttachments;
		mapiMessage.lpFiles = new MapiFileDesc[nNumAttachments];
		if (mapiMessage.lpFiles)
		{
			ZeroMemory(mapiMessage.lpFiles, sizeof(*mapiMessage.lpFiles) * nNumAttachments);
			for (nFileIndex = 0; nFileIndex < nNumAttachments; ++nFileIndex)
			{
				MapiFileDesc& file = mapiMessage.lpFiles[nFileIndex];
				file.nPosition = (ULONG)-1;
				PCTSTR pszFileName = arrAttachments[nFileIndex];
#ifdef _UNICODE
				int nSize = WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, NULL, 0, NULL, NULL);
#else
				int nSize = strlen(pszFileName) + 1;
#endif
				file.lpszPathName = new CHAR[nSize];
				if (file.lpszPathName)
				{
#ifdef _UNICODE
					WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, file.lpszPathName, nSize, NULL, NULL);
#else
					strcpy_s(file.lpszPathName, nSize, pszFileName);
#endif
				}
				if (nNumAttachmentTitles)
				{
					PCTSTR pszTitle = arrAttachmentTitles[nFileIndex];
#ifdef _UNICODE
					nSize = WideCharToMultiByte(CP_ACP, 0, pszTitle, -1, NULL, 0, NULL, NULL);
#else
					nSize = strlen(pszTitle) + 1;
#endif
					file.lpszFileName = new CHAR[nSize];
					if (file.lpszFileName)
					{
#ifdef _UNICODE
						WideCharToMultiByte(CP_ACP, 0, pszTitle, -1, file.lpszFileName, nSize, NULL, NULL);
#else
						strcpy_s(file.lpszFileName, nSize, pszTitle);
#endif
					}
				}
				else
					file.lpszFileName = NULL;
			}
		}
	}

	const CStrHolder& strSubject = rMessage.GetSubject();
	const CStrHolder& strBody = rMessage.GetBody();
#ifdef _UNICODE
	PCWSTR pszSubject = strSubject;
	PSTR pszSubjectA;
	if (pszSubject)
	{
#ifdef _UNICODE
		int nSubjectSize = WideCharToMultiByte(CP_ACP, 0, pszSubject, -1, NULL, 0, NULL, NULL);
#else
		int nSubjectSize = strSubject.GetLength() + 1;
#endif
		pszSubjectA = new CHAR[nSubjectSize];
		if (pszSubjectA)
		{
			// Perform Unicode to ANSI conversion
			WideCharToMultiByte(CP_ACP, 0, pszSubject, -1, pszSubjectA, nSubjectSize, NULL, NULL);
		}
	}
	else
		pszSubjectA = NULL;
	mapiMessage.lpszSubject = pszSubjectA;

	PCWSTR pszBody = strBody;
	PSTR pszBodyA;
	if (pszBody)
	{
#ifdef _UNICODE
		int nBodySize = WideCharToMultiByte(CP_ACP, 0, pszBody, -1, NULL, 0, NULL, NULL);
#else
		int nBodySize = strBody.GetLength() + 1;
#endif
		pszBodyA = new CHAR[nBodySize];
		if (pszBodyA)
		{
			// Perform Unicode to ANSI conversion
			WideCharToMultiByte(CP_ACP, 0, pszBody, -1, pszBodyA, nBodySize, NULL, NULL);
		}
	}
	else
		pszBodyA = NULL;
	mapiMessage.lpszNoteText = pszBodyA;
#else
	mapiMessage.lpszSubject = (PSTR)(PCSTR)strSubject;
	mapiMessage.lpszNoteText = (PSTR)(PCSTR)strBody;
#endif

	// Initialize flags and UIParam parameters used in the MAPISendMail() call
	FLAGS flags = 0;

	// Force user to interactively input recipients list and other info.
	if (bShowMessageEditor)
	{
		flags |= MAPI_DIALOG;
		if (! m_hSession)
		{
			flags |= MAPI_LOGON_UI;
			if (bNewSession)
				flags |= MAPI_NEW_SESSION;
		}
	}

	// Do the actual send using MAPISendMail()
	m_ulLastError = m_pfnMAPISendMail(m_hSession, (ULONG)hwndParent, &mapiMessage, flags, 0);
	_ASSERTE(m_ulLastError == SUCCESS_SUCCESS);

	// Tidy up the attachments
	if (nNumAttachments)
	{
		for (nFileIndex = 0; nFileIndex < nNumAttachments; ++nFileIndex)
		{
			MapiFileDesc& file = mapiMessage.lpFiles[nFileIndex];
			delete[] file.lpszPathName;
			delete[] file.lpszFileName;
		}
		delete [] mapiMessage.lpFiles;
	}

	// Free up the recipients memory
	if (nRecipCount > 0)
	{
		for (nRecipIndex = 0; nRecipIndex < nRecipCount; ++nRecipIndex)
		{
			delete[] mapiMessage.lpRecips[nRecipIndex].lpszName;
			delete[] mapiMessage.lpRecips[nRecipIndex].lpszAddress;
		}
		delete [] mapiMessage.lpRecips;
		if (mapiMessage.lpOriginator)
		{
			delete[] mapiMessage.lpOriginator->lpszName;
			delete[] mapiMessage.lpOriginator->lpszAddress;
		}
	}

#ifdef _UNICODE
	delete[] pszSubjectA;
	delete[] pszBodyA;
#endif

	return (m_ulLastError == SUCCESS_SUCCESS);
}
