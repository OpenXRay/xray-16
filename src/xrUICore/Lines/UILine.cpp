// File:		UILine.cpp
// Description:	Single text line
// Created:		05.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "pch.hpp"
#include "UILine.h"
#include "ui_base.h"
#include "xrEngine/GameFont.h"

void CUILine::ProcessNewLines()
{
    for (u32 i = 0; i < m_subLines.size(); i++)
    {
        auto pos = m_subLines[i].m_text.find("\\n");
        //		if (pos != xr_string::npos)
        //			pos = m_subLines[i].m_text.find('\r');

        if (pos != xr_string::npos)
        {
            CUISubLine sbLine;
            if (pos)
                sbLine = m_subLines[i].Cut2Pos(pos - 1);
            sbLine.m_last_in_line = true;
            m_subLines.insert(m_subLines.begin() + i, sbLine);
            m_subLines[i + 1].m_text.erase(0, 2);
            if (m_subLines[i + 1].m_text.empty())
            {
                m_subLines.erase(m_subLines.begin() + i + 1);
            }
        }
    }
}

void CUILine::Draw(CGameFont* pFont, float x, float y) const
{
    float length = 0;

    for (const auto& subline : m_subLines)
    {
        subline.Draw(pFont, x + length, y);
        float ll = pFont->SizeOf_(subline.m_text.c_str()); //. all ok
        UI().ClientToScreenScaledWidth(ll);
        length += ll;
    }
}
