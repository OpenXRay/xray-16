#include "StdAfx.h"
#include "UIMapInfo.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "string_table.h"

CUIMapInfo::CUIMapInfo()
{
    m_view = new CUIScrollView();
    AttachChild(m_view);
}

CUIMapInfo::~CUIMapInfo() { xr_delete(m_view); }
void CUIMapInfo::InitMapInfo(Fvector2 pos, Fvector2 size)
{
    SetWndPos(pos);
    SetWndSize(size);
    //.	m_view->SetWndPos(pos);
    m_view->SetWndSize(size);
    m_view->InitScrollView();
    m_view->SetFixedScrollBar(false);
}

#define ADD_TEXT(x, y, z)                                           \
    text = *StringTable().translate(x);                                   \
    text += ": ";                                                   \
    text += txt_color_tag;                                          \
    if (ltx.line_exist("map_info", y))                              \
        text += *StringTable().translate(ltx.r_string_wb("map_info", y)); \
    else                                                            \
        text += *StringTable().translate(z);                              \
    text += "%c[default]\\n";                                       \
    st = new CUITextWnd();                                          \
    st->SetTextComplexMode(true);                                   \
    st->SetFont(txt_font);                                          \
    st->SetTextColor(header_color);                                 \
    st->SetText(text.c_str());                                      \
    st->SetWidth(m_view->GetDesiredChildWidth());                   \
    st->AdjustHeightToText();                                       \
    m_view->AddWindow(st, true)

void CUIMapInfo::InitMap(LPCSTR map_name, LPCSTR map_ver)
{
    m_view->Clear();
    if (NULL == map_name)
        return;

    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_mapinfo.xml");

    CUITextWnd* st;
    // try to find file with info
    xr_string info_path = "text" DELIMITER "map_desc" DELIMITER;
    info_path += map_name;
    info_path += ".ltx";

    if (FS.exist("$game_config$", info_path.c_str()))
    {
        string_path ltxPath;
        FS.update_path(ltxPath, CONFIG_PATH, info_path.c_str());
        CInifile ltx(ltxPath);
        xr_string text;

        // map name
        st = new CUITextWnd();
        CUIXmlInit::InitTextWnd(xml_doc, "map_name", 0, st);

        xr_string S = StringTable().translate(map_name).c_str();
        if (map_ver)
        {
            S += "[";
            S += map_ver;
            S += "]";
        }
        st->SetText(S.c_str());
        st->SetWidth(m_view->GetDesiredChildWidth());
        st->AdjustHeightToText();
        m_view->AddWindow(st, true);

        u32 header_color;
        u32 txt_color;
        CGameFont* txt_font;
        CUIXmlInit::InitFont(xml_doc, "header", 0, header_color, txt_font);
        txt_color = CUIXmlInit::GetColor(xml_doc, "txt:text", 0, 0x00);
        string64 txt_color_tag;
        xr_sprintf(txt_color_tag, "%s[%u,%u,%u,%u]", "%c", color_get_A(txt_color), color_get_R(txt_color),
            color_get_G(txt_color), color_get_B(txt_color));

        ADD_TEXT("mp_players", "players", "Unknown");

        shared_str _modes = ltx.r_string_wb("map_info", "modes");

        text = *StringTable().translate("modes");
        text += ": ";
        text += txt_color_tag;
        bool b_ = false;
        if (strstr(_modes.c_str(), "st_deathmatch"))
        {
            text += *StringTable().translate("st_deathmatch");
            b_ = true;
        }
        if (strstr(_modes.c_str(), "st_team_deathmatch"))
        {
            if (b_)
                text += ", ";
            text += *StringTable().translate("st_team_deathmatch");
            b_ = true;
        }
        if (strstr(_modes.c_str(), "st_artefacthunt"))
        {
            if (b_)
                text += ", ";
            text += *StringTable().translate("st_artefacthunt");
        }

        text += "%c[default]\\n";

        st = new CUITextWnd();
        st->SetTextComplexMode(true);
        st->SetFont(txt_font);
        st->SetTextColor(header_color);
        st->SetText(text.c_str());
        st->SetWidth(m_view->GetDesiredChildWidth());
        st->AdjustHeightToText();
        m_view->AddWindow(st, true);

        ADD_TEXT("mp_description", "short_desc", "");

        if (ltx.line_exist("map_info", "large_desc"))
            m_large_desc = StringTable().translate(ltx.r_string_wb("map_info", "large_desc"));
    }
    else
    {
        st = new CUITextWnd();
        CUIXmlInit::InitTextWnd(xml_doc, "map_name", 0, st);
        st->SetTextST(map_name);
        st->SetWidth(m_view->GetDesiredChildWidth());
        st->AdjustHeightToText();
        m_view->AddWindow(st, true);
    }
}

const char* CUIMapInfo::GetLargeDesc() { return *m_large_desc; }
