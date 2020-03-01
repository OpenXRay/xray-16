#include "pch.hpp"
#include "UIBtnHint.h"
#include "Static/UIStatic.h"
#include "XML/UIXmlInitBase.h"

CUIButtonHint* g_btnHint = nullptr;
CUIButtonHint* g_statHint = nullptr;

CUIButtonHint::CUIButtonHint()
    : m_ownerWnd(nullptr)
    , m_enabledOnFrame(false)
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "hint_item.xml");
    CUIXmlInitBase::InitFrameWindow(uiXml, "button_hint", 0, this);

    m_text = new CUITextWnd();
    m_text->SetAutoDelete(true);
    CUIWindow::AttachChild(m_text);
    CUIXmlInitBase::InitTextWnd(uiXml, "button_hint:description", 0, m_text);
}

CUIButtonHint::~CUIButtonHint()
{}

void CUIButtonHint::OnRender()
{
    if (m_enabledOnFrame)
    {
        m_text->Update();
        SetTextureColor(color_rgba(255, 255, 255, color_get_A(m_text->GetTextColor())));
        Draw();
        m_enabledOnFrame = false;
    }
}

void CUIButtonHint::SetHintText(CUIWindow* w, LPCSTR text)
{
    m_ownerWnd = w;
    m_text->SetTextST(text);

    m_text->AdjustHeightToText();

    Fvector2 new_size;
    new_size.x = GetWndSize().x;
    new_size.y = m_text->GetWndSize().y + 20.0f;

    SetWndSize(new_size);

    m_text->ResetColorAnimation();
}
