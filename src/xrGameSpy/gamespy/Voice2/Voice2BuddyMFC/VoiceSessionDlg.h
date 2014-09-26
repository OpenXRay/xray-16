#if !defined(AFX_VOICESESSIONDLG_H__4D78C45C_9CB3_4F4B_81DF_8BF65316598E__INCLUDED_)
#define AFX_VOICESESSIONDLG_H__4D78C45C_9CB3_4F4B_81DF_8BF65316598E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VoiceSessionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVoiceSessionDlg dialog
#include "SetupDlg.h"
#include "../../gt2/gt2.h"
#include "../../natneg/natneg.h"

class CVoiceSessionDlg : public CDialog
{
public:
	int  m_NNCookie; // used to connect the two buddies
	BOOL m_IsHost;   // TRUE = I invited the buddy, FALSE = Buddy invited me

	VoiceSetupInfo m_SetupInfo; // device info

	GT2Socket      m_Socket;
	GT2Connection  m_Connection;
	unsigned int   m_RemoteAddr;    // IP addr of the buddy (used for identification)

	// MFC STUFF BELOW

// Construction
public:
	CVoiceSessionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVoiceSessionDlg)
	enum { IDD = IDD_SESSION };
	CStatic	m_RemoteSpeaking;
	CStatic	m_LocalSpeaking;
	CString	m_DisplayText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVoiceSessionDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVoiceSessionDlg)
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOICESESSIONDLG_H__4D78C45C_9CB3_4F4B_81DF_8BF65316598E__INCLUDED_)
