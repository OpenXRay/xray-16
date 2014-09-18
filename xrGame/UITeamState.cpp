#include "StdAfx.h"

#include "UITeamState.h"
#include "UITeamPanels.h"

#include "UIPlayerItem.h"
#include "UITeamHeader.h"

#include "ui/UIFrameWindow.h"
#include "ui/UIScrollView.h"
#include "ui/UIStatic.h"

#include "game_cl_mp.h"

UITeamState::UITeamState()
{
	m_artefact_count = 0;
}
UITeamState::UITeamState(ETeam teamId, UITeamPanels *teamPanels)
{
	myTeam = teamId;
	/*myScrollList = xr_new<CUIScrollView>();
	myScrollList->SetAutoDelete(true);
	
	myScrollList->m_sort_function = fastdelegate::MakeDelegate
		(this, &UITeamState::SortingLessFunction);
	
	myTeamHeader = xr_new<UITeamHeader>(this);
	myTeamHeader->SetAutoDelete(true);
	
	AttachChild(myTeamHeader);
	AttachChild(myScrollList);*/

	mainUiXml = NULL;
	m_teamPanels = teamPanels;
}

UITeamState::~UITeamState()
{
	CleanupInternal();
}

void UITeamState::CleanupInternal()
{
	MapClientIdToUIPlayer::iterator it		= myPlayers.begin();
	MapClientIdToUIPlayer::iterator it_e	= myPlayers.end();
	for(;it!=it_e;++it)
	{
		xr_delete(it->second.m_player_wnd);
	}
	myPlayers.clear();
}

bool UITeamState::SortingLessFunction(CUIWindow* left, CUIWindow* right)
{
	UIPlayerItem* lpi = static_cast<UIPlayerItem*>(left);
	UIPlayerItem* rpi = static_cast<UIPlayerItem*>(right);
	VERIFY(lpi && rpi);
	
	if (lpi->GetPlayerCheckPoints() > rpi->GetPlayerCheckPoints())
		return true;

	return false;
}

void UITeamState::Init(CUIXml& uiXml, LPCSTR teamNodeName, int index)
{
	VERIFY(teamNodeName);
	mainUiXml = &uiXml;	//warning !
	teamXmlNode = uiXml.NavigateToNode(teamNodeName, index);
	VERIFY2(teamXmlNode, 
		make_string("team xml node (%s) not found", teamNodeName).c_str());

	CUIXmlInit::InitWindow(uiXml, teamNodeName, index, this);
	XML_NODE* tempRoot = uiXml.GetLocalRoot();
	
	uiXml.SetLocalRoot(teamXmlNode);

	InitScrollPanels();
	//CUIXmlInit::InitScrollView(uiXml, "scroll_panel", 0, myScrollList);
	//myTeamHeader->Init(uiXml, "team_header");
	
	uiXml.SetLocalRoot(tempRoot);
}
int	UITeamState::InitScrollPanels()
{
	XML_NODE* scroll_panels_root = mainUiXml->NavigateToNode("scroll_panels", 0);
	VERIFY2(scroll_panels_root, "scroll_panels tag not found");
	XML_NODE* tempRoot = mainUiXml->GetLocalRoot();
	
	int panels_count = mainUiXml->ReadAttribInt(scroll_panels_root, "count", 0);
	VERIFY2(panels_count, "count of scroll panels is 0, minimum is 1");

	for (int i = 0; i < panels_count; ++i)
	{
		mainUiXml->SetLocalRoot(scroll_panels_root);

		TScrollPanel temp_panel;
		XML_NODE* panel_root = mainUiXml->NavigateToNode("panel", i);
		if (!panel_root)
			break;

		mainUiXml->SetLocalRoot(panel_root);

		temp_panel.first = xr_new<CUIScrollView>();
		temp_panel.first->m_sort_function = fastdelegate::MakeDelegate
			(this, &UITeamState::SortingLessFunction);
		temp_panel.second = xr_new<UITeamHeader>(this);
		temp_panel.first->SetAutoDelete(true);
		temp_panel.second->SetAutoDelete(true);
		
		this->AttachChild(temp_panel.first);
		CUIXmlInit::InitScrollView(*mainUiXml, "scroll_panel", 0, temp_panel.first);
		this->AttachChild(temp_panel.second);
		temp_panel.second->Init(*mainUiXml, "team_header");
		m_scroll_panels.push_back(temp_panel);
	}
	m_last_panel = 0;
	mainUiXml->SetLocalRoot(tempRoot);
	return 0;
}

inline UITeamState::TScrollPanels::size_type UITeamState::GetNeedScrollPanelIndex()
{
	int panels_count	= m_scroll_panels.size();
	VERIFY				(panels_count);

	if (m_last_panel >= panels_count)
	{
		m_last_panel = 0;
		return 0;
	}
	return m_last_panel++;
}

void UITeamState::ReStoreAllPlayers()
{
	typedef MapClientIdToUIPlayer::iterator PlayerIter;
	
	if (m_scroll_panels.size() == 1)
		return;
	
	PlayerIter ite	= myPlayers.end();
	m_last_panel	= 0;

	for (PlayerIter i = myPlayers.begin(); i != ite; ++i)
	{
		VERIFY	(i->second.m_panel_number < m_scroll_panels.size());
		m_scroll_panels[i->second.m_panel_number].first->RemoveWindow	(i->second.m_player_wnd);
		TScrollPanels::size_type new_store_panel = GetNeedScrollPanelIndex();

		VERIFY	(new_store_panel < m_scroll_panels.size());
		m_scroll_panels[new_store_panel].first->AddWindow				(i->second.m_player_wnd, false);
		i->second.m_panel_number = new_store_panel;
	}
}



void UITeamState::AddPlayer(ClientID const & clientId)
{
	game_cl_GameState::PLAYERS_MAP & playersMap = Game().players;
	game_cl_GameState::PLAYERS_MAP::iterator pi = playersMap.find(clientId);
	VERIFY(pi != playersMap.end());
	/*if (pi == playersMap.end())
	{
		Msg("--- Player not found by ClientID = 0x%08x", clientId.value());
		return;
	}*/
	game_PlayerState *ps = pi->second;
	VERIFY(ps);
	/*if (!ps)
	{
		return;
	}*/
	// if player not in my team...
	if (Game().IsPlayerInTeam(ps, myTeam) == false)
	{
		return;
	}

#ifdef DEBUG
	Msg("--- UITeamState: adding player (ClientID = 0x%08x) to %d team (0x%08x)", clientId.value(), myTeam, this);
#endif // #ifdef DEBUG

	UIPlayerItem* tempPlayerItem = xr_new<UIPlayerItem>(static_cast<ETeam>(ps->team), 
		clientId,
		this, 
			m_teamPanels);

	VERIFY2(tempPlayerItem, make_string("failed to create player with ClientID = 0x%08x", clientId.value()).c_str());
	
	TScrollPanels::size_type panel_index = GetNeedScrollPanelIndex();

	VERIFY(panel_index < m_scroll_panels.size());
	m_scroll_panels[panel_index].first->AddWindow(tempPlayerItem, false);
	
	VERIFY2(mainUiXml, "main UI XML not initialized");
	
	XML_NODE* tempRoot = mainUiXml->GetLocalRoot();
	mainUiXml->SetLocalRoot(teamXmlNode);

	if (clientId == Game().local_svdpnid)
	{
		tempPlayerItem->Init(*mainUiXml, "local_player_item", 0);
	} else 
	{
		tempPlayerItem->Init(*mainUiXml, "player_item", 0);
	}
	
	
	mainUiXml->SetLocalRoot(tempRoot);
	
	myPlayers.insert(std::make_pair(clientId, TPlayerItem(tempPlayerItem, panel_index)));
}

void UITeamState::RemovePlayer(ClientID const & clientId)
{
	MapClientIdToUIPlayer::iterator tempIter = myPlayers.find(clientId);
	if (tempIter != myPlayers.end())
	{
		toDeletePlayers.push_back(clientId);
	}
}

bool UITeamState::UpdatePlayer(ClientID const & clientId)
{
	bool retVal = false;
	MapClientIdToUIPlayer::iterator tempIter = myPlayers.find(clientId);
	if (tempIter != myPlayers.end())
	{
		retVal = true;
		game_cl_GameState::PLAYERS_MAP & playersMap = Game().players;
		game_cl_GameState::PLAYERS_MAP::iterator pi = playersMap.find(clientId);
		VERIFY2(pi != playersMap.end(), "player not found in Game().player list, but in UI list it exist");
		game_PlayerState *ps = pi->second;
		// it can be NULL... and player will be removed by player item window
		VERIFY(ps);
		/*if (!ps)
		{
			Msg("--- Player state of ClientID = 0x%08x is NULL", clientId.value());
			return true;
		}*/
		if (Game().IsPlayerInTeam(ps, myTeam) == false)
		{
			RemovePlayer(clientId);
			retVal = false;	//tricky step :) player will be added by UITeamPanels::UpdatePlayer method :)
		}
		// warning ! after this Update tempIter will be not valid !!!
		Update();
		// --^
	}
	return retVal;
}

s32 UITeamState::GetFieldValue(shared_str const & field_name) const
{
	if (field_name.equal("mp_artefacts_upcase"))
	{
		return m_artefact_count;
	} else if (field_name.equal("mp_players"))
	{
		return static_cast<s32>(myPlayers.size());
	} else if (field_name.equal("mp_frags_upcase"))
	{

		return GetSummaryFrags();
	}
	return -1;
}

s32 UITeamState::GetSummaryFrags() const
{
	typedef game_cl_GameState::PLAYERS_MAP::const_iterator PlCIter;
	game_cl_GameState::PLAYERS_MAP & playersMap = Game().players;
	
	s32		sum = 0;
	PlCIter	eiter = playersMap.end();

	for (PlCIter i = playersMap.begin(); i != eiter; ++i)
	{
		game_PlayerState*	ps = i->second;
		if (!ps)
			continue;
		game_cl_mp* tempGame = smart_cast<game_cl_mp*>(&Game());
		R_ASSERT(tempGame);
		if (static_cast<ETeam>(tempGame->ModifyTeam(ps->team)) == myTeam)
			sum = sum + ps->m_iRivalKills;
	}
	return sum;
}

void UITeamState::Draw()
{
	inherited::Draw();
}

void UITeamState::Update()
{
	if (toDeletePlayers.size())
	{
		xr_vector<ClientID>::iterator ie = toDeletePlayers.end();
		for (xr_vector<ClientID>::iterator i = toDeletePlayers.begin();
			i != ie; ++i)
		{
			MapClientIdToUIPlayer::iterator tempIter = myPlayers.find(*i);
			VERIFY2(tempIter != myPlayers.end(), "player not found while deleting");
#ifdef DEBUG
			Msg("--- UITeamState: deleting player (ClientID = 0x%08x) from %d team (0x%08x)", i->value(), myTeam, this);
#endif // #ifdef DEBUG
			VERIFY(m_scroll_panels.size() > tempIter->second.m_panel_number);
			m_scroll_panels[tempIter->second.m_panel_number].first->RemoveWindow(tempIter->second.m_player_wnd);
			xr_delete(tempIter->second.m_player_wnd);
			myPlayers.erase(tempIter);
		}
		ReStoreAllPlayers();	//warning ! uses myPlayers
		toDeletePlayers.clear();
	}
	TScrollPanels::iterator ite = m_scroll_panels.end();
	for (TScrollPanels::iterator it = m_scroll_panels.begin(); it != ite; ++it)
	{
		it->first->ForceUpdate();
	}
	inherited::Update();
}

void UITeamState::SetArtefactsCount(s32 greenTeamArtC, s32 blueTeamArtC)
{
	if (myTeam == etGreenTeam)
	{
		m_artefact_count = greenTeamArtC;
	} else if (myTeam == etBlueTeam)
	{
		m_artefact_count = blueTeamArtC;
	}
}