// Voice2BuddyMFCDlg.h : header file
//

#if !defined(AFX_VOICE2BUDDYMFCDLG_H__F4BA3DE6_2C65_4B7D_A1BC_A9C3BAF53DEE__INCLUDED_)
#define AFX_VOICE2BUDDYMFCDLG_H__F4BA3DE6_2C65_4B7D_A1BC_A9C3BAF53DEE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CVoice2BuddyMFCDlg dialog
#include "../../gp/gp.h"
#include "SetupDlg.h"

class CVoice2BuddyMFCDlg : public CDialog
{
	// Non MFC
public:
	GPConnection  m_GP;
	BOOL          m_Initialized;
	BOOL          m_Connected;
	unsigned int  m_NNCookie;
	GPProfile     m_InvitedPlayer; // the last player we invited
	GPProfile     m_MyProfileId;   // the local player's profile id

	VoiceSetupInfo   m_SetupInfo;

	void DoLogin(const CString& theEmail, const CString& theNickname, const CString& thePassword);

	
// Construction
public:
	CVoice2BuddyMFCDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVoice2BuddyMFCDlg)
	enum { IDD = IDD_VOICE2BUDDYMFC_DIALOG };
	CButton	m_SetupButton;
	CButton	m_VoiceChatButton;
	CListBox	m_BuddyList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVoice2BuddyMFCDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVoice2BuddyMFCDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSetup();
	afx_msg void OnVoiceChat();
	afx_msg void OnExit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeBuddylist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOICE2BUDDYMFCDLG_H__F4BA3DE6_2C65_4B7D_A1BC_A9C3BAF53DEE__INCLUDED_)
