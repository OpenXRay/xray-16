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
#include "xrCore/Text/Utf8Utils.hpp"

constexpr auto COLOR_TAG_BEGIN = "%c[";
constexpr auto COLOR_TAG_END = ']';

namespace
{
constexpr size_t MAX_MB_CHARS = 4096;

// Same rules as CUILine::ProcessNewLines: LTX "\\n" plus real CR/LF (UTF-8: 0x0D/0x0A are single-byte).
bool next_line_break_in_scan(pcstr pszSearch, size_t& out_off, size_t& out_skip)
{
    out_off = 0;
    out_skip = 0;
    pcstr best = nullptr;
    size_t best_skip = 0;
    auto consider = [&](pcstr p, size_t sk)
    {
        if (!p)
            return;
        if (!best || p < best)
        {
            best = p;
            best_skip = sk;
        }
    };

    consider(strstr(pszSearch, "\\n"), 2);

    for (pcstr q = pszSearch; *q;)
    {
        const unsigned char c = static_cast<unsigned char>(*q);
        if (c == '\r' && q[1] == '\n')
        {
            consider(q, 2);
            q += 2;
        }
        else if (c == '\r')
        {
            consider(q, 1);
            ++q;
        }
        else if (c == '\n')
        {
            consider(q, 1);
            ++q;
        }
        else
            ++q;
    }

    if (!best)
        return false;
    out_off = static_cast<size_t>(best - pszSearch);
    out_skip = best_skip;
    return true;
}
} // namespace

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
void CUILines::SetTextST(LPCSTR str_id) { SetText(StringTable().translate(str_id).c_str()); }
LPCSTR CUILines::GetText() const { return m_text.c_str(); }
void CUILines::Reset() { m_lines.clear(); }

// Copy up to 'count' bytes from 'src' into 'dest' and trim any trailing
// incomplete UTF-8 codepoint. This is a safety net for SplitByWidth returning
// a position that falls in the middle of a multi-byte codepoint.
static size_t copy_valid_utf8(char* dest, size_t destsz, pcstr src, size_t count)
{
    size_t copy_len = std::min(count, destsz - 1);
    strncpy_s(dest, destsz, src, copy_len);
    dest[copy_len] = '\0';

    while (copy_len > 0 && !XRay::Utf8::IsValid(dest))
    {
        --copy_len;
        dest[copy_len] = '\0';
    }

    return copy_len;
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
        CUILine tmp_line;
        const size_t vsz = line.m_subLines.size();
        VERIFY(vsz);
        for (size_t i = 0; i < vsz; i++)
        {
            const u32 tcolor = line.m_subLines[i].m_color;
            char szTempLine[MAX_MB_CHARS], *pszSearch = nullptr;
            [[maybe_unused]] const auto llen = line.m_subLines[i].m_text.size();
            VERIFY(llen < MAX_MB_CHARS);
            xr_strcpy(szTempLine, line.m_subLines[i].m_text.c_str());
            pszSearch = szTempLine;
            size_t br_off = 0, br_skip = 0;
            while (next_line_break_in_scan(pszSearch, br_off, br_skip))
            {
                bNewLines = true;
                char* seg_end = const_cast<char*>(pszSearch) + br_off;
                *seg_end = '\0';
                tmp_line.AddSubLine({ pszSearch, tcolor, true });
                pszSearch = seg_end + br_skip;
            }
            tmp_line.AddSubLine(pszSearch, tcolor);
        }
        line = std::move(tmp_line);
    }

    // All TTF/OTF fonts are multibyte; use the UTF-8 aware SplitByWidth path.
    constexpr size_t UBUFFER_SIZE = 256;
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
                copy_valid_utf8(szTempLine, MAX_MB_CHARS, pszText + uFrom, uPartLen);
                tmp_line.AddSubLine(szTempLine, tcolor);
                m_lines.emplace_back(tmp_line);
                tmp_line.Clear();
                uFrom += uPartLen;
            }
            copy_valid_utf8(szTempLine, MAX_MB_CHARS, pszText + uFrom, MAX_MB_CHARS - 1);
            tmp_line.AddSubLine(szTempLine, tcolor);
            if (subLine.m_last_in_line || i == (vsz -1))
            {
                m_lines.emplace_back(tmp_line);
                tmp_line.Clear();
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
        // Use CurrentHeight_() (which applies g_text_scale) instead of GetHeight()
        // so that vertical centering/bottom alignment matches the actually drawn
        // text height when the global text scale differs from 1.0.
        float _curr_h = m_pFont->CurrentHeight_();
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

        size_t buff_pos = 0;
        for (pcstr p = source_text; *p; p = XRay::Utf8::Next(p))
        {
            pcstr next = XRay::Utf8::Next(p);
            const size_t cp_bytes = next - p;

            xr_string slice(source_text, next - source_text);
            float slice_len = pFont->SizeOf_(slice.c_str());
            UI().ClientToScreenScaledWidth(slice_len);

            if (slice_len + el_len < width)
            {
                if (buff_pos + cp_bytes >= static_cast<size_t>(buff_len))
                    break;

                strncpy_s(buff + buff_pos, static_cast<size_t>(buff_len) - buff_pos, p, cp_bytes);
                buff_pos += cp_bytes;
                buff[buff_pos] = 0;
            }
            else
            {
                break;
            }
        }

        xr_strcat(buff, buff_len, "..");
        return buff;
    }
}

void CUILines::Draw(float x, float y)
{
    x += m_TextOffset.x;
    y += m_TextOffset.y;

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
            const size_t sz = XRay::Utf8::LengthCodepoints(m_text.c_str());
            xr_string passText(sz, '*');
            m_pFont->SetAligment((CGameFont::EAligment)m_eTextAlign);
            m_pFont->Out(text_pos.x, text_pos.y, "%s", passText.c_str());
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

bool CUILines::BuildVisualLineRanges(xr_vector<std::pair<size_t, size_t>>& out_ranges)
{
    out_ranges.clear();
    ParseText(true);
    const size_t text_len = m_text.size();
    if (text_len == 0)
        return true;

    if (m_lines.empty())
        return false;

    size_t accum = 0;
    for (auto& l : m_lines)
    {
        size_t line_len = 0;
        for (auto& sl : l.m_subLines)
            line_len += sl.m_text.length();
        out_ranges.emplace_back(accum, line_len);
        accum += line_len;

        // Parsed visual lines don't keep newline separators; remap to source text indices.
        if (accum < text_len)
        {
            if (m_text[accum] == '\r')
            {
                ++accum;
                if (accum < text_len && m_text[accum] == '\n')
                    ++accum;
            }
            else if (m_text[accum] == '\n')
            {
                ++accum;
            }
            else if (m_text[accum] == '\\' && (accum + 1) < text_len && m_text[accum + 1] == 'n')
            {
                accum += 2;
            }
        }
    }
    return true;
}

bool CUILines::ComputeCursorPlacement(size_t cursor_pos, float& out_x_in_line, size_t& out_visual_line_idx)
{
    out_x_in_line = 0.f;
    out_visual_line_idx = 0;

    if (!m_pFont)
        return false;

    if (!uFlags.test(flComplexMode))
    {
        const size_t c = std::min(cursor_pos, m_text.size());
        xr_string slice;
        slice.assign(m_text.c_str(), c);
        // Same units as CUICustomEdit::Draw: raw SizeOf_, then parent scales with ClientToScreenScaled once.
        out_x_in_line = m_pFont->SizeOf_(slice.c_str());
        return true;
    }

    if (m_text.empty())
        return true;

    xr_vector<std::pair<size_t, size_t>> line_ranges;
    if (!BuildVisualLineRanges(line_ranges) || line_ranges.empty())
        return false;

    const size_t text_len = m_text.size();
    const size_t c = std::min(cursor_pos, text_len);

    size_t vi = 0;
    bool found = false;
    for (size_t i = 0; i < line_ranges.size(); ++i)
    {
        const size_t start = line_ranges[i].first;
        const size_t len = line_ranges[i].second;
        if (c >= start && c <= start + len)
        {
            vi = i;
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    const size_t line_start = line_ranges[vi].first;
    const size_t span = c - line_start;

    xr_string slice;
    slice.assign(m_text.c_str() + line_start, span);
    out_x_in_line = m_pFont->SizeOf_(slice.c_str());
    out_visual_line_idx = vi;
    return true;
}

bool CUILines::CursorPosFromLocalPoint(float local_x, float local_y, size_t& out_pos, float scroll_offset_y,
    const float* vindent_override)
{
    out_pos = 0;
    if (!m_pFont)
        return false;

    auto prefix_width = [&](size_t glyph_start, size_t n) -> float
    {
        if (n == 0)
            return 0.f;
        xr_string slice;
        slice.assign(m_text.c_str() + glyph_start, n);
        float w = m_pFont->SizeOf_(slice.c_str());
        UI().ClientToScreenScaledWidth(w);
        return w;
    };

    if (!uFlags.test(flComplexMode))
    {
        const size_t len = m_text.size();
        if (len == 0)
            return true;

        const float rel_x = local_x - m_TextOffset.x - GetIndentByAlign();
        if (rel_x <= 0.f)
        {
            out_pos = 0;
            return true;
        }

        pcstr p = m_text.c_str();
        while (*p)
        {
            pcstr next = XRay::Utf8::Next(p);
            const float w = prefix_width(0, next - m_text.c_str());
            if (rel_x <= w)
            {
                out_pos = next - m_text.c_str();
                return true;
            }
            p = next;
        }
        out_pos = len;
        return true;
    }

    ParseText(true);
    if (m_text.empty())
        return true;

    xr_vector<std::pair<size_t, size_t>> line_ranges;
    if (!BuildVisualLineRanges(line_ranges) || line_ranges.empty())
        return false;

    float line_h = m_pFont->CurrentHeight_();
    UI().ClientToScreenScaledHeight(line_h);

    const float vindent = vindent_override ? *vindent_override : GetVIndentByAlign();
    const float rel_y = local_y - m_TextOffset.y - vindent + scroll_offset_y;
    const float rel_x = local_x - m_TextOffset.x - GetIndentByAlign();

    const float total_h = line_h * static_cast<float>(line_ranges.size());
    int line_idx = 0;
    if (line_h > EPS_L)
    {
        if (rel_y < 0.f)
            line_idx = 0;
        else if (rel_y >= total_h)
            line_idx = static_cast<int>(line_ranges.size()) - 1;
        else
            line_idx = static_cast<int>(rel_y / line_h);
    }

    {
        const int max_i = static_cast<int>(line_ranges.size()) - 1;
        if (line_idx < 0)
            line_idx = 0;
        else if (line_idx > max_i)
            line_idx = max_i;
    }

    const size_t start = line_ranges[line_idx].first;
    const size_t line_len = line_ranges[line_idx].second;
    const size_t line_end = start + line_len;

    if (rel_x <= 0.f)
    {
        out_pos = start;
        return true;
    }

    pcstr line_start = m_text.c_str() + start;
    pcstr p = line_start;
    while (*p)
    {
        pcstr next = XRay::Utf8::Next(p);
        if (next > m_text.c_str() + line_end)
            break;

        const float w = prefix_width(start, next - line_start);
        if (rel_x <= w)
        {
            out_pos = next - m_text.c_str();
            return true;
        }
        p = next;
    }
    out_pos = line_end;
    return true;
}

bool CUILines::MoveCursorByVisualLine(size_t& io_cursor, int delta)
{
    if (!uFlags.test(flComplexMode) || delta == 0 || (delta != -1 && delta != 1))
        return false;

    if (m_text.empty())
        return false;

    xr_vector<std::pair<size_t, size_t>> line_ranges;
    if (!BuildVisualLineRanges(line_ranges) || line_ranges.empty())
        return false;

    const size_t text_len = m_text.size();

    const size_t c = std::min(io_cursor, text_len);
    size_t vi = 0;
    size_t col = 0;
    bool found = false;
    for (size_t i = 0; i < line_ranges.size(); ++i)
    {
        const size_t start = line_ranges[i].first;
        const size_t len = line_ranges[i].second;
        if (c >= start && c <= start + len)
        {
            vi = i;
            col = XRay::Utf8::DistanceCodepoints(m_text.c_str() + start, m_text.c_str() + c);
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    const int target_vi = static_cast<int>(vi) + delta;
    if (target_vi < 0 || static_cast<size_t>(target_vi) >= line_ranges.size())
        return false;

    const size_t tgt_start = line_ranges[target_vi].first;
    const size_t tgt_len = line_ranges[target_vi].second;
    const size_t tgt_cp_len = XRay::Utf8::DistanceCodepoints(
        m_text.c_str() + tgt_start, m_text.c_str() + tgt_start + tgt_len);
    const size_t new_col = std::min(col, tgt_cp_len);
    io_cursor = XRay::Utf8::Advance(m_text.c_str() + tgt_start, new_col) - m_text.c_str();
    return true;
}

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
