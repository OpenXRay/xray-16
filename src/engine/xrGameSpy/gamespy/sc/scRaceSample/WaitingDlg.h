#if !defined(AFX_WAITINGDLG_H__874B6CC6_8058_4BDF_B714_8FE3610A12B4__INCLUDED_)
#define AFX_WAITINGDLG_H__874B6CC6_8058_4BDF_B714_8FE3610A12B4__INCLUDED_

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
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITINGDLG_H__874B6CC6_8058_4BDF_B714_8FE3610A12B4__INCLUDED_)
