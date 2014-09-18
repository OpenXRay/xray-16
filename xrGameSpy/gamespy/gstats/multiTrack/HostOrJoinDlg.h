#if !defined(AFX_HOSTORJOINDLG_H__7D7ACBE1_D82A_4CB0_B070_368D4263332E__INCLUDED_)
#define AFX_HOSTORJOINDLG_H__7D7ACBE1_D82A_4CB0_B070_368D4263332E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HostOrJoinDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHostOrJoinDlg dialog

#define HOSTORJOIN_HOST       0
#define HOSTORJOIN_JOIN       1

class CHostOrJoinDlg : public CDialog
{
// Construction
public:
	CHostOrJoinDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHostOrJoinDlg)
	enum { IDD = IDD_HOST_OR_JOIN };
	int		m_hostOrJoin;
	CString	m_joinAddress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostOrJoinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHostOrJoinDlg)
	virtual void OnOK();
	afx_msg void OnSetfocusJoinAddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTORJOINDLG_H__7D7ACBE1_D82A_4CB0_B070_368D4263332E__INCLUDED_)
