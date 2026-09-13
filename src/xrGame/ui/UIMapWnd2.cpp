#include "StdAfx.h"
#include "UIMapWnd.h"
#include "UIMap.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIHelper.h"
#include "UITaskWnd.h"

void CUIMapWnd::init_xml_nav(CUIXml& xml, pcstr start_from, bool critical)
{
    m_btn_nav_parent = UIHelper::CreateStatic(xml, "btn_nav_parent", this, critical);

    if (m_btn_nav_parent)
    {
        VERIFY(hint_wnd);
        for (u8 i = 0; i < max_btn_nav; ++i)
        {
            string64 buf;
            xr_sprintf(buf, "btn_nav_parent:btn_nav_%d", i);

            m_btn_nav[i] = UIHelper::Create3tButton(xml, buf, m_btn_nav_parent);
            Register(m_btn_nav[i]);
            //.		m_btn_nav[i]->set_hint_wnd( hint_wnd );
        }
    }
    else // Shadow of Chernobyl
    {
        string512 pth;
        strconcat(pth, start_from, ":main_wnd:map_header_frame_line:tool_bar");

        string512 temp;
        m_btn_nav[btn_zoom_reset] = UIHelper::Create3tButton(xml, strconcat(temp, pth, ":global_map_btn"), m_UIMainMapHeader, false);
        m_btn_nav[btn_actor]      = UIHelper::Create3tButton(xml, strconcat(temp, pth, ":actor_btn"     ), m_UIMainMapHeader, false);
        m_btn_nav[btn_zoom_more]  = UIHelper::Create3tButton(xml, strconcat(temp, pth, ":zoom_in_btn"   ), m_UIMainMapHeader, false);
        m_btn_nav[btn_zoom_less]  = UIHelper::Create3tButton(xml, strconcat(temp, pth, ":zoom_out_btn"  ), m_UIMainMapHeader, false);

        for (CUI3tButton* button : m_btn_nav)
        {
            if (button)
                Register(button);
        }
    }

    const s16 clickEvent = ShadowOfChernobylMode ? BUTTON_CLICKED : BUTTON_DOWN;

    if (m_btn_nav[btn_legend])
        AddCallback(m_btn_nav[btn_legend], clickEvent, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnLegend_Push));
    //	AddCallback( m_btn_nav[btn_up]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnUp_Push		) );
    if (m_btn_nav[btn_zoom_more])
        AddCallback(
            m_btn_nav[btn_zoom_more], clickEvent, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomMore_Push));

    //	AddCallback( m_btn_nav[btn_left]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnLeft_Push	) );
    if (m_btn_nav[btn_actor])
        AddCallback(m_btn_nav[btn_actor], clickEvent, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnActor_Push));
    //	AddCallback( m_btn_nav[btn_right]->WindowName(),		BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnRight_Push	) );

    if (m_btn_nav[btn_zoom_less])
        AddCallback(
            m_btn_nav[btn_zoom_less], clickEvent, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomLess_Push));
    //	AddCallback( m_btn_nav[btn_down]->WindowName(),			BUTTON_DOWN, CUIWndCallback::void_function( this,
    //&CUIMapWnd::OnBtnDown_Push	) );
    if (m_btn_nav[btn_zoom_reset])
        AddCallback(
            m_btn_nav[btn_zoom_reset], clickEvent, CUIWndCallback::void_function(this, &CUIMapWnd::OnBtnZoomReset_Push));
}

void CUIMapWnd::UpdateNav()
{
    if (Device.dwTimeGlobal - m_nav_timing < 10)
    {
        return;
    }
    m_nav_timing = Device.dwTimeGlobal;

    if (m_btn_nav[btn_up] && m_btn_nav[btn_up]->CursorOverWindow() &&
        m_btn_nav[btn_up]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(0.0f, m_map_move_step));
    }
    else if (m_btn_nav[btn_left] && m_btn_nav[btn_left]->CursorOverWindow() &&
        m_btn_nav[btn_left]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(m_map_move_step, 0.0f));
    }
    else if (m_btn_nav[btn_right] && m_btn_nav[btn_right]->CursorOverWindow() &&
        m_btn_nav[btn_right]->GetButtonState() == CUIButton::BUTTON_PUSHED)
    {
        MoveMap(Fvector2().set(-m_map_move_step, 0.0f));
    }
    else if (m_btn_nav[btn_down] && m_btn_nav[btn_down]->CursorOverWindow() &&
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
