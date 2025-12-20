#include "StdAfx.h"

#include "UIServerInfo.h"

#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Cursor/UICursor.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Static/UIStatic.h"

#include "game_cl_mp.h"
#include "Level.h"
#include "UIGameCustom.h"
#include "UIHelper.h"
#include "UIMapInfo.h"
#include "UIXmlInit.h"

#include "xrCore/Media/Image.hpp"

CUIServerInfo::CUIServerInfo() : CUIDialogWnd(CUIServerInfo::GetDebugType())
{
    CUIXml xml_doc;
    if (xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "server_info.xml", false))
    {
        CUIXmlInit::InitWindow(xml_doc, "server_info", 0, this);
        xml_doc.SetLocalRoot(xml_doc.NavigateToNode("server_info", 0));
    }
    else
    {
        xml_doc.ClearInternal();
        xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "map_desc.xml");

        CUIXmlInit::InitWindow(xml_doc, "map_desc", 0, this);
        xml_doc.SetLocalRoot(xml_doc.NavigateToNode("map_desc", 0));
    }

    std::ignore = UIHelper::CreateStatic(xml_doc, "background", 0, this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "caption", 0, this);

    m_image = UIHelper::CreateStatic(xml_doc, "image", this);
    {
        xr_string map_name = "intro\\intro_map_pic_";

        map_name += Level().name().c_str();
        xr_string full_name = map_name + ".dds";

        Frect orig_rect = m_image->GetTextureRect();
        if (pGameIni->section_exist(Level().name()) && pGameIni->line_exist(Level().name(), "texture"))
        {
            m_image->InitTexture(pGameIni->r_string(Level().name(), "texture"));
        }
        else if (FS.exist("$game_textures$", full_name.c_str()))
            m_image->InitTexture(map_name.c_str());
        else
            m_image->InitTexture("ui\\ui_noise");
        m_image->SetTextureRect(orig_rect);
        m_image->SetStretchTexture(true);
    }

    const auto text_desc = UIHelper::CreateScrollView(xml_doc, "text_desc", this);

    m_text_body = UIHelper::CreateStatic(xml_doc, "text_body", nullptr, false);
    if (m_text_body)
    {
        m_text_body->SetTextComplexMode(true);
        m_text_body->SetWidth(text_desc->GetDesiredChildWidth());
        text_desc->AddWindow(m_text_body, true);
    }
    else
    {
        auto map_info = xr_new<CUIMapInfo>();
        map_info->SetAutoDelete(true);
        AttachChild(map_info);

        CUIXmlInit::InitWindow(xml_doc, "map_info", 0, map_info);
        map_info->InitMapInfo(map_info->GetWndPos(), map_info->GetWndSize());
        map_info->InitMap(Level().name().c_str(), nullptr);

        pcstr description = map_info->GetLargeDesc();
        if (description && description[0])
        {
            ADD_TEXT_TO_VIEW2(map_info->GetLargeDesc(), text_desc);
            m_has_info = true;
        }
    }

    std::ignore = UIHelper::CreateStatic(xml_doc, "image_frames_1", this, false);
    std::ignore = UIHelper::CreateStatic(xml_doc, "image_frames_2", this, false);
    std::ignore = UIHelper::CreateStatic(xml_doc, "image_frames_3", this, false);

    const auto btn_next      = UIHelper::Create3tButton(xml_doc, "btn_next", this);
    const auto btn_spectator = UIHelper::Create3tButton(xml_doc, "btn_spectator", this);

    Register(btn_next);
    Register(btn_spectator);

    AddCallback(btn_next, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIServerInfo::OnNextBtnClick));
    AddCallback(btn_spectator, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIServerInfo::OnSpectatorBtnClick));
    // AddCallback(this, WINDOW_KEY_PRESSED, CUIWndCallback::void_function(this, &CUIServerInfo::OnNextBtnClick));
}

char const* CUIServerInfo::tmp_logo_file_name = "tmp_sv_logo.dds";
void CUIServerInfo::SetServerLogo(u8 const* data_ptr, u32 const data_size)
{
    XRay::Media::Image img;
    if (!img.OpenJPEG(data_ptr, data_size))
    {
        Msg("! ERROR: Failed to decode server logo image as JPEG.");
        return;
    }

    IWriter* tmp_writer = FS.w_open("$game_saves$", tmp_logo_file_name);
    if (!tmp_writer)
    {
        Msg("! ERROR: failed to create temporary dds file");
        return;
    }
    // XXX: real convert jpg to dds
    tmp_writer->w((void*)data_ptr, data_size); // sorry :(
    FS.w_close(tmp_writer);
    m_image->InitTexture(tmp_logo_file_name);
    FS.file_delete("$game_saves$", tmp_logo_file_name);
    m_has_info = true;
}

void CUIServerInfo::SetServerRules(u8 const* data_ptr, u32 const data_size)
{
    if (!m_text_body)
        return;

    string4096 tmp_string;
    u32 new_size = data_size;
    if (new_size > (sizeof(tmp_string) - 1))
        new_size = (sizeof(tmp_string) - 1);

    strncpy_s(tmp_string, sizeof(tmp_string), reinterpret_cast<char const*>(data_ptr), new_size);
    tmp_string[new_size] = 0;

    // std::replace(tmp_string, tmp_string + new_size, '\r', _DELIMITER);
    // std::replace(tmp_string, tmp_string + new_size, '\n', 'n');
    char* tmp_iter = strstr(tmp_string, "\r\n");
    while (tmp_iter != NULL)
    {
        *tmp_iter = _DELIMITER;
        *(tmp_iter + 1) = 'n';
        tmp_iter += 2;
        tmp_iter = strstr(tmp_iter, "\r\n");
    }

    m_text_body->SetText(tmp_string); // will create shared_str
    m_text_body->AdjustHeightToText();
    m_has_info = true;
}

void CUIServerInfo::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUIServerInfo::OnSpectatorBtnClick(CUIWindow* w, void* d)
{
    game_cl_mp* mp_game = smart_cast<game_cl_mp*>(&Game());
    VERIFY(mp_game);

    HideDialog();
    mp_game->OnSpectatorSelect();
}

void CUIServerInfo::OnNextBtnClick(CUIWindow* w, void* d)
{
    game_cl_mp* mp_game = smart_cast<game_cl_mp*>(&Game());
    VERIFY(mp_game);

    HideDialog();
    mp_game->OnMapInfoAccept();
}

bool CUIServerInfo::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    switch (GetBindedAction(dik))
    {
    case kJUMP:
    case kENTER:
    {
        OnNextBtnClick(NULL, 0);
        return true;
    }
    } // switch (GetBindedAction(dik))

    return false;
}
