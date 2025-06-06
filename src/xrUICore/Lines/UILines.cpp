// File:		UILines.cpp
// Description:	Multi-line Text Control
// Created:		12.03.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "pch.hpp"
#include "UILines.h"
#include "XML/UIXmlInitBase.h"
#include "xrCore/Text/StringConversion.hpp"

constexpr auto COLOR_TAG_BEGIN = "%c[";
constexpr auto COLOR_TAG_END = ']';

CUILines::CUILines()
{
    uFlags.set(flNeedReparse, false);
    uFlags.set(flComplexMode, false);
    uFlags.set(flPasswordMode, false);
    uFlags.set(flColoringMode, true);
    uFlags.set(flCutWordsMode, false);
    uFlags.set(flRecognizeNewLine, true);
}

void CUILines::SetTextComplexMode(bool mode)
{
    uFlags.set(flComplexMode, mode);
    if (mode)
        uFlags.set(flPasswordMode, false);
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
        uFlags.set(flNeedReparse, true);
    }
    else
    {
        m_text = "";
        Reset();
    }
}
void CUILines::SetTextST(LPCSTR str_id) { SetText(*StringTable().translate(str_id)); }
LPCSTR CUILines::GetText() const { return m_text.c_str(); }
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

    if (!m_pFont)
        return;

    Reset();

    CUILine line;
    if (uFlags.test(flColoringMode))
        line = ParseTextToColoredLine(m_text.c_str());
    else
        line.AddSubLine({ m_text.c_str(), GetTextColor() });

    bool bNewLines = false;

    if (uFlags.test(flRecognizeNewLine))
    {
        if (m_pFont->IsMultibyte())
        {
            CUILine tmp_line;
            const size_t vsz = line.m_subLines.size();
            VERIFY(vsz);
            for (size_t i = 0; i < vsz; i++)
            {
                char* pszTemp;
                const u32 tcolor = line.m_subLines[i].m_color;
                char szTempLine[MAX_MB_CHARS], *pszSearch = nullptr;
                [[maybe_unused]] const auto llen = line.m_subLines[i].m_text.size();
                VERIFY(llen < MAX_MB_CHARS);
                xr_strcpy(szTempLine, line.m_subLines[i].m_text.c_str());
                pszSearch = szTempLine;
                while ((pszTemp = strstr(pszSearch, "\\n")) != nullptr)
                {
                    bNewLines = true;
                    *pszTemp = '\0';
                    tmp_line.AddSubLine({ pszSearch, tcolor, true });
                    pszSearch = pszTemp + 2;
                }
                tmp_line.AddSubLine(pszSearch, tcolor);
            }
            line = std::move(tmp_line);
        }
        else
        {
            line.ProcessNewLines();
        }
    }
    if (m_pFont->IsMultibyte())
    {
#define UBUFFER_SIZE 100
        u16 aMarkers[UBUFFER_SIZE];
        char szTempLine[MAX_MB_CHARS];
        float fTargetWidth = 1.0f;
        UI().ClientToScreenScaledWidth(fTargetWidth);
        VERIFY((m_wndSize.x > 0) && (fTargetWidth > 0));
        fTargetWidth = m_wndSize.x / fTargetWidth;
        size_t vsz = line.m_subLines.size();
        VERIFY(vsz);
        if ((vsz > 1) && (!bNewLines))
        { // only colored line
            for (auto& subLine : line.m_subLines)
            {
                VERIFY(subLine.m_text.data());
                subLine.m_last_in_line = false;
            }
            m_lines.emplace_back(std::move(line));
        }
        else
        {
            CUILine tmp_line;
            for (size_t i = 0; i < vsz; i++)
            {
                CUISubLine subLine = line.m_subLines[i];
                const char* pszText = subLine.m_text.c_str();
                const u32 tcolor = subLine.m_color;
                u16 uFrom = 0;
                VERIFY(pszText);
                u16 nMarkers = m_pFont->SplitByWidth(aMarkers, UBUFFER_SIZE, fTargetWidth, pszText);
                for (u16 j = 0; j < nMarkers; j++)
                {
                    const u16 uPartLen = aMarkers[j] - uFrom;
                    VERIFY((uPartLen > 0) && (uPartLen < MAX_MB_CHARS));
                    strncpy_s(szTempLine, pszText + uFrom, uPartLen);
                    szTempLine[uPartLen] = '\0';
                    tmp_line.AddSubLine(szTempLine, tcolor);
                    m_lines.emplace_back(tmp_line);
                    tmp_line.Clear();
                    uFrom += uPartLen;
                }
                strncpy_s(szTempLine, pszText + uFrom, MAX_MB_CHARS);
                tmp_line.AddSubLine(szTempLine, tcolor);
                if (subLine.m_last_in_line || i == (vsz -1))
                {
                    m_lines.emplace_back(tmp_line);
                    tmp_line.Clear();
                }
            }
        }
    }
    else
    {
        float max_width = m_wndSize.x;
        size_t sbl_cnt = line.m_subLines.size();
        CUILine tmp_line;
        string4096 buff;
        float curr_width = 0.0f;

        float eps = get_str_width(m_pFont, 'o'); // hack -(
        for (size_t sbl_idx = 0; sbl_idx < sbl_cnt; ++sbl_idx)
        {
            bool b_last_subl = (sbl_idx == sbl_cnt - 1);
            CUISubLine& sbl = line.m_subLines[sbl_idx];
            size_t sub_len = sbl.m_text.length();
            size_t curr_w_pos = 0;

            size_t last_space_idx = 0;
            for (size_t idx = 0; idx < sub_len; ++idx)
            {
                bool b_last_ch = (idx == sub_len - 1);

                if (isspace((unsigned char)sbl.m_text[idx]))
                    last_space_idx = idx;

                float w1 = get_str_width(m_pFont, sbl.m_text[idx]);
                bool bOver = (curr_width + w1 + eps > max_width);

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
                    m_lines.emplace_back(tmp_line);
                    tmp_line.Clear();
                    curr_width = 0.0f;
                }
            }
            if (b_last_subl && !tmp_line.IsEmpty())
            {
                m_lines.emplace_back(tmp_line);
                tmp_line.Clear();
                curr_width = 0.0f;
            }
        }
    }
    uFlags.set(flNeedReparse, false);
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

LPCSTR GetElipsisText(CGameFont* pFont, float width, LPCSTR source_text, pstr buff, int buff_len)
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

    if (m_text.empty())
        return;

    R_ASSERT(m_pFont);
    m_pFont->SetColor(m_dwTextColor);

    if (!uFlags.is(flComplexMode))
    {
        Fvector2 text_pos;
        text_pos.set(0, 0);

        text_pos.x = x + GetIndentByAlign();
        text_pos.y = y + GetVIndentByAlign();
        UI().ClientToScreenScaled(text_pos);

        if (uFlags.test(flPasswordMode))
        {
            const size_t sz = m_text.size();
            for (size_t i = 0; i < sz; i++)
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

                char* p = static_cast<char*>(xr_alloca(buff_len));
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

        m_pFont->SetAligment((CGameFont::EAligment)m_eTextAlign);
        for (const auto& line : m_lines)
        {
            pos.x = x + GetIndentByAlign();
            line.Draw(m_pFont, pos.x, pos.y);
            pos.y += height;
        }
    }
    m_pFont->OnRender();
}

void CUILines::OnDeviceReset() { uFlags.set(flNeedReparse, true); }
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
    case valBottom: return m_wndSize.y - GetVisibleHeight();
    default: NODEFAULT;
    }
#ifdef DEBUG
    return 0;
#endif
}

// %c[255,255,255,255]
// %c[default]
// %c[color_name]
u32 CUILines::GetColorFromText(const xr_string& str) const
{
    auto begin = str.find(COLOR_TAG_BEGIN);
    const auto end = str.find(COLOR_TAG_END, begin);

    // Check if there even is a valid color tag
    if (begin == xr_string::npos || end == xr_string::npos || end - begin < 3)
        return m_dwTextColor;

    // Extract color tag value
    const xr_string color_tag = str.substr(begin + 3, end - begin - 3);

    // Try default color
    if (color_tag == "default")
        return m_dwTextColor;

    // Try predefined XML colors
    const auto* color_defs = CUIXmlInitBase::GetColorDefs();
    if (color_defs->find(color_tag.c_str()) != color_defs->end())
        return color_defs->at(color_tag.c_str());

    // Try parse values separated by commas
    const auto comma1_pos = str.find(',', begin);
    const auto comma2_pos = str.find(',', comma1_pos + 1);
    const auto comma3_pos = str.find(',', comma2_pos + 1);
    if (comma1_pos == xr_string::npos || comma2_pos == xr_string::npos || comma3_pos == xr_string::npos)
        return m_dwTextColor;

    xr_string single_color;

    begin += 3;

    single_color = str.substr(begin, comma1_pos - 1);
    const u32 a = atoi(single_color.c_str());
    single_color = str.substr(comma1_pos + 1, comma2_pos - 1);
    const u32 r = atoi(single_color.c_str());
    single_color = str.substr(comma2_pos + 1, comma3_pos - 1);
    const u32 g = atoi(single_color.c_str());
    single_color = str.substr(comma3_pos + 1, end - 1);
    const u32 b = atoi(single_color.c_str());

    return color_argb(a, r, g, b);
}

CUILine CUILines::ParseTextToColoredLine(const std::string_view& str)
{
    xr_string tmp{ str };

    CUILine line;
    do
    {
        u32 color;
        xr_string entry = CutFirstColoredTextEntry(color, tmp);
        line.AddSubLine({ std::move(entry), subst_alpha(color, color_get_A(GetTextColor())) });
    } while (!tmp.empty());

    return line;
}

xr_string CUILines::CutFirstColoredTextEntry(u32& color, xr_string& text) const
{
    xr_string entry;

    auto begin = text.find(COLOR_TAG_BEGIN);
    auto end = text.find(COLOR_TAG_END, begin);
    if (xr_string::npos == end)
        begin = end;
    auto begin2 = text.find(COLOR_TAG_BEGIN, end);
    auto end2 = text.find(COLOR_TAG_END, begin2);
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

    return entry;
}
