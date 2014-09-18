#if !defined(AFX_CHANNELMODEDLG_H__87449420_C42C_11D3_BD38_00C0F056BC39__INCLUDED_)
#define AFX_CHANNELMODEDLG_H__87449420_C42C_11D3_BD38_00C0F056BC39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChannelModeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChannelModeDlg dialog

class CChannelModeDlg : public CDialog
{
// Construction
public:
	CChannelModeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChannelModeDlg)
	enum { IDD = IDD_CHANNEL_MODE };
	BOOL	m_inviteOnly;
	int		m_limit;
	BOOL	m_moderated;
	BOOL	m_noExternalMessages;
	BOOL	m_onlyOpsChangeTopic;
	BOOL	m_private;
	BOOL	m_secret;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChannelModeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChannelModeDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHANNELMODEDLG_H__87449420_C42C_11D3_BD38_00C0F056BC39__INCLUDED_)
