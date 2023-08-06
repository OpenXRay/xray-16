#include "StdAfx.h"

#include "UIDebugFonts.h"

CUIDebugFonts::CUIDebugFonts()
    : CUIDialogWnd(CUIDebugFonts::GetDebugType()),
      m_background("Background")
{
    AttachChild(&m_background);
    InitDebugFonts({ 0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT });
}

void CUIDebugFonts::InitDebugFonts(Frect&& r)
{
    CUIDialogWnd::SetWndRect(r);

    FillUpList();

    m_background.SetWndRect(r);
    m_background.InitTexture("ui" DELIMITER "ui_debug_font");
}

bool CUIDebugFonts::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    switch (GetBindedAction(dik))
    {
    case kQUIT:
        HideDialog();
        break;

    case kSCREENSHOT:
        return false;
    }

    return true;
}

void CUIDebugFonts::FillUpList()
{
    CFontManager::FONTS_VEC& v = UI().Font().m_all_fonts;
    CFontManager::FONTS_VEC_IT it = v.begin();
    CFontManager::FONTS_VEC_IT it_e = v.end();
    Fvector2 pos, sz;
    pos.set(0, 0);
    sz.set(UI_BASE_WIDTH, UI_BASE_HEIGHT);
    string256 str;
    for (; it != it_e; ++it)
    {
        CGameFont* F = *(*it);
        CUITextWnd* pItem = xr_new<CUITextWnd>();
        pItem->SetWndPos(pos);
        pItem->SetWndSize(sz);
#ifdef DEBUG
        xr_sprintf(str, "%s:%s", F->m_font_name.c_str(), StringTable().translate("Test_Font_String").c_str());
#endif
        pItem->SetFont(F);
        pItem->SetText(str);
        pItem->SetTextComplexMode(false);
        pItem->SetVTextAlignment(valCenter);
        pItem->SetTextAlignment(CGameFont::alCenter);
        pItem->AdjustHeightToText();
        pos.y += pItem->GetHeight() + 20.0f;
        pItem->SetAutoDelete(true);
        AttachChild(pItem);
    }
}
