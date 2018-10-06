// CUIEditBox.cpp: ввод строки с клавиатуры
//
//////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "UIEditBox.h"
#include "Windows/UIFrameLineWnd.h"

CUIEditBox::CUIEditBox() : m_frameLine(NULL) {}
void CUIEditBox::InitCustomEdit(Fvector2 pos, Fvector2 size)
{
    if (m_frameLine)
    {
        m_frameLine->SetWndPos(Fvector2().set(0, 0));
        m_frameLine->SetWndSize(size);
    }
    CUICustomEdit::InitCustomEdit(pos, size);
}

void CUIEditBox::InitTextureEx(LPCSTR texture, LPCSTR shader)
{
    if (!m_frameLine)
    {
        m_frameLine = new CUIFrameLineWnd();
        AttachChild(m_frameLine);
        m_frameLine->SetAutoDelete(true);
    }
    m_frameLine->InitTexture(texture, shader);
    m_frameLine->SetWndPos(Fvector2().set(0, 0));
    m_frameLine->SetWndSize(GetWndSize());
}

void CUIEditBox::InitTexture(LPCSTR texture) { InitTextureEx(texture, "hud" DELIMITER "default"); }
void CUIEditBox::SetCurrentOptValue()
{
    SetText(GetOptStringValue());
}

void CUIEditBox::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();
    SaveOptStringValue(GetText());
}

void CUIEditBox::SaveBackUpOptValue()
{
    m_opt_backup_value = GetText();
}

void CUIEditBox::UndoOptValue()
{
    SetText(m_opt_backup_value.c_str());
    CUIOptionsItem::UndoOptValue();
}

bool CUIEditBox::IsChangedOptValue() const { return 0 != xr_strcmp(m_opt_backup_value.c_str(), GetText()); }
