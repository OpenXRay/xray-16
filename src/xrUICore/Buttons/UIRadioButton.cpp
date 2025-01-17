#include "pch.hpp"
#include "UIRadioButton.h"
#include "Lines/UILines.h"

void CUIRadioButton::InitButton(Fvector2 pos, Fvector2 size)
{
    inherited::InitButton(pos, size);

    CUI3tButton::InitTexture("ui_radio");

    const Fvector2 sz = m_background->Get(S_Enabled)->GetStaticItem()->GetSize();
    TextItemControl()->m_TextOffset.x = sz.x;

    CUI3tButton::InitButton(pos, { size.x, sz.y - 5.0f });

    TextItemControl()->m_wndPos.set(pos);
    TextItemControl()->m_wndSize.set({ size.x, m_background->Get(S_Enabled)->GetStaticItem()->GetSize().y });
}

bool CUIRadioButton::InitTexture(pcstr /*texture*/, bool /*fatal = true*/)
{
    // do nothing
    return true;
}

void CUIRadioButton::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (!IsEnabled())
        return;

    inherited::SendMessage(pWnd, msg, pData);

    if (msg == TAB_CHANGED && this == pWnd)
    {
        GetMessageTarget()->SendMessage(this, RADIOBUTTON_SET, nullptr);
    }
}

bool CUIRadioButton::OnMouseDown(int mouse_btn)
{
    const bool result = inherited::OnMouseDown(mouse_btn);

    if (result)
        GetMessageTarget()->SendMessage(this, RADIOBUTTON_SET, nullptr);

    return result;
}
