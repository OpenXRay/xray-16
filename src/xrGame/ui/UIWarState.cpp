////////////////////////////////////////////////////////////////////////////
//	Module 		: UIWarState.cpp
//	Created 	: 15.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI war state (PDA) window class implementation
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIWarState.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "UIHelper.h"

UIWarState::UIWarState()
{
    //m_installed = false;
    //m_def_texture._set( NULL );
}

void UIWarState::InitXML(CUIXml& xml, LPCSTR att_name, CUIWindow* parent)
{
    VERIFY( parent );
    parent->AttachChild(this);
    SetAutoDelete(true);
    string256 buf;

    CUIXmlInit::InitWindow(xml, att_name, 0, this);

    strconcat(sizeof buf, buf, att_name, ":img");
    m_static = UIHelper::CreateStatic(xml, buf, this);

    /*	strconcat( sizeof(buf), buf, att_name, ":img:texture" );
        m_def_texture._set( xml.Read( buf, 0, NULL ) );
        VERIFY( m_def_texture.size() );
    */
    set_hint_delay((u32)xml.ReadAttribInt(att_name, 0, "delay", 0));

    //	ui_war_state_no_hint = xml.ReadAttrib( att_name, 0, "hint_disable" );
}

void UIWarState::ClearInfo()
{
    //m_installed = false;
    //	m_static->SetVisible( false );
    SetVisible(false);
    set_hint_text_ST("");

    //m_static->InitTexture( m_def_texture.c_str() );
}

bool UIWarState::UpdateInfo(LPCSTR icon, LPCSTR hint_text)
{
    if (!icon || !xr_strlen(icon))
    {
        return false;
    }

    //	m_installed = true;
    //	m_static->SetVisible( true );
    SetVisible(true);
    m_static->InitTexture(icon);

    if (!hint_text || !xr_strlen(hint_text))
    {
        set_hint_text_ST("");
    }
    else
    {
        set_hint_text_ST(hint_text);
    }
    return true;
}

void UIWarState::Draw()
{
    /*	u32 cr = color_rgba( 200, 200, 200, 100 );
        if ( m_installed )
        {
            cr = color_rgba( 255, 255, 255, 250 );
        }
        m_static->SetColor( cr );
        */

    if (IsShown())
    {
        inherited::Draw();
    }

    //	if ( GetVisible() )
    //	{
    //		inherited::Draw();
    //		m_static->Draw();
    //	}
}
