#include "StdAfx.h"
#include "UIPlayerItem.h"
#include "UITeamState.h"
#include "UITeamPanels.h"

#include "xrUICore/Static/UIStatic.h"
#include "ui/UIStatsIcon.h"

#include "game_cl_capture_the_artefact.h"
#include "game_cl_artefacthunt.h"

UIPlayerItem::UIPlayerItem() {}
UIPlayerItem::UIPlayerItem(ETeam team, ClientID const& clientId, UITeamState* tstate, UITeamPanels* tpanels)
{
    VERIFY(tstate);
    m_teamState = tstate;
    m_teamPanels = tpanels;
    VERIFY(m_teamState);
    VERIFY(m_teamPanels);
    myClientId = clientId;
    m_prevTeam = team;
    m_player_node_root = nullptr;
}

UIPlayerItem::~UIPlayerItem() {}
void UIPlayerItem::Init(CUIXml& uiXml, LPCSTR playerNode, int index)
{
    CUIXmlInit::InitWindow(uiXml, playerNode, index, this);
    m_player_node_root = uiXml.NavigateToNode(playerNode, index);
    VERIFY2(m_player_node_root, "player item in team xml node not initialized");
    XML_NODE prev_root = uiXml.GetLocalRoot();
    uiXml.SetLocalRoot(m_player_node_root);
    InitTextParams(uiXml);
    InitIconParams(uiXml);
    uiXml.SetLocalRoot(prev_root);
}

s32 UIPlayerItem::GetPlayerCheckPoints() const { return m_checkPoints; }
s32 UIPlayerItem::CalculateCheckPoints(game_PlayerState const* ps) const
{
    return (ps->m_iRivalKills + (ps->af_count * 3) - (ps->m_iTeamKills * 2));
}

#define TEXTPARAM_NODE_NAME "textparam"
void UIPlayerItem::InitTextParams(CUIXml& uiXml)
{
    VERIFY(m_player_node_root);
    int temp_number = uiXml.GetNodesNum(m_player_node_root, TEXTPARAM_NODE_NAME);
    for (int i = 0; i < temp_number; ++i)
    {
        XML_NODE text_param_node = uiXml.NavigateToNode(TEXTPARAM_NODE_NAME, i);
        if (!text_param_node)
            break;
        LPCSTR param_name = uiXml.ReadAttrib(text_param_node, "name", "param_name_not_set_in_name_attribute");
        CUITextWnd* temp_static = new CUITextWnd();
        VERIFY(temp_static);
        this->AttachChild(temp_static);
        temp_static->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(uiXml, TEXTPARAM_NODE_NAME, i, temp_static);
        m_text_params.insert(std::make_pair(shared_str(param_name), temp_static));
    }
}

#define ICONPARAM_NODE_NAME "iconparam"
void UIPlayerItem::InitIconParams(CUIXml& uiXml)
{
    VERIFY(m_player_node_root);
    int temp_number = uiXml.GetNodesNum(m_player_node_root, ICONPARAM_NODE_NAME);
    for (int i = 0; i < temp_number; ++i)
    {
        XML_NODE icon_param_node = uiXml.NavigateToNode(ICONPARAM_NODE_NAME, i);
        if (!icon_param_node)
            break;
        LPCSTR param_name = uiXml.ReadAttrib(icon_param_node, "name", "param_name_not_set_in_name_attribute");
        CUIStatsIcon* temp_static = new CUIStatsIcon();
        VERIFY(temp_static);
        this->AttachChild(temp_static);
        temp_static->SetAutoDelete(true);
        CUIXmlInit::InitStatic(uiXml, ICONPARAM_NODE_NAME, i, temp_static);
        m_icon_params.insert(std::make_pair(shared_str(param_name), temp_static));
    }
}

void UIPlayerItem::UpdateTextParams(game_PlayerState const* ps)
{
    buffer_vector<char> value_store(_alloca(512), 512, 512, char(0));
    TMapStrToUIText::iterator ie = m_text_params.end();
    for (TMapStrToUIText::iterator i = m_text_params.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        GetTextParamValue(ps, i->first, value_store);
        i->second->SetText(value_store.begin());
        std::fill(value_store.begin(), value_store.end(), char(0));
    }
}

void UIPlayerItem::UpdateIconParams(game_PlayerState const* ps)
{
    buffer_vector<char> value_store(_alloca(512), 512, 512, char(0));
    TMapStrToUIStatic::iterator ie = m_icon_params.end();
    for (TMapStrToUIStatic::iterator i = m_icon_params.begin(); i != ie; ++i)
    {
        VERIFY(i->second);
        GetIconParamValue(ps, i->first, value_store);
        i->second->SetValue(value_store.begin());
        std::fill(value_store.begin(), value_store.end(), char(0));
    }
}

void UIPlayerItem::GetTextParamValue(
    game_PlayerState const* ps, shared_str const& param_name, buffer_vector<char>& dest)
{
    VERIFY(ps);
    if (param_name.equal("mp_name"))
    {
        xr_strcpy(dest.begin(), dest.size(), ps->getName());
    }
    else if (param_name.equal("mp_frags"))
    {
        xr_sprintf(dest.begin(), dest.size(), "%d", ps->m_iRivalKills - ps->m_iSelfKills);
    }
    else if (param_name.equal("mp_deaths"))
    {
        xr_sprintf(dest.begin(), dest.size(), "%d", ps->m_iDeaths);
    }
    else if (param_name.equal("mp_artefacts"))
    {
        xr_sprintf(dest.begin(), dest.size(), "%d", ps->af_count);
    }
    else if (param_name.equal("mp_spots"))
    {
        xr_sprintf(dest.begin(), dest.size(), "%d", m_checkPoints);
    }
    else if (param_name.equal("mp_status"))
    {
        if (ps->testFlag(GAME_PLAYER_FLAG_READY))
            xr_strcpy(dest.begin(), dest.size(), StringTable().translate("st_mp_ready").c_str());
    }
    else if (param_name.equal("mp_ping"))
    {
        xr_sprintf(dest.begin(), dest.size(), "%d", ps->ping);
    }
}

void UIPlayerItem::GetIconParamValue(
    game_PlayerState const* ps, shared_str const& param_name, buffer_vector<char>& dest)
{
    VERIFY(ps);
    game_cl_mp* cl_game = static_cast<game_cl_mp*>(&Game());
    VERIFY(cl_game);
    if (param_name.equal("rank"))
    {
        if (ETeam(cl_game->ModifyTeam(ps->team)) == etGreenTeam)
        {
            xr_sprintf(dest.begin(), dest.size(), "ui_hud_status_green_0%d", ps->rank + 1);
        }
        else if (ETeam(cl_game->ModifyTeam(ps->team)) == etBlueTeam)
        {
            xr_sprintf(dest.begin(), dest.size(), "ui_hud_status_blue_0%d", ps->rank + 1);
        }
    }
    else if (param_name.equal("death_atf"))
    {
        if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        {
            xr_strcpy(dest.begin(), dest.size(), "death");
            return;
        }
        if (cl_game->Type() == eGameIDCaptureTheArtefact)
        {
            game_cl_CaptureTheArtefact* cta_cl_game = static_cast<game_cl_CaptureTheArtefact*>(cl_game);
            R_ASSERT(cta_cl_game);
            if (ps->GameID == cta_cl_game->GetGreenArtefactOwnerID() ||
                ps->GameID == cta_cl_game->GetBlueArtefactOwnerID())
            {
                xr_strcpy(dest.begin(), dest.size(), "artefact");
            }
        }
        else if (cl_game->Type() == eGameIDArtefactHunt)
        {
            game_cl_ArtefactHunt* ahunt_cl_game = static_cast<game_cl_ArtefactHunt*>(cl_game);
            R_ASSERT(ahunt_cl_game);
            if (ps->GameID == ahunt_cl_game->artefactBearerID)
            {
                xr_strcpy(dest.begin(), dest.size(), "artefact");
            }
        }
    }
    else
    {
        VERIFY2(false, make_string("unknown icon parameter: %s", param_name.c_str()).c_str());
    }
}

void UIPlayerItem::Update()
{
    game_cl_GameState::PLAYERS_MAP& playersMap = Game().players;
    game_cl_GameState::PLAYERS_MAP::iterator pi = playersMap.find(myClientId);

    if (pi == playersMap.end())
    {
        m_teamState->RemovePlayer(myClientId);
        return;
    }

    game_PlayerState* ps = pi->second;
    VERIFY(ps);

    m_checkPoints = CalculateCheckPoints(ps);
    UpdateTextParams(ps);
    UpdateIconParams(ps);

    if (ps->team != m_prevTeam)
    {
        m_prevTeam = static_cast<ETeam>(ps->team);
        m_teamPanels->NeedUpdatePlayers();
        return;
    }
}
