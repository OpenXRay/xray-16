#if !defined(AFX_TITLEPAGE_H__15BC897B_80B2_4DED_8622_568C60F30225__INCLUDED_)
#define AFX_TITLEPAGE_H__15BC897B_80B2_4DED_8622_568C60F30225__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TitlePage.h : header file
//

#include "../peer.h"

/////////////////////////////////////////////////////////////////////////////
// CTitlePage dialog

class CTitlePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CTitlePage)

// Construction
public:
	void JoinGroup(int nIndex);
	CTitlePage();
	~CTitlePage();

// Dialog Data
	//{{AFX_DATA(CTitlePage)
	enum { IDD = IDD_TITLE_PAGE };
	CListCtrl	m_groups;
	CListCtrl	m_players;
	CListBox	m_chatWindow;
	CString	m_message;
	//}}AFX_DATA

	void UpdatePlayerPing(const char * nick, int ping);
	int FindPlayer(const char * nick);
	void RemovePlayer(const char * nick);
	void ChangePlayerNick(const char * oldNick, const char * newNick);

	void SendMessage();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTitlePage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTitlePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickGroups(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkGroups(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragGroups(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CTitlePage * TitlePage;

#endif // !defined(AFX_TITLEPAGE_H__15BC897B_80B2_4DED_8622_568C60F30225__INCLUDED_)
