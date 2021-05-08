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
#include "UIDialogHolder.h"
#include "xrUICore/Windows/UIFrameWindow.h"

CUIChangeMap::CUIChangeMap()
{
    m_prev_upd_time = 0;

    bkgrnd = xr_new<CUIStatic>();
    bkgrnd->SetAutoDelete(true);
    AttachChild(bkgrnd);

    header = xr_new<CUITextWnd>();
    header->SetAutoDelete(true);
    AttachChild(header);

    map_pic = xr_new<CUIStatic>();
    map_pic->SetAutoDelete(true);
    AttachChild(map_pic);

    map_frame = xr_new<CUIStatic>();
    map_frame->SetAutoDelete(true);
    AttachChild(map_frame);

    map_version = xr_new<CUITextWnd>();
    map_version->SetAutoDelete(true);
    AttachChild(map_version);

    frame = xr_new<CUIFrameWindow>();
    frame->SetAutoDelete(true);
    AttachChild(frame);

    lst_back = xr_new<CUIFrameWindow>();
    lst_back->SetAutoDelete(true);
    AttachChild(lst_back);

    lst = xr_new<CUIListBox>();
    lst->SetAutoDelete(true);
    AttachChild(lst);

    btn_ok = xr_new<CUI3tButton>();
    btn_ok->SetAutoDelete(true);
    AttachChild(btn_ok);

    btn_cancel = xr_new<CUI3tButton>();
    btn_cancel->SetAutoDelete(true);
    AttachChild(btn_cancel);
}

CUIChangeMap::~CUIChangeMap() {}
void CUIChangeMap::InitChangeMap(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "change_map", 0, this);
    CUIXmlInit::InitTextWnd(xml_doc, "change_map:header", 0, header);
    CUIXmlInit::InitStatic(xml_doc, "change_map:background", 0, bkgrnd);
    CUIXmlInit::InitStatic(xml_doc, "change_map:map_frame", 0, map_frame);
    CUIXmlInit::InitTextWnd(xml_doc, "change_map:map_ver_txt", 0, map_version);
    CUIXmlInit::InitStatic(xml_doc, "change_map:map_pic", 0, map_pic);
    //	CUIXmlInit::InitFrameWindow			(xml_doc,			"change_map:list_back", 0, lst_back);
    //	CUIXmlInit::InitFrameWindow			(xml_doc,			"change_map:frame", 0, frame);
    CUIXmlInit::InitListBox(xml_doc, "change_map:list", 0, lst);
    CUIXmlInit::Init3tButton(xml_doc, "change_map:btn_ok", 0, btn_ok);
    CUIXmlInit::Init3tButton(xml_doc, "change_map:btn_cancel", 0, btn_cancel);

    FillUpList();
}

bool CUIChangeMap::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (dik == SDL_SCANCODE_ESCAPE)
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
    u32 idx = lst->GetSelectedIDX();
    if (idx == u32(-1))
        return;

    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    const shared_str& name = M.m_map_names[idx].map_name;
    pstr map_ver = NULL;
    STRCONCAT(map_ver, "[", M.m_map_names[idx].map_ver.c_str() ? M.m_map_names[idx].map_ver.c_str() : "unknown", "]");
    xr_string map_name = "intro" DELIMITER "intro_map_pic_";
    map_name += name.c_str();
    xr_string full_name = map_name + ".dds";

    Frect orig_rect = map_pic->GetTextureRect();
    if (FS.exist("$game_textures$", full_name.c_str()))
        map_pic->InitTexture(map_name.c_str());
    else
        map_pic->InitTexture("ui" DELIMITER "ui_noise");

    map_pic->SetTextureRect(orig_rect);
    map_version->SetText(map_ver);
}

void CUIChangeMap::OnBtnOk()
{
    u32 idx = lst->GetSelectedIDX();
    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    if (idx >= 0 && idx < M.m_map_names.size())
    {
        const shared_str& name = M.m_map_names[idx].map_name;
        const shared_str& ver = M.m_map_names[idx].map_ver;

        string512 command;
        xr_sprintf(command, "cl_votestart changemap %s %s", name.c_str(), ver.c_str());
        Console->Execute(command);
        HideDialog();
    }
}
#include "string_table.h"
void CUIChangeMap::FillUpList()
{
    lst->Clear();

    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    u32 cnt = M.m_map_names.size();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUIListBoxItem* itm = lst->AddTextItem(StringTable().translate(M.m_map_names[i].map_name).c_str());
        itm->Enable(true); // m_pExtraContentFilter->IsDataEnabled(M.m_map_names[i].map_name.c_str()));
    }
}

void CUIChangeMap::OnBtnCancel() { HideDialog(); }
