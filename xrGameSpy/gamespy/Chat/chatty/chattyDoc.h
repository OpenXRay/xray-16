// chattyDoc.h : interface of the CChattyDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHATTYDOC_H__A9A7E462_6E3D_4AE0_9DF8_355AE2D28FBC__INCLUDED_)
#define AFX_CHATTYDOC_H__A9A7E462_6E3D_4AE0_9DF8_355AE2D28FBC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define NEW      1
#define DEL      2
#define RENAME   3
#define MODE     4
struct CModUsers
{
	int type;
	CString nick;
	CString newNick;
	int mode;
};

class CChattyDoc : public CDocument
{
protected: // create from serialization only
	CChattyDoc();
	DECLARE_DYNCREATE(CChattyDoc)

// Attributes
public:
	BOOL m_inChannel;
	CString m_channelName;
	CList <CString *, CString *> m_newStuff;
	CString m_topic;
	CList <CString *, CString *> m_addCallbacks;
	CList <CModUsers *, CModUsers *> m_modUsers;
	BOOL m_hide;
	int m_numUsers;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChattyDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChattyDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CChattyDoc)
	afx_msg void OnChannelSetTopic();
	afx_msg void OnChannelMode();
	afx_msg void OnChannelTalk();
	afx_msg void OnChannelPassword();
	afx_msg void OnChannelGetbanlist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHATTYDOC_H__A9A7E462_6E3D_4AE0_9DF8_355AE2D28FBC__INCLUDED_)
