#include "StdAfx.h"
#include "UISpeechMenu.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIGameCustom.h"
#include "UIXmlInit.h"
#include "game_cl_mp.h"
#include "Level.h"
#include "string_table.h"

CUISpeechMenu::CUISpeechMenu(LPCSTR section_name)
{
    m_pList = new CUIScrollView();
    AttachChild(m_pList);
    m_pList->SetAutoDelete(true);
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "maingame.xml");
    CUIXmlInit::InitWindow(xml_doc, "speech_menu", 0, this);
    CUIXmlInit::InitScrollView(xml_doc, "speech_menu", 0, m_pList);
    m_pList->SetWndPos(Fvector2().set(0, 0));
    m_text_color = 0xffffffff;
    CUIXmlInit::InitFont(xml_doc, "speech_menu:text", 0, m_text_color, m_pFont);
    InitList(section_name);
}

CUISpeechMenu::~CUISpeechMenu()
{
    int x = 0;
    x = x;
}

void CUISpeechMenu::InitList(LPCSTR section_name)
{
    R_ASSERT2(pSettings->section_exist(section_name), section_name);
    CUITextWnd* pItem = NULL;

    string64 phrase;
    string256 str;
    for (int i = 0; true; ++i)
    {
        xr_sprintf(phrase, "phrase_%i", i);
        if (pSettings->line_exist(section_name, phrase))
        {
            LPCSTR s = pSettings->r_string(section_name, phrase);
            _GetItem(s, 0, phrase);
            xr_sprintf(str, "%d. %s", i + 1, StringTable().translate(phrase).c_str());

            ADD_TEXT_TO_VIEW3(str, pItem, m_pList);
            pItem->SetFont(m_pFont);
            pItem->SetTextColor(m_text_color);
        }
        else
            break;
    }
}

bool CUISpeechMenu::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (dik < SDL_SCANCODE_1 || dik > SDL_SCANCODE_0)
        return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);

    game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());

    HideDialog();
    game->OnMessageSelected(this, static_cast<u8>(dik - SDL_SCANCODE_1));

    return true;
}
