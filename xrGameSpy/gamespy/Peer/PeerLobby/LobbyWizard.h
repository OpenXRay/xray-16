#if !defined(AFX_LOBBYWIZARD_H__683E0CE1_CBA4_46D1_948A_6047481CDBF6__INCLUDED_)
#define AFX_LOBBYWIZARD_H__683E0CE1_CBA4_46D1_948A_6047481CDBF6__INCLUDED_

#include "../peer.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LobbyWizard.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLobbyWizard

#define CONNECT_PAGE   0
#define TITLE_PAGE     1
#define GROUP_PAGE     2
#define CREATE_PAGE    3
#define STAGING_PAGE   4

class CLobbyWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CLobbyWizard)

// Construction
public:
	CLobbyWizard(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CLobbyWizard(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	PEER m_peer;        // The peer object.
	BOOL m_hosting;     // TRUE if hosting a game.
	BOOL m_groupRooms;  // TRUE if using group rooms.

	HWND m_nextButtonWnd;

	HICON m_greenSmileyIcon;
	int m_greenSmileyIndex;
	HICON m_yellowSmileyIcon;
	int m_yellowSmileyIndex;
	HICON m_redSmileyIcon;
	int m_redSmileyIndex;
	HICON m_stagingRoomIcon;
	int m_stagingRoomIndex;
	HICON m_runningGameIcon;
	int m_runningGameIndex;
	CImageList m_imageList;

	HCURSOR m_lastCursor;

// Operations
public:
	void StartHourglass();
	void StopHourglass();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLobbyWizard)
	public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLobbyWizard();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLobbyWizard)
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CLobbyWizard * Wizard;

#endif // !defined(AFX_LOBBYWIZARD_H__683E0CE1_CBA4_46D1_948A_6047481CDBF6__INCLUDED_)
