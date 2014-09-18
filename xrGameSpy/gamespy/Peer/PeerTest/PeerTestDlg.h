// PeerTestDlg.h : header file
//

#if !defined(AFX_PEERTESTDLG_H__3043C7C9_6FE5_433F_83A6_8186F9E145A8__INCLUDED_)
#define AFX_PEERTESTDLG_H__3043C7C9_6FE5_433F_83A6_8186F9E145A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPeerTestDlg dialog

class CPeerTestDlg : public CDialog
{
// Construction
public:
	CPeerTestDlg(CWnd* pParent = NULL);	// standard constructor

	void FillPlayerList(RoomType roomType);
	SBServer GetCurrentServer();

	PEER m_peer;
	int m_count;
	CString m_selectedNick;
	RoomType m_selectedRoom;

// Dialog Data
	//{{AFX_DATA(CPeerTestDlg)
	enum { IDD = IDD_PEERTEST_DIALOG };
	CListBox	m_pingPlayers;
	CProgressCtrl	m_progress;
	CListBox	m_playersT;
	CListBox	m_playersG;
	CListBox	m_playersS;
	CButton	m_playersBoxT;
	CButton	m_playersBoxG;
	CButton	m_playersBoxS;
	CListBox	m_chatList;
	CListBox	m_rooms;
	CButton	m_roomsBox;
	CButton	m_readyCtl;
	BOOL	m_blocking;
	CString	m_nick;
	CString m_title;
	BOOL	m_ready;
	CString	m_message;
	CString	m_password;
	CString	m_name;
	BOOL	m_quiet;
	CString	m_key;
	CString	m_player;
	int		m_keyType;
	int		m_keyRoom;
	CString	m_value;
	BOOL	m_away;
	CString	m_awayReason;
	CString	m_raw;
	CString	m_filter;
	CString	m_secretKey;
	CString	m_cdKey;
	CString	m_autoMatchStatus;
	int		m_maxPlayers;
	CString	m_email;
	CString	m_loginPassword;
	CString	m_namespace;
	CString	m_authToken;
	CString	m_partnerChallenge;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPeerTestDlg)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPeerTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton1();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	afx_msg void OnButton4();
	afx_msg void OnButton5();
	afx_msg void OnButton8();
	afx_msg void OnButton9();
	afx_msg void OnButton10();
	afx_msg void OnButton12();
	afx_msg void OnButton14();
	afx_msg void OnCheck1();
	afx_msg void OnButton15();
	afx_msg void OnButton13();
	afx_msg void OnButton20();
	afx_msg void OnButton25();
	afx_msg void OnButton17();
	afx_msg void OnButton22();
	afx_msg void OnButton18();
	afx_msg void OnButton23();
	afx_msg void OnButton26();
	afx_msg void OnButton27();
	afx_msg void OnButton28();
	afx_msg void OnButton16();
	afx_msg void OnButton29();
	afx_msg void OnButton30();
	afx_msg void OnSelchangeTitlePlayers();
	afx_msg void OnSelchangeGroupPlayers();
	afx_msg void OnSelchangeStagingPlayers();
	afx_msg void OnButton6();
	afx_msg void OnButton7();
	afx_msg void OnButton21();
	afx_msg void OnButton24();
	afx_msg void OnButton31();
	afx_msg void OnButton32();
	afx_msg void OnButton33();
	afx_msg void OnButton34();
	afx_msg void OnButton35();
	afx_msg void OnChangeNick();
	afx_msg void OnQuiet();
	afx_msg void OnButton37();
	afx_msg void OnButton38();
	afx_msg void OnSetfocusTitlePlayers();
	afx_msg void OnSetfocusGroupPlayers();
	afx_msg void OnSetfocusStagingPlayers();
	afx_msg void OnAway();
	afx_msg void OnButton39();
	afx_msg void OnButton40();
	afx_msg void OnFixNick();
	afx_msg void OnButton11();
	afx_msg void OnButton42();
	afx_msg void OnButton43();
	afx_msg void OnButton44();
	afx_msg void OnButton45();
	afx_msg void OnButton46();
	afx_msg void OnDblclkRooms();
	afx_msg void OnButton47();
	afx_msg void OnButton48();
	afx_msg void OnStartAutoMatch();
	afx_msg void OnStopAutoMatch();
	afx_msg void OnButton36();
	afx_msg void OnButton49();
	afx_msg void OnButton50();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CPeerTestDlg * dlg;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PEERTESTDLG_H__3043C7C9_6FE5_433F_83A6_8186F9E145A8__INCLUDED_)
