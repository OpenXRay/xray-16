// gptestDlg.h : header file
//

#include "afxcmn.h"
#include "afxwin.h"
#if !defined(AFX_GPTESTDLG_H__AA6A1E3F_9C28_4A6C_902F_328A8707E479__INCLUDED_)
#define AFX_GPTESTDLG_H__AA6A1E3F_9C28_4A6C_902F_328A8707E479__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGptestDlg dialog

class CGptestDlg : public CDialog
{
// Construction
public:
	CGptestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGptestDlg)
	enum { IDD = IDD_GPTEST_DIALOG };
	CButton	m_connectunique;
	CButton	m_connectpreauth;
	CButton	m_newuser;
	CButton	m_search;
	CListBox	m_results;
	CButton	m_update;
	CButton	m_send;
	CButton	m_set;
	CListBox	m_buddies;
	CButton	m_disconnect;
	CButton	m_connect;
	int		m_partnerid;
	CString	m_email;
	CString	m_nick;
	CString	m_password;
	GPConnection m_connection;
	CString	m_locationString;
	int		m_status;
	CString	m_statusString;
	CString	m_iaddress;
	CString	m_icountrycode;
	CString	m_iemail;
	CString	m_ihomepage;
	int		m_iicquin;
	CString	m_inick;
	CString	m_isex;
	CString	m_message;
	CString	m_sfirstname;
	CString	m_snick;
	int		m_sicquin;
	CString	m_slastname;
	CString	m_semail;
	CString	m_string;
	CString	m_code;
	CString	m_reason;
	int		m_rnick;
	BOOL	m_infoCache;
	BOOL	m_blocking;
	CString	m_ilastname;
	CString	m_ifirstname;
	int		m_ibirthday;
	int		m_ibirthmonth;
	int		m_ibirthyear;
	BOOL	m_ipmbirthday;
	BOOL	m_ipmcountrycode;
	BOOL	m_ipmhomepage;
	BOOL	m_ipmsex;
	BOOL	m_ipmzipcode;
	CString	m_newnick;
	BOOL	m_replace;
	CString	m_izipcode;
    UINT	m_invitePlayerID;
	int		m_server;
	CString	m_otherServer;
	CString	m_name;
	CString	m_path;
	CString	m_searchServer;
	BOOL	m_ipmemail;
	BOOL	m_firewall;
	float	m_ilatitude;
	CString	m_iplace;
	float	m_ilongitude;
	CString	m_suniquenick;
	CString	m_iuniquenick;
	CString	m_uniquenick;
	CString	m_authtoken;
	CString	m_partnerchallenge;
	CString	m_namespace;
	int		m_productid;
    CListBox	m_blocklist;
    CButton	m_getblockedlist;
    CButton	m_addblock;
    CButton	m_removeblock;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGptestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	mutable long m_InTimer;
	
	

	void CodeToString(CString & string);
	void SetHost();

	// Generated message map functions
	//{{AFX_MSG(CGptestDlg)
	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnConnect();
	afx_msg void OnDisconnect();
	afx_msg void OnDestroy();
	afx_msg void OnSet();
	afx_msg void OnSelchangeBuddies();
	afx_msg void OnSend();
	afx_msg void OnRefresh();
	afx_msg void OnUpdate();
	afx_msg void OnSearch();
	afx_msg void OnSelchangeResults();
	afx_msg void OnNewuser();
	afx_msg void OnSendrequest();
	afx_msg void OnInfoCache();
	afx_msg void OnDelete();
	afx_msg void OnTest();
	afx_msg void OnSetinfo();
	afx_msg void OnDeletepro();
	afx_msg void OnNewpro();
	afx_msg void OnDeleteall();
	afx_msg void OnValidate();
	afx_msg void OnNicks();
	afx_msg void OnInvitePlayer();
	afx_msg void OnReport();
	afx_msg void OnCheck();
	afx_msg void OnSendFiles();
	afx_msg void OnChangeSearchServer();
	afx_msg void OnPublicmaskAll();
	afx_msg void OnPublicmaskNone();
	afx_msg void OnReverseBuddies();
	afx_msg void OnRevoke();
	afx_msg void OnSuggest();
	afx_msg void OnConnectunique();
	afx_msg void OnConnectpreauth();
	afx_msg void OnInitialize();
	afx_msg void OnDestroyGP();
	afx_msg void OnUTM();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	int m_StatusState;
	
	short m_QueryPort;
	short m_HostPort;
	afx_msg void OnSetstatusinfo();
	CListBox m_SessionFlags;
	CString m_RichStatus;
	CString m_GameType;
	CString m_GameVariant;
	CString m_GameMapname;
	afx_msg void OnAddSetKey();
	CString m_KeyName;
	CString m_KeyValue;
	afx_msg void OnGetKeyValue();
	afx_msg void OnGetBuddyKeys();
	CString m_HostIp;
	CString m_HostPrivateIp;
	afx_msg void OnDelKeyVal();
	CString m_cdkey;
	afx_msg void OnRegisterCdKey();
    afx_msg void OnGetBlocked();
    afx_msg void OnAddBlock();
    afx_msg void OnRemoveBlock();
	afx_msg void OnSelchangeBlocklist();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPTESTDLG_H__AA6A1E3F_9C28_4A6C_902F_328A8707E479__INCLUDED_)
