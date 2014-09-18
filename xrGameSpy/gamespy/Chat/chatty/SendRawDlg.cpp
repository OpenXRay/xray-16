// SendRawDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "SendRawDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSendRawDlg dialog


CSendRawDlg::CSendRawDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSendRawDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSendRawDlg)
	m_raw = _T("");
	//}}AFX_DATA_INIT
}


void CSendRawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSendRawDlg)
	DDX_Text(pDX, IDC_RAW, m_raw);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSendRawDlg, CDialog)
	//{{AFX_MSG_MAP(CSendRawDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendRawDlg message handlers
