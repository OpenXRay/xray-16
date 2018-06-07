// File:		UILines.cpp
// Description:	Multi-line Text Control
// Created:		12.03.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "StdAfx.h"

#include "UILines.h"
#include "UIXmlInit.h"
#include "uilinestd.h"
#include "string_table.h"
#include "xrCore/Text/MbHelpers.h"

CUILines::CUILines()
{
    m_pFont = NULL;
    m_eTextAlign = CGameFont::alLeft;
    m_eVTextAlign = valTop;
    m_dwTextColor = 0xffffffff;
    m_TextOffset.set(0.0f, 0.0f);
    m_wndSize.set(0.f, 0.f);
    m_wndPos.set(0.f, 0.f);
    m_text = "";
    uFlags.zero();
    uFlags.set(flNeedReparse, FALSE);
    uFlags.set(flComplexMode, FALSE);
    uFlags.set(flPasswordMode, FALSE);
    uFlags.set(flColoringMode, TRUE);
    uFlags.set(flCutWordsMode, FALSE);
    uFlags.set(flRecognizeNewLine, TRUE);
}

CUILines::~CUILines() {}
void CUILines::SetTextComplexMode(bool mode)
{
    uFlags.set(flComplexMode, mode);
    if (mode)
        uFlags.set(flPasswordMode, FALSE);
}

void CUILines::SetPasswordMode(bool mode)
{
    uFlags.set(flPasswordMode, mode);
    if (mode)
        uFlags.set(flComplexMode, false);
}

void CUILines::SetColoringMode(bool mode) { uFlags.set(flColoringMode, mode); }
void CUILines::SetCutWordsMode(bool mode) { uFlags.set(flCutWordsMode, mode); }
void CUILines::SetEllipsis(bool mode) { uFlags.set(flEllipsis, mode); }
void CUILines::SetUseNewLineMode(bool mode) { uFlags.set(flRecognizeNewLine, mode); }
void CUILines::SetText(const char* text)
{
    if (!m_pFont)
        m_pFont = UI().Font().pFontLetterica16Russian;

    if (text && text[0] != 0)
    {
        if (m_text == text)
            return;
        m_text = text;
        uFlags.set(flNeedReparse, TRUE);
    }
    else
    {
        m_text = "";
        Reset();
    }
}
void CUILines::SetTextST(LPCSTR str_id) { SetText(*CStringTable().translate(str_id)); }
LPCSTR CUILines::GetText() { return m_text.c_str(); }
void CUILines::Reset() { m_lines.clear(); }
float get_str_width(CGameFont* pFont, char ch)
{
    float ll = pFont->SizeOf_(ch);
    UI().ClientToScreenScaledWidth(ll);
    return ll;
}

void CUILines::ParseText(bool force)
{
    if (!force && (!uFlags.test(flComplexMode) || !uFlags.test(flNeedReparse)))
        return;

    if (NULL == m_pFont)
        return;

    Reset();

    CUILine* line = NULL;
    if (uFlags.test(flColoringMode))
        line = ParseTextToColoredLine(m_text.c_str());
    else
    {
        line = new CUILine();
        CUISubLine subline;
        subline.m_text = m_text.c_str();
        subline.m_color = GetTextColor();
        line->AddSubLine(&subline);
    }

    BOOL bNewLines = FALSE;

    if (uFlags.test(flRecognizeNewLine))
        if (m_pFont->IsMultibyte())
        {
            CUILine* ptmp_line = new CUILine();
            int vsz = line->m_subLines.size();
            VERIFY(vsz);
            for (int i = 0; i < vsz; i++)
            {
                char* pszTemp = NULL;
                const u32 tcolor = line->m_subLines[i].m_color;
                char szTempLine[MAX_MB_CHARS], *pszSearch = NULL;
                size_t llen = xr_strlen(line->m_subLines[i].m_text.c_str());
                VERIFY(llen < MAX_MB_CHARS);
                xr_strcpy(szTempLine, line->m_subLines[i].m_text.c_str());
                pszSearch = szTempLine;
                while ((pszTemp = strstr(pszSearch, "\\n")) != NULL)
                {
                    bNewLines = TRUE;
                    *pszTemp = '\0';
                    ptmp_line->AddSubLine(pszSearch, tcolor);
                    pszSearch = pszTemp + 2;
                }
                ptmp_line->AddSubLine(pszSearch, tcolor);
            }
            line->Clear();
            xr_free(line);
            line = ptmp_line;
        }
        else
        {
            line->ProcessNewLines();
        }

    if (m_pFont->IsMultibyte())
    {
#define UBUFFER_SIZE 100
        u16 aMarkers[UBUFFER_SIZE];
        CUILine tmp_line;
        char szTempLine[MAX_MB_CHARS];
        float fTargetWidth = 1.0f;
        UI().ClientToScreenScaledWidth(fTargetWidth);
        VERIFY((m_wndSize.x > 0) && (fTargetWidth > 0));
        fTargetWidth = m_wndSize.x / fTargetWidth;
        int vsz = line->m_subLines.size();
        VERIFY(vsz);
        if ((vsz > 1) && (!bNewLines))
        { // only colored line, pizdets
            for (int i = 0; i < vsz; i++)
            {
                const char* pszText = line->m_subLines[i].m_text.c_str();
                const u32 tcolor = line->m_subLines[i].m_color;
                VERIFY(pszText);
                tmp_line.AddSubLine(pszText, tcolor);
            }
            m_lines.push_back(tmp_line);
            tmp_line.Clear();
        }
        else
        {
            for (int i = 0; i < vsz; i++)
            {
                const char* pszText = line->m_subLines[i].m_text.c_str();
                const u32 tcolor = line->m_subLines[i].m_color;
                u16 uFrom = 0, uPartLen = 0;
                VERIFY(pszText);
                u16 nMarkers = m_pFont->SplitByWidth(aMarkers, UBUFFER_SIZE, fTargetWidth, pszText);
                for (u16 j = 0; j < nMarkers; j++)
                {
                    uPartLen = aMarkers[j] - uFrom;
                    VERIFY((uPartLen > 0) && (uPartLen < MAX_MB_CHARS));
                    strncpy_s(szTempLine, pszText + uFrom, uPartLen);
                    szTempLine[uPartLen] = '\0';
                    tmp_line.AddSubLine(szTempLine, tcolor);
                    m_lines.push_back(tmp_line);
                    tmp_line.Clear();
// Compiler bug :)
#pragma warning(push)
#pragma warning(disable : 4244)
                    uFrom += uPartLen;
#pragma warning(pop)
                }
                strncpy_s(szTempLine, pszText + uFrom, MAX_MB_CHARS);
                tmp_line.AddSubLine(szTempLine, tcolor);
                m_lines.push_back(tmp_line);
                tmp_line.Clear();
            }
        }
    }
    else
    {
        float max_width = m_wndSize.x;
        u32 sbl_cnt = line->m_subLines.size();
        CUILine tmp_line;
        string4096 buff;
        float curr_width = 0.0f;
        bool bnew_line = false;
        float __eps = get_str_width(m_pFont, 'o'); // hack -(
        for (u32 sbl_idx = 0; sbl_idx < sbl_cnt; ++sbl_idx)
        {
            bool b_last_subl = (sbl_idx == sbl_cnt - 1);
            CUISubLine& sbl = line->m_subLines[sbl_idx];
            u32 sub_len = (u32)sbl.m_text.length();
            u32 curr_w_pos = 0;

            u32 last_space_idx = 0;
            for (u32 idx = 0; idx < sub_len; ++idx)
            {
                bool b_last_ch = (idx == sub_len - 1);

                if (isspace((unsigned char)sbl.m_text[idx]))
                    last_space_idx = idx;

                float w1 = get_str_width(m_pFont, sbl.m_text[idx]);
                bool bOver = (curr_width + w1 + __eps > max_width);

                if (bOver || b_last_ch)
                {
                    if (last_space_idx && !b_last_ch)
                    {
                        idx = last_space_idx;
                        last_space_idx = 0;
                    }

                    strncpy_s(buff, sizeof(buff), sbl.m_text.c_str() + curr_w_pos, idx - curr_w_pos + 1);
                    tmp_line.AddSubLine(buff, sbl.m_color);
                    curr_w_pos = idx + 1;
                }
                else
                    curr_width += w1;

                if (bOver || (b_last_ch && sbl.m_last_in_line))
                {
                    m_lines.push_back(tmp_line);
                    tmp_line.Clear();
                    curr_width = 0.0f;
                    bnew_line = false;
                }
            }
            if (b_last_subl && !tmp_line.IsEmpty())
            {
                m_lines.push_back(tmp_line);
                tmp_line.Clear();
                curr_width = 0.0f;
                bnew_line = false;
            }
        }
    }
    xr_delete(line);
    uFlags.set(flNeedReparse, FALSE);
}

float CUILines::GetVisibleHeight()
{
    if (uFlags.test(flComplexMode))
    {
        if (uFlags.test(flNeedReparse))
            ParseText();

        float _curr_h = m_pFont->CurrentHeight_();
        UI().ClientToScreenScaledHeight(_curr_h);
        return _curr_h * m_lines.size();
    }
    else
    {
        float _curr_h = m_pFont->GetHeight();
        UI().ClientToScreenScaledHeight(_curr_h);
        return _curr_h;
    }
}

void CUILines::SetTextColor(u32 color)
{
    if (color == m_dwTextColor)
        return;
    uFlags.set(flNeedReparse, true);
    m_dwTextColor = color;
}

void CUILines::SetFont(CGameFont* pFont)
{
    if (pFont == m_pFont)
        return;
    uFlags.set(flNeedReparse, true);
    m_pFont = pFont;
}

LPCSTR GetElipsisText(CGameFont* pFont, float width, LPCSTR source_text, LPSTR buff, int buff_len)
{
    float text_len = pFont->SizeOf_(source_text);
    UI().ClientToScreenScaledWidth(text_len);

    if (text_len < width)
    {
        return source_text;
    }
    else
    {
        buff[0] = 0;
        float el_len = pFont->SizeOf_("..");
        UI().ClientToScreenScaledWidth(el_len);
        float total = 0.0f;
        u16 pos = 0;

        while (total + el_len < width)
        {
            const char c = *(source_text + pos);
            float ch_len = pFont->SizeOf_(c);
            UI().ClientToScreenScaledWidth(ch_len);

            if (total + ch_len + el_len < width)
                buff[pos] = c;

            total += ch_len;
            ++pos;
            buff[pos] = 0;
        }

        xr_strcat(buff, buff_len, "..");
        return buff;
    }
}

void CUILines::Draw(float x, float y)
{
    x += m_TextOffset.x;
    y += m_TextOffset.y;

    static string256 passText;

    if (m_text.size() == 0)
        return;

    R_ASSERT(m_pFont);
    m_pFont->SetColor(m_dwTextColor);

    if (!uFlags.is(flComplexMode))
    {
        Fvector2 text_pos;
        text_pos.set(0, 0);

        text_pos.x = x + GetIndentByAlign();
        //		text_pos.y = y + GetVIndentByAlign();
        text_pos.y = y;
        UI().ClientToScreenScaled(text_pos);
        text_pos.y += GetVIndentByAlign();

        if (uFlags.test(flPasswordMode))
        {
            int sz = (int)m_text.size();
            for (int i = 0; i < sz; i++)
                passText[i] = '*';
            passText[sz] = 0;
            m_pFont->SetAligment((CGameFont::EAligment)m_eTextAlign);
            m_pFont->Out(text_pos.x, text_pos.y, "%s", passText);
        }
        else
        {
            m_pFont->SetAligment((CGameFont::EAligment)m_eTextAlign);
            if (uFlags.test(flEllipsis))
            {
                u32 buff_len = sizeof(char) * xr_strlen(m_text.c_str()) + 1;

                char* p = static_cast<char*>(_alloca(buff_len));
                LPCSTR str = GetElipsisText(m_pFont, m_wndSize.x, m_text.c_str(), p, buff_len);

                m_pFont->Out(text_pos.x, text_pos.y, "%s", str);
            }
            else
                m_pFont->Out(text_pos.x, text_pos.y, "%s", m_text.c_str());
        }
    }
    else
    {
        ParseText();

        Fvector2 pos;
        // get vertical indent
        pos.y = y + GetVIndentByAlign();
        float height = m_pFont->CurrentHeight_();
        UI().ClientToScreenScaledHeight(height);

        u32 size = m_lines.size();

        m_pFont->SetAligment((CGameFont::EAligment)m_eTextAlign);
        for (int i = 0; i < (int)size; i++)
        {
            pos.x = x + GetIndentByAlign();
            m_lines[i].Draw(m_pFont, pos.x, pos.y);
            pos.y += height;
        }
    }
    m_pFont->OnRender();
}

void CUILines::OnDeviceReset() { uFlags.set(flNeedReparse, TRUE); }
float CUILines::GetIndentByAlign() const
{
    switch (m_eTextAlign)
    {
    case CGameFont::alCenter: { return (m_wndSize.x) / 2;
    }
    break;
    case CGameFont::alLeft: { return 0;
    }
    break;
    case CGameFont::alRight: { return (m_wndSize.x);
    }
    break;
    default: NODEFAULT;
    }
#ifdef DEBUG
    return 0;
#endif
}

float CUILines::GetVIndentByAlign()
{
    switch (m_eVTextAlign)
    {
    case valTop: return 0;
    case valCenter: return (m_wndSize.y - GetVisibleHeight()) / 2;
    case valBotton: return m_wndSize.y - GetVisibleHeight();
    default: NODEFAULT;
    }
#ifdef DEBUG
    return 0;
#endif
}

// %c[255,255,255,255]
u32 CUILines::GetColorFromText(const xr_string& str) const
{
    StrSize begin, end, comma1_pos, comma2_pos, comma3_pos;

    begin = str.find(BEGIN);
    end = str.find(END, begin);
    if (begin == npos || end == npos)
        return m_dwTextColor;
    // try default color
    if (npos != str.find("%c[default]", begin, end - begin))
        return m_dwTextColor;

    // Try predefined in XML colors
    for (CUIXmlInit::ColorDefs::const_iterator it = CUIXmlInit::GetColorDefs()->begin();
         it != CUIXmlInit::GetColorDefs()->end(); ++it)
    {
        int cmp = str.compare(begin + 3, end - begin - 3, *it->first);
        if (cmp == 0)
            return it->second;
    }

    // try parse values separated by commas
    comma1_pos = str.find(',', begin);
    comma2_pos = str.find(',', comma1_pos + 1);
    comma3_pos = str.find(',', comma2_pos + 1);
    if (comma1_pos == npos || comma2_pos == npos || comma3_pos == npos)
        return m_dwTextColor;

    u32 a, r, g, b;
    xr_string single_color;

    begin += 3;

    single_color = str.substr(begin, comma1_pos - 1);
    a = atoi(single_color.c_str());
    single_color = str.substr(comma1_pos + 1, comma2_pos - 1);
    r = atoi(single_color.c_str());
    single_color = str.substr(comma2_pos + 1, comma3_pos - 1);
    g = atoi(single_color.c_str());
    single_color = str.substr(comma3_pos + 1, end - 1);
    b = atoi(single_color.c_str());

    return color_argb(a, r, g, b);
}

CUILine* CUILines::ParseTextToColoredLine(const xr_string& str)
{
    CUILine* line = new CUILine();
    xr_string tmp = str;
    xr_string entry;
    u32 color;

    do
    {
        CutFirstColoredTextEntry(entry, color, tmp);
        line->AddSubLine(entry, subst_alpha(color, color_get_A(GetTextColor())));
    } while (tmp.size() > 0);

    return line;
}

void CUILines::CutFirstColoredTextEntry(xr_string& entry, u32& color, xr_string& text) const
{
    entry.clear();

    StrSize begin = text.find(BEGIN);
    StrSize end = text.find(END, begin);
    if (xr_string::npos == end)
        begin = end;
    StrSize begin2 = text.find(BEGIN, end);
    StrSize end2 = text.find(END, begin2);
    if (xr_string::npos == end2)
        begin2 = end2;

    // if we do not have any color entry or it is single with 0 position
    if (xr_string::npos == begin)
    {
        entry = text;
        color = m_dwTextColor;
        text.clear();
    }
    else if (0 == begin && xr_string::npos == begin2)
    {
        entry = text;
        color = GetColorFromText(entry);
        entry.replace(begin, end - begin + 1, "");
        text.clear();
    }
    // if we have color entry not at begin
    else if (0 != begin)
    {
        entry = text.substr(0, begin);
        color = m_dwTextColor;
        text.replace(0, begin, "");
    }
    // if we have two color entries. and first has 0 position
    else if (0 == begin && xr_string::npos != begin2)
    {
        entry = text.substr(0, begin2);
        color = GetColorFromText(entry);
        entry.replace(begin, end - begin + 1, "");
        text.replace(0, begin2, "");
    }
}
