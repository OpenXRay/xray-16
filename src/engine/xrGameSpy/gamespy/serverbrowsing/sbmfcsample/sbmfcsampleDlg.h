// sbmfcsampleDlg.h : header file
//

#if !defined(AFX_SBMFCSAMPLEDLG_H__8B726D03_AF99_4938_A63C_18525912E55D__INCLUDED_)
#define AFX_SBMFCSAMPLEDLG_H__8B726D03_AF99_4938_A63C_18525912E55D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSbmfcsampleDlg dialog

//SB - header
#include "..\sb_serverbrowsing.h"
#include "..\..\qr2\qr2regkeys.h"
#include "..\..\common\gsAvailable.h"

class CSbmfcsampleDlg : public CDialog
{
// Construction
public:
	CSbmfcsampleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSbmfcsampleDlg)
	enum { IDD = IDD_SBMFCSAMPLE_DIALOG };
	CStatic	m_servers;
	CProgressCtrl	m_progress;
	CListCtrl	m_serverList;
	CListCtrl	m_playerList;
	CString	m_filter;
	CString	m_gamename;
	int m_startPort;
	int m_endPort;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSbmfcsampleDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static void SBCallback(ServerBrowser serverBrowser, SBCallbackReason reason, SBServer server, void * instance);
	static void SBConnectCallback(ServerBrowser serverBrowser, SBConnectToServerState state, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *instance);
	void AddServer(SBServer server, BOOL checkForReplace);
	void RemoveServer(SBServer server);
	int FindServer(SBServer server);
	BOOL CreateServerList();
	HICON m_hIcon;
	ServerBrowser m_serverBrowser;
	UINT m_timerID;
	int m_serverCount;

	// Generated message map functions
	//{{AFX_MSG(CSbmfcsampleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	afx_msg void OnClickServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnColumnclickServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SBMFCSAMPLEDLG_H__8B726D03_AF99_4938_A63C_18525912E55D__INCLUDED_)
