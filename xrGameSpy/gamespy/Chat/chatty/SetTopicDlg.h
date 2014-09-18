#if !defined(AFX_SETTOPICDLG_H__15C72001_C3D8_11D3_BD38_00C0F056BC39__INCLUDED_)
#define AFX_SETTOPICDLG_H__15C72001_C3D8_11D3_BD38_00C0F056BC39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetTopicDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetTopicDlg dialog

class CSetTopicDlg : public CDialog
{
// Construction
public:
	CSetTopicDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetTopicDlg)
	enum { IDD = IDD_SET_TOPIC };
	CString	m_topic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetTopicDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetTopicDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTOPICDLG_H__15C72001_C3D8_11D3_BD38_00C0F056BC39__INCLUDED_)
