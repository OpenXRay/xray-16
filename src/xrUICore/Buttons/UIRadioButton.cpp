#include "pch.hpp"
#include "UIRadioButton.h"
#include "Lines/UILines.h"

void CUIRadioButton::InitButton(Fvector2 pos, Fvector2 size)
{
    inherited::InitButton(pos, size);

    TextItemControl();
    CUI3tButton::InitTexture("ui_radio");
    Fvector2 sz = m_background->Get(S_Enabled)->GetStaticItem()->GetSize();
    TextItemControl()->m_TextOffset.x = sz.x;

    CUI3tButton::InitButton(pos, Fvector2().set(size.x, sz.y - 5.0f));

    TextItemControl()->m_wndPos.set(pos);
    TextItemControl()->m_wndSize.set(
        Fvector2().set(size.x, m_background->Get(S_Enabled)->GetStaticItem()->GetSize().y));
}

void CUIRadioButton::InitTexture(LPCSTR tex_name)
{
    // do nothing
}
