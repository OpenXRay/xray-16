// multiTrackDlg.h : header file
//

#if !defined(AFX_MULTITRACKDLG_H__4B977C2B_DD72_4B3A_B1F9_E95D26F0C328__INCLUDED_)
#define AFX_MULTITRACKDLG_H__4B977C2B_DD72_4B3A_B1F9_E95D26F0C328__INCLUDED_

#include "LoginDlg.h"
#include "HostOrJoinDlg.h"
#include "..\..\GT2\gt2.h"	// Added by ClassView
#include "WaitingDlg.h"	// Added by ClassView

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackDlg dialog

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

#define RACE_STEPS_50    60
#define RACE_STEPS_100  120
#define RACE_STEPS_200  240

#define EVENT_NONE    0
#define EVENT_50      1
#define EVENT_100     2
#define EVENT_200     3

class CMultiTrackDlg : public CDialog
{
// Construction
public:
	void UpdateRatingsDisplay();
	int m_localRatings[4];
	int m_remoteRatings[4];
	void UpdateStats();
	CString m_remoteNick;
	void ReportStats();
	DWORD m_remoteTime;
	DWORD m_localTime;
	void StartRace();
	int m_totalSteps;
	int m_event;
	int m_step;
	int m_numSteps;
	DWORD m_start;
	BOOL m_racing;
	void Countdown();
	int m_countdown;
	BOOL m_challenged;
	void Logout();
	CWaitingDlg m_waitingDlg;
	CString m_remoteResponse;
	int m_state;
	CLoginDlg m_loginDlg;
	CHostOrJoinDlg m_hostOrJoinDlg;
	BOOL m_hosting;
	CString m_challenge;
	int m_remoteProfile;

	GT2Connection m_connection;
	GT2Socket m_socket;

	BOOL SetupJoining();
	BOOL SetupHosting();
	BOOL SetupMatch();
	CMultiTrackDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMultiTrackDlg)
	enum { IDD = IDD_MULTITRACK_DIALOG };
	CButton	m_start100;
	CButton	m_start200;
	CProgressCtrl	m_remoteProgress;
	CProgressCtrl	m_localProgress;
	CButton	m_start50;
	CString	m_info;
	CString	m_localInfo100;
	CString	m_localInfo200;
	CString	m_localInfo50;
	CString	m_localInfoOverall;
	CString	m_remoteInfo100;
	CString	m_remoteInfo200;
	CString	m_remoteInfo50;
	CString	m_remoteInfoOverall;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiTrackDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMultiTrackDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLogout();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnStart50();
	afx_msg void OnStart100();
	afx_msg void OnStart200();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MULTITRACKDLG_H__4B977C2B_DD72_4B3A_B1F9_E95D26F0C328__INCLUDED_)
