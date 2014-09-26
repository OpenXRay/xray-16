#if !defined(AFX_STAGINGPAGE_H__E66D2915_B2B7_4A43_B875_D0C905FA1395__INCLUDED_)
#define AFX_STAGINGPAGE_H__E66D2915_B2B7_4A43_B875_D0C905FA1395__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StagingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStagingPage dialog

class CStagingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CStagingPage)

// Construction
public:
	CStagingPage();
	~CStagingPage();

// Dialog Data
	//{{AFX_DATA(CStagingPage)
	enum { IDD = IDD_STAGING_PAGE };
	CListCtrl	m_players;
	CListBox	m_chatWindow;
	CString	m_message;
	int		m_ready;
	//}}AFX_DATA

	int FindPlayer(const char * nick);
	void UpdatePlayerPing(const char * nick, int ping);
	void UpdatePlayerReady(const char * nick, BOOL ready);
	void RemovePlayer(const char * nick);
	void ChangePlayerNick(const char * oldNick, const char * newNick);

	void SendMessage();

	void CheckEnableFinish();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStagingPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStagingPage)
	afx_msg void OnReady();
	afx_msg void OnNotReady();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CStagingPage * StagingPage;

#endif // !defined(AFX_STAGINGPAGE_H__E66D2915_B2B7_4A43_B875_D0C905FA1395__INCLUDED_)
