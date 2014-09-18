#if !defined(AFX_SETPASSWORDDLG_H__666C66DA_E10E_420D_954E_FB2BCE01D798__INCLUDED_)
#define AFX_SETPASSWORDDLG_H__666C66DA_E10E_420D_954E_FB2BCE01D798__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetPasswordDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg dialog

class CSetPasswordDlg : public CDialog
{
// Construction
public:
	CSetPasswordDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetPasswordDlg)
	enum { IDD = IDD_SET_PASSWORD };
	CString	m_password;
	BOOL	m_enable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetPasswordDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetPasswordDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETPASSWORDDLG_H__666C66DA_E10E_420D_954E_FB2BCE01D798__INCLUDED_)
