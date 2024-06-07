#include "pch.hpp"
#include "UIBtnHint.h"
#include "Static/UIStatic.h"
#include "Windows/UIFrameLineWnd.h"
#include "XML/UIXmlInitBase.h"

CUIButtonHint* g_btnHint = nullptr;
CUIButtonHint* g_statHint = nullptr;

CUIButtonHint::CUIButtonHint() : CUIFrameWindow(CUIButtonHint::GetDebugType())
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "hint_item.xml");

    if (uiXml.NavigateToNode("button_hint:texture")) // COP
        CUIXmlInitBase::InitFrameWindow(uiXml, "button_hint", 0, this);
    else // CS
    {
        CUIXmlInitBase::InitWindow(uiXml, "button_hint", 0, this);

        m_border = xr_new<CUIFrameLineWnd>("Border");
        m_border->SetAutoDelete(true);
        AttachChild(m_border);
        CUIXmlInitBase::InitFrameLine(uiXml, "button_hint:frame_line", 0, m_border);
    }

    m_text = xr_new<CUIStatic>("Text");
    m_text->SetAutoDelete(true);
    CUIWindow::AttachChild(m_text);
    CUIXmlInitBase::InitStatic(uiXml, "button_hint:description", 0, m_text);
}

void CUIButtonHint::OnRender()
{
    if (m_enabledOnFrame)
    {
        m_text->Update();

        const u32 color = color_rgba(255, 255, 255, color_get_A(m_text->GetTextColor()));

        if (m_border)
            m_border->SetTextureColor(color);
        else
            SetTextureColor(color);

        Draw();
        m_enabledOnFrame = false;
    }
}

void CUIButtonHint::SetHintText(CUIWindow* w, LPCSTR text)
{
    m_ownerWnd = w;
    m_text->SetTextST(text);

    if (m_border)
    {
        m_text->AdjustWidthToText();
        const float hh = _max(m_text->GetWidth()+30.0f, 80.0f);
        SetWidth(hh);
        m_border->SetWidth(hh); // XXX: CUIFrameLineWnd ignores this. Fix
    }
    else
    {
        m_text->AdjustHeightToText();

        const Fvector2 new_size
        {
            GetWndSize().x,
            m_text->GetWndSize().y + 20.0f
        };

        SetWndSize(new_size);
    }

    m_text->ResetColorAnimation();
}
