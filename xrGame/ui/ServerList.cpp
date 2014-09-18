#include "StdAfx.h"
#include "ServerList.h"
#include "UIXmlInit.h"
#include "../string_table.h"
#include "../../xrEngine/xr_ioconsole.h"
#include "UIEditBox.h"
#include "UIMessageBoxEx.h"
#include "UIMessageBox.h"
#include "TeamInfo.h"
#include "../MainMenu.h"
#include "../login_manager.h"
#include "../GameSpy/GameSpy_Full.h"
#include "../GameSpy/GameSpy_Browser.h"


LPCSTR GameTypeToString(EGameIDs gt, bool bShort);
CGameSpy_Browser* g_gs_browser = NULL;

CServerList::CServerList()
{
	m_GSBrowser	= MainMenu()->GetGS()->GetGameSpyBrowser();
	browser().Init(this);

	for (int i = 0; i<LST_COLUMN_COUNT; i++)
		AttachChild(&m_header_frames[i]);

	for (int i = 0; i<LST_COLUMN_COUNT; i++)
		AttachChild(&m_header[i]);

	for (int i = 0; i<4; i++)
		AttachChild(&m_header2[i]);

	AttachChild(&m_edit_gs_filter);


	for (int i = 0; i<3; i++)
	{
		m_list[i].Show(true);
		AttachChild					(&m_frame[i]);
		AttachChild					(&m_list[i]);
	}

	m_bShowServerInfo				= false;
	m_bAnimation					= false;

	m_sort_func						= "none";
	m_message_box					= xr_new<CUIMessageBoxEx>();
	m_message_box->InitMessageBox	("message_box_password");
	m_message_box->SetMessageTarget	(this);
	
	m_b_local						= false;

	m_last_retreived_index			= u32(-1);
	m_need_refresh_fr				= u32(-1);
}

CServerList::~CServerList()
{
	xr_delete			(m_message_box);
	if (m_GSBrowser)
		m_GSBrowser->Clear	();

	DestroySrvItems		();
};

inline CGameSpy_Browser& CServerList::browser	() const
{
	VERIFY				( m_GSBrowser );
	return				( *m_GSBrowser );
}

void CServerList::on_game_spy_browser_destroy	(CGameSpy_Browser* browser)
{
	VERIFY				(m_GSBrowser);
	VERIFY				(m_GSBrowser == browser);
	m_GSBrowser			= 0;
}

void CServerList::Update()
{
	if (m_need_refresh_fr<Device.dwFrame+10)
		RefreshList_internal();

	if (m_bAnimation)
	{
//		m_pAnimation->Update();
//		m_frame[LST_SRV_PROP].SetColor(subst_alpha(0xffffffff, color_get_A(m_pAnimation->GetColor())));
//		m_frame[LST_PLAYERS].SetColor(subst_alpha(0xffffffff, color_get_A(m_pAnimation->GetColor())));


		if ( true /*m_pAnimation->Done()*/)
		{
			m_bAnimation = false;
			if (m_bShowServerInfo)
				AfterAppear();
			else
				AfterDisappear();
		}
	}
	CUIWindow::Update();
}

bool CServerList::NeedToRefreshCurServer	()
{
	CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
	if(!pItem)
		return false;
	return browser().HasAllKeys(pItem->GetInfo()->info.Index) == false;
};

void CServerList::SendMessage(CUIWindow* pWnd, s16 msg, void* pData){
	if (m_bShowServerInfo && LIST_ITEM_CLICKED == msg && &m_list[LST_SERVER] == pWnd){
		if (NeedToRefreshCurServer())
		{
			RefreshQuick();
		}
		else
		{
			ClearDetailedServerInfo();
			FillUpDetailedServerInfo();
		}
	}
	else if (BUTTON_CLICKED == msg){
		if (pWnd == &m_header[1]){
			SetSortFunc("server_name",true);
		}
		else if (pWnd == &m_header[2]){
			SetSortFunc("map",true);
		}
		else if (pWnd == &m_header[3]){
			SetSortFunc("game_type",true);
		}
		else if (pWnd == &m_header[4]){
			SetSortFunc("player",true);
		}
		else if (pWnd == &m_header[5]){
			SetSortFunc("ping",true);
		}
		else if (pWnd == &m_header[6]){
			SetSortFunc("version",true);
		}
	}else if( EDIT_TEXT_COMMIT == msg && pWnd==&m_edit_gs_filter){
		RefreshGameSpyList(m_b_local);
	}
	else if ( MESSAGE_BOX_YES_CLICKED == msg )
	{
		CUIListItemServer* item		= smart_cast<CUIListItemServer*>(m_list[LST_SERVER].GetSelectedItem());
		if(!item)
			return;
		xr_string					command;

		item->CreateConsoleCommand	(command, m_playerName.c_str(), m_message_box->m_pMessageBox->GetUserPassword(), m_message_box->GetPassword() );
		Console->Execute			(command.c_str());
	}
	else if ( WINDOW_LBUTTON_DB_CLICK == msg && &m_list[LST_SERVER] == pWnd )
	{
		ConnectToSelected();
	}
}

void CServerList::BeforeAppear()
{
    UpdateSizes				();
	UpdateVisibility		();
}

void CServerList::AfterAppear()
{
	FillUpDetailedServerInfo();
	UpdateVisibility		();
}

void CServerList::BeforeDisapear()
{
	ClearDetailedServerInfo	();
	UpdateVisibility		();
}

void CServerList::AfterDisappear()
{
	UpdateSizes				();
	UpdateVisibility		();
}

void CServerList::FillUpDetailedServerInfo()
{	

	bool t1 = false;
	bool t2 = false;
	bool spect = false;
		
	CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
	if(pItem)
	{
		ServerInfo srvInfo;
		browser().GetServerInfoByIndex(&srvInfo, pItem->GetInfo()->info.Index);
		u32 teams = srvInfo.m_aTeams.size();

		if (2 == teams)
		{
			LPSTR _buff = NULL;

			CUIListBoxItem* pItemAdv;

			// TEAM 1
			xr_vector<PlayerInfo>::iterator it;
			for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
			{
				PlayerInfo pf = *it;
				if (1 != pf.Team)
					continue;
				if (pf.Spectator)
					continue;

				if (!t1)		// add header
				{
					STRCONCAT(_buff, CStringTable().translate("ui_st_team").c_str(),
						"\"", CTeamInfo::GetTeam1_name().c_str(), "\"");

					pItemAdv					= m_list[LST_PLAYERS].AddItem();
					pItemAdv->SetTextColor		(m_list[LST_PLAYERS].GetTextColor());
					pItemAdv->SetFont			(m_list[LST_PLAYERS].GetFont());
					pItemAdv->SetText			(_buff);
					pItemAdv->GetTextItem()->SetWidth(m_list[LST_PLAYERS].GetDesiredChildWidth());
					t1 = true;
				}


 				pItemAdv						= m_list[LST_PLAYERS].AddItem();

				char buf[16];
				pItemAdv->SetTextColor			(m_list[LST_PLAYERS].GetTextColor());
				pItemAdv->SetFont				(m_list[LST_PLAYERS].GetFont());
				pItemAdv->SetText	(pf.Name);
				pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Frags);
				pItemAdv->AddTextField			(buf, m_header2[2].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Deaths);
				pItemAdv->AddTextField			(buf, m_header2[3].GetWidth());
			}

			
			// TEAM 2
			for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); it++)
			{
				PlayerInfo pf = *it;
				if (2 != pf.Team)
					continue;
				if (pf.Spectator)
					continue;

				if (!t2)
				{
					STRCONCAT(_buff, CStringTable().translate("ui_st_team").c_str(),
						"\"", CTeamInfo::GetTeam2_name().c_str(), "\"");

					m_list[LST_PLAYERS].AddTextItem	(_buff);

					t2 = true;
				}

				pItemAdv						= m_list[LST_PLAYERS].AddItem();

				char buf[16];
				pItemAdv->SetTextColor			(m_list[LST_PLAYERS].GetTextColor());
				pItemAdv->SetFont				(m_list[LST_PLAYERS].GetFont());
				pItemAdv->SetText				(pf.Name);
				pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());
				
				xr_sprintf						(buf,sizeof(buf),"%d",pf.Frags);
				pItemAdv->AddTextField			(buf, m_header2[2].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Deaths);
				pItemAdv->AddTextField			(buf, m_header2[3].GetWidth());
			}

			// SPECTATORS
			for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end();++it)
			{
				PlayerInfo pf = *it;
				if (!pf.Spectator)
					continue;

				if (!spect)
				{
					pItemAdv					= m_list[LST_PLAYERS].AddTextItem(CStringTable().translate("mp_spectator").c_str());
					spect = true;
				}

				pItemAdv						= m_list[LST_PLAYERS].AddItem();

				char buf[16];
				pItemAdv->SetFont				(m_list[LST_PLAYERS].GetFont());
				pItemAdv->SetTextColor			(m_list[LST_PLAYERS].GetTextColor());
				pItemAdv->SetText				(pf.Name);
				pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Frags);
				pItemAdv->AddTextField			(buf, m_header2[2].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Deaths);
				pItemAdv->AddTextField			(buf, m_header2[3].GetWidth());
			}

		}
		else
		{
			xr_vector<PlayerInfo>::iterator it;
			for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
			{
				PlayerInfo pf = *it;
				CUIListBoxItem* pItemAdv		= m_list[LST_PLAYERS].AddItem();

				char buf[16];

				pItemAdv->SetTextColor			(m_list[LST_PLAYERS].GetTextColor());
				pItemAdv->SetFont				(m_list[LST_PLAYERS].GetFont());
				pItemAdv->SetText				(pf.Name);
				pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Frags);
				pItemAdv->AddTextField			(buf, m_header2[2].GetWidth());

				xr_sprintf						(buf,sizeof(buf),"%d",pf.Deaths);
				pItemAdv->AddTextField			(buf, m_header2[3].GetWidth());
			}
		}

		xr_vector<GameInfo>::iterator it;
		for (it = srvInfo.m_aInfos.begin(); it != srvInfo.m_aInfos.end(); ++it)
		{
			GameInfo gi							= *it;
			CUIListBoxItem* pItemAdv			= m_list[LST_SRV_PROP].AddItem();

			pItemAdv->SetText					(gi.InfoName.c_str());
			pItemAdv->GetTextItem()->SetWidth	(m_list[LST_SRV_PROP].GetWidth()/2);

			pItemAdv->AddTextField				(gi.InfoData.c_str(), m_list[LST_SRV_PROP].GetWidth()/2);
		}
	}
	else
		ClearDetailedServerInfo		();
}

void CServerList::ClearDetailedServerInfo()
{
	m_list[LST_SRV_PROP].Clear();
	m_list[LST_PLAYERS].Clear();
}

void CServerList::ShowServerInfo()
{
	if (!m_bShowServerInfo && NeedToRefreshCurServer())
	{
		RefreshQuick();
	}
	m_bShowServerInfo				= !m_bShowServerInfo;
	m_bAnimation					= true;
	if (m_bShowServerInfo)
		BeforeAppear				();
	else
		BeforeDisapear				();
}

void CServerList::UpdateSizes()
{
	float height = m_bShowServerInfo ? m_fListH[1] : m_fListH[0];
	m_list[LST_SERVER].SetHeight(height);
	int page_size = (m_list[LST_SERVER].GetSize()*m_list[LST_SERVER].GetItemHeight() < height) ? 0 : int(height);
	m_list[LST_SERVER].ScrollBar()->SetPageSize(page_size);
	m_list[LST_SERVER].ForceUpdate();

	m_frame[LST_SERVER].SetHeight	(height+2.0f);
	Fvector2 pos					= m_edit_gs_filter.GetWndPos();
	pos.y							= m_bShowServerInfo?m_fEditPos[1]:m_fEditPos[0];
	m_edit_gs_filter.SetWndPos		(pos);
}

void CServerList::UpdateVisibility()
{
	m_list[LST_SRV_PROP].Show		(m_bShowServerInfo ? !m_bAnimation : false);
	m_list[LST_PLAYERS].Show		(m_bShowServerInfo ? !m_bAnimation: false);
	m_frame[LST_SRV_PROP].Show		(m_bShowServerInfo ? true : m_bAnimation);
	m_frame[LST_PLAYERS].Show		(m_bShowServerInfo ? true : m_bAnimation);

	for (int i = 0; i< 4; i++)
		m_header2[i].Show(m_bShowServerInfo ? true : m_bAnimation);
}

void CServerList::SetFilters(SServerFilters& sf)
{
	m_sf = sf;
	RefreshList();
}

void CServerList::SetPlayerName(LPCSTR name)
{
	m_playerName = name;
}

bool CServerList::IsValidItem(ServerInfo& item)
{
	bool result = true;

	result &= !m_sf.empty ? (m_sf.empty == (item.m_ServerNumPlayers == 0))						: true;
	result &= !m_sf.full ? (m_sf.full == (item.m_ServerNumPlayers == item.m_ServerMaxPlayers))	: true;
	result &= !m_sf.with_pass ? (m_sf.with_pass == item.m_bPassword)							: true;
	result &= !m_sf.without_pass ? (m_sf.without_pass != item.m_bPassword)						: true;
	result &= !m_sf.without_ff ? (m_sf.without_ff != item.m_bFFire)								: true;
	result &= !m_sf.listen_servers ? (m_sf.listen_servers != item.m_bDedicated)					: true;

	return result;
}

void CServerList::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
	CUIXmlInit::InitWindow			(xml_doc, path, 0, this);
	string256 buf;
	CUIXmlInit::InitListBox			(xml_doc, strconcat(sizeof(buf),buf,path,":list"),							0, &m_list[LST_SERVER]);
	m_fListH[0] =					m_list[LST_SERVER].GetHeight();
	m_fListH[1] =					xml_doc.ReadAttribFlt(buf,0,"height2");
	CUIXmlInit::InitListBox			(xml_doc, strconcat(sizeof(buf),buf,path,":list_server_properties"),		0, &m_list[LST_SRV_PROP]);
	CUIXmlInit::InitListBox			(xml_doc, strconcat(sizeof(buf),buf,path,":list_players_list"),				0, &m_list[LST_PLAYERS]);
	CUIXmlInit::InitFrameWindow		(xml_doc, strconcat(sizeof(buf),buf,path,":frame"),							0, &m_frame[LST_SERVER]);
	CUIXmlInit::InitFrameWindow		(xml_doc, strconcat(sizeof(buf),buf,path,":list_server_properties:frame"),	0, &m_frame[LST_SRV_PROP]);
	CUIXmlInit::InitFrameWindow		(xml_doc, strconcat(sizeof(buf),buf,path,":list_players_list:frame"),		0, &m_frame[LST_PLAYERS]);
	CUIXmlInit::InitFont			(xml_doc, strconcat(sizeof(buf),buf,path,":list_item:text"),				0, m_itemInfo.text_color, m_itemInfo.text_font);
	CUIXmlInit::InitEditBox			(xml_doc, strconcat(sizeof(buf),buf,path,":edit_gs_filter"),				0, &m_edit_gs_filter);
	m_fEditPos[0] =					m_edit_gs_filter.GetWndPos().y;
	m_fEditPos[1] =					xml_doc.ReadAttribFlt(buf,0,"y2");
	CUIXmlInit::InitFrameLine	(xml_doc, strconcat(sizeof(buf),buf,path,":cap_server_properties"),			0, &m_header2[0]);
	CUIXmlInit::InitFrameLine	(xml_doc, strconcat(sizeof(buf),buf,path,":cap_players_list"),				0, &m_header2[1]);
	CUIXmlInit::InitFrameLine	(xml_doc, strconcat(sizeof(buf),buf,path,":cap_frags"),						0, &m_header2[2]);
	CUIXmlInit::InitFrameLine	(xml_doc, strconcat(sizeof(buf),buf,path,":cap_death"),						0, &m_header2[3]);
	
	m_itemInfo.size.icon			= xml_doc.ReadAttribFlt( strconcat(sizeof(buf),buf, path, ":sizes"), 0, "icon");
	m_itemInfo.size.server			= xml_doc.ReadAttribFlt( buf, 0, "server");
	m_itemInfo.size.map				= xml_doc.ReadAttribFlt( buf, 0, "map");
	m_itemInfo.size.game			= xml_doc.ReadAttribFlt( buf, 0, "game");
	m_itemInfo.size.players			= xml_doc.ReadAttribFlt( buf, 0, "players");
	m_itemInfo.size.ping			= xml_doc.ReadAttribFlt( buf, 0, "ping");
	m_itemInfo.size.version			= xml_doc.ReadAttribFlt( buf, 0, "version");

	// init header elements
	for (int i = 0; i<LST_COLUMN_COUNT; i++)
	{
		CUIXmlInit::Init3tButton	(xml_doc,strconcat(sizeof(buf),buf,path,":header"),			0, &m_header[i]);
		CUIXmlInit::InitFrameLine	(xml_doc,strconcat(sizeof(buf),buf,path,":header_frames"),	0, &m_header_frames[i]);
	}
	m_header[0].Enable				(false);
	InitHeader						();
	UpdateSizes						();
	UpdateVisibility				();
}

void CServerList::ConnectToSelected()
{
	gamespy_gp::login_manager const * lmngr = MainMenu()->GetLoginMngr();
	R_ASSERT(lmngr);
	gamespy_gp::profile const * tmp_profile = lmngr->get_current_profile(); 
	R_ASSERT2(tmp_profile, "need first to log in");
	if (tmp_profile->online())
	{
		if (!MainMenu()->ValidateCDKey())
			return;

		if (!xr_strcmp(tmp_profile->unique_nick(), "@unregistered"))
		{
			if (m_connect_cb)
				m_connect_cb(ece_unique_nick_not_registred, "mp_gp_unique_nick_not_registred");
			return;
		}
		if (!xr_strcmp(tmp_profile->unique_nick(), "@expired"))
		{
			if (m_connect_cb)
				m_connect_cb(ece_unique_nick_expired, "mp_gp_unique_nick_has_expired");
			return;
		}
	}


	CUIListItemServer* item = smart_cast<CUIListItemServer*>(m_list[LST_SERVER].GetSelectedItem());
	if(!item)
		return;
	if (!browser().CheckDirectConnection(item->GetInfo()->info.Index))
	{
		Msg("! Direct connection to this server is not available -> its behind firewall");
		return;
	}

	if (xr_strcmp(item->GetInfo()->info.version, MainMenu()->GetGSVer()))
	{
		MainMenu()->SetErrorDialog(CMainMenu::ErrDifferentVersion);
		return;
	}


	if (item->GetInfo()->info.icons.pass || item->GetInfo()->info.icons.user_pass)
	{
		m_message_box->m_pMessageBox->SetUserPasswordMode	(item->GetInfo()->info.icons.user_pass);
		m_message_box->m_pMessageBox->SetPasswordMode		(item->GetInfo()->info.icons.pass);
		m_message_box->ShowDialog(true);
	}
	else
	{
		xr_string command;

		item->CreateConsoleCommand(command, m_playerName.c_str(), "", "" );

		Console->Execute(command.c_str());
	}
}

void CServerList::InitHeader()
{
	Fvector2				pos;
	pos.set					(0,0);

	m_header[0].SetWidth	(m_itemInfo.size.icon);
	pos.x					+= m_itemInfo.size.icon;
	m_header[1].SetWidth	(m_itemInfo.size.server);
	m_header[1].SetWndPos	(pos);
	m_header[1].TextItemControl()->SetTextST	("server name");
	pos.x					+= m_itemInfo.size.server;
	m_header[2].SetWidth	(m_itemInfo.size.map);
	m_header[2].SetWndPos	(pos);
	m_header[2].TextItemControl()->SetTextST		("map");
	pos.x					+= m_itemInfo.size.map;
	m_header[3].SetWidth	(m_itemInfo.size.game);
	m_header[3].SetWndPos	(pos);
	m_header[3].TextItemControl()->SetTextST	("game type");
	pos.x					+= m_itemInfo.size.game;
	m_header[4].SetWidth	(m_itemInfo.size.players);
	m_header[4].SetWndPos	(pos);
	m_header[4].TextItemControl()->SetTextST	("players");
	pos.x					+= m_itemInfo.size.players;
	m_header[5].SetWidth	(m_itemInfo.size.ping);
	m_header[5].SetWndPos	(pos);
	m_header[5].TextItemControl()->SetTextST	("ping");
	pos.x					+= m_itemInfo.size.ping;
	m_header[6].SetWidth	(m_itemInfo.size.version);
	m_header[6].SetWndPos	(pos);
	m_header[6].TextItemControl()->SetTextST	("version");

	for(int i=0; i<LST_COLUMN_COUNT;++i)
	{
		m_header_frames[i].SetWndPos	(m_header[i].GetWndPos());
		m_header_frames[i].SetWidth		(m_header[i].GetWndSize().x);
		m_header_frames[i].Enable		(true);
		m_header_frames[i].Show			(true);
		m_header_frames[i].SetVisible	(true);
	}
}

void CServerList::NetRadioChanged(bool Local)
{
	m_edit_gs_filter.Enable	(!Local);
	m_b_local				= Local;
}

void CServerList::RefreshGameSpyList(bool Local)
{
	SetSortFunc			("",		false);
	SetSortFunc			("ping",	false);
	browser().RefreshList_Full(Local, m_edit_gs_filter.GetText());

	ResetCurItem		();
	RefreshList			();
}


void CServerList::AddServerToList	(ServerInfo* pServerInfo)
{
	if (!IsValidItem(*pServerInfo)) return;
	
	CUIListItemServer* item		= GetFreeItem();

	SrvInfo2LstSrvInfo			(pServerInfo);

	m_itemInfo.size.height		= m_list[LST_SERVER].GetItemHeight();
	item->InitItemServer		(m_itemInfo);

	m_list[LST_SERVER].AddExistingItem		(item);
	item->SetAutoDelete			(false);
};

void	CServerList::UpdateServerInList(ServerInfo* pServerInfo, int index)
{
	int sz = m_list[LST_SERVER].GetSize();

	for (int i = 0; i< sz; i++)
	{
		CUIListItemServer* pItem = static_cast<CUIListItemServer*>(m_list[LST_SERVER].GetItemByIDX(i));
		if (pItem->Get_gs_index() == index)
		{
			UpdateServerInList(pServerInfo, pItem);
			return;
		}
	}

    R_ASSERT2(false, "CServerList::UpdateServerInList - invalid index");

};

void	CServerList::UpdateServerInList(ServerInfo* pServerInfo, CUIListItemServer* pItem)
{
	SrvInfo2LstSrvInfo(pServerInfo);
	pItem->SetParams(m_itemInfo);
};

void	CServerList::RefreshList()
{
	m_need_refresh_fr	= Device.dwFrame;
}

void	CServerList::RefreshList_internal()
{
	m_need_refresh_fr				= u32(-1);
	SaveCurItem						();
	m_list[LST_SERVER].Clear();
	ClearSrvItems					();

	u32 NumServersFound				= browser().GetServersCount();
	g_gs_browser					= m_GSBrowser;
	m_tmp_srv_lst.resize			(NumServersFound);

	
	for (u32 i=0; i<NumServersFound; i++)
		m_tmp_srv_lst[i] = i;

	if (0 == xr_strcmp(m_sort_func, "server_name"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_ServerName);

	else if (0 == xr_strcmp(m_sort_func, "map"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_Map);

	else if (0 == xr_strcmp(m_sort_func, "game_type"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_GameType);

	else if (0 == xr_strcmp(m_sort_func, "player"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_Players);

	else if (0 == xr_strcmp(m_sort_func, "ping"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_Ping);

	else if (0 == xr_strcmp(m_sort_func, "version"))
		std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), sort_by_Version);

	for (u32 i=0; i<NumServersFound; i++)
	{
		ServerInfo							NewServerInfo;
		browser().GetServerInfoByIndex		(&NewServerInfo, m_tmp_srv_lst[i]);

		AddServerToList						(&NewServerInfo);
	}
	UpdateSizes();
	RestoreCurItem();
};

void CServerList::RefreshQuick()
{
	CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
	if(!pItem)
		return;
	browser().RefreshQuick(pItem->GetInfo()->info.Index);
	
	RefreshList();

	if (m_bShowServerInfo)
	{
		ClearDetailedServerInfo();
		FillUpDetailedServerInfo();
	}	
}

bool g_bSort_Ascending = true;
void CServerList::SetSortFunc(const char* func_name, bool make_sort)
{
	if (!xr_strcmp(m_sort_func, func_name))
	{
		g_bSort_Ascending = !g_bSort_Ascending;
	}
	else
		g_bSort_Ascending = true;

	m_sort_func = func_name;

	if (make_sort)
		RefreshList();
}

void CServerList::SrvInfo2LstSrvInfo(const ServerInfo* pServerInfo)
{
	m_itemInfo.info.server			= pServerInfo->m_ServerName;
	xr_string address				= pServerInfo->m_HostName;
	char							port[8];
	address							+= "/port=";	
	address							+= itoa(pServerInfo->m_Port, port, 10);
	m_itemInfo.info.address			= address.c_str();
	m_itemInfo.info.map				= pServerInfo->m_SessionName;
	m_itemInfo.info.game			= GameTypeToString( (EGameIDs)pServerInfo->m_GameType, true);
	m_itemInfo.info.players.printf	("%d/%d", pServerInfo->m_ServerNumPlayers, pServerInfo->m_ServerMaxPlayers);
	m_itemInfo.info.ping.printf		("%d", pServerInfo->m_Ping);
	m_itemInfo.info.version			= pServerInfo->m_ServerVersion;
	m_itemInfo.info.icons.pass		= pServerInfo->m_bPassword;
	m_itemInfo.info.icons.dedicated	= pServerInfo->m_bDedicated;
	m_itemInfo.info.icons.punkbuster= false;//	= pServerInfo->m_bPunkBuster;
	m_itemInfo.info.icons.user_pass	= pServerInfo->m_bUserPass;

	m_itemInfo.info.Index			= pServerInfo->Index;   
}


bool CServerList::sort_by_ServerName(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
    gs_browser.GetServerInfoByIndex(&info2, p2);

	int res = xr_strcmp(info1.m_ServerName, info2.m_ServerName);
	return (g_bSort_Ascending) ? (-1 == res) : (1 == res);
}

bool CServerList::sort_by_Map(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
    gs_browser.GetServerInfoByIndex(&info2, p2);

	int res = xr_strcmp(info1.m_SessionName, info2.m_SessionName);
	return (g_bSort_Ascending) ? (-1 == res) : (1 == res);
}

bool CServerList::sort_by_GameType(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
    gs_browser.GetServerInfoByIndex(&info2, p2);

	int res = xr_strcmp(info1.m_ServerGameType, info2.m_ServerGameType);
	return (g_bSort_Ascending) ? (-1 == res) : (1 == res);
}

bool CServerList::sort_by_Players(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
    gs_browser.GetServerInfoByIndex(&info2, p2);

	return (g_bSort_Ascending) ? (info1.m_ServerNumPlayers < info2.m_ServerNumPlayers) : (info1.m_ServerNumPlayers > info2.m_ServerNumPlayers);
}

bool CServerList::sort_by_Ping(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
    gs_browser.GetServerInfoByIndex(&info2, p2);

	return (g_bSort_Ascending) ? (info1.m_Ping < info2.m_Ping) : (info1.m_Ping > info2.m_Ping);
}

bool CServerList::sort_by_Version(int p1, int p2)
{
	CGameSpy_Browser& gs_browser = *g_gs_browser;
	ServerInfo info1,info2;

	gs_browser.GetServerInfoByIndex(&info1, p1);
	gs_browser.GetServerInfoByIndex(&info2, p2);

	int res = xr_strcmp(info1.m_ServerVersion, info2.m_ServerVersion);
	return (g_bSort_Ascending) ? (-1 == res) : (1 == res);
}

void CServerList::SaveCurItem()
{
	CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
	if(!pItem)
	{
		m_cur_item = -1;
		return;
	}
	R_ASSERT(pItem);
	m_cur_item = pItem->GetTAG();
}

void CServerList::RestoreCurItem()
{
	if (-1 == m_cur_item)		
		return;

	m_list[LST_SERVER].SetSelectedTAG(m_cur_item);	
	m_list[LST_SERVER].SetScrollPos(m_list[LST_SERVER].GetSelectedIDX());
}

void CServerList::ResetCurItem()
{
	m_list[LST_SERVER].SetSelectedIDX(u32(-1));
	m_list[LST_SERVER].ScrollToBegin();
}


void CServerList::DestroySrvItems()
{
	m_last_retreived_index	= u32(-1);

	m_list[LST_SERVER].Clear();
	SrvItems_It it		= m_items_cache.begin	();
	SrvItems_It it_e	= m_items_cache.end		();

	for(;it!=it_e;++it)
		xr_delete		( (*it).m_ui_item );
}

void CServerList::ClearSrvItems()
{
	SrvItems_It it			= m_items_cache.begin();
	SrvItems_It it_e		= m_items_cache.end();
	for(;it!=it_e;++it)
		(*it).m_busy = false;

	m_last_retreived_index	= u32(-1);
}

CUIListItemServer* CServerList::GetFreeItem()
{
	SrvItems_It it			= m_items_cache.begin();
	SrvItems_It it_e		= m_items_cache.end();
	
	if(m_last_retreived_index != u32(-1))
		std::advance		(it, m_last_retreived_index);

	for(;it!=it_e;++it)
	{
		if(it->m_busy==false)
		{
			it->m_busy				= true;
			m_last_retreived_index	= (u32)(it - m_items_cache.begin());
			return					it->m_ui_item;
		}
	}
	m_items_cache.push_back(SrvItem(m_list[LST_SERVER].GetItemHeight()));
	m_last_retreived_index	= m_items_cache.size() - 1; 
	return					m_items_cache.back().m_ui_item;
}