#include "pch.hpp"
#include "FontManager.h"
#include "xrEngine/GameFont.h"

CFontManager::CFontManager()
{
    Device.seqDeviceReset.Add(this, REG_PRIORITY_HIGH);

    m_all_fonts.push_back(&pFontMedium); // used cpp
    m_all_fonts.push_back(&pFontDI); // used cpp
    m_all_fonts.push_back(&pFontArial14); // used xml
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
    InitializeFont(pFontMedium, "hud_font_medium");
    InitializeFont(pFontDI, "hud_font_di", CGameFont::fsGradient | CGameFont::fsDeviceIndependent);
    InitializeFont(pFontArial14, "ui_font_arial_14");
    InitializeFont(pFontGraffiti19Russian, "ui_font_graffiti19_russian");
    InitializeFont(pFontGraffiti22Russian, "ui_font_graffiti22_russian");
    InitializeFont(pFontLetterica16Russian, "ui_font_letterica16_russian");
    InitializeFont(pFontLetterica18Russian, "ui_font_letterica18_russian");
    InitializeFont(pFontGraffiti32Russian, "ui_font_graff_32");
    InitializeFont(pFontGraffiti50Russian, "ui_font_graff_50");
    InitializeFont(pFontLetterica25, "ui_font_letter_25");
    InitializeFont(pFontStat, "stat_font", CGameFont::fsDeviceIndependent);
    pFontStat->SetInterval(0.75f, 1.0f);
}

LPCSTR CFontManager::GetFontTexName(LPCSTR section)
{
    constexpr pcstr tex_names[] = { "texture800", "texture", "texture1600" };
    int def_idx = 1; // default 1024x768
    int idx = def_idx;

    u32 h = Device.dwHeight;

    if (h <= 600)
        idx = 0;
    else if (h < 1024)
        idx = 1;
    else
        idx = 2;

    while (idx >= 0)
    {
        if (pSettings->line_exist(section, tex_names[idx]))
            return pSettings->r_string(section, tex_names[idx]);
        --idx;
    }
    return pSettings->r_string(section, tex_names[def_idx]);
}

void CFontManager::InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
    LPCSTR font_tex_name = GetFontTexName(section);
    R_ASSERT(font_tex_name);

    LPCSTR sh_name = pSettings->r_string(section, "shader");
    if (!F)
        F = new CGameFont(sh_name, font_tex_name, flags);
    else
        F->Initialize(sh_name, font_tex_name);

#ifdef DEBUG
    F->m_font_name = section;
#endif
}

CFontManager::~CFontManager()
{
    Device.seqDeviceReset.Remove(this);
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
void CFontManager::OnDeviceReset() { InitializeFonts(); }
