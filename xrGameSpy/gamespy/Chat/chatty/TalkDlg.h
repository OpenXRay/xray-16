#if !defined(AFX_TALKDLG_H__87449421_C42C_11D3_BD38_00C0F056BC39__INCLUDED_)
#define AFX_TALKDLG_H__87449421_C42C_11D3_BD38_00C0F056BC39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TalkDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTalkDlg dialog

class CTalkDlg : public CDialog
{
// Construction
public:
	CTalkDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTalkDlg)
	enum { IDD = IDD_TALK };
	CString	m_message;
	int		m_type;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTalkDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTalkDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TALKDLG_H__87449421_C42C_11D3_BD38_00C0F056BC39__INCLUDED_)
