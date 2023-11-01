#include "StdAfx.h"

#include "UIInventoryUtilities.h"

#include "UICharacterInfo.h"
#include "Actor.h"
#include "Level.h"
#include "xrServerEntities/character_info.h"
#include "relation_registry.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/ScrollView/UIScrollView.h"

#include "alife_simulator.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "xrServer.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "UIHelper.h"

using namespace InventoryUtilities;

CSE_ALifeTraderAbstract* ch_info_get_from_id(u16 id)
{
    if (ai().get_alife() && ai().get_game_graph())
    {
        return smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(id));
    }
    else
    {
        return smart_cast<CSE_ALifeTraderAbstract*>(Level().Server->GetGameState()->get_entity_from_eid(id));
    }
}

void CUICharacterInfo::InitCharacterInfo(Fvector2 pos, Fvector2 size, CUIXml* xml_doc)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);

    Init_IconInfoItem(*xml_doc, "icon", eIcon);
    if (!m_icons[eIcon])
        Init_IconInfoItem(*xml_doc, "icon_static", eIcon);

    Init_IconInfoItem(*xml_doc, "icon_over", eIconOver);

    Init_IconInfoItem(*xml_doc, "rank_icon", eRankIcon);
    Init_IconInfoItem(*xml_doc, "rank_icon_over", eRankIconOver);

    Init_IconInfoItem(*xml_doc, "commumity_icon", eCommunityIcon);
    Init_IconInfoItem(*xml_doc, "commumity_icon_over", eCommunityIconOver);

    Init_IconInfoItem(*xml_doc, "commumity_big_icon", eCommunityBigIcon);
    Init_IconInfoItem(*xml_doc, "commumity_big_icon_over", eCommunityBigIconOver);

    VERIFY(m_icons[eIcon]);
    m_deadbody_color = color_argb(160, 160, 160, 160);
    if (xml_doc->NavigateToNode("icon:deadbody", 0))
    {
        m_deadbody_color = CUIXmlInit::GetColor(*xml_doc, "icon:deadbody", 0, m_deadbody_color);
    }

    // ----------------------------
    Init_StrInfoItem(*xml_doc, "name_caption", eNameCaption);
    Init_StrInfoItem(*xml_doc, "name_static", eName);

    Init_StrInfoItem(*xml_doc, "rank_caption", eRankCaption);
    Init_StrInfoItem(*xml_doc, "rank_static", eRank);

    Init_StrInfoItem(*xml_doc, "community_caption", eCommunityCaption);
    Init_StrInfoItem(*xml_doc, "community_static", eCommunity);

    Init_StrInfoItem(*xml_doc, "reputation_caption", eReputationCaption);
    Init_StrInfoItem(*xml_doc, "reputation_static", eReputation);

    Init_StrInfoItem(*xml_doc, "relation_caption", eRelationCaption);
    Init_StrInfoItem(*xml_doc, "relation_static", eRelation);

    pUIBio = UIHelper::CreateScrollView(*xml_doc, "biography_list", this, false);
}

void CUICharacterInfo::Init_StrInfoItem(CUIXml& xml_doc, LPCSTR item_str, UIItemType type)
{
    CUIStatic* item = UIHelper::CreateStatic(xml_doc, item_str, this, false);
    if (item)
    {
        m_icons[type] = item;
    }
}

void CUICharacterInfo::Init_IconInfoItem(CUIXml& xml_doc, LPCSTR item_str, UIItemType type)
{
    CUIStatic* item = UIHelper::CreateStatic(xml_doc, item_str, this, false);
    if (item)
    {
        //.		item->ClipperOn();
        item->Show(true);
        m_icons[type] = item;
    }
}

void CUICharacterInfo::InitCharacterInfo(Fvector2 pos, Fvector2 size, cpcstr xml_name, cpcstr xml_name2 /*= nullptr*/)
{
    CUIXml uiXml;
    bool result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name, false);
    if (!result)
        result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name2, false);
    R_ASSERT3(result, "Cannot load XML file", xml_name);
    InitCharacterInfo(pos, size, &uiXml);
}

void CUICharacterInfo::InitCharacterInfo(CUIXml* xml_doc, LPCSTR node_str)
{
    Fvector2 pos, size;
    XML_NODE stored_root = xml_doc->GetLocalRoot();
    XML_NODE ch_node = xml_doc->NavigateToNode(node_str, 0);
    xml_doc->SetLocalRoot(ch_node);
    pos.x = xml_doc->ReadAttribFlt(ch_node, "x");
    pos.y = xml_doc->ReadAttribFlt(ch_node, "y");
    size.x = xml_doc->ReadAttribFlt(ch_node, "width");
    size.y = xml_doc->ReadAttribFlt(ch_node, "height");
    InitCharacterInfo(pos, size, xml_doc);
    xml_doc->SetLocalRoot(stored_root);
}

void CUICharacterInfo::InitCharacter(u16 id)
{
    m_ownerID = id;

    CSE_ALifeTraderAbstract* T = ch_info_get_from_id(m_ownerID);

    CCharacterInfo chInfo;
    chInfo.Init(T);

    if (m_icons[eName])
    {
        m_icons[eName]->TextItemControl()->SetTextST(T->m_character_name.c_str());
    }
    if (m_icons[eRank])
    {
        m_icons[eRank]->TextItemControl()->SetTextST(GetRankAsText(chInfo.Rank().value()));
    }
    if (m_icons[eCommunity])
    {
        m_icons[eCommunity]->TextItemControl()->SetTextST(chInfo.Community().id().c_str());
    }
    if (m_icons[eReputation])
    {
        m_icons[eReputation]->TextItemControl()->SetTextST(GetReputationAsText(chInfo.Reputation().value()));
    }

    // Bio
    if (pUIBio && pUIBio->IsEnabled())
    {
        pUIBio->Clear();
        if (chInfo.Bio().size())
        {
            CUITextWnd* pItem = xr_new<CUITextWnd>();
            pItem->SetWidth(pUIBio->GetDesiredChildWidth());
            pItem->SetText(chInfo.Bio().c_str());
            pItem->AdjustHeightToText();
            pUIBio->AddWindow(pItem, true);
        }
    }

    shared_str const& comm_id = chInfo.Community().id();
    LPCSTR community0 = comm_id.c_str();
    string64 community1;
    xr_strcpy(community1, sizeof(community1), community0);
    xr_strcat(community1, sizeof(community1), "_icon");

    string64 community2;
    xr_strcpy(community2, sizeof(community2), community0);
    xr_strcat(community2, sizeof(community2), "_wide");

    m_bForceUpdate = true;
    for (int i = eIcon; i < eMaxCaption; ++i)
    {
        if (m_icons[i])
        {
            m_icons[i]->Show(true);
        }
    }

    m_texture_name = chInfo.IconName();
    if (m_icons[eIcon])
    {
        m_icons[eIcon]->InitTexture(m_texture_name.c_str());
    }
    if (m_icons[eRankIcon])
        m_icons[eRankIcon]->InitTexture(chInfo.Rank().id().c_str());

    if (Actor()->ID() != m_ownerID && !ignore_community(comm_id))
    {
        if (m_icons[eCommunityIcon])
            m_icons[eCommunityIcon]->InitTexture(community1);
        if (m_icons[eCommunityBigIcon])
            m_icons[eCommunityBigIcon]->InitTexture(community2);
        return;
    }

    shared_str our_comm, enemy;
    if (CUICharacterInfo::get_actor_community(&our_comm, &enemy))
    {
        if (xr_strcmp(our_comm, "actor")) // !=
        {
            xr_strcpy(community1, sizeof(community1), our_comm.c_str());
            xr_strcat(community1, sizeof(community1), "_icon");

            xr_strcpy(community2, sizeof(community2), our_comm.c_str());
            xr_strcat(community2, sizeof(community2), "_wide");

            if (m_icons[eCommunityIcon])
                m_icons[eCommunityIcon]->InitTexture(community1);
            if (m_icons[eCommunityBigIcon])
                m_icons[eCommunityBigIcon]->InitTexture(community2);
            return;
        }
    }

    if (m_icons[eCommunityIcon])
        m_icons[eCommunityIcon]->Show(false);
    if (m_icons[eCommunityBigIcon])
        m_icons[eCommunityBigIcon]->Show(false);
    if (m_icons[eCommunityIconOver])
        m_icons[eCommunityIconOver]->Show(false);
    if (m_icons[eCommunityBigIconOver])
        m_icons[eCommunityBigIconOver]->Show(false);
}

void CUICharacterInfo::InitCharacterMP(LPCSTR player_name, LPCSTR player_icon)
{
    ClearInfo();

    if (m_icons[eName])
    {
        m_icons[eName]->TextItemControl()->SetTextST(player_name);
        m_icons[eName]->Show(true);
    }

    if (m_icons[eIcon])
    {
        m_icons[eIcon]->InitTexture(player_icon);
        m_icons[eIcon]->Show(true);
    }
    if (m_icons[eIconOver])
    {
        m_icons[eIconOver]->Show(true);
    }
}

void CUICharacterInfo::SetRelation(ALife::ERelationType relation, CHARACTER_GOODWILL goodwill)
{
    if (!m_icons[eRelation] || !m_icons[eRelationCaption])
    {
        return;
    }
    m_icons[eRelation]->TextItemControl()->SetTextColor(GetRelationColor(relation));
    m_icons[eRelation]->TextItemControl()->SetTextST(GetGoodwillAsText(goodwill));
}

//////////////////////////////////////////////////////////////////////////

void CUICharacterInfo::ResetAllStrings()
{
    if (m_icons[eName])
        m_icons[eName]->TextItemControl()->SetText("");
    if (m_icons[eRank])
        m_icons[eRank]->TextItemControl()->SetText("");
    if (m_icons[eCommunity])
        m_icons[eCommunity]->TextItemControl()->SetText("");
    if (m_icons[eReputation])
        m_icons[eReputation]->TextItemControl()->SetText("");
    if (m_icons[eRelation])
        m_icons[eRelation]->TextItemControl()->SetText("");
}

void CUICharacterInfo::UpdateRelation()
{
    if (!m_icons[eRelation] || !m_icons[eRelationCaption])
    {
        return;
    }

    if (Actor()->ID() == m_ownerID || !hasOwner())
    {
        m_icons[eRelationCaption]->Show(false);
        m_icons[eRelation]->Show(false);
    }
    else
    {
        m_icons[eRelationCaption]->Show(true);
        m_icons[eRelation]->Show(true);

        CSE_ALifeTraderAbstract* T = ch_info_get_from_id(m_ownerID);
        CSE_ALifeTraderAbstract* TA = ch_info_get_from_id(Actor()->ID());

        SetRelation(RELATION_REGISTRY().GetRelationType(T, TA), RELATION_REGISTRY().GetAttitude(T, TA));
    }
}

namespace detail
{ // helper function implemented in file alife_simulator.cpp
bool object_exists_in_alife_registry(u32 id);
} // namespace detail

void CUICharacterInfo::Update()
{
    inherited::Update();

    if (hasOwner() && (m_bForceUpdate || (Device.dwFrame % 50 == 0)))
    {
        m_bForceUpdate = false;

        CSE_ALifeTraderAbstract* T =
            ::detail::object_exists_in_alife_registry(m_ownerID) ? ch_info_get_from_id(m_ownerID) : NULL;
        if (NULL == T)
        {
            m_ownerID = u16(-1);
            return;
        }
        else
        {
            UpdateRelation();
        }

        if (m_icons[eIcon])
        {
            CSE_ALifeCreatureAbstract* pCreature = smart_cast<CSE_ALifeCreatureAbstract*>(T);
            if (pCreature)
            {
                if (pCreature->g_Alive())
                    m_icons[eIcon]->SetTextureColor(color_argb(255, 255, 255, 160));
                else
                    m_icons[eIcon]->SetTextureColor(color_argb(255, 255, 160, 160));
            }
        }
    }
}

void CUICharacterInfo::ClearInfo()
{
    ResetAllStrings();

    for (int i = eIcon; i < eMaxCaption; ++i)
    {
        if (m_icons[i])
        {
            m_icons[i]->Show(false);
        }
    }
}

// ------- static ---------
bool CUICharacterInfo::get_actor_community(shared_str* our, shared_str* enemy)
{
    our->_set(nullptr);
    enemy->_set(nullptr);

    const auto& actor_team = Actor()->CharacterInfo().Community().id();

    pcstr vs_teams = READ_IF_EXISTS(pSettings, r_string, "actor_communities", actor_team.c_str(), nullptr);
    if (!vs_teams || _GetItemCount(vs_teams) != 2)
    {
        return false;
    }
    const size_t size_temp = (xr_strlen(vs_teams) + 1) * sizeof(char);
    pstr our_fract = (pstr)xr_alloca(size_temp);
    pstr enemy_fract = (pstr)xr_alloca(size_temp);
    _GetItem(vs_teams, 0, our_fract, size_temp);
    _GetItem(vs_teams, 1, enemy_fract, size_temp);

    if (xr_strlen(our_fract) == 0 || xr_strlen(enemy_fract) == 0)
    {
        return false;
    }

    our->_set(our_fract);
    enemy->_set(enemy_fract);
    return true;
}

bool CUICharacterInfo::ignore_community(shared_str const& check_community)
{
    pcstr comm_section_str = "ignore_icons_communities";
    if (!pSettings->section_exist(comm_section_str))
        return false;

    CInifile::Sect& faction_section = pSettings->r_section(comm_section_str);
    auto ib = faction_section.Data.begin();
    auto ie = faction_section.Data.end();
    for (; ib != ie; ++ib)
    {
        if (check_community == (*ib).first)
        {
            return true;
        }
    }
    return false;
}
