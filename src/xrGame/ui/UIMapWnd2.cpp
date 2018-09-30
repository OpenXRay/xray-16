#include "StdAfx.h"
#include "UIMapWnd.h"
#include "UIMap.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIHelper.h"
#include "UITaskWnd.h"

void CUIMapWnd::init_xml_nav(CUIXml& xml)
{
    m_btn_nav_parent = UIHelper::CreateStatic(xml, "btn_nav_parent", this);

    VERIFY(hint_wnd);

    string64 buf;
    for (u8 i = 0; i < max_btn_nav; ++i)
    {
        xr_sprintf(buf, "btn_nav_parent:btn_nav_%d", i);

        m_btn_nav[i] = UIHelper::Create3tButton(xml, buf, m_btn_nav_parent);
        Register(m_btn_nav[i]);
        //.		m_btn_nav[i]->set_hint_wnd( hint_wnd );
    }

    AddCallback(m_btn_nav[btn_legend], BUTTON_DOWN, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnLegend_Push));
    //	AddCallback( m_btn_nav[btn_up]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnUp_Push		) );
    AddCallback(
        m_btn_nav[btn_zoom_more], BUTTON_DOWN, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomMore_Push));

    //	AddCallback( m_btn_nav[btn_left]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnLeft_Push	) );
    AddCallback(m_btn_nav[btn_actor], BUTTON_DOWN, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnActor_Push));
    //	AddCallback( m_btn_nav[btn_right]->WindowName(),		BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnRight_Push	) );

    AddCallback(
        m_btn_nav[btn_zoom_less], BUTTON_DOWN, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomLess_Push));
    //	AddCallback( m_btn_nav[btn_down]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnDown_Push	) );
    AddCallback(
        m_btn_nav[btn_zoom_reset], BUTTON_DOWN, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomReset_Push));
}

void CUIMapWnd::UpdateNav()
{
    if (Device.dwTimeGlobal - m_nav_timing < 10)
    {
        return;
    }
    m_nav_timing = Device.dwTimeGlobal;

    if (m_btn_nav[btn_up]->CursorOverWindow() && m_btn_nav[btn_up]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(0.0f, m_map_move_step));
    }
    else if (m_btn_nav[btn_left]->CursorOverWindow() &&
        m_btn_nav[btn_left]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(m_map_move_step, 0.0f));
    }
    else if (m_btn_nav[btn_right]->CursorOverWindow() &&
        m_btn_nav[btn_right]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(-m_map_move_step, 0.0f));
    }
    else if (m_btn_nav[btn_down]->CursorOverWindow() &&
        m_btn_nav[btn_down]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(0.0f, -m_map_move_step));
    }
}

void CUIMapWnd::OnBtnLegend_Push(CUIWindow*, void*)
{
    CUITaskWnd* parent_wnd = smart_cast<CUITaskWnd*>(m_pParentWnd);
    if (parent_wnd)
    {
        parent_wnd->Switch_ShowMapLegend();
    }
}
// void CUIMapWnd::OnBtnUp_Push(   CUIWindow*, void*) { MoveMap( Fvector2().set( 0.0f, m_map_move_step ) ); }
// void CUIMapWnd::OnBtnLeft_Push( CUIWindow*, void*) { MoveMap( Fvector2().set( m_map_move_step, 0.0f ) ); }
// void CUIMapWnd::OnBtnRight_Push(CUIWindow*, void*) { MoveMap( Fvector2().set( -m_map_move_step, 0.0f ) ); }
// void CUIMapWnd::OnBtnDown_Push( CUIWindow*, void*) { MoveMap( Fvector2().set( 0.0f, -m_map_move_step ) ); }

void CUIMapWnd::OnBtnZoomMore_Push(CUIWindow*, void*) { ViewZoomIn(); }
void CUIMapWnd::OnBtnActor_Push(CUIWindow*, void*) { ViewActor(); }
void CUIMapWnd::OnBtnZoomLess_Push(CUIWindow*, void*) { ViewZoomOut(); }
void CUIMapWnd::OnBtnZoomReset_Push(CUIWindow*, void*) { ViewGlobalMap(); }
