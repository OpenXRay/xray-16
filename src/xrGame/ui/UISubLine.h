// File:		UISubLine.h
// Description:	Text line. Owns color attribute
// Created:		04.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

// Attention! Destructor is not virtual.
// if you want to inherite this class then make _coresponding_ changes
class CUISubLine
{
public:
    CUISubLine();
    CUISubLine(const CUISubLine& other);
    ~CUISubLine();

    CUISubLine& operator=(const CUISubLine& other);

    const CUISubLine* Cut2Pos(int i);
    void Draw(CGameFont* pFont, float x, float y) const;

    xr_string m_text;
    u32 m_color;
    CUISubLine* m_pTempLine;
    bool m_last_in_line;
};
