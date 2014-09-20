// TalkDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "TalkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTalkDlg dialog


CTalkDlg::CTalkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTalkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTalkDlg)
	m_message = _T("");
	m_type = -1;
	//}}AFX_DATA_INIT
}


void CTalkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTalkDlg)
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	DDX_Radio(pDX, IDC_NORMAL, m_type);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTalkDlg, CDialog)
	//{{AFX_MSG_MAP(CTalkDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTalkDlg message handlers
