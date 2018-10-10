#include "StdAfx.h"

#include "UIListItem.h"

CUIListItem::CUIListItem(void)
{
    m_eButtonState = BUTTON_NORMAL;
    m_ePressMode = NORMAL_PRESS;

    m_bButtonClicked = false;

    m_pData = NULL;

    m_iIndex = -1;
    m_iValue = 0;
    m_bHighlightText = false;
    m_iGroupID = -1;
    SetAutoDelete(true);
    SetTextAlignment(CGameFont::alLeft);
}

CUIListItem::~CUIListItem(void) {}
void CUIListItem::InitListItem(Fvector2 pos, Fvector2 size)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);
    SetPressMode(CUIButton::DOWN_PRESS);
    SetPushOffset(Fvector2().set(0.0f, 0.0f));
}

void CUIListItem::InitTexture(LPCSTR tex_name)
{
    CUIButton::InitTexture(tex_name);
    SetTextX(m_UIStaticItem.GetRect().width());
}

/*
void CUIListItem::Init(const char* str, float x, float y, float width, float height)
{
    Init(x,y,width, height);
    SetTextST(str);
}*/

bool CUIListItem::IsHighlightText() { return CUIButton::IsHighlightText(); }
