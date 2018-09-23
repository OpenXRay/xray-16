////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankingWnd.cpp
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Ranking window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIRankingWnd.h"
#include "UIFixedScrollBar.h"
#include "UIXmlInit.h"
#include "UIProgressBar.h"
#include "UIFrameLineWnd.h"
#include "UIScrollView.h"
#include "UIHelper.h"
#include "UIInventoryUtilities.h"
#include "Actor.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "xrScriptEngine/script_engine.hpp"
#include "character_community.h"
#include "character_reputation.h"
#include "relation_registry.h"
#include "string_table.h"
#include "UICharacterInfo.h"
#include "ui_base.h"

#ifdef COC_EDITION
#define PDA_RANKING_XML "pda_ranking.xml"
#else
#ifdef CALLOFCHERNOBYL_RANKING
#define PDA_RANKING_XML "pda_ranking_coc.xml"
#else
#define PDA_RANKING_XML "pda_ranking.xml"
#endif
#endif

CUIRankingWnd::CUIRankingWnd()
{
#ifndef CALLOFCHERNOBYL_RANKING
    m_actor_ch_info = nullptr;
    m_last_monster_icon_back = "";
    m_last_monster_icon = "";
    m_last_weapon_icon = "";
#endif
    m_previous_time = Device.dwTimeGlobal;
    m_delay = 3000;
}

CUIRankingWnd::~CUIRankingWnd()
{
    for (auto& it : m_achieves_vec)
        xr_delete(it);
    m_achieves_vec.clear();

    //Alundaio: CoC Rankings
#ifdef CALLOFCHERNOBYL_RANKING
    for (auto& it : m_coc_ranking_vec)
        xr_delete(it);
	m_coc_ranking_vec.clear();
	
	xr_delete(m_coc_ranking_actor);
#endif
    //-Alundaio
}

void CUIRankingWnd::Show(bool status)
{
    if (status)
    {
#ifndef CALLOFCHERNOBYL_RANKING
        m_actor_ch_info->InitCharacter(Actor()->object_id());

        string64 buf;
        xr_sprintf(buf, sizeof(buf), "%d %s", Actor()->get_money(), "RU");
        m_money_value->SetText(buf);
        m_money_value->AdjustWidthToText();
#endif
        update_info();
    }
    inherited::Show(status);
}

void CUIRankingWnd::Update()
{
    if (Device.dwTimeGlobal - m_previous_time > m_delay)
    {
        m_previous_time = Device.dwTimeGlobal;
        update_info();
    }
}

void CUIRankingWnd::Init()
{
    Fvector2 pos;
    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_RANKING_XML);

    CUIXmlInit::InitWindow(xml, "main_wnd", 0, this);
    m_delay = (u32)xml.ReadAttribInt("main_wnd", 0, "delay", 3000);

    m_background = UIHelper::CreateFrameWindow(xml, "background", this);
    m_down_background = UIHelper::CreateFrameWindow(xml, "down_background", this);

#ifndef CALLOFCHERNOBYL_RANKING
    m_actor_ch_info = new CUICharacterInfo();
    m_actor_ch_info->SetAutoDelete(true);
    AttachChild(m_actor_ch_info);
    m_actor_ch_info->InitCharacterInfo(&xml, "actor_ch_info");

    m_icon_overlay = UIHelper::CreateFrameWindow(xml, "actor_icon_over", this);
    m_money_caption = UIHelper::CreateTextWnd(xml, "money_caption", this);
    m_money_value = UIHelper::CreateTextWnd(xml, "money_value", this);

    m_money_caption->AdjustWidthToText();
    pos = m_money_caption->GetWndPos();
    pos.x += m_money_caption->GetWndSize().x + 10.0f;
    m_money_value->SetWndPos(pos);

    m_center_caption = UIHelper::CreateTextWnd(xml, "center_caption", this);
#endif

    // Dynamic stats
    const XML_NODE stored_root = xml.GetLocalRoot();
    const XML_NODE node = xml.NavigateToNode("stat_info", 0);
    xml.SetLocalRoot(node);

    m_stat_count = (u32)xml.GetNodesNum(node, "stat");
    const u32 value_color = CUIXmlInit::GetColor(xml, "value", 0, 0xFFffffff);

    for (u8 i = 0; i < m_stat_count; ++i)
    {
        m_stat_caption[i] = new CUITextWnd();
        AttachChild(m_stat_caption[i]);
        m_stat_caption[i]->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(xml, "stat", i, m_stat_caption[i]);
        m_stat_caption[i]->AdjustWidthToText();

        m_stat_info[i] = new CUITextWnd();
        AttachChild(m_stat_info[i]);
        m_stat_info[i]->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(xml, "stat", i, m_stat_info[i]);

        m_stat_info[i]->SetTextColor(value_color);

        pos.y = m_stat_caption[i]->GetWndPos().y;
        pos.x = m_stat_caption[i]->GetWndPos().x + m_stat_caption[i]->GetWndSize().x + 5.0f;
        m_stat_info[i]->SetWndPos(pos);
    }
    xml.SetLocalRoot(stored_root);

#ifndef CALLOFCHERNOBYL_RANKING
    string256 buf;
    xr_strcpy(buf, sizeof(buf), m_center_caption->GetText());
    xr_strcat(buf, sizeof(buf), CStringTable().translate("ui_ranking_center_caption").c_str());
    m_center_caption->SetText(buf);

    m_monster_icon_back = UIHelper::CreateStatic(xml, "monster_icon_back", this);
    m_monster_icon = UIHelper::CreateStatic(xml, "monster_icon", this);
    m_monster_background = UIHelper::CreateFrameWindow(xml, "monster_background", this);
    m_monster_over = UIHelper::CreateFrameWindow(xml, "monster_over", this);

    m_favorite_weapon_bckgrnd = UIHelper::CreateStatic(xml, "favorite_weapon_back", this);
    m_favorite_weapon_icon = UIHelper::CreateStatic(xml, "favorite_weapon_icon", this);
    m_favorite_weapon_ramka = UIHelper::CreateFrameWindow(xml, "favorite_weapon_ramka", this);
    m_favorite_weapon_over = UIHelper::CreateFrameWindow(xml, "favorite_weapon_over", this);
#endif

    // Achievements
    m_achievements_background = UIHelper::CreateFrameWindow(xml, "achievements_background", this);
    m_achievements = new CUIScrollView();
    CUIXmlInit::InitScrollView(xml, "achievements_wnd", 0, m_achievements);
    m_achievements->SetAutoDelete(true);
    AttachChild(m_achievements);
    m_achievements->SetWindowName("achievements_list");

    pcstr section = "achievements";
    VERIFY2(pSettings->section_exist(section), make_string("Section [%s] does not exist!", section));

    CInifile::Sect& achievs_section = pSettings->r_section(section);
    for (auto& it : achievs_section.Data)
        add_achievement(xml, it.first);

    //Alundaio: CoC Rankings
#ifdef CALLOFCHERNOBYL_RANKING
    m_coc_ranking_background = UIHelper::CreateFrameWindow(xml, "coc_ranking_background", this);
    m_coc_ranking = new CUIScrollView();
    CUIXmlInit::InitScrollView(xml, "coc_ranking_wnd", 0, m_coc_ranking);
    m_coc_ranking->SetAutoDelete(true);
    AttachChild(m_coc_ranking);
    m_coc_ranking->SetWindowName("coc_ranking_list");

    u8 topRankCount = 50;
    luabind::functor<u8> getRankingArraySize;
    
    if (GEnv.ScriptEngine->functor("pda.get_rankings_array_size", getRankingArraySize))
    	topRankCount = getRankingArraySize();
    
    for(u8 i=1; i <= topRankCount; i++)
    {
    	CUIRankingsCoC* character_rank_item = new CUIRankingsCoC(m_coc_ranking);
    	character_rank_item->init_from_xml(xml,i, false);
    	m_coc_ranking_vec.push_back(character_rank_item);
    }
    
    m_coc_ranking_actor_view = new CUIScrollView();
    CUIXmlInit::InitScrollView(xml, "coc_ranking_wnd_actor", 0, m_coc_ranking_actor_view);
    m_coc_ranking_actor_view->SetAutoDelete(true);
    AttachChild(m_coc_ranking_actor_view);
    m_coc_ranking_actor_view->SetWindowName("coc_ranking_list_actor");

    m_coc_ranking_actor = new CUIRankingsCoC(m_coc_ranking_actor_view);
    m_coc_ranking_actor->init_from_xml(xml, topRankCount + 1, true);
#endif
    //-Alundaio

    xml.SetLocalRoot(stored_root);
}

void CUIRankingWnd::add_achievement(CUIXml& xml, shared_str const& achiev_id)
{
    CUIAchievements* achievement = new CUIAchievements(m_achievements);
    VERIFY2(pSettings->section_exist(achiev_id), make_string("Section [%s] does not exist!", achiev_id));
    achievement->init_from_xml(xml);

    achievement->SetName(pSettings->r_string(achiev_id, "name"));
    achievement->SetDescription(pSettings->r_string(achiev_id, "desc"));
    achievement->SetHint(pSettings->r_string(achiev_id, "hint"));
    achievement->SetIcon(pSettings->r_string(achiev_id, "icon"));
    achievement->SetFunctor(pSettings->r_string(achiev_id, "functor"));
    achievement->SetRepeatable(!!READ_IF_EXISTS(pSettings, r_bool, achiev_id, "repeatable", false));

    m_achieves_vec.push_back(achievement);
}

void CUIRankingWnd::update_info()
{
    for (auto& it : m_achieves_vec)
        it->Update();

    //Alundaio: CoC Ranking
#ifdef CALLOFCHERNOBYL_RANKING
    for (auto& it : m_coc_ranking_vec)
        it->Update();

    m_coc_ranking_actor->Update();
#endif
    //-Alundaio

    get_statistic();
#ifndef CALLOFCHERNOBYL_RANKING
    get_best_monster();
    get_favorite_weapon();
#endif
}

void CUIRankingWnd::DrawHint()
{
    for (auto& it : m_achieves_vec)
    {
        if (it->IsShown())
            it->DrawHint();
    }

	//Alundaio: CoC Ranking
#ifdef CALLOFCHERNOBYL_RANKING
	for(auto& it : m_coc_ranking_vec)
	{
		if(it->IsShown())
			it->DrawHint();
	}
	
	if (m_coc_ranking_actor->IsShown())
		m_coc_ranking_actor->DrawHint();
#endif
	//-Alundaio
}

void CUIRankingWnd::get_statistic()
{
    /*
    string128 buf;
    InventoryUtilities::GetTimePeriodAsString(buf, sizeof(buf), Level().GetStartGameTime(), Level().GetGameTime());
    m_stat_info[0]->SetTextColor(color_rgba(170, 170, 170, 255));
    m_stat_info[0]->SetText(buf);
    */

    for (u8 i = 0; i < m_stat_count; ++i)
    {
        luabind::functor<pcstr> funct;
        if (GEnv.ScriptEngine->functor("pda.get_stat", funct))
        {
            pcstr const str = funct(i);
            m_stat_info[i]->SetTextColor(color_rgba(170, 170, 170, 255));
            m_stat_info[i]->TextItemControl().SetColoringMode(true);
            m_stat_info[i]->SetTextST(str);
        }
    }
}

#ifndef CALLOFCHERNOBYL_RANKING
void CUIRankingWnd::get_best_monster()
{
    luabind::functor<pcstr> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_monster_back", funct));
    pcstr str = funct();
    if (!xr_strcmp(str, ""))
        return;

    if (xr_strcmp(str, m_last_monster_icon_back))
    {
        m_monster_icon_back->TextureOn();
        m_monster_icon_back->InitTexture(str);
        m_last_monster_icon_back = str;
    }

    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_monster_icon", funct));
    str = funct();
    if (!xr_strcmp(str, ""))
        return;

    if (xr_strcmp(str, m_last_monster_icon))
    {
        m_monster_icon->TextureOn();
        m_monster_icon->InitTexture(str);
        m_last_monster_icon = str;
    }
}
void CUIRankingWnd::get_favorite_weapon()
{
    luabind::functor<pcstr> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_favorite_weapon", funct));
    pcstr str = funct();

    if (!xr_strcmp(str, ""))
        return;

    if (xr_strcmp(str, m_last_weapon_icon))
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
#endif

void CUIRankingWnd::ResetAll()
{
#ifndef CALLOFCHERNOBYL_RANKING
    m_last_monster_icon_back = "";
    m_last_monster_icon = "";
    m_last_weapon_icon = "";
    m_monster_icon_back->TextureOff();
    m_monster_icon->TextureOff();
    m_favorite_weapon_icon->TextureOff();
#endif

    for (auto& it : m_achieves_vec)
        it->Reset();

    //Alundaio: CoC Rankings
#ifdef CALLOFCHERNOBYL_RANKING
    for (auto& it : m_coc_ranking_vec)
        it->Reset();

    m_coc_ranking_actor->Reset();
#endif
    //-Alundaio

    inherited::ResetAll();
}
