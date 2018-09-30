////////////////////////////////////////////////////////////////////////////
//	Module 		: UIFactionWarWnd.cpp
//	Created 	: 26.12.2007
//	Author		: Evgeniy Sokolov
//	Description : UI Faction War window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "StdAfx.h"
#include "UIFactionWarWnd.h"

#include "UIXmlInit.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UIHelper.h"

#include "FactionState.h"
#include "UIPdaWnd.h"
#include "UICharacterInfo.h"

#include "Actor.h"
#include "PDA.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

#define PDA_FACTION_WAR_XML		"pda_fraction_war.xml"

CUIFactionWarWnd::CUIFactionWarWnd()
{
    Reset();
}

CUIFactionWarWnd::~CUIFactionWarWnd()
{
}

void CUIFactionWarWnd::Reset()
{
    m_previous_time    = Device.dwTimeGlobal;
    m_update_delay     = 3000;
    m_max_member_count = 100;
    m_max_resource     = 100;
    m_max_power        = 100;
    m_war_states_dx    = 0.0f;
    hint_wnd           = NULL;
    m_tc_pos.set       ( 0.0f, 0.0f );
    m_td_pos.set       ( 0.0f, 0.0f );
}

void CUIFactionWarWnd::Init()
{
	CUIXml xml;
	xml.Load( CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_FACTION_WAR_XML );

	CUIXmlInit::InitWindow( xml, "main_wnd", 0, this );

	m_background			= UIHelper::CreateFrameWindow( xml, "background", this );

	m_center_background		= UIHelper::CreateFrameWindow( xml, "center_background", this );

	m_target_static			= UIHelper::CreateTextWnd( xml, "target_static", this );
	m_target_caption		= UIHelper::CreateTextWnd( xml, "target_caption", this );
	//m_target_caption->SetElipsis( 1, 0 );
	m_tc_pos				= m_target_caption->GetWndPos();

	m_target_desc			= UIHelper::CreateTextWnd( xml, "target_decs", this );
	m_td_pos				= m_target_desc->GetWndPos();

	m_state_static			= UIHelper::CreateStatic( xml, "state_static", this );
	
	m_our_icon				= UIHelper::CreateStatic( xml, "static_our_icon", this );
	m_our_icon_over			= UIHelper::CreateStatic( xml, "static_our_icon_over", this );
	m_our_name				= UIHelper::CreateTextWnd( xml, "static_our_name", this );
	m_st_our_frac_info		= UIHelper::CreateStatic( xml, "static_our_frac_info", this );
	m_st_our_mem_count		= UIHelper::CreateStatic( xml, "static_our_mem_count", this );
	m_st_our_resource		= UIHelper::CreateStatic( xml, "static_our_resource", this );
	
	m_pb_our_state			= UIHelper::CreateProgressBar( xml, "progress_our_state", this );
	m_pb_our_mem_count		= UIHelper::CreateProgressBar( xml, "progress_our_mem_count", this );
	m_pb_our_resource		= UIHelper::CreateProgressBar( xml, "progress_our_resource", this );

	m_enemy_icon			= UIHelper::CreateStatic( xml, "static_enemy_icon", this );
	m_enemy_icon_over		= UIHelper::CreateStatic( xml, "static_enemy_icon_over", this );
	m_enemy_name			= UIHelper::CreateTextWnd( xml, "static_enemy_name", this );
	m_st_enemy_frac_info	= UIHelper::CreateStatic( xml, "static_enemy_frac_info", this );
	m_st_enemy_mem_count	= UIHelper::CreateStatic( xml, "static_enemy_mem_count", this );
	m_st_enemy_resource		= UIHelper::CreateStatic( xml, "static_enemy_resource", this );

	m_pb_enemy_state		= UIHelper::CreateProgressBar( xml, "progress_enemy_state", this );
	m_pb_enemy_mem_count	= UIHelper::CreateProgressBar( xml, "progress_enemy_mem_count", this );	
	m_pb_enemy_resource		= UIHelper::CreateProgressBar( xml, "progress_enemy_resource", this );	

	m_static_line1			= UIHelper::CreateFrameLine( xml, "static_line1", this );
	m_static_line2			= UIHelper::CreateFrameLine( xml, "static_line2", this );
	m_static_line3			= UIHelper::CreateFrameLine( xml, "static_line3", this );
	m_static_line4			= UIHelper::CreateFrameLine( xml, "static_line4", this );
	m_static_line_left		= UIHelper::CreateFrameLine( xml, "static_line_left", this );
	m_static_line_right		= UIHelper::CreateFrameLine( xml, "static_line_right", this );

	VERIFY( hint_wnd );
	m_war_states_parent = new CUIWindow();
	m_war_states_parent->SetAutoDelete( true );
	AttachChild( m_war_states_parent );
	Fvector2 pos;
	pos.x = xml.ReadAttribFlt( "static_vs_state", 0, "x" );
	pos.y = xml.ReadAttribFlt( "static_vs_state", 0, "y" );
	m_war_states_parent->SetWndPos( pos );

	for ( u8 i = 0; i < max_war_state; ++i )
	{
		m_war_state[i] = new UIWarState();
		m_war_state[i]->InitXML( xml, "static_vs_state", m_war_states_parent );
		m_war_state[i]->set_hint_wnd( hint_wnd );
	}
	
	float dx = xml.ReadAttribFlt( "static_vs_state", 0, "dx" );
	m_war_states_dx = dx;
	m_war_states_xcenter = xml.ReadAttribFlt( "static_vs_state", 0, "xcenter", 511.0f );

	pos.set( 0.0f, 0.0f );
	m_war_state[0]->SetWndPos( pos );
	for ( u8 i = 1; i < max_war_state; ++i )
	{
		pos.x += m_war_state[i-1]->GetWndSize().x + dx;
		m_war_state[i]->SetWndPos( pos );
	}

	for ( u8 i = 0; i < max_bonuce; ++i )
	{
		m_our_bonuces[i] = UIHelper::CreateStatic( xml, "static_our_bonuce", this );
	}
	dx = xml.ReadAttribFlt( "static_our_bonuce", 0, "dx" );
	pos = m_our_bonuces[0]->GetWndPos();
	for ( u8 i = 1; i < max_bonuce; ++i )
	{
		pos.x += m_our_bonuces[i-1]->GetWndSize().x + dx;
		m_our_bonuces[i]->SetWndPos( pos );
	}

	for ( u8 i = 0; i < max_bonuce; ++i )
	{
		m_enemy_bonuces[i] = UIHelper::CreateStatic( xml, "static_enemy_bonuce", this );
	}
	dx = xml.ReadAttribFlt( "static_enemy_bonuce", 0, "dx" );
	pos = m_enemy_bonuces[0]->GetWndPos();
	for ( u8 i = 1; i < max_bonuce; ++i )
	{
		pos.x += m_enemy_bonuces[i-1]->GetWndSize().x + dx;
		m_enemy_bonuces[i]->SetWndPos( pos );
	}
	int delay = xml.ReadAttribInt( "main_wnd", 0, "update_delay", 3000 );
	m_update_delay = (0 < delay)? (u32)delay : 0;
}

void CUIFactionWarWnd::ShowInfo( bool status )
{
//	m_target_static->Show( status );
//	m_target_caption->Show( status );
//	m_target_desc->Show( status );
    m_state_static->Show( status );

//	m_static_line1->Show( status );
    m_static_line2->Show( status );
    m_static_line3->Show( status );
    m_static_line4->Show( status );
    m_static_line_left->Show( status );
    m_static_line_right->Show( status );

    m_our_icon->Show( status );
    m_our_icon_over->Show( status );
    m_our_name->Show( status );
    m_st_our_frac_info->Show( status );
    m_st_our_mem_count->Show( status );
    m_st_our_resource->Show( status );

    m_pb_our_state->Show( status );
    m_pb_our_mem_count->Show( status );
    m_pb_our_resource->Show( status );

    m_enemy_icon->Show( status );
    m_enemy_icon_over->Show( status );
    m_enemy_name->Show( status );
    m_st_enemy_frac_info->Show( status );
    m_st_enemy_mem_count->Show( status );
    m_st_enemy_resource->Show( status );

    m_pb_enemy_state->Show( status );
    m_pb_enemy_mem_count->Show( status );
    m_pb_enemy_resource->Show( status );

    m_war_states_parent->Show( status );

    for ( u8 i = 0; i < max_bonuce; ++i )
    {
        m_our_bonuces[i]->Show( status );
        m_enemy_bonuces[i]->Show( status );
    }
}

void CUIFactionWarWnd::SendMessage( CUIWindow* pWnd, s16 msg, void* pData )
{
    CUIWndCallback::OnEvent( pWnd, msg, pData );
}

void CUIFactionWarWnd::Show( bool status )
{
    if ( status )
    {
        InitFactions();
    }
    for ( u8 i = 0; i < max_war_state; ++i )
    {
        m_war_state[i]->ClearInfo();
    }
    inherited::Show( status );
}

void CUIFactionWarWnd::Update()
{
    inherited::Update();
    if ( !IsShown() )
    {
        Reset();
    }
    if ( Device.dwTimeGlobal - m_previous_time > m_update_delay )
    {
        m_previous_time = Device.dwTimeGlobal;
        UpdateInfo();
    }
}

bool CUIFactionWarWnd::InitFactions()
{
    shared_str our, enemy;
    if ( !CUICharacterInfo::get_actor_community( &our, &enemy ) )
    {
        return false;
    }


    /*
    shared_str const& actor_team = Actor()->CharacterInfo().Community().id();

    LPCSTR vs_teams  = pSettings->r_string( "actor_communities", actor_team.c_str() );
    if ( _GetItemCount( vs_teams ) != 2 )
    {
        return false;
    }
    u32   size_temp   = (xr_strlen(vs_teams) + 1) * sizeof(char);
    PSTR  our_fract   = (PSTR)_alloca( size_temp );
    PSTR  enemy_fract = (PSTR)_alloca( size_temp );
    _GetItem( vs_teams, 0, our_fract );
    _GetItem( vs_teams, 1, enemy_fract );

    if ( xr_strlen(our_fract) == 0 || xr_strlen(enemy_fract) == 0 )
    {
        return false;
    }*/
    m_our_faction.set_faction_id2( our );
    m_enemy_faction.set_faction_id2( enemy );

    UpdateInfo();
    return true;
}

void CUIFactionWarWnd::UpdateInfo()
{
    if ( m_our_faction.get_faction_id2().size() == 0 )
    {
        if ( !InitFactions() )
        {
            R_ASSERT2( 0, "Actor`s faction is unknown!" );
        }
    }
    m_max_member_count = get_max_member_count();
    m_max_resource     = get_max_resource();
    m_max_power        = get_max_power();

    m_our_faction.update_info();

    m_target_caption->SetText( m_our_faction.get_target() );
    m_target_caption->AdjustHeightToText();

    Fvector2 pos = m_td_pos;
    pos.y = m_target_caption->GetWndPos().y + m_target_caption->GetHeight() + 8.0f;
    m_target_desc->SetWndPos( pos );
    m_target_desc->SetText( m_our_faction.get_target_desc() );

    if ( m_enemy_faction.get_faction_id2().size() == 0 || m_our_faction.member_count == 0 || xr_strlen( m_our_faction.get_name() )==0 )
    {
        ShowInfo( false );
        return;
    }
    m_enemy_faction.update_info();
    ShowInfo( true );

    UpdateWarStates( m_our_faction );

    //our
    m_our_name->SetTextST(   m_our_faction.get_name() );
    m_our_icon->InitTexture( m_our_faction.get_icon_big() );


    m_pb_our_state->SetRange( 0.0f, m_max_power );
    m_pb_our_state->SetProgressPos( m_our_faction.power );

    m_pb_our_mem_count->SetRange( 0.0f, (float)m_max_member_count );
    m_pb_our_mem_count->SetProgressPos( (float)m_our_faction.member_count );

    m_pb_our_resource->SetRange( 0.0f, m_max_resource );
    m_pb_our_resource->SetProgressPos( m_our_faction.resource );
    set_amount_our_bonus( m_our_faction.bonus );

    //enemy
    m_enemy_name->SetTextST(   m_enemy_faction.get_name() );
    m_enemy_icon->InitTexture( m_enemy_faction.get_icon_big() );

    m_pb_enemy_state->SetRange( 0.0f, m_max_power );
    m_pb_enemy_state->SetProgressPos( m_enemy_faction.power );

    m_pb_enemy_mem_count->SetRange( 0.0f, (float)m_max_member_count );
    m_pb_enemy_mem_count->SetProgressPos( (float)m_enemy_faction.member_count );

    m_pb_enemy_resource->SetRange( 0.0f, m_max_resource );
    m_pb_enemy_resource->SetProgressPos(  m_enemy_faction.resource );

    set_amount_enemy_bonus( m_enemy_faction.bonus );
}

void CUIFactionWarWnd::UpdateWarStates( FactionState const& faction )
{
    Fvector2 pos;
    pos = m_war_states_parent->GetWndPos();

    float sx = 0.0f;
    u8 cnt = 0;
    for ( u8 i = 0; i < max_war_state; ++i )
    {
        if ( !m_war_state[i]->UpdateInfo( faction.get_war_state(i), faction.get_war_state_hint(i) ) )
        {
            break; // for i
        }
        ++cnt;
        sx += m_war_state[i]->GetWndSize().x + m_war_states_dx;
    }
    if ( cnt == 0 )
    {
        m_war_states_parent->SetWndPos( pos );
        return;
    }
    sx -= m_war_states_dx;

    pos.x = m_war_states_xcenter - sx * 0.5f;
    m_war_states_parent->SetWndPos( pos );
}

void CUIFactionWarWnd::set_amount_our_bonus( int value )
{
    for ( u32 i = 0; i < max_bonuce; ++i )
    {
        m_our_bonuces[i]->SetTextureColor( color_rgba( 255, 255, 255, 70) );
    }
    u32 cr = color_rgba( 0, 255, 0, 255);
    for ( int i = 0; i < value; ++i )
    {
        m_our_bonuces[i]->SetTextureColor( cr );
    }
}

void CUIFactionWarWnd::set_amount_enemy_bonus( int value )
{
    for ( u32 i = 0; i < max_bonuce; ++i )
    {
        m_enemy_bonuces[i]->SetTextureColor( color_rgba( 255, 255, 255, 70) );
    }
    u32 cr = color_rgba( 0, 255, 0, 255);
    for ( int i = 0; i < value; ++i )
    {
        m_enemy_bonuces[i]->SetTextureColor( cr );
    }
}

// -------------------------------------------------------------------------------------------------
int CUIFactionWarWnd::get_max_member_count()
{
    luabind::functor<int> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_max_member_count", funct));
    return funct();
}

float CUIFactionWarWnd::get_max_resource()
{
    luabind::functor<float> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_max_resource", funct));
    return funct();
}

float CUIFactionWarWnd::get_max_power()
{
    luabind::functor<float> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("pda.get_max_power", funct));
    return funct();
}
