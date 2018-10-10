#include "StdAfx.h"
#include "UIServerInfo.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Cursor/UICursor.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIGameCustom.h"
#include "Level.h"
#include "game_cl_mp.h"
#include "ximage.h"
#include "xmemfile.h"

CUIServerInfo::CUIServerInfo()
{
    m_dds_file_created = false;

    m_background = new CUIStatic();
    AttachChild(m_background);
    m_background->SetAutoDelete(true);

    m_caption = new CUIStatic();
    AttachChild(m_caption);
    m_caption->SetAutoDelete(true);

    m_image = new CUIStatic();
    AttachChild(m_image);
    m_image->SetAutoDelete(true);

    m_text_desc = new CUIScrollView();
    AttachChild(m_text_desc);
    m_text_desc->SetAutoDelete(true);

    m_text_body = new CUITextWnd();
    // m_text_desc->AttachChild		(m_text_body);
    // m_text_body->SetAutoDelete		(true);

    m_btn_spectator = new CUI3tButton();
    AttachChild(m_btn_spectator);
    m_btn_spectator->SetAutoDelete(true);

    m_btn_next = new CUI3tButton();
    AttachChild(m_btn_next);
    m_btn_next->SetAutoDelete(true);

    Init();
}

CUIServerInfo::~CUIServerInfo() {}
void CUIServerInfo::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
void CUIServerInfo::Init()
{
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "server_info.xml");

    CUIXmlInit::InitWindow(xml_doc, "server_info", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "server_info:caption", 0, m_caption);
    CUIXmlInit::InitStatic(xml_doc, "server_info:background", 0, m_background);
    CUIXmlInit::InitScrollView(xml_doc, "server_info:text_desc", 0, m_text_desc);
    CUIXmlInit::InitStatic(xml_doc, "server_info:image", 0, m_image);

    CUIXmlInit::InitTextWnd(xml_doc, "server_info:text_body", 0, m_text_body);
    m_text_body->SetTextComplexMode(true);
    m_text_body->SetWidth(m_text_desc->GetDesiredChildWidth());
    m_text_desc->AddWindow(m_text_body, true);

    Frect orig_rect = m_image->GetTextureRect();
    m_image->InitTexture("ui" DELIMITER "ui_noise");
    m_image->SetTextureRect(orig_rect);
    m_image->SetStretchTexture(true);

    CUIXmlInit::Init3tButton(xml_doc, "server_info:btn_next", 0, m_btn_next);
    CUIXmlInit::Init3tButton(xml_doc, "server_info:btn_spectator", 0, m_btn_spectator);

    InitCallbacks();
}

void CUIServerInfo::InitCallbacks()
{
    Register(m_btn_next);
    Register(m_btn_spectator);

    AddCallback(m_btn_next, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIServerInfo::OnNextBtnClick));
    AddCallback(
        m_btn_spectator, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIServerInfo::OnSpectatorBtnClick));
    // AddCallback	(this,				WINDOW_KEY_PRESSED,	CUIWndCallback::void_function(this,
    // &CUIServerInfo::OnNextBtnClick));
}

char const* CUIServerInfo::tmp_logo_file_name = "tmp_sv_logo.dds";
void CUIServerInfo::SetServerLogo(u8 const* data_ptr, u32 const data_size)
{
    CxMemFile tmp_memfile(const_cast<BYTE*>(data_ptr), data_size);
    CxImage tmp_image;
    if (!tmp_image.Decode(&tmp_memfile, CXIMAGE_FORMAT_JPG))
    {
        Msg("! ERROR: Failed to decode server logo image as JPEG formated.");
        return;
    }

    IWriter* tmp_writer = FS.w_open("$game_saves$", tmp_logo_file_name);
    if (!tmp_writer)
    {
        Msg("! ERROR: failed to create temporary dds file");
        return;
    }
    tmp_writer->w((void*)data_ptr, data_size); // sorry :(
    FS.w_close(tmp_writer);
    m_dds_file_created = true;
    m_image->InitTexture(tmp_logo_file_name);
    FS.file_delete("$game_saves$", tmp_logo_file_name);
}

void CUIServerInfo::SetServerRules(u8 const* data_ptr, u32 const data_size)
{
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
}

void xr_stdcall CUIServerInfo::OnSpectatorBtnClick(CUIWindow* w, void* d)
{
    game_cl_mp* mp_game = smart_cast<game_cl_mp*>(&Game());
    VERIFY(mp_game);

    HideDialog();
    mp_game->OnSpectatorSelect();
}

void xr_stdcall CUIServerInfo::OnNextBtnClick(CUIWindow* w, void* d)
{
    game_cl_mp* mp_game = smart_cast<game_cl_mp*>(&Game());
    VERIFY(mp_game);

    HideDialog();
    mp_game->OnMapInfoAccept();
}

bool CUIServerInfo::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    switch (dik)
    {
    case SDL_SCANCODE_SPACE:
    case SDL_SCANCODE_RETURN:
    {
        OnNextBtnClick(NULL, 0);
        return true;
    }
    break;
    }; // switch (dik)
    return false;
}
