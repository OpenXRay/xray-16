// ladderTrackDlg.h : header file
//

#if !defined(AFX_LADDERTRACKDLG_H__82121F55_1FDB_427A_B616_B59EB16C5E6D__INCLUDED_)
#define AFX_LADDERTRACKDLG_H__82121F55_1FDB_427A_B616_B59EB16C5E6D__INCLUDED_

#include "LoginDlg.h"
#include "HostOrJoinDlg.h"
#include "..\..\GT2\gt2.h"	// Added by ClassView
#include "..\..\GT2\gt2Encode.h"
#include "WaitingDlg.h"	// Added by ClassView

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLadderTrackDlg dialog

#define LOGGED_OUT            1
#define SETTING_UP            2
#define RACING                3

#define HOST_LISTENING       11
#define HOST_CHALLENGING     12
#define HOST_CONNECTED       13
#define HOST_ERROR           14

#define JOIN_CONNECTING      21
#define JOIN_WAITING         22
#define JOIN_CONNECTED       23
#define JOIN_ERROR           24

#define NONE  -1
#define LEFT   0
#define RIGHT  1

#define RACE_STEPS           60

class CLadderTrackDlg : public CDialog
{
// Construction
public:
	void FakeStats();
	CString m_remoteNick;
	DWORD m_remoteTime;
	DWORD m_localTime;
	int m_totalSteps;
	int m_step;
	int m_numSteps;
	DWORD m_start;
	BOOL m_racing;
	int m_countdown;
	BOOL m_challenged;
	CString m_remoteResponse;
	int m_state;
	BOOL m_hosting;
	CString m_challenge;
	int m_remoteProfile;
	CLoginDlg m_loginDlg;
	CHostOrJoinDlg m_hostOrJoinDlg;
	CWaitingDlg m_waitingDlg;

	GT2Socket	  m_GT2Socket;		// raw socket
	GT2Connection m_GT2Connection;	// established connection

	BOOL SetupHosting();
	BOOL SetupJoining();
	BOOL SetupMatch();
	void UpdatePlayerPositions();
	void Countdown();
	void Logout();
	void StartRace();
	void ReportStats();
	CLadderTrackDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CLadderTrackDlg)
	enum { IDD = IDD_LADDERTRACK_DIALOG };
	CButton	m_startRace;
	CProgressCtrl	m_remoteProgress;
	CProgressCtrl	m_localProgress;
	CString	m_localPosition;
	CString	m_remotePosition;
	CString	m_info;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLadderTrackDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CLadderTrackDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLogout();
	afx_msg void OnUpdatePositions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LADDERTRACKDLG_H__82121F55_1FDB_427A_B616_B59EB16C5E6D__INCLUDED_)
