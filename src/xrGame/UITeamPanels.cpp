#include "StdAfx.h"
#include "UITeamPanels.h"
#include "ui/UIStatsIcon.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIStatic.h"

UITeamPanels::UITeamPanels()
{
    need_update_players = false;
    need_update_panels = false;
}

UITeamPanels::~UITeamPanels() { CUIStatsIcon::FreeTexInfo(); }
#define TEAM_NODE_NAME "team"
#define FRAME_NODE_NAME "frame"

void UITeamPanels::Init(LPCSTR xmlName, LPCSTR panelsRootNode)
{
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xmlName);
    CUIXmlInit::InitWindow(uiXml, panelsRootNode, 0, this);
    XML_NODE panelsRoot = uiXml.NavigateToNode(panelsRootNode, 0);
    VERIFY(panelsRoot);
    uiXml.SetLocalRoot(panelsRoot);

    InitAllFrames(FRAME_NODE_NAME);
    InitAllTeams(TEAM_NODE_NAME);

    UpdateExistingPlayers();
    UpdatePanels();
}

void UITeamPanels::InitAllFrames(shared_str const& frame_node)
{
    int number_of_items = uiXml.GetNodesNum(uiXml.GetLocalRoot(), frame_node.c_str());
    for (int i = 0; i < number_of_items; ++i)
    {
        XML_NODE tempFrameNode = uiXml.NavigateToNode(frame_node.c_str(), i);
        if (!tempFrameNode)
            break;
        LPCSTR frame_class = uiXml.ReadAttrib(tempFrameNode, "class", "class_of_frame_not_defined");
        if (!xr_strcmp(frame_class, "frame_line"))
        {
            CUIFrameLineWnd* temp_frame_line = new CUIFrameLineWnd();
            CUIXmlInit::InitFrameLine(uiXml, frame_node.c_str(), i, temp_frame_line);
            temp_frame_line->SetAutoDelete(true);
            AttachChild(temp_frame_line);
        }
        else if (!xr_strcmp(frame_class, "static"))
        {
            CUIStatic* temp_static = new CUIStatic();
            CUIXmlInit::InitStatic(uiXml, frame_node.c_str(), i, temp_static);
            temp_static->SetAutoDelete(true);
            AttachChild(temp_static);
        }
    }
}

void UITeamPanels::InitAllTeams(shared_str const& team_node)
{
    int numberOfTeams = uiXml.GetNodesNum(uiXml.GetLocalRoot(), team_node.c_str());
    for (int i = 0; i < numberOfTeams; ++i)
    {
        XML_NODE tempTeamNode = uiXml.NavigateToNode(team_node.c_str(), i);
        if (!tempTeamNode)
            break;
        LPCSTR tempTeamName = uiXml.ReadAttrib(tempTeamNode, "tname", "team_not_set_in_tname_xml_attribute");
        UITeamState* tempTeamPanel = panelsFactory.CreateTeamPanel(tempTeamName, this);
        VERIFY2(tempTeamPanel, make_string("failed to create team panel \"%s\"", tempTeamName).c_str());
        tempTeamPanel->Init(uiXml, team_node.c_str(), i);
        tempTeamPanel->SetAutoDelete(true);
        AttachChild(tempTeamPanel);
        myPanels.insert(std::make_pair(shared_str(tempTeamName), tempTeamPanel));
    }
}

void UITeamPanels::UpdatePanels()
{
    TTeamsMap::iterator ie = myPanels.end();
    u32 game_phase = Game().Phase();

    shared_str green_team_pending("greenteam_pending");
    shared_str blue_team_pending("blueteam_pending");
    shared_str spectators_team("spectatorsteam");
    shared_str green_team("greenteam");
    shared_str blue_team("blueteam");

    for (TTeamsMap::iterator i = myPanels.begin(); i != ie; ++i)
    {
        bool need_show = false;
        switch (game_phase)
        {
        case GAME_PHASE_PENDING:
        {
            if (i->first.equal(green_team_pending) || i->first.equal(blue_team_pending) ||
                i->first.equal(spectators_team))
            {
                need_show = true;
            }
            break;
        };
        case GAME_PHASE_PLAYER_SCORES:
        case GAME_PHASE_TEAM1_SCORES:
        case GAME_PHASE_TEAM2_SCORES:
        case GAME_PHASE_INPROGRESS:
        {
            if (i->first.equal(green_team) || i->first.equal(blue_team) || i->first.equal(spectators_team))
            {
                need_show = true;
            }
            break;
        };
        }; // end switch
        i->second->Show(need_show);
    }
    need_update_panels = false;
}

void UITeamPanels::UpdateExistingPlayers()
{
    game_cl_GameState::PLAYERS_MAP_IT ie = Game().players.end();
    for (game_cl_GameState::PLAYERS_MAP_IT i = Game().players.begin(); i != ie; ++i)
    {
        UpdatePlayer(i->first);
    }
    need_update_players = false;
}

void UITeamPanels::AddPlayer(ClientID const& clientId)
{
    TTeamsMap::iterator ie = myPanels.end();
    for (TTeamsMap::iterator i = myPanels.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        i->second->AddPlayer(clientId);
    }
}

void UITeamPanels::RemovePlayer(ClientID const& clientId)
{
    TTeamsMap::iterator ie = myPanels.end();
    for (TTeamsMap::iterator i = myPanels.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        i->second->RemovePlayer(clientId);
    }
}

void UITeamPanels::UpdatePlayer(ClientID const& clientId)
{
    bool playerFound = false;
    TTeamsMap::iterator ie = myPanels.end();
    for (TTeamsMap::iterator i = myPanels.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        if (i->second->UpdatePlayer(clientId))
        {
            playerFound = true;
        }
    }
    if (!playerFound)
    {
        AddPlayer(clientId);
    }
}

void UITeamPanels::NeedUpdatePlayers() { need_update_players = true; }
void UITeamPanels::NeedUpdatePanels() { need_update_panels = true; }
void UITeamPanels::Update()
{
    if (need_update_players)
        UpdateExistingPlayers();
    if (need_update_panels)
        UpdatePanels();
    inherited::Update();
}

// in the future it can be associative vector
void UITeamPanels::SetArtefactsCount(s32 greenTeamArtC, s32 blueTeamArtC)
{
    TTeamsMap::iterator ie = myPanels.end();
    for (TTeamsMap::iterator i = myPanels.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        i->second->SetArtefactsCount(greenTeamArtC, blueTeamArtC);
    }
}
