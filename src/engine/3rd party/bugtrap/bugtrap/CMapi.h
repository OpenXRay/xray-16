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

#pragma once

#include "Array.h"
#include "StrHolder.h"

/// Class which encapsulates a MAPI mail message.
class CMapiMessage
{
public:
	/// Get message subject.
	const CStrHolder& GetSubject(void) const;
	/// Set message subject.
	void SetSubject(PCTSTR pszSubject);
	/// Get message body.
	const CStrHolder& GetBody(void) const;
	/// Set message body.
	void SetBody(PCTSTR pszBody);
	/// Get message sender.
	const CStrHolder& GetFrom(void) const;
	/// Set message sender.
	void SetFrom(PCTSTR pszFrom);
	/// Get message recipients.
	CArray<CStrHolder>& GetTo(void);
	/// Get message recipients.
	const CArray<CStrHolder>& GetTo(void) const;
	/// Get message carbon copies.
	CArray<CStrHolder>& GetCC(void);
	/// Get message carbon copies.
	const CArray<CStrHolder>& GetCC(void) const;
	/// Get message blind carbon copies.
	CArray<CStrHolder>& GetBCC(void);
	/// Get message blind carbon copies.
	const CArray<CStrHolder>& GetBCC(void) const;
	/// Get message attachments.
	CArray<CStrHolder>& GetAttachments(void);
	/// Get message attachments.
	const CArray<CStrHolder>& GetAttachments(void) const;
	/// Get message attachment titles.
	CArray<CStrHolder>& GetAttachmentTitles(void);
	/// Get message attachment titles.
	const CArray<CStrHolder>& GetAttachmentTitles(void) const;

private:
	/// The Subject of the message.
	CStrHolder m_pszSubject;
	/// The Body of the message.
	CStrHolder m_pszBody;
	/// The From sender.
	CStrHolder m_pszFrom;
	/// The To recipients.
	CArray<CStrHolder> m_arrTo;
	/// The CC recipients.
	CArray<CStrHolder> m_arrCC;
	/// The BCC recipients.
	CArray<CStrHolder> m_arrBCC;
	/// Files to attach to the email.
	CArray<CStrHolder> m_arrAttachments;
	/// Titles to use for the email file attachments.
	CArray<CStrHolder> m_arrAttachmentTitles;
};

/// The class which encapsulates the MAPI connection.
class CMapiSession
{
public:
	/// Initialize the object.
	CMapiSession(void);
	/// Destroy the object.
	~CMapiSession(void);

	/// Begin a Simple MAPI session and load the default message store and address book providers.
	BOOL Logon(void);
	/// Begin a Simple MAPI session and load the default message store and address book providers.
	BOOL Logon(PCTSTR pszProfileName, PCTSTR pszPassword = NULL, BOOL bNewSession = FALSE);
	/// Begin a Simple MAPI session and load the default message store and address book providers.
	BOOL Logon(HWND hwndParent, BOOL bNewSession = FALSE);
	/// End a session with the messaging system.
	BOOL Logoff(void);
	/// Check if Simple MAPI session is already loaded.
	BOOL LoggedOn(void) const;

	/// Send a message.
	BOOL Send(const CMapiMessage& rMessage, BOOL bShowMessageEditor, HWND hwndParent = NULL, BOOL bNewSession = FALSE);
	/// Check if Simple MAPI is installed on user computer.
	BOOL MapiInstalled(void) const;

	/// Get recent error code.
	ULONG GetLastError(void) const;

private:
	/// Protect the class from being accidentally copied.
	CMapiSession(const CMapiSession& rMapiSession);
	/// Protect the class from being accidentally copied.
	CMapiSession& operator=(const CMapiSession& rMapiSession);

	/// Initialize internal object structures.
	void Initialise(void);
	/// De-initialize internal object structures.
	void Deinitialise(void);
	/// Transforms a message recipient's name as entered by a user to an unambiguous address list entry.
	BOOL Resolve(const CStrHolder& strName, lpMapiRecipDesc& lpRecip);
	/// Free memory allocated by MAPI calls.
	BOOL FreeBuffer(PVOID pBuffer);
	/// Initialize particular recipient record.
	void InitRecipient(ULONG ulRecipClass, MapiRecipDesc& rRecipDesc, const CStrHolder& strName);
	/// Initialize recipients list.
	void InitRecipients(ULONG ulRecipClass, lpMapiRecipDesc lpRecips, int& nRecipIndex, const CArray<CStrHolder>& arrRecipients);

	/// Mapi session handle.
	LHANDLE m_hSession;
	/// Last Mapi error value.
	ULONG m_ulLastError;
	/// Instance handle of the Mapi dll.
	HINSTANCE m_hMapi;
	/// MAPILogon() function pointer.
	LPMAPILOGON m_pfnMAPILogon;
	/// MAPILogoff() function pointer.
	LPMAPILOGOFF m_pfnMAPILogoff;
	/// MAPISendMail() function pointer.
	LPMAPISENDMAIL m_pfnMAPISendMail;
	/// MAPIResolveName() function pointer.
	LPMAPIRESOLVENAME m_pfnMAPIResolveName;
	/// MAPIFreeBuffer() function pointer.
	LPMAPIFREEBUFFER m_pfnMAPIFreeBuffer;
};

/**
 * @return message subject.
 */
inline const CStrHolder& CMapiMessage::GetSubject(void) const
{
	return m_pszSubject;
}

/**
 * @param pszSubject - message subject.
 */
inline void CMapiMessage::SetSubject(PCTSTR pszSubject)
{
	m_pszSubject = pszSubject;
}

/**
 * @return message body.
 */
inline const CStrHolder& CMapiMessage::GetBody(void) const
{
	return m_pszBody;
}

/**
 * @param pszBody - message body.
 */
inline void CMapiMessage::SetBody(PCTSTR pszBody)
{
	m_pszBody = pszBody;
}

/**
 * @return message sender.
 */
inline const CStrHolder& CMapiMessage::GetFrom(void) const
{
	return m_pszFrom;
}

/**
 * @param pszFrom - message sender.
 */
inline void CMapiMessage::SetFrom(PCTSTR pszFrom)
{
	m_pszFrom = pszFrom;
}

/**
 * @return message recipients.
 */
inline CArray<CStrHolder>& CMapiMessage::GetTo(void)
{
	return m_arrTo;
}

/**
* @return message recipients.
*/
inline const CArray<CStrHolder>& CMapiMessage::GetTo(void) const
{
	return m_arrTo;
}

/**
 * @return message carbon copies.
 */
inline CArray<CStrHolder>& CMapiMessage::GetCC(void)
{
	return m_arrCC;
}

/**
 * @return message carbon copies.
 */
inline const CArray<CStrHolder>& CMapiMessage::GetCC(void) const
{
	return m_arrCC;
}

/**
 * @return message blind carbon copies.
 */
inline CArray<CStrHolder>& CMapiMessage::GetBCC(void)
{
	return m_arrBCC;
}

/**
 * @return message blind carbon copies.
 */
inline const CArray<CStrHolder>& CMapiMessage::GetBCC(void) const
{
	return m_arrBCC;
}

/**
 * @return message attachments.
 */
inline CArray<CStrHolder>& CMapiMessage::GetAttachments(void)
{
	return m_arrAttachments;
}

/**
 * @return message attachments.
 */
inline const CArray<CStrHolder>& CMapiMessage::GetAttachments(void) const
{
	return m_arrAttachments;
}

/**
 * @return message attachment titles.
 */
inline CArray<CStrHolder>& CMapiMessage::GetAttachmentTitles(void)
{
	return m_arrAttachmentTitles;
}

/**
 * @return message attachment titles.
 */
inline const CArray<CStrHolder>& CMapiMessage::GetAttachmentTitles(void) const
{
	return m_arrAttachmentTitles;
}

/**
 * @return true if user already logged.
 */
inline BOOL CMapiSession::LoggedOn(void) const
{
	return (m_hSession != NULL);
}

/**
 * @return true if Mapi is present on user computer.
 */
inline BOOL CMapiSession::MapiInstalled(void) const
{
	return (m_hMapi != NULL);
}

/**
 * @return error code of recent operation.
 */
inline ULONG CMapiSession::GetLastError(void) const
{
	return m_ulLastError;
}
