// File:		UILine.h
// Description:	Single text line
// Created:		11.03.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once
#include "UISubLine.h"

class Word
{
public:
    Word()
    {
        len = 0;
        len_full = 0;
        pos = 0;
    }
    int len;
    int len_full;
    int pos;
    IC int last_symbol() { return pos + len - 1; }
    IC int last_space() { return pos + len_full - 1; }
    IC bool exist() { return len > 0; }
};

class Position
{
public:
    Word word_1;
    Word word_2;
    u32 curr_subline;
    u32 slash_n_size() { return 2; }
    IC bool is_separated() { return (0 == word_2.pos) || (word_1.pos >= word_2.pos); }
};

// Attention! Destructor is not virtual.
// if you want to inherit this class then make _coresponding_ changes
class CUILine
{
    friend class CUILines;

public:
    CUILine();
    ~CUILine();
    CUILine(const CUILine& other);
    CUILine& operator=(const CUILine& other);
    void AddSubLine(const xr_string& str, u32 color);
    void AddSubLine(const char* str, u32 color);
    void AddSubLine(const CUISubLine* subLine);
    void Clear();
    void ProcessNewLines();

    void Draw(CGameFont* pFont, float x, float y) const;
    bool IsEmpty() { return m_subLines.empty(); }
protected:
    int GetSize();
    const CUILine* Cut2Pos(Position& pos, bool to_first = true);
    const CUILine* GetEmptyLine();

    xr_vector<CUISubLine> m_subLines;

    CUILine* m_tmpLine;
};
