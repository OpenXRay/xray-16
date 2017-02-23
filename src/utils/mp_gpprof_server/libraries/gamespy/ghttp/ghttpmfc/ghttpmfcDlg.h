// ghttpmfcDlg.h : header file
//

#if !defined(AFX_GHTTPMFCDLG_H__14B35CAF_3960_4669_972D_59B741AA032C__INCLUDED_)
#define AFX_GHTTPMFCDLG_H__14B35CAF_3960_4669_972D_59B741AA032C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGhttpmfcDlg dialog

class CGhttpmfcDlg : public CDialog
{
// Construction
public:
	CGhttpmfcDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGhttpmfcDlg)
	enum { IDD = IDD_GHTTPMFC_DIALOG };
	CProgressCtrl	m_progress;
	BOOL	m_blocking;
	BOOL	m_completedCallback;
	CString	m_headers;
	BOOL	m_progressCallback;
	CString	m_url;
	int		m_bufferSize;
	CString	m_saveAs;
	BOOL	m_userBuffer;
	int		m_type;
	CString	m_file;
	CString	m_soFar;
	int		m_state;
	BOOL	m_throttle;
	CString	m_headersRecv;
	CString	m_status;
	BOOL	m_stepThink;
	BOOL	m_postFile;
	CString	m_postObjects;
	CString	m_postBytes;
	CString	m_proxy;
	//}}AFX_DATA

	GHTTPRequest m_request;
	char * m_memFile;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGhttpmfcDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGhttpmfcDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnCancel_();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnThrottle();
	afx_msg void OnThink();
	afx_msg void OnSetProxy();
	afx_msg void OnIeSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GHTTPMFCDLG_H__14B35CAF_3960_4669_972D_59B741AA032C__INCLUDED_)
