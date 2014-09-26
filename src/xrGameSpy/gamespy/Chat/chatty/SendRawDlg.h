#if !defined(AFX_SENDRAWDLG_H__543BE7A8_4BE8_4ED0_972D_2339364077F6__INCLUDED_)
#define AFX_SENDRAWDLG_H__543BE7A8_4BE8_4ED0_972D_2339364077F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SendRawDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSendRawDlg dialog

class CSendRawDlg : public CDialog
{
// Construction
public:
	CSendRawDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSendRawDlg)
	enum { IDD = IDD_SEND_RAW };
	CString	m_raw;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSendRawDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSendRawDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SENDRAWDLG_H__543BE7A8_4BE8_4ED0_972D_2339364077F6__INCLUDED_)
