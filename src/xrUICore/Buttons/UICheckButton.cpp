// UICheckButton.cpp: класс кнопки, имеющей 2 состояния:
// с галочкой и без
//////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "UICheckButton.h"
#include "Hint/UIHint.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/xr_input_xinput.h"

CUICheckButton::CUICheckButton()
{
    TextItemControl()->SetTextAlignment(CGameFont::alLeft);
    m_pDependControl = NULL;
}

CUICheckButton::~CUICheckButton() {}
void CUICheckButton::SetDependControl(CUIWindow* pWnd) { m_pDependControl = pWnd; }
void CUICheckButton::Update()
{
    CUI3tButton::Update();
    if (m_pDependControl)
        m_pDependControl->Enable(GetCheck());
}

void CUICheckButton::SetCurrentOptValue()
{
    SetCheck(GetOptBoolValue());
}

void CUICheckButton::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();
    SaveOptBoolValue(GetCheck());
}

void CUICheckButton::SaveBackUpOptValue()
{
    m_opt_backup_value = GetCheck();
}

bool CUICheckButton::IsChangedOptValue() const { return m_opt_backup_value != GetCheck(); }
void CUICheckButton::UndoOptValue()
{
    SetCheck(m_opt_backup_value);
    CUIOptionsItem::UndoOptValue();
}

void CUICheckButton::InitCheckButton(Fvector2 pos, Fvector2 size, LPCSTR texture_name)
{
    InitButton(pos, size);
    InitTexture2(texture_name);
    TextItemControl()->m_wndPos.set(pos);
    TextItemControl()->m_wndSize.set(
        Fvector2().set(size.x, m_background->Get(S_Enabled)->GetStaticItem()->GetSize().y));
}

void CUICheckButton::InitTexture2(LPCSTR texture_name)
{
    CUI3tButton::InitTexture(texture_name); // "ui_checker"
    Frect r = m_background->Get(S_Enabled)->GetStaticItem()->GetTextureRect();
    TextItemControl()->m_TextOffset.x = TextItemControl()->m_TextOffset.x + r.width();
}

void CUICheckButton::OnFocusLost()
{
    if (m_eButtonState == BUTTON_PUSHED && pInput->iGetAsyncBtnState(0))
        return;

    inherited::OnFocusLost();
}

void CUICheckButton::OnFocusReceive() { inherited::OnFocusReceive(); }
void CUICheckButton::Show(bool status) { inherited::Show(status); }
bool CUICheckButton::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        if (GetButtonState() == BUTTON_NORMAL)
            SetButtonState(BUTTON_PUSHED);
        else
            SetButtonState(BUTTON_NORMAL);
    }
    GetMessageTarget()->SendMessage(this, BUTTON_CLICKED, NULL);
    return true;
}

bool CUICheckButton::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    return CUIWindow::OnMouseAction(x, y, mouse_action);
}
