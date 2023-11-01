// File:        UIListWndEx.cpp
// Description: Extended ListItem
//              Required to use feature "Selected Item"
// Created:
// Author:      Serhiy O. Vynnychenko
// Mail:        narrator@gsc-game.kiev.ua

// Copyright:   2004 GSC Game World

#pragma once

#include "UIListItem.h"

class XRUICORE_API CUIListItemEx : public CUIListItem
{
protected:
    using inherited = CUIListItem;

public:
    CUIListItemEx(void);
    virtual ~CUIListItemEx(void);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual void SetSelectionColor(u32 dwColor);
    virtual void Draw();

    pcstr GetDebugType() override { return "CUIListItemEx"; }

protected:
    u32 m_dwSelectionColor;
};
