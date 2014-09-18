////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankFraction.cpp
//	Created 	: 22.01.2008
//  Modified 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Rank Fraction info window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIRankFraction.h"

#include "UIXmlInit.h"
#include "UIProgressBar.h"
#include "UIHelper.h"

#include "../actor.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "../character_community.h"
#include "../character_reputation.h"
#include "../relation_registry.h"

CUIRankFraction::CUIRankFraction()
{
}

CUIRankFraction::CUIRankFraction( shared_str const& fraction_id )
	: m_fraction_state( fraction_id )
{
}

CUIRankFraction::~CUIRankFraction()
{
}

float CUIRankFraction::get_fraction_power()
{
	return m_fraction_state.power;
}

void CUIRankFraction::init_from_xml( CUIXml& xml )
{
	CUIXmlInit::InitWindow( xml, "fraction_stand_wnd", 0, this );

	m_sn              = UIHelper::CreateStatic( xml, "serial_number", this );
	m_name            = UIHelper::CreateStatic( xml, "name", this );
	m_icon            = UIHelper::CreateStatic( xml, "icon", this );
	m_location_static = UIHelper::CreateStatic( xml, "location_static", this );
	m_location_value  = UIHelper::CreateStatic( xml, "location_value", this );
	m_power_static    = UIHelper::CreateStatic( xml, "power_static", this );
	m_power_value     = UIHelper::CreateStatic( xml, "power_value", this );

	m_relation_minus        = UIHelper::CreateProgressBar( xml, "relation_minus", this );
	m_relation_center_minus = UIHelper::CreateProgressBar( xml, "relation_center_minus", this );
	m_relation_center_plus  = UIHelper::CreateProgressBar( xml, "relation_center_plus", this );
	m_relation_plus         = UIHelper::CreateProgressBar( xml, "relation_plus", this );
	
	m_origin_static   = UIHelper::CreateStatic( xml, "origin_static", this );
	m_border_minus    = UIHelper::CreateStatic( xml, "border_minus", this );
	m_border_plus     = UIHelper::CreateStatic( xml, "border_plus", this );
	m_enemy_static    = UIHelper::CreateStatic( xml, "enemy_static", this );
	m_frined_static   = UIHelper::CreateStatic( xml, "frined_static", this );

	Fvector2 pos;
	m_location_static->AdjustWidthToText();
	pos.x = m_location_static->GetWndPos().x + m_location_static->GetWidth() + 10.0f;
	pos.y = m_location_value->GetWndPos().y;
	m_location_value->SetWndPos( pos );

	m_power_static->AdjustWidthToText();
	pos.x = m_power_static->GetWndPos().x + m_power_static->GetWidth() + 10.0f;
	pos.y = m_power_value->GetWndPos().y;
	m_power_value->SetWndPos( pos );
}

void CUIRankFraction::update_info( u8 sn )
{
	m_fraction_state.update_info();

	string32 buf;
	xr_sprintf( buf, sizeof(buf), "%d", sn );
	m_sn->SetText( buf );

	m_name->SetText( m_fraction_state.get_name() );
	m_icon->InitTexture( m_fraction_state.get_icon() );
	
	m_location_value->SetText( m_fraction_state.get_location() );
	
	xr_sprintf( buf, sizeof(buf), "%.0f", m_fraction_state.power );
	m_power_value->SetText( buf );
	
	float gw = (float)m_fraction_state.get_actor_goodwill();
	if ( gw > 0.0f )
	{
		m_relation_center_plus->SetProgressPos( gw );
		float gw_plus = m_relation_center_plus->GetRange_max();
		if ( gw > gw_plus )
		{
			m_relation_plus->SetProgressPos( gw - gw_plus );
		}
	}
	else if ( gw < 0.0f )
	{
		m_relation_center_minus->SetProgressPos( -gw );
		float gw_minus = -1.0f * m_relation_center_minus->GetRange_max();
		if ( gw < gw_minus )
		{
			m_relation_minus->SetProgressPos( gw_minus - gw );
		}
	}

}