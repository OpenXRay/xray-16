// chattyDoc.cpp : implementation of the CChattyDoc class
//

#include "stdafx.h"
#include "chatty.h"

#include "chattyDoc.h"
#include "EnterDlg.h"
#include "SetTopicDlg.h"
#include "ChannelModeDlg.h"
#include "TalkDlg.h"
#include "SetPasswordDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChattyDoc

IMPLEMENT_DYNCREATE(CChattyDoc, CDocument)

BEGIN_MESSAGE_MAP(CChattyDoc, CDocument)
	//{{AFX_MSG_MAP(CChattyDoc)
	ON_COMMAND(ID_CHANNEL_SET_TOPIC, OnChannelSetTopic)
	ON_COMMAND(ID_CHANNEL_MODE, OnChannelMode)
	ON_COMMAND(ID_CHANNEL_TALK, OnChannelTalk)
	ON_COMMAND(ID_CHANNEL_PASSWORD, OnChannelPassword)
	ON_COMMAND(ID_CHANNEL_GETBANLIST, OnChannelGetbanlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChattyDoc construction/destruction

CChattyDoc::CChattyDoc()
{
	m_inChannel = FALSE;
	m_hide = FALSE;
}

CChattyDoc::~CChattyDoc()
{
}

/**********************
** CHANNEL CALLBACKS **
**********************/
#if 0
CChattyDoc * pDoc;  // Get rid of VA complaints.
#endif
#define DOC			CChattyDoc * pDoc;\
					pDoc = (CChattyDoc *)param;\
					ASSERT(pDoc != NULL);\
					if(!pDoc->m_inChannel) { OutputDebugString("Not in channel anymore!\n"); return; }
#define ADD(s)		*str += " | ";\
					*str += #s;\
					*str += "--";\
					*str += s;
char __i__[32];
#define ADD_INT(i)	*str += " | ";\
					*str += #i;\
					*str += "--";\
					itoa(i, __i__, 10);\
					*str += __i__;
void ChannelMessage(CHAT chat, const char * channel, const char * user, const char * message, int type, void * param)
{
	OutputDebugString("ChannelMessage called\n");
	DOC;
	CString * str;

	str = new CString("ChannelMessage");
	ADD(channel);
	ADD(user);
	ADD(message);
	ADD_INT(type);
	pDoc->m_addCallbacks.AddTail(str);  // MORE DESCRIPTIVE (SHOW PARAMS)

	str = new CString;

	if(type == CHAT_MESSAGE)
	{
		*str = user;
		*str += ": ";
		*str += message;
	}
	else if(type == CHAT_ACTION)
	{
		*str += user;
		*str += " ";
		*str += message;
	}
	else
	{
		*str += user;
		*str += "* ";
		*str += message;
	}

	pDoc->m_newStuff.AddTail(str);

	GSI_UNUSED(chat);
}

void Kicked(CHAT chat, const char * channel, const char * user, const char * reason, void * param)
{
	OutputDebugString("Kicked called\n");
	DOC;

	// Not in the channel anymore.
	//////////////////////////////
	pDoc->m_inChannel = FALSE;

	POSITION pos = pDoc->GetFirstViewPosition();
	pDoc->GetNextView(pos)->GetParentFrame()->DestroyWindow();

	GSI_UNUSED(reason);
	GSI_UNUSED(user);
	GSI_UNUSED(channel);
	GSI_UNUSED(chat);
}

void UserJoined(CHAT chat, const char * channel, const char * user, int mode, void * param)
{
	OutputDebugString("UserJoined called\n");
	DOC;
	CString * str;

	if(!pDoc->m_hide)
	{
		str = new CString("UserJoined");
		ADD(channel);
		ADD(user);
		ADD_INT(mode);
		pDoc->m_addCallbacks.AddTail(str);
	}

	CModUsers * mod = new CModUsers;
	mod->type = NEW;
	mod->nick = user;
	mod->mode = mode;
	pDoc->m_modUsers.AddTail(mod);

	GSI_UNUSED(chat);
}

void UserParted(CHAT chat, const char * channel, const char * user, int why, const char * reason, const char * kicker, void * param)
{
	OutputDebugString("UserParted called\n");
	DOC;
	CString * str;

	if(!pDoc->m_hide)
	{
		str = new CString("UserParted");
		ADD(channel);
		ADD(user);
		ADD_INT(why);
		ADD(reason);
		ADD(kicker);
		pDoc->m_addCallbacks.AddTail(str);
	}

	CModUsers * mod = new CModUsers;
	mod->type = DEL;
	mod->nick = user;
	pDoc->m_modUsers.AddTail(mod);

	GSI_UNUSED(chat);
}

void UserChangedNick(CHAT chat, const char * channel, const char * oldNick, const char * newNick, void * param)
{
	OutputDebugString("UserChangedNick called\n");
	DOC;
	CString * str;

	if(!pDoc->m_hide)
	{
		str = new CString("UserChangedNick");
		ADD(channel);
		ADD(oldNick);
		ADD(newNick);
		pDoc->m_addCallbacks.AddTail(str);
	}

	CModUsers * mod = new CModUsers;
	mod->type = RENAME;
	mod->nick = oldNick;
	mod->newNick = newNick;
	pDoc->m_modUsers.AddTail(mod);

	GSI_UNUSED(chat);
}

void UserModeChanged(CHAT chat, const char * channel, const char * user, int mode, void * param)
{
	OutputDebugString("UserModeChanged called\n");
	DOC;
	CString *str;

	str = new CString("UserModeChanged");
	ADD(channel);
	ADD(user);
	ADD_INT(mode);
	pDoc->m_addCallbacks.AddTail(str);

	CModUsers * mod = new CModUsers;
	mod->type = MODE;
	mod->nick = user;
	mod->mode = mode;
	pDoc->m_modUsers.AddTail(mod);

	GSI_UNUSED(chat);
}

void TopicChanged(CHAT chat, const char * channel, const char * topic, void * param)
{
	OutputDebugString("TopicChanged called\n");
	DOC;
	CString * str;

	str = new CString("TopicChanged");
	ADD(channel);
	ADD(topic);
	pDoc->m_addCallbacks.AddTail(str);

	CString title = channel;
	title += " - ";
	title += topic;
	pDoc->SetTitle(title);
	
	pDoc->m_topic = topic;

	GSI_UNUSED(chat);
}

void ChannelModeChanged(CHAT chat, const char * channel, CHATChannelMode * mode, void * param)
{
	OutputDebugString("ChannelModeChanged called\n");
	DOC;
	CString * str;

	str = new CString("ChannelModeChanged");
	ADD(channel);
	ADD_INT(mode->InviteOnly);
	ADD_INT(mode->Limit);
	ADD_INT(mode->Moderated);
	ADD_INT(mode->NoExternalMessages);
	ADD_INT(mode->OnlyOpsChangeTopic);
	ADD_INT(mode->Private);
	ADD_INT(mode->Secret);
	pDoc->m_addCallbacks.AddTail(str);

	str = new CString("Channel Mode Changed");
	pDoc->m_newStuff.AddTail(str);

	GSI_UNUSED(chat);
}

void UserListUpdated(CHAT chat, const char * channel, void * param)
{
	OutputDebugString("UserListUpdated called\n");
	DOC;
	CString * str;

	if(!pDoc->m_hide)
	{
		str = new CString("UserListUpdated");
		ADD(channel);
		pDoc->m_addCallbacks.AddTail(str);
	}

	GSI_UNUSED(chat);
}

void EnumUsersCallback(CHAT chat, CHATBool success, const char * channel, int numUsers, const char ** users, int * modes, void * param)
{
	OutputDebugString("EnumUsersCallback called\n");
	CChattyDoc * pDoc = (CChattyDoc *)param;
	
	if(success)
	{
		pDoc->m_numUsers = numUsers;

		int i;
		for(i = 0 ; i < numUsers ; i++)
		{
			CModUsers * mod = new CModUsers;
			ASSERT(mod != NULL);
			mod->type = NEW;
			mod->nick = users[i];
			mod->mode = modes[i];
			pDoc->m_modUsers.AddTail(mod);
		}
	}

	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
}

void EnterChannelCallback(CHAT chat, CHATBool success, CHATEnterResult result, const char * channel, void * param)
{
	OutputDebugString("EnterChannelCallback called\n");
	CChattyDoc * pDoc = (CChattyDoc *)param;

	// Check for success or failure.
	////////////////////////////////
	if(success)
	{
		pDoc->m_inChannel = TRUE;

		// Get the inital list of users.
		////////////////////////////////
		DWORD before = GetTickCount();
		chatEnumUsers(theApp.m_chat, channel, EnumUsersCallback, pDoc, CHATTrue);
		DWORD after = GetTickCount();
		char buffer[128];
		sprintf(buffer, "%dms to enum %d users in %s\n", after - before, pDoc->m_numUsers, pDoc->m_channelName);
		OutputDebugString(buffer);
	}

	GSI_UNUSED(chat);
	GSI_UNUSED(result);
}

BOOL CChattyDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// Check for no connection.
	///////////////////////////
	if(theApp.m_chat == NULL)
		return FALSE;

	// Set the dialog defaults.
	///////////////////////////
	CEnterDlg dlg;
	//dlg.m_channel = "#test";
	//dlg.m_channel = "#montreal";
	dlg.m_channel = "#istanbul";
	dlg.m_password = "";

	// Go modal.
	////////////
	int rcode = dlg.DoModal();

	// Check for a cancel.
	//////////////////////
	if(rcode != IDOK)
		return FALSE;

	// Setup the callbacks.
	///////////////////////
	chatChannelCallbacks callbacks;
	memset(&callbacks, 0, sizeof(chatChannelCallbacks));
	callbacks.channelMessage = ChannelMessage;
	callbacks.channelModeChanged = ChannelModeChanged;
	callbacks.kicked = Kicked;
	callbacks.topicChanged = TopicChanged;
	callbacks.userParted = UserParted;
	callbacks.userJoined = UserJoined;
	callbacks.userListUpdated = UserListUpdated;
	callbacks.userModeChanged = UserModeChanged;
	callbacks.userChangedNick = UserChangedNick;
	callbacks.param = this;

	// Join the group.
	//////////////////
	if(dlg.m_quickChannel != "")
		m_channelName = dlg.m_quickChannel;
	else
		m_channelName = dlg.m_channel;
	chatEnterChannel(theApp.m_chat, m_channelName, dlg.m_password, &callbacks, EnterChannelCallback, this, CHATTrue);

	// Check for failure.
	/////////////////////
	if(!m_inChannel)
		return FALSE;

	// Set the name.
	////////////////
	SetTitle(m_channelName);

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CChattyDoc serialization

void CChattyDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CChattyDoc diagnostics

#ifdef _DEBUG
void CChattyDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CChattyDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChattyDoc commands

void CChattyDoc::OnCloseDocument() 
{
	// Did we ever get in the channel?
	//////////////////////////////////
	if(m_inChannel)
	{
		if(theApp.m_chat != NULL)
		{
			// Leave the channel.
			/////////////////////
			chatLeaveChannel(theApp.m_chat, m_channelName, NULL);

			OutputDebugString("LEFT channel\n");
		}

		m_inChannel = FALSE;
	}
	
	CDocument::OnCloseDocument();
}

void CChattyDoc::OnChannelSetTopic() 
{
	if(theApp.m_chat != NULL)
	{
		CSetTopicDlg dlg;
		dlg.m_topic = m_topic;

		if(dlg.DoModal() == IDOK)
		{
			chatSetChannelTopic(theApp.m_chat, m_channelName, dlg.m_topic);
		}
	}
}

void GetChannelModeCallback(CHAT chat, CHATBool success, const char * channel, CHATChannelMode * mode, void * param)
{
	CHATChannelMode * modeParam = (CHATChannelMode *)param;
	*modeParam = *mode;

	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
	GSI_UNUSED(success);
}

void CChattyDoc::OnChannelMode() 
{
	if(theApp.m_chat != NULL)
	{
		CHATChannelMode mode;

		// Get the channel mode.
		////////////////////////
		chatGetChannelMode(theApp.m_chat, m_channelName, GetChannelModeCallback, &mode, CHATTrue);

		// Setup the dlg.
		/////////////////
		CChannelModeDlg dlg;
		dlg.m_inviteOnly = mode.InviteOnly;
		dlg.m_private = mode.Private;
		dlg.m_secret = mode.Secret;
		dlg.m_moderated = mode.Moderated;
		dlg.m_noExternalMessages = mode.NoExternalMessages;
		dlg.m_onlyOpsChangeTopic = mode.OnlyOpsChangeTopic;
		dlg.m_limit = mode.Limit;

		// Show the dlg.
		////////////////
		if(dlg.DoModal() == IDOK)
		{
			// Copy back the new mode.
			//////////////////////////
			mode.InviteOnly = (CHATBool)dlg.m_inviteOnly;
			mode.Private = (CHATBool)dlg.m_private;
			mode.Secret = (CHATBool)dlg.m_secret;
			mode.Moderated = (CHATBool)dlg.m_moderated;
			mode.NoExternalMessages = (CHATBool)dlg.m_noExternalMessages;
			mode.OnlyOpsChangeTopic = (CHATBool)dlg.m_onlyOpsChangeTopic;
			mode.Limit = (CHATBool)dlg.m_limit;

			// Set the mode.
			////////////////
			chatSetChannelMode(theApp.m_chat, m_channelName, &mode);
		}
	}
}

void CChattyDoc::OnChannelTalk() 
{
	if(theApp.m_chat != NULL)
	{
		CTalkDlg dlg;
		dlg.m_type = 0;
		if(dlg.DoModal() == IDOK)
		{
			int type;
			if(dlg.m_type == 0)
				type = CHAT_NORMAL;
			else if(dlg.m_type == 1)
				type = CHAT_ACTION;
			else
				type = CHAT_NOTICE;
			chatSendChannelMessage(theApp.m_chat, m_channelName, dlg.m_message, type);
		}
	}
}

void GetChannelPasswordCallback(CHAT chat, CHATBool success, const char * channel, CHATBool enabled, const char * password, void * param)
{
	CSetPasswordDlg * dlg = (CSetPasswordDlg *)param;
	if(success)
	{
		dlg->m_password = password;
		dlg->m_enable = enabled;
	}
	else
	{
		dlg->m_password = "";
		dlg->m_enable = FALSE;
	}

	GSI_UNUSED(channel);
	GSI_UNUSED(chat);
}

void CChattyDoc::OnChannelPassword() 
{
	if(theApp.m_chat != NULL)
	{
		CSetPasswordDlg dlg;
		chatGetChannelPassword(theApp.m_chat, m_channelName, GetChannelPasswordCallback, &dlg, CHATTrue);
		if(dlg.DoModal() == IDOK)
		{
			chatSetChannelPassword(theApp.m_chat, m_channelName, (CHATBool)dlg.m_enable, dlg.m_password);
		}
	}
}

void EnumChannelBansCallback(CHAT chat, CHATBool success, const char * channel, int numBans, const char ** bans, void * param)
{
	DOC;
	int i;

	pDoc->m_newStuff.AddTail(new CString("Bans:"));
	for(i = 0 ; i < numBans ; i++)
	{
		pDoc->m_newStuff.AddTail(new CString(bans[i]));
	}

	GSI_UNUSED(channel);
	GSI_UNUSED(success);
	GSI_UNUSED(chat);
}

void CChattyDoc::OnChannelGetbanlist() 
{
	if(theApp.m_chat != NULL)
	{
		chatEnumChannelBans(theApp.m_chat, m_channelName, EnumChannelBansCallback, this, CHATFalse);
	}
}
