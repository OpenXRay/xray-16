#include "pch.hpp"

#include "UIListItem.h"

CUIListItem::CUIListItem()
{
    m_eButtonState = BUTTON_NORMAL;

    m_pData = NULL;

    m_iIndex = -1;
    m_iValue = 0;
    m_bHighlightText = false;
    m_iGroupID = -1;
    SetAutoDelete(true);
}

CUIListItem::~CUIListItem()
{
}

void CUIListItem::InitListItem(Fvector2 pos, Fvector2 size)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);
}

void CUIListItem::InitTexture(pcstr tex_name)
{
    CUIButton::InitTexture(tex_name);
    SetTextX(m_UIStaticItem.GetTextureRect().width());
}

/*
void CUIListItem::Init(const char* str, float x, float y, float width, float height)
{
    Init(x,y,width, height);
    SetTextST(str);
}*/

bool CUIListItem::IsHighlightText()
{
    return CursorOverWindow();
}
