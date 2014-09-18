// trackDlg.h : header file
//

#if !defined(AFX_TRACKDLG_H__E16D2398_5E86_47A2_80DA_8F843E24C4A6__INCLUDED_)
#define AFX_TRACKDLG_H__E16D2398_5E86_47A2_80DA_8F843E24C4A6__INCLUDED_

#include "LoginDlg.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTrackDlg dialog

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

class CTrackDlg : public CDialog
{
// Construction
public:
	void SetupUser();
	DWORD m_start;
	int m_count;
	int m_event;
	BOOL m_racing;
	int m_numSteps;
	int m_step;
	int m_totalSteps;
	void ReportStats(DWORD time);

	CTrackDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTrackDlg)
	enum { IDD = IDD_TRACK_DIALOG };
	CProgressCtrl	m_progress;
	CString	m_info;
	CString	m_best100;
	CString	m_best200;
	CString	m_best50;
	CString	m_top100;
	CString	m_top200;
	CString	m_top50;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrackDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	CLoginDlg m_loginDlg;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTrackDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLogout();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnStart50();
	afx_msg void OnStart100();
	afx_msg void OnStart200();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACKDLG_H__E16D2398_5E86_47A2_80DA_8F843E24C4A6__INCLUDED_)
