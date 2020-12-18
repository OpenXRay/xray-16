// File:        UIListWndEx.cpp
// Description: Extended ListItem
//              Required to use feature "Selected Item"
// Created:     
// Author:      Serhiy O. Vynnychenko
// Mail:        narrator@gsc-game.kiev.ua

// Copyright:   2004 GSC Game World

#include "pch.hpp"

#include "UIListItemEx.h"

CUIListItemEx::CUIListItemEx() : m_dwSelectionColor(color_argb(200, 95, 82, 74))
{
    //.	InitTexture("ui\\hud_map_point");
    //.	SetStretchTexture(true);
    CUIStatic::SetColor(color_argb(0, 0, 0, 0));
}

CUIListItemEx::~CUIListItemEx()
{
}

void CUIListItemEx::SendMessage(CUIWindow* /*pWnd*/, s16 msg, void* /*pData*/)
{
    //inherited::SendMessage(pWnd, msg, pData);

    switch (msg)
    {
    case LIST_ITEM_SELECT:
        this->SetColor(m_dwSelectionColor);
        //this->Draw();
        break;
    case LIST_ITEM_UNSELECT:
        this->SetColor(color_argb(0, 0, 0, 0));
        //this->Draw();
        break;
    }
}

void CUIListItemEx::SetSelectionColor(u32 dwColor)
{
    m_dwSelectionColor = dwColor;
}

void CUIListItemEx::Draw()
{
    //if (m_bPerformTextLimit)
    //    PerformTextLengthLimit();
    inherited::Draw();
}
