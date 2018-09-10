////////////////////////////////////////////////////////////////////////////
//	Module 		: UIHint.cpp
//	Created 	: 16.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Hint (for UIHintWindow) window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "UIHint.h"

#include "Static/UIStatic.h"
#include "Windows/UIFrameWindow.h"
#include "XML/UIXmlInitBase.h"
#include "xrEngine/StringTable/IStringTable.h"

UIHint::UIHint()
{
    m_visible = false;
    m_rect.set(0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT);
}

void UIHint::init_from_xml(CUIXml& xml, LPCSTR path)
{
    CUIXmlInitBase::InitWindow(xml, path, 0, this);

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);

    m_background = new CUIFrameWindow();
    AttachChild(m_background);
    m_background->SetAutoDelete(true);
    CUIXmlInitBase::InitFrameWindow(xml, "background", 0, m_background);

    m_text = new CUITextWnd();
    AttachChild(m_text);
    m_text->SetAutoDelete(true);
    CUIXmlInitBase::InitTextWnd(xml, "text", 0, m_text);

    m_border = xml.ReadAttribFlt("background", 0, "border", 0.0f);

    xml.SetLocalRoot(stored_root);
    m_visible = false;
}

void UIHint::set_text(LPCSTR text)
{
    if (!text || !xr_strlen(text))
    {
        m_visible = false;
        return;
    }
    m_visible = true;
    m_text->SetText(text);
    m_text->AdjustHeightToText();

    Fvector2 new_size;
    new_size.x = GetWndSize().x;
    new_size.y = m_text->GetWndSize().y + 20.0f;

    m_background->SetWndSize(new_size);
    SetWndSize(m_background->GetWndSize());
}

LPCSTR UIHint::get_text() const { return m_text->GetText(); }
void UIHint::Draw()
{
    if (m_visible)
    {
        fit_in_rect(this, m_rect, m_border);
        inherited::Draw();
    }
}

// =================================================================================================

UIHintWindow::UIHintWindow() : m_hint_wnd(NULL), m_hint_delay(1000), m_enable(false) {}
void UIHintWindow::disable_hint()
{
    if (!m_hint_wnd)
    {
        //		Msg( "! class <UIHintWindow> has not <hint_wnd>!" );
        return;
    }
    m_hint_wnd->set_text(NULL);
    m_enable = false;
}

void UIHintWindow::set_hint_text(shared_str const& text)
{
    if (text.size() == 0)
    {
        disable_hint();
    }
    else
    {
        m_hint_text = text;
    }
    if (m_hint_wnd)
    {
        update_hint_text();
    }
}

void UIHintWindow::set_hint_text_ST(shared_str const& text)
{
    set_hint_text(gStringTable->translate(text));
}

void UIHintWindow::update_hint_text()
{
    if (!m_bCursorOverWindow || !m_hint_text.size() || !m_enable)
    {
        return;
    }
    if (Device.dwTimeGlobal < (m_dwFocusReceiveTime + m_hint_delay))
    {
        return;
    }

    if (!m_hint_wnd)
    {
        Msg("! class <UIHintWindow> has not <hint_wnd> (%s)!", m_hint_text.c_str());
        return;
    }

    m_hint_wnd->set_text(m_hint_text.c_str());
}

void UIHintWindow::Update()
{
    inherited::Update();
    update_hint_text();
}

void UIHintWindow::OnFocusReceive()
{
    inherited::OnFocusReceive();
    disable_hint();
    m_enable = true;
}

void UIHintWindow::OnFocusLost()
{
    inherited::OnFocusLost();
    disable_hint();
}

void UIHintWindow::Show(bool status)
{
    inherited::Show(status);
    disable_hint();
}
