#include "StdAfx.h"
#include "UIMPChangeMapAdm.h"
#include "UIXmlInit.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ListBox/UIListBoxItem.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIDialogWnd.h"
#include "Level.h"
#include "xrEngine/XR_IOConsole.h"
#include "UIGameCustom.h"
#include "string_table.h"

CUIMpChangeMapAdm::CUIMpChangeMapAdm()
{
    map_pic = xr_new<CUIStatic>();
    map_pic->SetAutoDelete(true);
    AttachChild(map_pic);

    map_frame = xr_new<CUIStatic>();
    map_frame->SetAutoDelete(true);
    AttachChild(map_frame);

    map_version = xr_new<CUITextWnd>();
    map_version->SetAutoDelete(true);
    AttachChild(map_version);

    lst = xr_new<CUIListBox>();
    lst->SetAutoDelete(true);
    AttachChild(lst);

    btn_ok = xr_new<CUI3tButton>();
    btn_ok->SetAutoDelete(true);
    AttachChild(btn_ok);
}

CUIMpChangeMapAdm::~CUIMpChangeMapAdm() {}
void CUIMpChangeMapAdm::Init(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "change_map_adm", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "change_map_adm:map_frame", 0, map_frame);
    CUIXmlInit::InitTextWnd(xml_doc, "change_map_adm:map_ver_txt", 0, map_version);
    CUIXmlInit::InitStatic(xml_doc, "change_map_adm:map_pic", 0, map_pic);
    CUIXmlInit::InitListBox(xml_doc, "change_map_adm:list", 0, lst);
    CUIXmlInit::Init3tButton(xml_doc, "change_map_adm:btn_ok", 0, btn_ok);

    FillUpList();
}

void CUIMpChangeMapAdm::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (LIST_ITEM_SELECT == msg && pWnd == lst)
        OnItemSelect();
    else if (BUTTON_CLICKED == msg)
    {
        if (pWnd == btn_ok)
            OnBtnOk();
    }
}

void CUIMpChangeMapAdm::OnItemSelect()
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

void CUIMpChangeMapAdm::OnBtnOk()
{
    u32 idx = lst->GetSelectedIDX();
    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    if (idx >= 0 && idx < M.m_map_names.size())
    {
        const shared_str& name = M.m_map_names[idx].map_name;
        const shared_str& ver = M.m_map_names[idx].map_ver;
        string512 command;
        xr_sprintf(command, "ra sv_changelevel %s %s", name.c_str(), ver.c_str());
        Console->Execute(command);
        smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
    }
}
void CUIMpChangeMapAdm::FillUpList()
{
    lst->Clear();
    const SGameTypeMaps& M = gMapListHelper.GetMapListFor((EGameIDs)GameID());
    u32 cnt = M.m_map_names.size();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUIListBoxItem* itm = lst->AddTextItem(StringTable().translate(M.m_map_names[i].map_name).c_str());
        itm->Enable(true);
    }
}
