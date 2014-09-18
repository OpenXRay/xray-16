// chattyView.h : interface of the CChattyView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHATTYVIEW_H__E1B25967_4DA7_4ABD_A404_9C60D59E1C60__INCLUDED_)
#define AFX_CHATTYVIEW_H__E1B25967_4DA7_4ABD_A404_9C60D59E1C60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CChattyView : public CFormView
{
protected: // create from serialization only
	CChattyView();
	DECLARE_DYNCREATE(CChattyView)

public:
	//{{AFX_DATA(CChattyView)
	enum { IDD = IDD_CHATTY_FORM };
	CEdit	m_numUsers;
	CListBox	m_callbacks;
	CListBox	m_users;
	CListBox	m_list;
	CString	m_edit;
	BOOL	m_hide;
	//}}AFX_DATA

// Attributes
public:
	CChattyDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChattyView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChattyView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CChattyView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnHide();
	afx_msg void OnSelchangeCallbacks();
	afx_msg void OnDblclkUsers();
	afx_msg void OnUserBan();
	afx_msg void OnUserGetinfo();
	afx_msg void OnUserKick();
	afx_msg void OnUserModeOp();
	afx_msg void OnUserModeVoice();
	afx_msg void OnUserModeNormal();
	afx_msg void OnSendbutt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in chattyView.cpp
inline CChattyDoc* CChattyView::GetDocument()
   { return (CChattyDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHATTYVIEW_H__E1B25967_4DA7_4ABD_A404_9C60D59E1C60__INCLUDED_)
