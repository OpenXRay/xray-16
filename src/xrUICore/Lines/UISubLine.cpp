// File:		UISubLine.cpp
// Description:	Text line. Owns color attribute
// Created:		04.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "pch.hpp"
#include "UISubLine.h"
#include "uilinestd.h"
#include "ui_base.h"
#include "xrEngine/GameFont.h"

CUISubLine::CUISubLine(const CUISubLine& other)
{
    m_color = other.m_color;
    m_last_in_line = other.m_last_in_line;
    m_text = other.m_text;
    m_pTempLine = NULL;
}

CUISubLine& CUISubLine::operator=(const CUISubLine& other)
{
    m_color = other.m_color;
    m_text = other.m_text;
    m_last_in_line = other.m_last_in_line;
    xr_delete(m_pTempLine);
    return (*this);
}

CUISubLine::CUISubLine() : m_color(0), m_pTempLine(NULL), m_last_in_line(false)
{
}

CUISubLine::~CUISubLine()
{
    xr_delete(m_pTempLine);
}

const CUISubLine* CUISubLine::Cut2Pos(int i)
{
    R_ASSERT2(i < (int)m_text.size(),
        make_string("CUISubLine::Cut2Pos - invalid parameter [%d][%d]", i, m_text.size()).c_str());

    if (!m_pTempLine)
        m_pTempLine = new CUISubLine();
    m_pTempLine->m_color = m_color;
    m_pTempLine->m_text.assign(m_text, 0, i + 1);
    m_text.replace(0, i + 1, "");

    return m_pTempLine;
}

void CUISubLine::Draw(CGameFont* pFont, float x, float y) const
{
    pFont->SetColor(m_color);
    pFont->Out(UI().ClientToScreenScaledX(x), UI().ClientToScreenScaledY(y), "%s", m_text.c_str());
}
