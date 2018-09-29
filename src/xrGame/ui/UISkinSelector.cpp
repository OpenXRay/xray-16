#include "StdAfx.h"
#include "UISkinSelector.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIStatix.h"
#include "xrUICore/Cursor/UICursor.h"
#include "UIGameCustom.h"
#include "game_cl_deathmatch.h"
#include "xr_level_controller.h"
#include "Level.h"
#include "Common/object_broker.h"

CUISkinSelectorWnd::CUISkinSelectorWnd(const char* strSectionName, s16 team)
{
    m_team = team;
    m_iActiveIndex = -1;
    m_pBackground = new CUIStatic();
    AttachChild(m_pBackground);
    m_pCaption = new CUIStatic();
    AttachChild(m_pCaption);

    m_pFrames = new CUIStatic();
    AttachChild(m_pFrames);

    for (int i = 0; i < 6; i++)
    {
        m_pImage[i] = new CUIStatix();
        AttachChild(m_pImage[i]);
    }
    //	m_pAnims[0]		= new CUIAnimatedStatic(); m_pFrames->AttachChild(m_pAnims[0]);
    //	m_pAnims[1]		= new CUIAnimatedStatic(); m_pFrames->AttachChild(m_pAnims[1]);
    //	m_pButtons[0]	= new CUI3tButton();	m_pFrames->AttachChild(m_pButtons[0]);
    // m_pButtons[0]->SetMessageTarget(this);
    //	m_pButtons[1]	= new CUI3tButton();	m_pFrames->AttachChild(m_pButtons[1]);
    // m_pButtons[1]->SetMessageTarget(this);

    m_pBtnAutoSelect = new CUI3tButton();
    AttachChild(m_pBtnAutoSelect);
    m_pBtnSpectator = new CUI3tButton();
    AttachChild(m_pBtnSpectator);
    m_pBtnBack = new CUI3tButton();
    AttachChild(m_pBtnBack);

    m_firstSkin = 0;
    Init(strSectionName);
}

CUISkinSelectorWnd::~CUISkinSelectorWnd()
{
    xr_delete(m_pCaption);
    xr_delete(m_pBackground);
    xr_delete(m_pFrames);
    //	xr_delete(m_pButtons[0]);
    //	xr_delete(m_pButtons[1]);
    //	xr_delete(m_pAnims[0]);
    //	xr_delete(m_pAnims[1]);
    xr_delete(m_pBtnAutoSelect);
    xr_delete(m_pBtnSpectator);
    xr_delete(m_pBtnBack);
    for (int i = 0; i < p_image_count; i++)
        xr_delete(m_pImage[i]);

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
    for (int i = 0; i < 6; i++)
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

    //	m_pButtons[0]->Enable(m_firstSkin > 0);
    //	m_pButtons[1]->Enable(m_firstSkin + 4 < (int)m_skins.size());
}

void CUISkinSelectorWnd::Init(const char* strSectionName)
{
    R_ASSERT(0 != strSectionName[0]);
    m_strSection = strSectionName;

    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "skin_selector.xml");

    CUIXmlInit::InitWindow(xml_doc, "skin_selector", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "skin_selector:caption", 0, m_pCaption);
    CUIXmlInit::InitStatic(xml_doc, "skin_selector:background", 0, m_pBackground);
    CUIXmlInit::InitStatic(xml_doc, "skin_selector:image_frames", 0, m_pFrames);

    //	CUIXmlInit::Init3tButton(xml_doc,"skin_selector:image_frames:btn_left",	0,	m_pButtons[0]);
    //	CUIXmlInit::Init3tButton(xml_doc,"skin_selector:image_frames:btn_right",0,	m_pButtons[1]);

    //	CUIXmlInit::InitAnimatedStatic(xml_doc,"skin_selector:image_frames:a_static_1",	0,	m_pAnims[0]);
    //	CUIXmlInit::InitAnimatedStatic(xml_doc,"skin_selector:image_frames:a_static_2",	0,	m_pAnims[1]);

    CUIXmlInit::Init3tButton(xml_doc, "skin_selector:btn_spectator", 0, m_pBtnSpectator);
    CUIXmlInit::Init3tButton(xml_doc, "skin_selector:btn_autoselect", 0, m_pBtnAutoSelect);
    CUIXmlInit::Init3tButton(xml_doc, "skin_selector:btn_back", 0, m_pBtnBack);

    if (xml_doc.NavigateToNode("skin_selector:skin_shader", 0))
        m_shader = xml_doc.Read("skin_selector:skin_shader", 0, "");

    InitSkins();
    string64 buff;
    for (int i = 0; i < 6; i++)
    {
        xr_sprintf(buff, "skin_selector:image_%d", i);
        CUIXmlInit::InitStatic(xml_doc, buff, 0, m_pImage[i]);
    }
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
        /*
			if (pWnd == m_pButtons[0])
				OnKeyLeft();
			else if (pWnd == m_pButtons[1])
				OnKeyRight();
			else */ if (pWnd == m_pBtnAutoSelect)
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
            for (int i = 0; i < 6; i++)
                if (pWnd == m_pImage[i])
                {
                    m_iActiveIndex = m_firstSkin + i;
                    OnBtnOK();
                }
        break;
    case WINDOW_FOCUS_RECEIVED:
        /*
                    if (pWnd == m_pButtons[0])
                    {
                        m_pAnims[0]->Rewind(0);
                        m_pAnims[0]->Play();
                    }
                    else if (pWnd == m_pButtons[1])
                    {
                        m_pAnims[1]->Rewind(0);
                        m_pAnims[1]->Play();
                    }
        */
        break;
    }
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
    if (WINDOW_KEY_PRESSED != keyboard_action)
    {
        if (dik == SDL_SCANCODE_TAB)
        {
            ShowChildren(true);
            game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
            game->OnKeyboardRelease(kSCORES);
            UI().GetUICursor().Show();
        }

        return false;
    }

    if (dik == SDL_SCANCODE_TAB)
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

    switch (dik)
    {
    case SDL_SCANCODE_ESCAPE: OnBtnCancel(); return true;
    case SDL_SCANCODE_SPACE: // do autoselect
        m_iActiveIndex = -1;
    case SDL_SCANCODE_RETURN: OnBtnOK(); return true;
    case SDL_SCANCODE_LEFT: OnKeyLeft(); return true;
    case SDL_SCANCODE_RIGHT: OnKeyRight(); return true;
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
    if (m_firstSkin + 6 < (int)m_skins.size())
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

    if (m_iActiveIndex != -1 && (m_iActiveIndex < m_firstSkin || m_iActiveIndex > m_firstSkin + 5))
    {
        if (m_iActiveIndex > (int)m_skins.size() - 6)
            m_firstSkin = (int)m_skins.size() - 6;
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
