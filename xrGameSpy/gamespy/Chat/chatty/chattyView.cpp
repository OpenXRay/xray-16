// chattyView.cpp : implementation of the CChattyView class
//

#include "stdafx.h"
#include "chatty.h"

#include "chattyDoc.h"
#include "chattyView.h"
#include "TalkDlg.h"
#include "KickReasonDlg.h"
#include "GetUserInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChattyView

IMPLEMENT_DYNCREATE(CChattyView, CFormView)

BEGIN_MESSAGE_MAP(CChattyView, CFormView)
	//{{AFX_MSG_MAP(CChattyView)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_HIDE, OnHide)
	ON_LBN_SELCHANGE(IDC_CALLBACKS, OnSelchangeCallbacks)
	ON_LBN_DBLCLK(IDC_USERS, OnDblclkUsers)
	ON_COMMAND(ID_USER_BAN, OnUserBan)
	ON_COMMAND(ID_USER_GETINFO, OnUserGetinfo)
	ON_COMMAND(ID_USER_KICK, OnUserKick)
	ON_COMMAND(ID_USER_MODE_OP, OnUserModeOp)
	ON_COMMAND(ID_USER_MODE_VOICE, OnUserModeVoice)
	ON_COMMAND(ID_USER_MODE_NORMAL, OnUserModeNormal)
	ON_BN_CLICKED(IDC_SENDBUTT, OnSendbutt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChattyView construction/destruction

CChattyView::CChattyView()
	: CFormView(CChattyView::IDD)
{
	//{{AFX_DATA_INIT(CChattyView)
	m_edit = _T("");
	m_hide = FALSE;
	//}}AFX_DATA_INIT
	// TODO: add construction code here
}

CChattyView::~CChattyView()
{
}

void CChattyView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChattyView)
	DDX_Control(pDX, IDC_NUM_USERS, m_numUsers);
	DDX_Control(pDX, IDC_CALLBACKS, m_callbacks);
	DDX_Control(pDX, IDC_USERS, m_users);
	DDX_Control(pDX, IDC_LIST, m_list);
	DDX_Text(pDX, IDC_EDIT, m_edit);
	DDX_Check(pDX, IDC_HIDE, m_hide);
	//}}AFX_DATA_MAP
}

BOOL CChattyView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CChattyView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	ResizeParentToFit();

	SetTimer(100, 50, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CChattyView diagnostics

#ifdef _DEBUG
void CChattyView::AssertValid() const
{
	CFormView::AssertValid();
}

void CChattyView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CChattyDoc* CChattyView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CChattyDoc)));
	return (CChattyDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChattyView message handlers

int CChattyView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFormView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void CChattyView::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 100)
	{
		CChattyDoc * pDoc = GetDocument();
		if(pDoc != NULL)
		{
			CString * str;
			while(!pDoc->m_newStuff.IsEmpty())
			{
				str = pDoc->m_newStuff.GetHead();
				pDoc->m_newStuff.RemoveHead();

				m_list.InsertString(0, *str);

				delete str;
			}

			while(!pDoc->m_addCallbacks.IsEmpty())
			{
				str = pDoc->m_addCallbacks.GetHead();
				pDoc->m_addCallbacks.RemoveHead();

				m_callbacks.InsertString(0, *str);

				delete str;
			}

			CModUsers * mod;
			CString name;
			CString newName;
			UINT index;
			while(!pDoc->m_modUsers.IsEmpty())
			{
				mod = pDoc->m_modUsers.GetHead();
				pDoc->m_modUsers.RemoveHead();

				if(mod->type == NEW)
				{
					name = mod->nick;
					if(mod->mode & CHAT_OP)
						name.Insert(0, '@');
					else if(mod->mode & CHAT_VOICE)
						name.Insert(0, '?');

					m_users.AddString(name);
				}
				else if(mod->type == DEL)
				{
					name = mod->nick;
					index = m_users.FindStringExact(-1, name);
					if(index == LB_ERR)
						index = m_users.FindStringExact(-1, '@' + name);
					if(index == LB_ERR)
						index = m_users.FindStringExact(-1, '?' + name);
					if(index == LB_ERR)
						ASSERT(0);

					m_users.DeleteString(index);
				}
				else if(mod->type == RENAME)
				{
					name = mod->nick;
					newName = "";
					index = m_users.FindStringExact(-1, name);
					if(index == LB_ERR)
					{
						newName = '@';
						index = m_users.FindStringExact(-1, '@' + name);
					}
					if(index == LB_ERR)
					{
						newName = '?';
						index = m_users.FindStringExact(-1, '?' + name);
					}
					if(index == LB_ERR)
						ASSERT(0);

					m_users.DeleteString(index);

					newName += mod->newNick;
					m_users.AddString(newName);
				}
				else if(mod->type == MODE)
				{
					name = mod->nick;
					index = m_users.FindStringExact(-1, name);
					if(index == LB_ERR)
						index = m_users.FindStringExact(-1, '@' + name);
					if(index == LB_ERR)
						index = m_users.FindStringExact(-1, '?' + name);
					if(index == LB_ERR)
						ASSERT(0);

					m_users.DeleteString(index);

					if(mod->mode & CHAT_OP)
						name.Insert(0, '@');
					else if(mod->mode & CHAT_VOICE)
						name.Insert(0, '?');

					m_users.AddString(name);
				}
				else
					ASSERT(0);

				delete mod;
			}

			char buf[16];
			itoa(m_users.GetCount(), buf, 10);
			m_numUsers.SetWindowText(buf);
		}

		SetTimer(100, 50, NULL);
	}
	
	CFormView::OnTimer(nIDEvent);
}

void CChattyView::OnHide() 
{
	UpdateData();
	GetDocument()->m_hide = m_hide;
}

void CChattyView::OnSelchangeCallbacks() 
{
	UpdateData();
	m_callbacks.GetText(m_callbacks.GetCurSel(), m_edit);
	UpdateData(FALSE);
}

void CChattyView::OnDblclkUsers() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);
			CTalkDlg dlg;
			dlg.m_type = 0;
			if(dlg.DoModal() == IDOK)
				chatSendUserMessage(theApp.m_chat, user, dlg.m_message, dlg.m_type);
		}
	}
}

void CChattyView::OnUserBan() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			chatBanUser(theApp.m_chat, GetDocument()->m_channelName, user);
		}
	}
}

void CChattyView::OnUserGetinfo() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			CGetUserInfoDlg dlg;
			dlg.m_user = user;
			dlg.DoModal();
		}
	}
}

void CChattyView::OnUserKick() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			CKickReasonDlg dlg;
			if(dlg.DoModal() == IDOK)
			{
				chatKickUser(theApp.m_chat, GetDocument()->m_channelName, user, dlg.m_reason);
			}
		}
	}
}

void CChattyView::OnUserModeOp() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			chatSetUserMode(theApp.m_chat, GetDocument()->m_channelName, user, CHAT_OP);
		}
	}
}

void CChattyView::OnUserModeVoice() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			chatSetUserMode(theApp.m_chat, GetDocument()->m_channelName, user, CHAT_VOICE);
		}
	}
}

void CChattyView::OnUserModeNormal() 
{
	if(theApp.m_chat != NULL)
	{
		int index = m_users.GetCurSel();
		if(index != LB_ERR)
		{
			CString user;
			m_users.GetText(index, user);
			if((user.Left(1) == "@") || (user.Left(1) == "?"))
				user = user.Mid(1);

			chatSetUserMode(theApp.m_chat, GetDocument()->m_channelName, user, CHAT_NORMAL);
		}
	}
}


void CChattyView::OnSendbutt() 
{
	// TODO: Add your control notification handler code here
	if(theApp.m_chat != NULL)
	{
		UpdateData();
		chatSendChannelMessage(theApp.m_chat, GetDocument()->m_channelName, (LPCSTR)m_edit,CHAT_MESSAGE);
		m_edit.Empty();
		UpdateData(FALSE);
	}	
	
}
