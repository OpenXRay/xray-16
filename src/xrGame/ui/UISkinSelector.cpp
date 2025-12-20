#include "StdAfx.h"
#include "UISkinSelector.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIStatix.h"
#include "xrUICore/Cursor/UICursor.h"
#include "UIGameCustom.h"
#include "game_cl_deathmatch.h"
#include "xrEngine/xr_level_controller.h"
#include "Level.h"
#include "UIHelper.h"
#include "Common/object_broker.h"

CUISkinSelectorWnd::CUISkinSelectorWnd(const char* strSectionName, s16 team)
    : CUIDialogWnd(CUISkinSelectorWnd::GetDebugType())
{
    m_team = team;
    m_iActiveIndex = -1;

    m_firstSkin = 0;
    Init(strSectionName);
}

CUISkinSelectorWnd::~CUISkinSelectorWnd()
{
    delete_data(m_skinsEnabled);
}

void CUISkinSelectorWnd::InitSkins()
{
    R_ASSERT2(pSettings->section_exist(m_strSection), *m_strSection);
    R_ASSERT2(pSettings->line_exist(m_strSection, "skins"), *m_strSection);

    LPCSTR lst = pSettings->r_string(m_strSection, "skins");
    string256 singleItem;
    u32 count = _GetItemCount(lst);
    R_ASSERT2(count, "no skins in this game");
    for (u32 j = 0; j < count; ++j)
    {
        _GetItem(lst, j, singleItem);
        m_skins.push_back(singleItem);
        m_skinsEnabled.push_back(j);
    }
}

void CUISkinSelectorWnd::UpdateSkins()
{
    for (int i = 0; i < (int)m_pImage.size(); i++)
    {
        if (!!m_shader)
            m_pImage[i]->InitTextureEx(m_skins[i + m_firstSkin].c_str(), m_shader.c_str());
        else
            m_pImage[i]->InitTexture(m_skins[i + m_firstSkin].c_str());

        if (m_iActiveIndex - m_firstSkin == i)
            m_pImage[i]->SetSelectedState(true);
        else
            m_pImage[i]->SetSelectedState(false);

        string16 buf;
        if (m_firstSkin + i < 10)
        {
            xr_itoa((m_firstSkin + 1 + i) % 10, buf, 10);
            xr_strcat(buf, sizeof(buf), " ");
            m_pImage[i]->TextItemControl()->SetText(buf);
        }
        else
            m_pImage[i]->TextItemControl()->SetText("");

        xr_vector<int>::iterator it = std::find(m_skinsEnabled.begin(), m_skinsEnabled.end(), i + m_firstSkin);
        m_pImage[i]->Enable(it != m_skinsEnabled.end());
    }

    if (m_pButtons[0])
        m_pButtons[0]->Enable(m_firstSkin > 0);
    if (m_pButtons[1])
        m_pButtons[1]->Enable(m_firstSkin + (int)m_pImage.size() < (int)m_skins.size());
}

void CUISkinSelectorWnd::Init(const char* strSectionName)
{
    R_ASSERT(0 != strSectionName[0]);
    m_strSection = strSectionName;

    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "skin_selector.xml");

    CUIXmlInit::InitWindow(xml_doc, "skin_selector", 0, this);

    std::ignore = UIHelper::CreateStatic(xml_doc, "skin_selector:background", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "skin_selector:caption", this);

    const auto frames = UIHelper::CreateStatic(xml_doc, "skin_selector:image_frames", this);

    if (xml_doc.NavigateToNode("skin_selector:image_frames:a_static_1"))
    {
        m_pAnims[0] = xr_new<CUIAnimatedStatic>();
        m_pAnims[0]->SetAutoDelete(true);
        frames->AttachChild(m_pAnims[0]);
        CUIXmlInit::InitAnimatedStatic(xml_doc, "skin_selector:image_frames:a_static_1", 0, m_pAnims[0]);
    }

    if (xml_doc.NavigateToNode("skin_selector:image_frames:a_static_2"))
    {
        m_pAnims[1] = xr_new<CUIAnimatedStatic>();
        m_pAnims[1]->SetAutoDelete(true);
        frames->AttachChild(m_pAnims[1]);
        CUIXmlInit::InitAnimatedStatic(xml_doc, "skin_selector:image_frames:a_static_2", 0, m_pAnims[1]);
    }

    m_pButtons[0] = UIHelper::Create3tButton(xml_doc,"skin_selector:image_frames:btn_left", frames, false);
    m_pButtons[1] = UIHelper::Create3tButton(xml_doc,"skin_selector:image_frames:btn_right", frames, false);

    if (m_pButtons[0])
        m_pButtons[0]->SetMessageTarget(this);
    if (m_pButtons[1])
        m_pButtons[1]->SetMessageTarget(this);

    int i = 0;
    while (true)
    {
        string64 buff;
        xr_sprintf(buff, "skin_selector:image_%d", i);

        if (!xml_doc.NavigateToNode(buff) && i != 0) // must have at least one image
            break;

        const auto image = xr_new<CUIStatix>();
        image->SetAutoDelete(true);
        AttachChild(image);

        CUIXmlInit::InitStatic(xml_doc, buff, 0, image);
        m_pImage.emplace_back(image);
        ++i;
    }

    m_pBtnAutoSelect = UIHelper::Create3tButton(xml_doc, "skin_selector:btn_autoselect", this);
    m_pBtnSpectator  = UIHelper::Create3tButton(xml_doc, "skin_selector:btn_spectator", this);
    m_pBtnBack       = UIHelper::Create3tButton(xml_doc, "skin_selector:btn_back", this);

    if (xml_doc.NavigateToNode("skin_selector:skin_shader", 0))
        m_shader = xml_doc.Read("skin_selector:skin_shader", 0, "");

    InitSkins();
    UpdateSkins();
}

void CUISkinSelectorWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    game_cl_mp* game = NULL;
    // game_cl_Deathmatch * dm = NULL;
    switch (msg)
    {
    case BUTTON_CLICKED:
        game = smart_cast<game_cl_mp*>(&(Game()));
        // dm = smart_cast<game_cl_Deathmatch *>(&(Game()));

        if (pWnd == m_pButtons[0])
            OnKeyLeft();
        else if (pWnd == m_pButtons[1])
            OnKeyRight();
        else if (pWnd == m_pBtnAutoSelect)
        {
            m_iActiveIndex = -1;
            OnBtnOK();
        }
        else if (pWnd == m_pBtnSpectator)
        {
            HideDialog();
            game->OnSpectatorSelect();
        }
        else if (pWnd == m_pBtnBack)
        {
            HideDialog();
            game->OnSkinMenuBack();
        }
        else
        {
            for (int i = 0; i < (int)m_pImage.size(); i++)
            {
                if (pWnd == m_pImage[i])
                {
                    m_iActiveIndex = m_firstSkin + i;
                    OnBtnOK();
                }
            }
        }
        break;
    case WINDOW_FOCUS_RECEIVED:
        if (m_pButtons[0] && pWnd == m_pButtons[0])
        {
            if (m_pAnims[0])
            {
                m_pAnims[0]->Rewind(0);
                m_pAnims[0]->Play();
            }
        }
        else if (m_pButtons[1] && pWnd == m_pButtons[1])
        {
            if (m_pAnims[1])
            {
                m_pAnims[1]->Rewind(0);
                m_pAnims[1]->Play();
            }
        }
        break;
    } // switch (msg)
}

void CUISkinSelectorWnd::OnBtnCancel()
{
    HideDialog();
    game_cl_mp* mp = smart_cast<game_cl_mp*>(&(Game()));
    mp->OnSkinMenu_Cancel();
}

void CUISkinSelectorWnd::OnBtnOK()
{
    HideDialog();
    game_cl_mp* game = smart_cast<game_cl_mp*>(&(Game()));
    VERIFY(game);
    // game_cl_Deathmatch * dm = smart_cast<game_cl_Deathmatch *>(&(Game()));

    if (m_iActiveIndex == -1)
    {
        m_iActiveIndex = m_skinsEnabled[::Random.randI(m_skinsEnabled.size())];
    }
    game->OnSkinMenu_Ok();
}

bool CUISkinSelectorWnd::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    return CUIWindow::OnMouseAction(x, y, mouse_action);
}

bool CUISkinSelectorWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
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

    int right_border = (int)m_skins.size();
    if (right_border > 9)
        right_border = 9;

    if (dik >= SDL_SCANCODE_1 && dik < (int)right_border + SDL_SCANCODE_1)
    {
        int NewIndex = dik - SDL_SCANCODE_1;
        //		Msg("Selected %d", NewIndex);
        //		for (u32 i=0; i<m_skinsEnabled.size(); i++)
        //			Msg("Enabled - %d", m_skinsEnabled[i]);
        xr_vector<int>::iterator It = std::find(m_skinsEnabled.begin(), m_skinsEnabled.end(), NewIndex);
        if (It != m_skinsEnabled.end())
        {
            m_iActiveIndex = NewIndex;
            OnBtnOK();
        }
        return true;
    }

    //	game_cl_Deathmatch * dm = smart_cast<game_cl_Deathmatch *>(&(Game()));

    switch (action)
    {
    case kQUIT:
        OnBtnCancel();
        return true;

    case kJUMP: // do autoselect
        m_iActiveIndex = -1;
        [[fallthrough]];

    case kENTER:
        OnBtnOK();
        return true;

    case kLEFT:
        OnKeyLeft();
        return true;

    case kRIGHT:
        OnKeyRight();
        return true;
    }

    return false;
}

void CUISkinSelectorWnd::OnKeyLeft()
{
    if (m_firstSkin > 0)
    {
        m_firstSkin--;
        UpdateSkins();
    }
}

void CUISkinSelectorWnd::OnKeyRight()
{
    if (m_firstSkin + (int)m_pImage.size() < (int)m_skins.size())
    {
        m_firstSkin++;
        UpdateSkins();
    }
}

int CUISkinSelectorWnd::GetActiveIndex()
{
    if (-1 == m_iActiveIndex)
        return -1;
    else
        return m_iActiveIndex;
}

void CUISkinSelectorWnd::SetVisibleForBtn(ESKINMENU_BTN btn, bool state)
{
    switch (btn)
    {
    case SKIN_MENU_BACK: this->m_pBtnBack->SetVisible(state); break;
    case SKIN_MENU_SPECTATOR: this->m_pBtnSpectator->SetVisible(state); break;
    case SKIN_MENU_AUTOSELECT: this->m_pBtnAutoSelect->SetVisible(state); break;
    default: R_ASSERT2(false, "invalid btn ID");
    }
}

void CUISkinSelectorWnd::SetCurSkin(int skin)
{
    R_ASSERT2(skin >= -1 && skin <= (int)m_skins.size(), "invalid skin index");

    m_iActiveIndex = skin;

    if (m_iActiveIndex != -1 && (m_iActiveIndex < m_firstSkin || m_iActiveIndex > m_firstSkin + (m_pImage.size() - 1)))
    {
        if (m_iActiveIndex > (int)m_skins.size() - (int)m_pImage.size())
            m_firstSkin = (int)m_skins.size() - (int)m_pImage.size();
        else
            m_firstSkin = m_iActiveIndex;
    }
    UpdateSkins();
}

void CUISkinSelectorWnd::Update()
{
    UpdateSkins();
    inherited::Update();
}
