// File:		UISubLine.cpp
// Description:	Text line. Owns color attribute
// Created:		04.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "pch.hpp"
#include "UISubLine.h"
#include "ui_base.h"
#include "xrEngine/GameFont.h"

CUISubLine CUISubLine::Cut2Pos(size_t i)
{
    CUISubLine result;
    result.m_color = m_color;

    R_ASSERT2_CURE(i < m_text.size(),
        make_string("CUISubLine::Cut2Pos - invalid parameter [%zu][%zu]", i, m_text.size()).c_str(),
        return result);

    result.m_text.assign(m_text, 0, i + 1);
    m_text.replace(0, i + 1, "");

    return result;
}

void CUISubLine::Draw(CGameFont* pFont, float x, float y) const
{
    pFont->SetColor(m_color);
    pFont->Out(UI().ClientToScreenScaledX(x), UI().ClientToScreenScaledY(y), "%s", m_text.c_str());
}
