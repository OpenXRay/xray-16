#if !defined(AFX_ENTERDLG_H__56D121BA_9750_4FF9_AEA2_0CDAC1CB1DC2__INCLUDED_)
#define AFX_ENTERDLG_H__56D121BA_9750_4FF9_AEA2_0CDAC1CB1DC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnterDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEnterDlg dialog

class CEnterDlg : public CDialog
{
// Construction
public:
	CEnterDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEnterDlg)
	enum { IDD = IDD_ENTER };
	CString	m_channel;
	CString	m_password;
	//}}AFX_DATA

	CString m_quickChannel;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEnterDlg)
	afx_msg void OnQuick1();
	afx_msg void OnQuick2();
	afx_msg void OnQuick3();
	afx_msg void OnQuick4();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTERDLG_H__56D121BA_9750_4FF9_AEA2_0CDAC1CB1DC2__INCLUDED_)
