// gt2testDlg.h : header file
//

#if !defined(AFX_GT2TESTDLG_H__0E64F324_FFA4_4A94_A87D_75652134DC2D__INCLUDED_)
#define AFX_GT2TESTDLG_H__0E64F324_FFA4_4A94_A87D_75652134DC2D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGt2testDlg dialog

struct ConnectionInfo
{
	GT2Connection connection;
	CString messages;
	BOOL sendROT13;
	BOOL receiveROT13;
	BOOL sendDrop;
	BOOL receiveDrop;
	CString sendDropValue;
	CString receiveDropValue;
	BOOL sendDelay;
	BOOL receiveDelay;
	CString sendDelayValue;
	CString receiveDelayValue;
};

struct DelayedMessage
{
	GT2Connection connection;
	int filterID;
	GT2Byte * message;
	int len;
	GT2Bool reliable;
	unsigned long startTime;
	unsigned long delayTime;
};

class CGt2testDlg : public CDialog
{
// Construction
public:
	void AddMessageString(GT2Connection connection, const char * string);
	void RemoveConnection(GT2Connection connection);
	GT2Connection GetActiveConnection();
	int GetConnectionIndex(GT2Connection connection);
	GT2Connection GetConnection(int nIndex);
	ConnectionInfo * GetConnectionInfo(int nIndex);
	ConnectionInfo * GetConnectionInfo(GT2Connection connection);
	void SetActiveConnection(GT2Connection connection, int nIndex);
	void SetActiveConnection(GT2Connection connection);
	void SetActiveConnection(int nIndex);
	void SetupConnectionCallbacks(GT2ConnectionCallbacks & callbacks);
	void AddConnection(GT2Connection connection);
	void EnableSocketControls(BOOL enable = TRUE);
	void EnableListenControls(BOOL enable = TRUE);
	void EnableConnectionControls(BOOL enable = TRUE);
	CGt2testDlg(CWnd* pParent = NULL);	// standard constructor

	DArray m_delayedSends;
	DArray m_delayedReceives;

// Dialog Data
	//{{AFX_DATA(CGt2testDlg)
	enum { IDD = IDD_GT2TEST_DIALOG };
	CStatic	m_outgoingBufferSize;
	CProgressCtrl	m_outgoingBufferProgress;
	CStatic	m_incomingBufferSize;
	CProgressCtrl	m_incomingBufferProgress;
	CComboBox	m_connections;
	CIPAddressCtrl	m_addressIP;
	CString	m_addressAliases;
	CString	m_addressHostname;
	CString	m_addressIPs;
	CString	m_addressPort;
	CString	m_addressString;
	CString	m_outBufferSize;
	CString	m_rejectReason;
	int		m_acceptMode;
	BOOL	m_alwaysThink;
	BOOL	m_blocking;
	CString	m_localAddress;
	CString	m_remoteAddress;
	CString	m_inBufferSize;
	BOOL	m_listen;
	int		m_connectionState;
	BOOL	m_reliable;
	CString	m_message;
	CString	m_messages;
	BOOL	m_receiveDelay;
	CString	m_receiveDelayValue;
	BOOL	m_receiveDrop;
	CString	m_receiveDropValue;
	BOOL	m_receiveROT13;
	BOOL	m_sendDelay;
	CString	m_sendDelayValue;
	BOOL	m_sendDrop;
	CString	m_sendDropValue;
	BOOL	m_sendROT13;
	CString	m_connectMessage;
	CString	m_timeout;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGt2testDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void HandleHostInfo(const char * hostname, char ** aliases, unsigned int ** ips);
	HICON m_hIcon;

	GT2Socket m_socket;
	BOOL m_thinking;

	// Generated message map functions
	//{{AFX_MSG(CGt2testDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnAddressTo();
	afx_msg void OnAddressFrom();
	afx_msg void OnGetIpHostInfo();
	afx_msg void OnGetStringHostInfo();
	afx_msg void OnCloseSocket();
	afx_msg void OnCreateSocket();
	afx_msg void OnConnect();
	afx_msg void OnThink();
	afx_msg void OnListen();
	afx_msg void OnCloseAllConnections();
	afx_msg void OnCloseAllConnectionsHard();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSend();
	afx_msg void OnPing();
	afx_msg void OnCloseConnection();
	afx_msg void OnCloseConnectionHard();
	afx_msg void OnSelchangeConnections();
	afx_msg void OnSendRot13();
	afx_msg void OnReceiveRot13();
	afx_msg void OnSendDrop();
	afx_msg void OnReceiveDrop();
	afx_msg void OnSendDelay();
	afx_msg void OnReceiveDelay();
	afx_msg void OnDestroy();
	afx_msg void OnChangeSendDropValue();
	afx_msg void OnChangeSendDelayValue();
	afx_msg void OnChangeReceiveDropValue();
	afx_msg void OnChangeReceiveDelayValue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GT2TESTDLG_H__0E64F324_FFA4_4A94_A87D_75652134DC2D__INCLUDED_)
