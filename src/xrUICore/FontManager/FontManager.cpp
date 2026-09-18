#include "pch.hpp"
#include "FontManager.h"
#include "xrEngine/GameFont.h"

CFontManager::CFontManager()
{
    m_all_fonts.push_back(&pFontMedium);
    m_all_fonts.push_back(&pFontDI);
    m_all_fonts.push_back(&pFontArial14);
    m_all_fonts.push_back(&pFontGraffiti19Russian);
    m_all_fonts.push_back(&pFontGraffiti22Russian);
    m_all_fonts.push_back(&pFontLetterica16Russian);
    m_all_fonts.push_back(&pFontLetterica18Russian);
    m_all_fonts.push_back(&pFontGraffiti32Russian);
    m_all_fonts.push_back(&pFontGraffiti50Russian);
    m_all_fonts.push_back(&pFontLetterica25);
    m_all_fonts.push_back(&pFontStat);

    FONTS_VEC_IT it = m_all_fonts.begin();
    FONTS_VEC_IT it_e = m_all_fonts.end();
    for (; it != it_e; ++it)
        (**it) = NULL;

    InitializeFonts();
}

void CFontManager::InitializeFonts()
{
    InitializeFont(pFontMedium, "hud_font_medium", 0);
    InitializeFont(pFontDI, "hud_font_di", CGameFont::fsGradient | CGameFont::fsDeviceIndependent);
    InitializeFont(pFontArial14, "ui_font_arial_14", 0);
    InitializeFont(pFontGraffiti19Russian, "ui_font_graffiti19_russian", 0);
    InitializeFont(pFontGraffiti22Russian, "ui_font_graffiti22_russian", 0);
    InitializeFont(pFontLetterica16Russian, "ui_font_letterica16_russian", 0);
    InitializeFont(pFontLetterica18Russian, "ui_font_letterica18_russian", 0);
    InitializeFont(pFontGraffiti32Russian, "ui_font_graff_32", 0);
    InitializeFont(pFontGraffiti50Russian, "ui_font_graff_50", 0);
    InitializeFont(pFontLetterica25, "ui_font_letter_25", 0);
    InitializeFont(pFontStat, "stat_font", CGameFont::fsDeviceIndependent);
}

void CFontManager::InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
    if (!F)
        F = xr_new<CGameFont>(section, (u8)flags);
    else
        F->ReInit();

#ifdef DEBUG
    F->m_font_name = section;
#endif

    if (pSettings->line_exist(section, "interval"))
        F->SetInterval(pSettings->r_fvector2(section, "interval"));
}

CFontManager::~CFontManager()
{
    FONTS_VEC_IT it = m_all_fonts.begin();
    FONTS_VEC_IT it_e = m_all_fonts.end();
    for (; it != it_e; ++it)
        xr_delete(**it);
}

void CFontManager::Render()
{
    FONTS_VEC_IT it = m_all_fonts.begin();
    FONTS_VEC_IT it_e = m_all_fonts.end();
    for (; it != it_e; ++it)
        (**it)->OnRender();
}

void CFontManager::OnUIReset()
{
    InitializeFonts();
}
