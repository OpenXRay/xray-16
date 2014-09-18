#if !defined(AFX_SIDEBARCTRL_H__CC0DCA41_C330_11D5_A480_000102C2601F__INCLUDED_)
#define AFX_SIDEBARCTRL_H__CC0DCA41_C330_11D5_A480_000102C2601F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SideBarCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSideBarCtrl window

class CSideBarCtrl : public CStatic
{
// Construction
public:
	CSideBarCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSideBarCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSideBarCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSideBarCtrl)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG

	void DrawGradient(CDC* pDC, const CRect &rcGrad, COLORREF clrStart, COLORREF clrEnd, BOOL bVertical, COLORREF clrDither = 0xFFFFFFFF );

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIDEBARCTRL_H__CC0DCA41_C330_11D5_A480_000102C2601F__INCLUDED_)
