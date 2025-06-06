// File:		UISubLine.h
// Description:	Text line. Owns color attribute
// Created:		04.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

class CUISubLine final
{
public:
    [[nodiscard]]
    CUISubLine Cut2Pos(size_t i);

    void Draw(CGameFont* pFont, float x, float y) const;

public:
    xr_string m_text;
    u32 m_color{};
    bool m_last_in_line{};
};

static_assert(std::is_aggregate_v<CUISubLine>,
    "CUISubLine being aggregate is just convenient. "
    "If you don't think like that just remove this assert, "
    "but make sure that code using CUISubLine is working correctly.");
