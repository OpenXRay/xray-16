// gp_statsDlg.h : header file
//

#if !defined(AFX_GP_STATSDLG_H__B7B25B2A_61E9_4BFE_BBD7_D7C4D7242B7D__INCLUDED_)
#define AFX_GP_STATSDLG_H__B7B25B2A_61E9_4BFE_BBD7_D7C4D7242B7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../../gp/gp.h"

/////////////////////////////////////////////////////////////////////////////
// CGp_statsDlg dialog

class CGp_statsDlg : public CDialog
{
// Construction
public:
	CGp_statsDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGp_statsDlg)
	enum { IDD = IDD_GP_STATS_DIALOG };
	CListBox	m_keys;
	CString	m_email;
	CString	m_nick;
	CString	m_password;
	int		m_profileID;
	int		m_type;
	CString	m_value;
	CString	m_newKey;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGp_statsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

public:
	void GotData(LPCSTR data);

// Implementation
protected:
	HICON m_hIcon;

	BOOL m_authenticated;
	GPConnection m_gp;

	void Authenticated();
	void UnAuthenticate();
	BOOL CheckStatsConnection();
	BOOL CheckAccount();
	BOOL CreateAccount();
	void SendPassword();
	BOOL StatsAuthentication();
	void ClearKeys();

	// Generated message map functions
	//{{AFX_MSG(CGp_statsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAuthenticate();
	afx_msg void OnDestroy();
	afx_msg void OnGet();
	afx_msg void OnSet();
	afx_msg void OnAdd();
	afx_msg void OnSelchangeKeys();
	afx_msg void OnChangeValue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GP_STATSDLG_H__B7B25B2A_61E9_4BFE_BBD7_D7C4D7242B7D__INCLUDED_)
