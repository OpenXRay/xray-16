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

bool CUIEditBox::InitTextureEx(pcstr texture, pcstr shader, bool fatal /*= true*/)
{
    if (!m_frameLine)
    {
        m_frameLine = xr_new<CUIFrameLineWnd>();
        AttachChild(m_frameLine);
        m_frameLine->SetAutoDelete(true);
    }
    const bool result = m_frameLine->InitTexture(texture, shader, fatal);
    m_frameLine->SetWndPos(Fvector2().set(0, 0));
    m_frameLine->SetWndSize(GetWndSize());
    return result;
}

bool CUIEditBox::InitTexture(pcstr texture, bool fatal /*= true*/)
{
    return InitTextureEx(texture, "hud" DELIMITER "default", fatal);
}

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
