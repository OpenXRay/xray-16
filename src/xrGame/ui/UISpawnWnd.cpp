#include "StdAfx.h"
#include "UISpawnWnd.h"
#include "UIXmlInit.h"
#include "Level.h"
#include "game_cl_teamdeathmatch.h"
#include "UIStatix.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrEngine/xr_level_controller.h"
#include "xrUICore/Cursor/UICursor.h"
#include "UIGameCustom.h"
#include "UIHelper.h"

CUISpawnWnd::CUISpawnWnd()
    : CUIDialogWnd(CUISpawnWnd::GetDebugType()),
      m_pImage1{ xr_new<CUIStatix>() },
      m_pImage2{ xr_new<CUIStatix>() }
{
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "spawn.xml");

    CUIXmlInit::InitWindow(xml_doc, "team_selector", 0, this);

    std::ignore = UIHelper::CreateStatic(xml_doc, "team_selector:background", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "team_selector:caption", this);

    AttachChild(m_pImage1);
    AttachChild(m_pImage2);
    m_pImage1->SetAutoDelete(true);
    m_pImage2->SetAutoDelete(true);

    CUIXmlInit::InitStatic(xml_doc, "team_selector:image_0", 0, m_pImage1);
    // m_pImage1->SetStretchTexture(true);
    CUIXmlInit::InitStatic(xml_doc, "team_selector:image_1", 0, m_pImage2);
    // m_pImage2->SetStretchTexture(true);
    // InitTeamLogo();

    std::ignore = UIHelper::CreateStatic(xml_doc, "team_selector:image_frames_tl", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "team_selector:image_frames_tr", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "team_selector:image_frames_bottom", this, false);
    std::ignore = UIHelper::CreateScrollView(xml_doc, "team_selector:text_desc", this);

    m_pBtnAutoSelect = UIHelper::Create3tButton(xml_doc, "team_selector:btn_autoselect", this);
    m_pBtnSpectator  = UIHelper::Create3tButton(xml_doc, "team_selector:btn_spectator", this);
    m_pBtnBack       = UIHelper::Create3tButton(xml_doc, "team_selector:btn_back", this);
}

void CUISpawnWnd::InitTeamLogo()
{
    R_ASSERT(pSettings->section_exist("team_logo"));
    R_ASSERT(pSettings->line_exist("team_logo", "team1"));
    R_ASSERT(pSettings->line_exist("team_logo", "team2"));

    m_pImage1->InitTexture(pSettings->r_string("team_logo", "team1"));
    m_pImage2->InitTexture(pSettings->r_string("team_logo", "team2"));
}

void CUISpawnWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (BUTTON_CLICKED == msg)
    {
        HideDialog();
        game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
        VERIFY(game);
        // game_cl_TeamDeathmatch * tdm = smart_cast<game_cl_TeamDeathmatch *>(&(Game()));
        if (pWnd == m_pImage1)
            game->OnTeamSelect(0);
        else if (pWnd == m_pImage2)
            game->OnTeamSelect(1);
        else if (pWnd == m_pBtnAutoSelect)
            game->OnTeamSelect(-1);
        else if (pWnd == m_pBtnSpectator)
            game->OnSpectatorSelect();
        else if (pWnd == m_pBtnBack)
            game->OnTeamMenuBack();
    }

    inherited::SendMessage(pWnd, msg, pData);
}

////////////////////////////////////////////////////////////////////////////////

bool CUISpawnWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    auto action = GetBindedAction(dik);

    if (WINDOW_KEY_PRESSED != keyboard_action)
    {
        if (action == kSCORES)
        {
            ShowChildren(true);
            game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
            game->OnKeyboardRelease(kSCORES);
            UI().GetUICursor().Show();
        }
        return false;
    }

    if (action == kSCORES)
    {
        ShowChildren(false);
        game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
        game->OnKeyboardPress(kSCORES);
        UI().GetUICursor().Hide();
        return false;
    }

    game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
    VERIFY(game);
    // game_cl_TeamDeathmatch * dm = smart_cast<game_cl_TeamDeathmatch *>(&(Game()));

    if (SDL_SCANCODE_1 == dik || SDL_SCANCODE_2 == dik)
    {
        HideDialog();

        if (SDL_SCANCODE_1 == dik)
            game->OnTeamSelect(0);
        else
            game->OnTeamSelect(1);
        return true;
    }
    switch (action)
    {
    case kQUIT:
        HideDialog();
        game->OnTeamMenuBack();
        return true;

    case kJUMP:
        HideDialog();
        game->OnTeamSelect(-1);
        return true;

    case kENTER:
        HideDialog();
        if (m_pImage1->GetSelectedState())
            game->OnTeamSelect(0);
        else if (m_pImage2->GetSelectedState())
            game->OnTeamSelect(1);
        else
            game->OnTeamSelect(-1);
        return true;
    } // switch (action)

    return inherited::OnKeyboardAction(dik, keyboard_action);
}

void CUISpawnWnd::SetVisibleForBtn(ETEAMMENU_BTN btn, bool state)
{
    switch (btn)
    {
    case TEAM_MENU_BACK: this->m_pBtnBack->SetVisible(state); break;
    case TEAM_MENU_SPECTATOR: this->m_pBtnSpectator->SetVisible(state); break;
    case TEAM_MENU_AUTOSELECT: this->m_pBtnAutoSelect->SetVisible(state); break;
    default: R_ASSERT2(false, "invalid btn ID");
    }
}

void CUISpawnWnd::SetCurTeam(int team)
{
    R_ASSERT2(team >= -1 && team <= 1, "Invalid team number");

    m_iCurTeam = team;
    m_pImage1->SetSelectedState(0 == team ? true : false);
    m_pImage2->SetSelectedState(1 == team ? true : false);
}
