#pragma once
#include "xrUICore/Lines/UILine.h"
#include "xrUICore/uiabstract.h"

class XRUICORE_API CUILines : public CDeviceResetNotifier
{
    friend class CUICustomEdit;

public:
    CUILines();
    virtual ~CUILines();

    void SetText(LPCSTR text);
    void SetTextST(LPCSTR text);
    LPCSTR GetText();
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
    bool IsPasswordMode() { return !!uFlags.test(flPasswordMode); };
    void SetColoringMode(bool mode);
    void SetCutWordsMode(bool mode);
    void SetUseNewLineMode(bool mode);
    void SetEllipsis(bool mode);

    void Draw(float x, float y);

    // CDeviceResetNotifier methods
    virtual void OnDeviceReset();

    // own methods
    void Reset();
    void ParseText(bool force = false);
    float GetVisibleHeight();
    float GetIndentByAlign() const;

    Fvector2 m_TextOffset;
    Fvector2 m_wndSize;
    Fvector2 m_wndPos;

protected:
    // %c[255,255,255,255]
    u32 GetColorFromText(const xr_string& str) const;
    float GetVIndentByAlign();
    void CutFirstColoredTextEntry(xr_string& entry, u32& color, xr_string& text) const;
    CUILine* ParseTextToColoredLine(const xr_string& str);

    typedef xr_vector<CUILine> LinesVector;
    typedef LinesVector::iterator LinesVector_it;
    LinesVector m_lines; // parsed text

    shared_str m_text;

    ETextAlignment m_eTextAlign;
    EVTextAlignment m_eVTextAlign;
    u32 m_dwTextColor;

    CGameFont* m_pFont;

    enum
    {
        flNeedReparse = (1 << 0),
        flComplexMode = (1 << 1),
        flPasswordMode = (1 << 2),
        flColoringMode = (1 << 3),
        flCutWordsMode = (1 << 4),
        flRecognizeNewLine = (1 << 5),
        flEllipsis = (1 << 6)
    };

private:
    Flags8 uFlags;
};
