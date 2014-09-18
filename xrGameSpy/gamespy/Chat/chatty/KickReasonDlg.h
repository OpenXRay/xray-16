#if !defined(AFX_KICKREASONDLG_H__25C49432_E9A9_4BEF_8C47_31F8DF889DF0__INCLUDED_)
#define AFX_KICKREASONDLG_H__25C49432_E9A9_4BEF_8C47_31F8DF889DF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// KickReasonDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKickReasonDlg dialog

class CKickReasonDlg : public CDialog
{
// Construction
public:
	CKickReasonDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CKickReasonDlg)
	enum { IDD = IDD_KICK_REASON };
	CString	m_reason;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKickReasonDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CKickReasonDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_KICKREASONDLG_H__25C49432_E9A9_4BEF_8C47_31F8DF889DF0__INCLUDED_)
