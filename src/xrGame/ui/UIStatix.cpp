#include "StdAfx.h"

#include "UIStatix.h"

CUIStatix::CUIStatix() { m_bSelected = false; }
CUIStatix::~CUIStatix() {}
void CUIStatix::start_anim()
{
    SetColorAnimation("ui_slow_blinking", LA_CYCLIC | LA_ONLYALPHA | LA_TEXTCOLOR | LA_TEXTURECOLOR);
    ResetColorAnimation();
}

void CUIStatix::stop_anim() { SetColorAnimation(NULL, 0); }
void CUIStatix::Update()
{
    CUIStatic* child = smart_cast<CUIStatic*>(FindChild("auto_static_0"));
    if (child)
        child->SetTextureColor(0x00ffffff);
    SetTextureColor(0xffffffff);

    if (m_bCursorOverWindow)
    {
        if (child)
            child->SetTextureColor(0xff349F06);
        else
            SetTextureColor(0xff349F06);
    }

    if (!IsEnabled())
    {
        SetTextureColor(0x80ffffff);
    };

    CUIStatic::Update();
}

void CUIStatix::OnFocusLost()
{
    CUIStatic::OnFocusLost();
    CUIStatic* child = smart_cast<CUIStatic*>(FindChild("auto_static_0"));
    if (child)
        child->SetTextureColor(0x00ffffff);
    else
        SetTextureColor(0xffffffff);

    if (!IsEnabled())
    {
        SetTextureColor(0x80ffffff);
    };
}

void CUIStatix::OnFocusReceive()
{
    CUIStatic::OnFocusReceive();
    ResetColorAnimation();
}

bool CUIStatix::OnMouseDown(int mouse_btn)
{
    GetMessageTarget()->SendMessage(this, BUTTON_CLICKED);
    return true;
}

void CUIStatix::SetSelectedState(bool state)
{
    bool b = m_bSelected;
    m_bSelected = state;

    if (b == m_bSelected)
        return;

    if (!state)
        OnFocusLost();

    if (state)
        start_anim();
    else
        stop_anim();
}

bool CUIStatix::GetSelectedState() { return m_bSelected; }
