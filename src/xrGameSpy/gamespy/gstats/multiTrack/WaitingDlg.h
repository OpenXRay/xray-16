#if !defined(AFX_WAITINGDLG_H__E9C283EE_68D1_40E9_83C6_AAB81D13ED25__INCLUDED_)
#define AFX_WAITINGDLG_H__E9C283EE_68D1_40E9_83C6_AAB81D13ED25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaitingDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitingDlg dialog

class CWaitingDlg : public CDialog
{
// Construction
public:
	CWaitingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWaitingDlg)
	enum { IDD = IDD_WAITING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitingDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITINGDLG_H__E9C283EE_68D1_40E9_83C6_AAB81D13ED25__INCLUDED_)
