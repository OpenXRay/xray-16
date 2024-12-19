////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankingWnd.cpp
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Ranking window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIRankingWnd.h"
#include "UIXmlInit.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIHelper.h"
#include "UIInventoryUtilities.h"
#include "Actor.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "xrScriptEngine/script_engine.hpp"
#include "character_community.h"
#include "character_reputation.h"
#include "relation_registry.h"
#include "UICharacterInfo.h"
#include "UIRankFaction.h"
#include "UIAchievements.h"
#include "UIRankingsCoC.h"
#include "xrUICore/ui_base.h"

#define PDA_RANKING_XML "pda_ranking.xml"

CUIRankingWnd::CUIRankingWnd()
    : CUIWindow("CUIRankingWnd"), m_delay(3000), m_previous_time(Device.dwTimeGlobal),
      m_last_monster_icon_back(""), m_last_monster_icon(""), m_last_weapon_icon("") {}

CUIRankingWnd::~CUIRankingWnd()
{
    for (auto& achievement : m_achieves_vec)
        xr_delete(achievement);
    m_achieves_vec.clear();

	//Alundaio: CoC Rankings
    for (auto& ranking : m_coc_ranking_vec)
        xr_delete(ranking);
	m_coc_ranking_vec.clear();

	xr_delete(m_coc_ranking_actor);
	//-Alundaio
}

void CUIRankingWnd::Show(bool status)
{
    if (status)
    {
        if (m_actor_ch_info)
            m_actor_ch_info->InitCharacter(Actor()->object_id());

        if (m_money_value)
        {
            string64 buf;
            xr_sprintf(buf, sizeof(buf), "%d %s", Actor()->get_money(), "RU");
            m_money_value->SetText(buf);
            m_money_value->AdjustWidthToText();
        }
        update_info();
        inherited::Update();
    }
    inherited::Show(status);
}

void CUIRankingWnd::Update()
{
    inherited::Update();
    if (Device.dwTimeGlobal - m_previous_time > m_delay)
    {
        m_previous_time = Device.dwTimeGlobal;
        update_info();
    }
}

bool CUIRankingWnd::Init()
{
    Fvector2 pos;
    CUIXml xml;
    if (!xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_RANKING_XML, false))
        return false;

    CUIXmlInit::InitWindow(xml, "main_wnd", 0, this);
    m_delay = (u32)xml.ReadAttribInt("main_wnd", 0, "delay", 3000);

    if (!UIHelper::CreateFrameWindow(xml, "background", this, false))
        std::ignore = UIHelper::CreateFrameLine(xml, "background", this, false);

    std::ignore = UIHelper::CreateStatic(xml, "center_background", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml, "down_background", this, false);

    if (xml.NavigateToNode("actor_ch_info"))
    {
        m_actor_ch_info = xr_new<CUICharacterInfo>();
        m_actor_ch_info->SetAutoDelete(true);
        AttachChild(m_actor_ch_info);
        m_actor_ch_info->InitCharacterInfo(&xml, "actor_ch_info");

        auto* community = m_actor_ch_info->GetIcon(CUICharacterInfo::eCommunity);
        auto* communityCaption = m_actor_ch_info->GetIcon(CUICharacterInfo::eCommunityCaption);

        if (community && communityCaption)
        {
            communityCaption->AdjustWidthToText();
            pos = community->GetWndPos();
            pos.x = communityCaption->GetWndPos().x + communityCaption->GetWndSize().x + 10.0f;
            community->SetWndPos(pos);
        }
    }

    std::ignore = UIHelper::CreateFrameWindow(xml, "actor_icon_over", this, false);

    auto* money_caption = UIHelper::CreateStatic(xml, "money_caption", this, false);
    m_money_value = UIHelper::CreateStatic(xml, "money_value", this, false);
    if (money_caption && m_money_value)
    {
        money_caption->AdjustWidthToText();
        pos = money_caption->GetWndPos();
        pos.x += money_caption->GetWndSize().x + 10.0f;
        m_money_value->SetWndPos(pos);
    }

    if (auto* center_caption = UIHelper::CreateStatic(xml, "center_caption", this, false))
    {
        string256 buf;
        xr_strcpy(buf, center_caption->GetText());
        xr_strcat(buf, StringTable().translate("ui_ranking_center_caption").c_str());
        center_caption->SetText(buf);
    }

    std::ignore = UIHelper::CreateStatic   (xml, "fraction_static", this, false);
    std::ignore = UIHelper::CreateFrameLine(xml, "fraction_line1", this, false);
    std::ignore = UIHelper::CreateFrameLine(xml, "fraction_line2", this, false);

	// Dynamic stats
    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE node = xml.NavigateToNode("stat_info", 0);
    xml.SetLocalRoot(node);

    auto stat_count = xml.GetNodesNum(node, "stat");
    const u32 value_color = CUIXmlInit::GetColor(xml, "value", 0, 0xFFffffff);

    for (size_t i = 0; i < stat_count; ++i)
    {
        auto* stat_caption = UIHelper::CreateStatic(xml, "stat", i, this);
        stat_caption->AdjustWidthToText();

        auto* stat_info = m_stat_info.emplace_back(UIHelper::CreateStatic(xml, "stat", i, this));

        stat_info->SetTextColor(value_color);

        pos.y = stat_caption->GetWndPos().y;
        pos.x = stat_caption->GetWndPos().x + stat_caption->GetWndSize().x + 5.0f;
        stat_info->SetWndPos(pos);
    }
    xml.SetLocalRoot(stored_root);

    m_factions_list = UIHelper::CreateScrollView(xml, "fraction_list", this, false);
    if (m_factions_list)
    {
        m_factions_list->SetWindowName("---fraction_list");
        m_factions_list->m_sort_function = fastdelegate::MakeDelegate(this, &CUIRankingWnd::SortingLessFunction);

        cpcstr fract_section = "pda_rank_communities";

        if (pSettings->section_exist(fract_section))
        {
            node = xml.NavigateToNode("fraction_list", 0);
            xml.SetLocalRoot(node);
            CInifile::Sect& faction_section = pSettings->r_section(fract_section);
            for (const auto& item : faction_section.Data)
            {
                add_faction(xml, item.first);
            }
            node = xml.NavigateToNode("fraction_list", 0);
            xml.SetLocalRoot(stored_root);
        }
    }

    // Monsters & weapons
    m_monster_icon_back = UIHelper::CreateStatic(xml, "monster_icon_back", this, false);
    m_monster_icon = UIHelper::CreateStatic(xml, "monster_icon", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml, "monster_background", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml, "monster_over", this, false);

    std::ignore = UIHelper::CreateStatic(xml, "favorite_weapon_back", this, false);
    m_favorite_weapon_icon = UIHelper::CreateStatic(xml, "favorite_weapon_icon", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml, "favorite_weapon_ramka", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml, "favorite_weapon_over", this, false);

    // Achievements
    std::ignore = UIHelper::CreateFrameWindow(xml, "achievements_background", this, false);
    m_achievements = UIHelper::CreateScrollView(xml, "achievements_wnd", this, false);
    if (m_achievements)
    {
        m_achievements->SetWindowName("achievements_list");

        cpcstr section = "achievements";

        if (pSettings->section_exist(section))
        {
            const auto& achievs_section = pSettings->r_section(section);
            for (const auto& item : achievs_section.Data)
                add_achievement(xml, item.first);
        }
    }

    //Alundaio: CoC Rankings
    u8 topRankCount = 50;
    luabind::functor<u8> getRankingArraySize;

    if (GEnv.ScriptEngine->functor("pda.get_rankings_array_size", getRankingArraySize))
    {
        topRankCount = getRankingArraySize();
    }

    std::ignore = UIHelper::CreateFrameWindow(xml, "coc_ranking_background", this, false);
    if (auto* coc_ranking = UIHelper::CreateScrollView(xml, "coc_ranking_wnd", this, false))
    {
        coc_ranking->SetWindowName("coc_ranking_list");

        for (u8 i = 1; i <= topRankCount; i++)
        {
            auto character_rank_item = m_coc_ranking_vec.emplace_back(xr_new<CUIRankingsCoC>(coc_ranking));
            character_rank_item->init_from_xml(xml, i, false);
        }
    }

    if (auto* coc_ranking_actor_view = UIHelper::CreateScrollView(xml, "coc_ranking_wnd_actor", this, false))
    {
        coc_ranking_actor_view->SetWindowName("coc_ranking_list_actor");

        m_coc_ranking_actor = xr_new<CUIRankingsCoC>(coc_ranking_actor_view);
        m_coc_ranking_actor->init_from_xml(xml, topRankCount + 1, true);
    }
    //-Alundaio

    xml.SetLocalRoot(stored_root);

    return true;
}

void CUIRankingWnd::add_faction(CUIXml& xml, shared_str const& faction_id)
{
    CUIRankFaction* faction = xr_new<CUIRankFaction>(faction_id);
    faction->init_from_xml(xml);
    faction->SetWindowName("fraction_item");
    m_factions_list->AddWindow(faction, true);
    Register(faction);
}

void CUIRankingWnd::clear_all_factions()
{
    m_factions_list->Clear();
}

void CUIRankingWnd::add_achievement(CUIXml& xml, shared_str const& achiev_id)
{
    if (!pSettings->section_exist(achiev_id))
    {
        Msg("~ Achievement section [%s] does not exist!", achiev_id.c_str());
        return;
    }

    CUIAchievements* achievement = m_achieves_vec.emplace_back(xr_new<CUIAchievements>(m_achievements));

    achievement->init_from_xml(xml);

    achievement->SetName(pSettings->r_string(achiev_id, "name"));
    achievement->SetDescription(pSettings->r_string(achiev_id, "desc"));
    achievement->SetHint(pSettings->r_string(achiev_id, "hint"));
    achievement->SetIcon(pSettings->r_string(achiev_id, "icon"));
    achievement->SetFunctor(pSettings->r_string(achiev_id, "functor"));
    achievement->SetRepeatable(!!READ_IF_EXISTS(pSettings, r_bool, achiev_id, "repeatable", false));
}

void CUIRankingWnd::update_info()
{
    for (const auto& achievement : m_achieves_vec)
        achievement->Update();

    for (const auto& ranking : m_coc_ranking_vec)
        ranking->Update();

    get_statistic();
    get_best_monster();
    get_favorite_weapon();

    if (m_factions_list)
    {
        bool force_rating = false;
        for (u8 i = 0; i < m_factions_list->GetSize(); ++i)
        {
            if (const auto* ui_faction = smart_cast<CUIRankFaction*>(m_factions_list->GetItem(i)))
            {
                if (ui_faction->get_cur_sn() != i + 1)
                {
                    force_rating = true;
                    break;
                }
            }
        }

        for (u8 i = 0; i < m_factions_list->GetSize(); ++i)
        {
            if (auto* ui_faction = smart_cast<CUIRankFaction*>(m_factions_list->GetItem(i)))
            {
                ui_faction->update_info(i + 1);
                ui_faction->rating(i + 1, force_rating);
            }
        }

        m_factions_list->ForceUpdate();
    }
}


void CUIRankingWnd::DrawHint()
{
    for (const auto& achievement : m_achieves_vec)
    {
        if (achievement->IsShown())
            achievement->DrawHint();
    }

	//Alundaio: CoC Ranking
    for (const auto& ranking : m_coc_ranking_vec)
    {
        if (ranking->IsShown())
            ranking->DrawHint();
    }

    if (m_coc_ranking_actor && m_coc_ranking_actor->IsShown())
		m_coc_ranking_actor->DrawHint();
	//-Alundaio
}

void CUIRankingWnd::get_statistic() const
{
    u8 idx = 0;
    const auto size = m_stat_info.size();
    if (size && m_actor_ch_info && m_money_value) // hacky condition to determine CoC, which don't reserve first stat_info
    {
        string128 buf;
        InventoryUtilities::GetTimePeriodAsString(buf, sizeof(buf), Level().GetStartGameTime(), Level().GetGameTime());
        m_stat_info[0]->SetTextColor(color_rgba(170, 170, 170, 255));
        m_stat_info[0]->SetText(buf);
        idx = 1;
    }

    luabind::functor<pcstr> funct;
    if (!GEnv.ScriptEngine->functor("pda.get_stat", funct))
        return;

    for (u8 i = idx; i < size; ++i)
    {
        cpcstr str = funct(i);
        m_stat_info[i]->SetTextColor(color_rgba(170, 170, 170, 255));
        m_stat_info[i]->TextItemControl()->SetColoringMode(true);
        m_stat_info[i]->SetTextST(str);
    }
}

void CUIRankingWnd::get_best_monster()
{
    pcstr str;
    luabind::functor<pcstr> functor;

    if (GEnv.ScriptEngine->functor("pda.get_monster_back", functor))
    {
        str = functor();
        if (!xr_strcmp(str, ""))
            return;

        if (xr_strcmp(str, m_last_monster_icon_back))
        {
            if (m_monster_icon_back)
            {
                m_monster_icon_back->TextureOn();
                m_monster_icon_back->InitTexture(str);
            }
            m_last_monster_icon_back = str;
        }
    }

    if (GEnv.ScriptEngine->functor("pda.get_monster_icon", functor))
    {
        str = functor();
        if (!xr_strcmp(str, ""))
            return;

        if (xr_strcmp(str, m_last_monster_icon))
        {
            if (m_monster_icon)
            {
                m_monster_icon->TextureOn();
                m_monster_icon->InitTexture(str);
            }
            m_last_monster_icon = str;
        }
    }
}

void CUIRankingWnd::get_favorite_weapon()
{
    luabind::functor<pcstr> functor;
    if(!GEnv.ScriptEngine->functor("pda.get_favorite_weapon", functor))
        return;
    cpcstr str = functor();

    if (!xr_strcmp(str, ""))
        return;

    if (m_favorite_weapon_icon && xr_strcmp(str, m_last_weapon_icon))
    {
        if (pSettings->section_exist(str) && pSettings->line_exist(str, "upgr_icon_x"))
        {
            m_favorite_weapon_icon->SetShader(InventoryUtilities::GetWeaponUpgradeIconsShader());
            if (!xr_strcmp(str, "wpn_rpg7"))
                m_favorite_weapon_icon->SetShader(InventoryUtilities::GetOutfitUpgradeIconsShader());

            Frect tex_rect;
            tex_rect.x1 = float(pSettings->r_u32(str, "upgr_icon_x"));
            tex_rect.y1 = float(pSettings->r_u32(str, "upgr_icon_y"));
            tex_rect.x2 = float(pSettings->r_u32(str, "upgr_icon_width"));
            tex_rect.y2 = float(pSettings->r_u32(str, "upgr_icon_height"));
            tex_rect.rb.add(tex_rect.lt);
            m_favorite_weapon_icon->SetTextureRect(tex_rect);
            m_favorite_weapon_icon->TextureOn();
            m_favorite_weapon_icon->SetTextureColor(color_rgba(255, 255, 255, 255));
            m_favorite_weapon_icon->SetWndSize(Fvector2().set(
                (tex_rect.x2 - tex_rect.x1) * UI().get_current_kx() * 0.8, (tex_rect.y2 - tex_rect.y1) * 0.8));
            m_favorite_weapon_icon->SetStretchTexture(true);
        }
        m_last_weapon_icon = str;
    }
}

bool CUIRankingWnd::SortingLessFunction(CUIWindow* left, CUIWindow* right)
{
    CUIRankFaction* lpi = smart_cast<CUIRankFaction*>(left);
    CUIRankFaction* rpi = smart_cast<CUIRankFaction*>(right);
    VERIFY(lpi && rpi);
    return (lpi->get_faction_power() > rpi->get_faction_power());
}

void CUIRankingWnd::ResetAll()
{
    m_last_monster_icon_back = "";
    m_last_monster_icon = "";
    m_last_weapon_icon = "";
    if (m_monster_icon_back)
        m_monster_icon_back->TextureOff();
    if (m_monster_icon)
        m_monster_icon->TextureOff();
    if (m_favorite_weapon_icon)
        m_favorite_weapon_icon->TextureOff();

    for (const auto& achievement : m_achieves_vec)
        achievement->Reset();

	//Alundaio: CoC Rankings
    for (const auto& ranking : m_coc_ranking_vec)
        ranking->Reset();

    if (m_coc_ranking_actor)
        m_coc_ranking_actor->Reset();
	//-Alundaio

    inherited::ResetAll();
}
