#if !defined(AFX_CHANNELLISTDLG_H__2D417F5B_EB62_4913_BB7A_6B20477ED21D__INCLUDED_)
#define AFX_CHANNELLISTDLG_H__2D417F5B_EB62_4913_BB7A_6B20477ED21D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChannelListDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChannelListDlg dialog

class CChannelListDlg : public CDialog
{
// Construction
public:
	CChannelListDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChannelListDlg)
	enum { IDD = IDD_CHANNEL_LIST };
	CEdit	m_num;
	CListBox	m_list;
	CString	m_filter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChannelListDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChannelListDlg)
	afx_msg void OnChannels();
	afx_msg void OnUsers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHANNELLISTDLG_H__2D417F5B_EB62_4913_BB7A_6B20477ED21D__INCLUDED_)
