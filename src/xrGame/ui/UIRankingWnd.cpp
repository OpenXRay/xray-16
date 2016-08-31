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

#include "../actor.h"
#include "../ai_space.h"
#include "../alife_simulator.h"

#include "../../xrServerEntities/script_engine.h"
#include "../character_community.h"
#include "../character_reputation.h"
#include "../relation_registry.h"
#include "../string_table.h"
#include "UICharacterInfo.h"
#include "ui_base.h"

#define  PDA_RANKING_XML		"pda_ranking.xml"

using namespace luabind;

CUIRankingWnd::CUIRankingWnd()
{
	m_previous_time				= Device.dwTimeGlobal;
	m_delay						= 3000;
}

CUIRankingWnd::~CUIRankingWnd()
{
	ACHIEVES_VEC_IT b = m_achieves_vec.begin(), e = m_achieves_vec.end();
	for(; b!=e; b++)
		xr_delete(*b);
	m_achieves_vec.clear();

	//Alundaio: CoC Rankings
	RANKINGCOC_VEC_IT be = m_coc_ranking_vec.begin(), en = m_coc_ranking_vec.end();
	for (; be != en; be++)
		xr_delete(*be);
	m_coc_ranking_vec.clear();
	
	xr_delete(m_coc_ranking_actor);
	//-Alundaio
}

void CUIRankingWnd::Show( bool status )
{
	if ( status )
	{
		update_info();
	}
	inherited::Show( status );
}

void CUIRankingWnd::Update()
{
	if ( Device.dwTimeGlobal - m_previous_time > m_delay )
	{
		m_previous_time = Device.dwTimeGlobal;
		update_info();
	}
}

void CUIRankingWnd::Init()
{
	Fvector2 pos;
	CUIXml xml;
	xml.Load( CONFIG_PATH, UI_PATH, PDA_RANKING_XML );

	CUIXmlInit::InitWindow( xml, "main_wnd", 0, this );
	m_delay				= (u32)xml.ReadAttribInt( "main_wnd", 0, "delay",	3000 );

	m_background				= UIHelper::CreateFrameWindow(xml, "background", this);
	m_down_background			= UIHelper::CreateFrameWindow(xml, "down_background", this);

	//Dynamic stats
	XML_NODE* stored_root = xml.GetLocalRoot();
	XML_NODE* node = xml.NavigateToNode( "stat_info", 0 );
	xml.SetLocalRoot( node );

	m_stat_count = (u32)xml.GetNodesNum( node, "stat" );
	u32 value_color = CUIXmlInit::GetColor( xml, "value", 0, 0xFFffffff );

	for ( u8 i = 0; i < m_stat_count; ++i )
	{
		m_stat_caption[i]		= xr_new<CUITextWnd>();
		AttachChild				( m_stat_caption[i] );
		m_stat_caption[i]->SetAutoDelete( true );
		CUIXmlInit::InitTextWnd	( xml, "stat", i, m_stat_caption[i] );
		m_stat_caption[i]->AdjustWidthToText();

		m_stat_info[i]			= xr_new<CUITextWnd>();
		AttachChild				( m_stat_info[i] );
		m_stat_info[i]->SetAutoDelete( true );
		CUIXmlInit::InitTextWnd	( xml, "stat", i, m_stat_info[i] );
		
		m_stat_info[i]->SetTextColor( value_color );
		
		pos.y = m_stat_caption[i]->GetWndPos().y;
		pos.x = m_stat_caption[i]->GetWndPos().x + m_stat_caption[i]->GetWndSize().x + 5.0f;
		m_stat_info[i]->SetWndPos( pos );
	}
	xml.SetLocalRoot( stored_root );

	//Achievements
	m_achievements_background	= UIHelper::CreateFrameWindow(xml, "achievements_background", this);
	m_achievements = xr_new<CUIScrollView>();
	CUIXmlInit::InitScrollView(xml, "achievements_wnd", 0, m_achievements);
	m_achievements->SetAutoDelete(true);
	AttachChild(m_achievements);
	m_achievements->SetWindowName("achievements_list");

	LPCSTR section = "achievements";
	VERIFY2(pSettings->section_exist(section), make_string("Section [%s] does not exist!", section));

	CInifile::Sect&	achievs_section = pSettings->r_section(section);
	CInifile::SectIt_ ib			= achievs_section.Data.begin();
	CInifile::SectIt_ ie			= achievs_section.Data.end();
	for(u8 i = 0; ib != ie ; ++ib, ++i)
		add_achievement(xml, (*ib).first);

	
	//Alundaio: CoC Rankings
	m_coc_ranking_background = UIHelper::CreateFrameWindow(xml, "coc_ranking_background", this);
	m_coc_ranking = xr_new<CUIScrollView>();
	CUIXmlInit::InitScrollView(xml, "coc_ranking_wnd", 0, m_coc_ranking);
	m_coc_ranking->SetAutoDelete(true);
	AttachChild(m_coc_ranking);
	m_coc_ranking->SetWindowName("coc_ranking_list");

	u8 topRankCount = 50;
	luabind::functor<u8> getRankingArraySize;
	
	if (ai().script_engine().functor("pda.get_rankings_array_size", getRankingArraySize))
	{
		topRankCount = getRankingArraySize();
	}
	
	for(u8 i=1; i <= topRankCount; i++)
	{
		CUIRankingsCoC* character_rank_item = xr_new<CUIRankingsCoC>(m_coc_ranking);
		character_rank_item->init_from_xml(xml,i, false);
		m_coc_ranking_vec.push_back(character_rank_item);
	}
	
	m_coc_ranking_actor_view = xr_new<CUIScrollView>();
	CUIXmlInit::InitScrollView(xml, "coc_ranking_wnd_actor", 0, m_coc_ranking_actor_view);
	m_coc_ranking_actor_view->SetAutoDelete(true);
	AttachChild(m_coc_ranking_actor_view);
	m_coc_ranking_actor_view->SetWindowName("coc_ranking_list_actor");

	m_coc_ranking_actor = xr_new<CUIRankingsCoC>(m_coc_ranking_actor_view);
	m_coc_ranking_actor->init_from_xml(xml, topRankCount + 1, true);
	//-Alundaio
	
	xml.SetLocalRoot(stored_root);
}

void CUIRankingWnd::add_achievement(CUIXml& xml, shared_str const& achiev_id)
{
	CUIAchievements* achievement = xr_new<CUIAchievements>(m_achievements);
	VERIFY2(pSettings->section_exist(achiev_id), make_string("Section [%s] does not exist!", achiev_id));
	achievement->init_from_xml(xml);

	achievement->SetName(pSettings->r_string(achiev_id, "name"));
	achievement->SetDescription(pSettings->r_string(achiev_id, "desc"));
	achievement->SetHint(pSettings->r_string(achiev_id, "hint"));
	achievement->SetIcon(pSettings->r_string(achiev_id, "icon"));
	achievement->SetFunctor(pSettings->r_string(achiev_id, "functor"));
	achievement->SetRepeatable(!!READ_IF_EXISTS(pSettings,r_bool,achiev_id,"repeatable",false));

	m_achieves_vec.push_back(achievement);
}

void CUIRankingWnd::update_info()
{
	ACHIEVES_VEC_IT b = m_achieves_vec.begin(), e = m_achieves_vec.end();
	for(; b!=e; b++)
		(*b)->Update();
	
	//Alundaio: CoC Ranking
	RANKINGCOC_VEC_IT begin = m_coc_ranking_vec.begin(), end = m_coc_ranking_vec.end();
	for (; begin != end; begin++)
		(*begin)->Update();
	
	m_coc_ranking_actor->Update();
	//-Alundaio
	
	get_statistic();
}

void CUIRankingWnd::DrawHint()
{
	ACHIEVES_VEC_IT b = m_achieves_vec.begin(), e = m_achieves_vec.end();
	for(; b!=e; b++)
	{
		if((*b)->IsShown())
			(*b)->DrawHint();
	}
	
	//Alundaio: CoC Ranking
	RANKINGCOC_VEC_IT begin = m_coc_ranking_vec.begin(), end = m_coc_ranking_vec.end();
	for(; begin!=end; begin++)
	{
		if((*begin)->IsShown())
			(*begin)->DrawHint();
	}
	
	if (m_coc_ranking_actor->IsShown())
		m_coc_ranking_actor->DrawHint();
	//-Alundaio
}

void CUIRankingWnd::get_statistic()
{
	/*
	string128 buf;
	InventoryUtilities::GetTimePeriodAsString(buf, sizeof(buf), Level().GetStartGameTime(), Level().GetGameTime());
	m_stat_info[0]->SetTextColor(color_rgba(170,170,170,255));
	m_stat_info[0]->SetText(buf);
	*/

	for(u8 i = 0; i < m_stat_count; ++i)
	{
		luabind::functor<LPCSTR> funct;
		if (ai().script_engine().functor("pda.get_stat", funct))
		{
			LPCSTR str = funct(i);
			m_stat_info[i]->SetTextColor(color_rgba(170,170,170,255));
			m_stat_info[i]->TextItemControl().SetColoringMode(true);
			m_stat_info[i]->SetTextST(str);
		}
	}
}

void CUIRankingWnd::ResetAll()
{
	ACHIEVES_VEC_IT b = m_achieves_vec.begin(), e = m_achieves_vec.end();
	for(; b!=e; b++)
		(*b)->Reset();
	
	//Alundaio: CoC Rankings
	RANKINGCOC_VEC_IT be = m_coc_ranking_vec.begin(), ed = m_coc_ranking_vec.end();
	for(; be!=ed; be++)
		(*be)->Reset();
	
	m_coc_ranking_actor->Reset();
	//-Alundaio 
	
	inherited::ResetAll();
}