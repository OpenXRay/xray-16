#include "StdAfx.h"
#include "UIVotingCategory.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIKickPlayer.h"
#include "UIChangeMap.h"
#include "ChangeWeatherDialog.hpp"
#include "UIGameCustom.h"
#include "game_cl_teamdeathmatch.h"
#include "game_sv_mp_vote_flags.h"

CUIVotingCategory::CUIVotingCategory()
{
    xml_doc = NULL;
    kick = NULL;
    change_weather = NULL;
    change_map = NULL;
    change_gametype = NULL;

    bkgrnd = new CUIStatic();
    bkgrnd->SetAutoDelete(true);
    AttachChild(bkgrnd);
    header = new CUIStatic();
    header->SetAutoDelete(true);
    AttachChild(header);
    btn_cancel = new CUI3tButton();
    btn_cancel->SetAutoDelete(true);
    AttachChild(btn_cancel);

    for (int i = 0; i < 7; i++)
    {
        btn[i] = new CUI3tButton();
        btn[i]->SetAutoDelete(true);
        AttachChild(btn[i]);

        txt[i] = new CUIStatic();
        txt[i]->SetAutoDelete(true);
        AttachChild(txt[i]);
    }
    InitVotingCategory();
}

CUIVotingCategory::~CUIVotingCategory()
{
    xr_delete(kick);
    xr_delete(change_map);
    xr_delete(change_weather);
    xr_delete(change_gametype);

    xr_delete(xml_doc);
}

void CUIVotingCategory::InitVotingCategory()
{
    if (!xml_doc)
        xml_doc = new CUIXml();

    xml_doc->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "voting_category.xml");

    CUIXmlInit::InitWindow(*xml_doc, "category", 0, this);

    CUIXmlInit::InitStatic(*xml_doc, "category:header", 0, header);
    CUIXmlInit::InitStatic(*xml_doc, "category:background", 0, bkgrnd);

    string256 _path;
    for (int i = 0; i < 7; i++)
    {
        xr_sprintf(_path, "category:btn_%d", i + 1);
        CUIXmlInit::Init3tButton(*xml_doc, _path, 0, btn[i]);
        xr_sprintf(_path, "category:txt_%d", i + 1);
        CUIXmlInit::InitStatic(*xml_doc, _path, 0, txt[i]);
    }

    CUIXmlInit::Init3tButton(*xml_doc, "category:btn_cancel", 0, btn_cancel);
}

void CUIVotingCategory::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (BUTTON_CLICKED == msg)
    {
        if (btn_cancel == pWnd)
            OnBtnCancel();
        for (int i = 0; i < 7; i++)
        {
            if (btn[i] == pWnd)
            {
                OnBtn(i);
                return;
            }
        }
    }
}

bool CUIVotingCategory::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);

    if (WINDOW_KEY_PRESSED == keyboard_action)
    {
        if (SDL_SCANCODE_ESCAPE == dik)
        {
            OnBtnCancel();
            return true;
        }
        if (dik >= SDL_SCANCODE_1 && dik <= SDL_SCANCODE_7)
            OnBtn(dik - SDL_SCANCODE_1);
        return true;
    }
    return false;
}

#include "xrEngine/XR_IOConsole.h"

void CUIVotingCategory::OnBtn(int i)
{
    //	game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());

    // check buttons state, based on voting mask
    u16 flag = 1 << (u16(i + 1) & 0xff);
    if (Game().IsVotingEnabled(flag))
    {
        switch (i)
        {
        case 0:
            Console->Execute("cl_votestart restart");
            HideDialog();
            break;
        case 1:
            Console->Execute("cl_votestart restart_fast");
            HideDialog();
            break;
        case 2:
            HideDialog();
            if (!kick)
                kick = new CUIKickPlayer();
            kick->InitKick(*xml_doc);
            kick->ShowDialog(true);
            break;
        case 3:
            HideDialog();
            if (!kick)
                kick = new CUIKickPlayer();
            kick->InitBan(*xml_doc);
            kick->ShowDialog(true);
            break;
        case 4:
            HideDialog();
            if (!change_map)
                change_map = new CUIChangeMap();
            change_map->InitChangeMap(*xml_doc);
            change_map->ShowDialog(true);
            break;
        case 5:
            HideDialog();
            if (!change_weather)
                change_weather = new ChangeWeatherDialog();
            change_weather->InitChangeWeather(*xml_doc);
            change_weather->ShowDialog(true);
            break;
        case 6:
            HideDialog();
            if (!change_gametype)
                change_gametype = new ChangeGameTypeDialog();
            change_gametype->InitChangeGameType(*xml_doc);
            change_gametype->ShowDialog(true);
            break;
        case 7: break;
        }
    }
}

void CUIVotingCategory::OnBtnCancel() { HideDialog(); }
void CUIVotingCategory::Update()
{
    // check buttons state, based on voting mask
    for (int i = 0; i < 7; i++)
    {
        u16 flag = 1 << (u16(i + 1) & 0xff);

        btn[i]->Enable(Game().IsVotingEnabled(flag));
        txt[i]->Enable(Game().IsVotingEnabled(flag));
    }

    inherited::Update();
}
