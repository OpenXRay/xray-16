// File:        UIListWndEx.cpp
// Description: Extended ListItem
//              Requiered to use feature "Selected Item"
// Created:
// Author:      Serhiy O. Vynnychenko
// Mail:        narrator@gsc-game.kiev.ua

// Copyright:   2004 GSC Game World

#include "StdAfx.h"
#include "UIListItemEx.h"

CUIListItemEx::CUIListItemEx(void)
{
    //.	this->InitTexture("ui" DELIMITER "hud_map_point");
    //.	this->SetStretchTexture(true);
    this->m_dwSelectionColor = color_argb(200, 95, 82, 74);
    this->SetColor(color_argb(0, 0, 0, 0));
}

CUIListItemEx::~CUIListItemEx(void) {}
void CUIListItemEx::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    // inherited::SendMessage(pWnd, msg, pData);

    switch (msg)
    {
    case LIST_ITEM_SELECT:
        this->SetColor(m_dwSelectionColor);
        //		this->Draw();
        break;
    case LIST_ITEM_UNSELECT:
        this->SetColor(color_argb(0, 0, 0, 0));
        //		this->Draw();
        break;
    }
}

void CUIListItemEx::SetSelectionColor(u32 dwColor) { m_dwSelectionColor = dwColor; }
void CUIListItemEx::Draw()
{
    //	if (m_bPerformTextLimit)
    //		this->PerformTextLengthLimit();
    inherited::Draw();
}
