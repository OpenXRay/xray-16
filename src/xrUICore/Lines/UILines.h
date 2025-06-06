// File:		UILines.h
// Description:	Multilines Text Control
// Created:		11.03.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

#include "xrUICore/Lines/UILine.h"
#include "xrUICore/uiabstract.h"

class XRUICORE_API CUILines final : public CDeviceResetNotifier
{
    friend class CUICustomEdit;

public:
    CUILines();

    void SetText(LPCSTR text);
    void SetTextST(LPCSTR text);
    LPCSTR GetText() const;
    //--
    void SetTextColor(u32 color);
    u32 GetTextColor() { return m_dwTextColor; }
    void SetFont(CGameFont* pFont);
    CGameFont* GetFont() { return m_pFont; }
    void SetTextAlignment(ETextAlignment al) { m_eTextAlign = al; }
    ETextAlignment GetTextAlignment() { return m_eTextAlign; }
    void SetVTextAlignment(EVTextAlignment al) { m_eVTextAlign = al; }
    EVTextAlignment GetVTextAlignment() { return m_eVTextAlign; }
    void SetTextComplexMode(bool mode = true);
    void SetPasswordMode(bool mode = true);
    bool IsPasswordMode() { return !!uFlags.test(flPasswordMode); }
    void SetColoringMode(bool mode);
    void SetCutWordsMode(bool mode);
    void SetUseNewLineMode(bool mode);
    void SetEllipsis(bool mode);

    void Draw(float x, float y);

    // CDeviceResetNotifier methods
    void OnDeviceReset() override;

    // own methods
    void Reset();
    void ParseText(bool force = false);
    float GetVisibleHeight();
    float GetIndentByAlign() const;

    Fvector2 m_TextOffset{};
    Fvector2 m_wndSize{};
    Fvector2 m_wndPos{};

protected:
    // %c[255,255,255,255]
    u32 GetColorFromText(const xr_string& str) const;
    float GetVIndentByAlign();
    xr_string CutFirstColoredTextEntry(u32& color, xr_string& text) const;
    CUILine ParseTextToColoredLine(const std::string_view& str);

protected:
    enum : u8
    {
        flNeedReparse = (1 << 0),
        flComplexMode = (1 << 1),
        flPasswordMode = (1 << 2),
        flColoringMode = (1 << 3),
        flCutWordsMode = (1 << 4),
        flRecognizeNewLine = (1 << 5),
        flEllipsis = (1 << 6)
    };

    Flags8 uFlags{};
    ETextAlignment m_eTextAlign{ CGameFont::alLeft };
    EVTextAlignment m_eVTextAlign{ valTop };
    u32 m_dwTextColor{ 0xffffffff };

    xr_vector<CUILine> m_lines; // parsed text
    shared_str m_text{ "" };
    CGameFont* m_pFont{};
};
