#if !defined(AFX_GETUSERINFODLG_H__600E8857_A5B5_41DF_973E_0A3DCCF0DCEB__INCLUDED_)
#define AFX_GETUSERINFODLG_H__600E8857_A5B5_41DF_973E_0A3DCCF0DCEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GetUserInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGetUserInfoDlg dialog

class CGetUserInfoDlg : public CDialog
{
// Construction
public:
	CGetUserInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGetUserInfoDlg)
	enum { IDD = IDD_GET_USER_INFO };
	CListBox	m_channels;
	CString	m_address;
	CString	m_name;
	CString	m_user;
	CString	m_nick;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetUserInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGetUserInfoDlg)
	afx_msg void OnGetInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GETUSERINFODLG_H__600E8857_A5B5_41DF_973E_0A3DCCF0DCEB__INCLUDED_)
