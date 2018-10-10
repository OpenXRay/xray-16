// File:        UIListWndEx.cpp
// Description: Extended ListItem
//              Requiered to use feature "Selected Item"
// Created:
// Author:      Serhiy O. Vynnychenko
// Mail:        narrator@gsc-game.kiev.ua

// Copyright:   2004 GSC Game World

#pragma once
#include "UIListItem.h"

class CUIListItemEx : public CUIListItem
{
private:
    typedef CUIListItem inherited;

public:
    CUIListItemEx(void);
    virtual ~CUIListItemEx(void);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual void SetSelectionColor(u32 dwColor);
    virtual void Draw();
    virtual void dummy() {}
protected:
    u32 m_dwSelectionColor;
};
