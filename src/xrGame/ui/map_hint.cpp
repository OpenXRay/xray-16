#include "StdAfx.h"
#include "map_hint.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "map_location.h"
#include "map_spot.h"
#include "Actor.h"
#include "GametaskManager.h"
#include "GameTask.h"
#include "UIInventoryUtilities.h"
#include "string_table.h"
#include "UIHelper.h"

constexpr cpcstr HINT_ITEM_XML = "hint_item.xml";

void CUIMapLocationHint::Init(CUIXml& uiXml, LPCSTR path)
{
    CUIXml hintItemXml;
    const bool xmlLoaded = hintItemXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "hint_item.xml", false);

    const bool result = CUIXmlInit::InitFrameWindow(uiXml, path, 0, this, !xmlLoaded);
    if (!result) // SOC
    {
        const bool windowInit = CUIXmlInit::InitWindow(hintItemXml, "hint_item", 0, this, false);
        R_ASSERT4(windowInit, "Failed to initialize CUIMapLocationHint.\n"
            "Was trying both COP and SOC style, both attempts failed.",
            uiXml.m_xml_file_name, HINT_ITEM_XML);

        m_border = UIHelper::CreateFrameWindow(hintItemXml, "hint_item:frame", this);
        m_info["simple_text"] = UIHelper::CreateStatic(hintItemXml, "hint_item:description", this);
        return;
    }

    const auto init = [&](pcstr name)
    {
        string512 buff;
        strconcat(sizeof(buff), buff, path, ":", name);
        m_info[name] = UIHelper::CreateStatic(uiXml, buff, this);
    };

    init("simple_text");
    init("t_icon");
    init("t_caption");
    init("t_time");
    init("t_time_rem");
    init("t_hint_text");

    m_posx_icon = m_info["t_icon"]->GetWndPos().x;
    m_posx_caption = m_info["t_caption"]->GetWndPos().x;
}

void CUIMapLocationHint::SetInfoMode(u8 mode)
{
    const auto showIf = [&](pcstr name, bool condition)
    {
        if (m_info[name])
            m_info[name]->Show(condition);
    };
    showIf("simple_text",   mode == 1);
    showIf("t_icon",        mode == 2);
    showIf("t_caption",     mode == 2);
    showIf("t_time",        mode == 2);
    showIf("t_time_rem",    mode == 2);
    showIf("t_hint_text",   mode == 2);
}

void CUIMapLocationHint::SetInfoStr(pcstr info)
{
    SetInfoMode(1);

    CUIStatic* text = m_info["simple_text"];
    text->SetTextST(info);
    text->AdjustHeightToText();

    const float new_w = text->GetWndPos().x + text->GetWndSize().x + 20.0f;
    const float new_h = _max(64.0f, text->GetWndPos().y + text->GetWndSize().y + 20.0f);

    if (!m_border)
        SetWndSize(Fvector2().set(new_w, new_h));
    else
    {
        SetWndSize(Fvector2().set(GetWndSize().x, new_h));
        m_border->SetWidth(GetWndSize().x);
        m_border->SetHeight(GetWndSize().y);
    }
}

void CUIMapLocationHint::SetInfoMSpot(CMapSpot* spot)
{
    CMapLocation* ml = spot->MapLocation();

    CGameTask* gt = Level().GameTaskManager().HasGameTask(ml, true);
    if (gt)
        SetInfoTask(gt);
    else
        SetInfoStr(ml->GetHint());
}

void CUIMapLocationHint::SetInfoTask(CGameTask* task)
{
    SetInfoMode(2);
    CUIStatic* S = m_info["t_icon"];

    S->InitTexture(task->m_icon_texture_name.c_str());
    S->SetStretchTexture(true);

    S = m_info["t_caption"];
    S->TextItemControl()->SetTextST(task->m_Title.c_str());
    S->AdjustHeightToText();
    // float new_w						= S->GetWndPos().x + S->GetWndSize().x + 20.0f;

    S = m_info["t_time"];
    S->TextItemControl()->SetText(InventoryUtilities::GetTimeAndDateAsString(task->m_ReceiveTime).c_str());

    Fvector2 pos = S->GetWndPos();
    pos.y = m_info["t_caption"]->GetWndPos().y + m_info["t_caption"]->GetWndSize().y + 7;
    S->SetWndPos(pos);

    S = m_info["t_time_rem"];
    bool b_rem = (task->m_ReceiveTime != task->m_TimeToComplete);
    S->Show(b_rem);
    if (b_rem)
    {
        string512 buff, buff2;
        InventoryUtilities::GetTimePeriodAsString(buff, sizeof(buff), Level().GetGameTime(), task->m_TimeToComplete);

        strconcat(sizeof(buff2), buff2, StringTable().translate("ui_st_time_remains").c_str(), " ", buff);
        S->TextItemControl()->SetText(buff2);
    }
    pos = S->GetWndPos();
    pos.y = m_info["t_time"]->GetWndPos().y + m_info["t_time"]->GetWndSize().y + 7;
    S->SetWndPos(pos);

    S = m_info["t_hint_text"];
    S->TextItemControl()->SetTextST(task->m_Description.c_str());
    S->AdjustHeightToText();
    if (b_rem)
    {
        S = m_info["t_time_rem"];
    }
    else
    {
        S = m_info["t_time"];
    }
    pos.x = m_posx_icon;
    pos.y = S->GetWndPos().y + S->GetWndSize().y + 10;
    m_info["t_hint_text"]->SetWndPos(pos);

    if (task->GetTaskType() == eTaskTypeStoryline)
    {
        m_info["t_icon"]->Show(true);
        float w = m_info["t_time"]->GetWidth();

        Fvector2 pos = m_info["t_icon"]->GetWndPos();
        pos.x = m_posx_icon;
        m_info["t_icon"]->SetWndPos(pos);

        pos = m_info["t_caption"]->GetWndPos();
        pos.x = m_posx_caption;
        m_info["t_caption"]->SetWndPos(pos);
        m_info["t_caption"]->SetWidth(w);

        pos = m_info["t_time"]->GetWndPos();
        pos.x = m_posx_caption;
        m_info["t_time"]->SetWndPos(pos);

        pos = m_info["t_time_rem"]->GetWndPos();
        pos.x = m_posx_caption;
        m_info["t_time_rem"]->SetWndPos(pos);

        pos = m_info["t_hint_text"]->GetWndPos();
        pos.y = _max(pos.y, m_info["t_icon"]->GetWndPos().y + m_info["t_icon"]->GetWndSize().y + 7);
        m_info["t_hint_text"]->SetWndPos(pos);
    }
    else if (task->GetTaskType() == eTaskTypeAdditional)
    {
        m_info["t_icon"]->Show(false);
        float w = m_info["t_hint_text"]->GetWidth();

        Fvector2 pos = m_info["t_caption"]->GetWndPos();
        pos.x = m_posx_icon;
        m_info["t_caption"]->SetWndPos(pos);
        m_info["t_caption"]->SetWidth(w);

        pos = m_info["t_time"]->GetWndPos();
        pos.x = m_posx_icon;
        m_info["t_time"]->SetWndPos(pos);

        pos = m_info["t_time_rem"]->GetWndPos();
        pos.x = m_posx_icon;
        m_info["t_time_rem"]->SetWndPos(pos);
    }

    pos.x = m_info["t_hint_text"]->GetWndPos().x + m_info["t_hint_text"]->GetWndSize().x + 20.0f;
    pos.y = m_info["t_hint_text"]->GetWndPos().y + m_info["t_hint_text"]->GetWndSize().y + 20.0f;
    SetWndSize(pos);
}
