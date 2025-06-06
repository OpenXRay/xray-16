// File:		UILine.h
// Description:	Single text line
// Created:		11.03.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

#include "UISubLine.h"

class CUILine final
{
    friend class CUILines;

public:
    void AddSubLine(const xr_string& str, u32 color) { m_subLines.emplace_back(CUISubLine{ str, color }); }
    void AddSubLine(pcstr str, u32 color) { m_subLines.emplace_back(CUISubLine{ str, color }); }
    void AddSubLine(CUISubLine&& subLine) { m_subLines.emplace_back(std::move(subLine)); }

    void Clear() { m_subLines.clear(); }

    [[nodiscard]]
    bool IsEmpty() const { return m_subLines.empty(); }

    void ProcessNewLines();

    void Draw(CGameFont* pFont, float x, float y) const;

protected:
    xr_vector<CUISubLine> m_subLines;
};
