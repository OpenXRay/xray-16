#include "StdAfx.h"
#include "UIChangeMap.h"
#include "UIVotingCategory.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ListBox/UIListBoxItem.h"
#include "Level.h"
#include "game_cl_teamdeathmatch.h"
#include "xrEngine/XR_IOConsole.h"
#include "UIMapList.h"
#include "Common/object_broker.h"
#include "UIGameCustom.h"
#include "UIHelper.h"

CUIChangeMap::CUIChangeMap() : CUIDialogWnd(CUIChangeMap::GetDebugType()) {}

void CUIChangeMap::InitChangeMap(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "change_map", 0, this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "change_map:background", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "change_map:header", this);
    map_pic     = UIHelper::CreateStatic(xml_doc, "change_map:map_pic", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "change_map:map_frame", this);
    map_version = UIHelper::CreateStatic(xml_doc, "change_map:map_ver_txt", this);
    std::ignore = UIHelper::CreateFrameWindow(xml_doc, "change_map:frame", this, false);
    std::ignore = UIHelper::CreateFrameWindow(xml_doc, "change_map:list_back", this, false);
    lst         = UIHelper::CreateListBox(xml_doc, "change_map:list", this);
    btn_ok      = UIHelper::Create3tButton(xml_doc, "change_map:btn_ok", this);
    btn_cancel  = UIHelper::Create3tButton(xml_doc, "change_map:btn_cancel", this);

    FillUpList();
}

bool CUIChangeMap::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (IsBinded(kQUIT, dik))
    {
        OnBtnCancel();
        return true;
    }
    return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
}

void CUIChangeMap::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (LIST_ITEM_SELECT == msg && pWnd == lst)
    {
        OnItemSelect();
    }
    else if (BUTTON_CLICKED == msg)
    {
        if (pWnd == btn_ok)
            OnBtnOk();
        else if (pWnd == btn_cancel)
            OnBtnCancel();
    }
}

void CUIChangeMap::OnItemSelect()
{
    const u32 idx = lst->GetSelectedIDX();
    if (idx == u32(-1))
        return;

    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    const shared_str& name = M.m_map_names[idx].map_name;
    pstr map_ver = nullptr;
    STRCONCAT(map_ver, "[", M.m_map_names[idx].map_ver.c_str() ? M.m_map_names[idx].map_ver.c_str() : "unknown", "]");
    xr_string map_name = "intro" DELIMITER "intro_map_pic_";
    map_name += name.c_str();
    const xr_string full_name = map_name + ".dds";

    const Frect orig_rect = map_pic->GetTextureRect();
    if (FS.exist("$game_textures$", full_name.c_str()))
        map_pic->InitTexture(map_name.c_str());
    else
        map_pic->InitTexture("ui" DELIMITER "ui_noise");

    map_pic->SetTextureRect(orig_rect);
    map_version->SetText(map_ver);
}

void CUIChangeMap::OnBtnOk()
{
    const u32 idx = lst->GetSelectedIDX();
    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    if (idx < M.m_map_names.size())
    {
        const shared_str& name = M.m_map_names[idx].map_name;
        const shared_str& ver = M.m_map_names[idx].map_ver;

        string512 command;
        xr_sprintf(command, "cl_votestart changemap %s %s", name.c_str(), ver.c_str());
        Console->Execute(command);
        HideDialog();
    }
}

void CUIChangeMap::FillUpList()
{
    lst->Clear();

    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    const u32 cnt = M.m_map_names.size();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUIListBoxItem* itm = lst->AddTextItem(StringTable().translate(M.m_map_names[i].map_name).c_str());
        itm->Enable(true);
    }
}

void CUIChangeMap::OnBtnCancel() { HideDialog(); }
