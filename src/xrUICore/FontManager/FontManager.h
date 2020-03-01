#pragma once

struct XRUICORE_API CFontManager : public pureDeviceReset
{
    CFontManager();
    ~CFontManager();

    typedef xr_vector<CGameFont**> FONTS_VEC;
    typedef FONTS_VEC::iterator FONTS_VEC_IT;
    FONTS_VEC m_all_fonts;
    void Render();

    // hud font
    CGameFont* pFontMedium;
    CGameFont* pFontDI;

    CGameFont* pFontArial14;
    CGameFont* pFontGraffiti19Russian;
    CGameFont* pFontGraffiti22Russian;
    CGameFont* pFontLetterica16Russian;
    CGameFont* pFontLetterica18Russian;
    CGameFont* pFontGraffiti32Russian;
    CGameFont* pFontGraffiti50Russian;
    CGameFont* pFontLetterica25;
    CGameFont* pFontStat;

    void InitializeFonts();
    void InitializeFont(CGameFont*& F, LPCSTR section, u32 flags = 0);
    LPCSTR GetFontTexName(LPCSTR section);

    virtual void OnDeviceReset();
};
