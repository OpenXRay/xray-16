#include "StdAfx.h"
#include "UITaskItem.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "../GameTask.h"
#include "xrEngine/StringTable/StringTable.h"
#include "UIEventsWnd.h"
#include "xrUICore/EditBox/UIEditBoxEx.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "UIInventoryUtilities.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "../map_location.h"
#include "../map_manager.h"
#include "../Level.h"
#include "../Actor.h"
#include "../GametaskManager.h"

CUISocTaskItem::CUISocTaskItem(CUIEventsWnd* w) : m_GameTask(NULL), m_TaskObjectiveIdx(u16(-1)), m_EventsWnd(w) {}

CUISocTaskItem::~CUISocTaskItem() {}

void CUISocTaskItem::SetGameTask(CGameTask* gt, u16 obj_idx)
{
    m_GameTask = gt;
    m_TaskObjectiveIdx = obj_idx;
}

void CUISocTaskItem::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }

SGameTaskObjective* CUISocTaskItem::Objective() { return &m_GameTask->Objective(m_TaskObjectiveIdx); }

void CUISocTaskItem::Init()
{
    SetWindowName("job_item");
    Register(this);
    AddCallbackStr("job_item", BUTTON_CLICKED, fastdelegate::MakeDelegate(this, &CUISocTaskItem::OnItemClicked));
}

void CUISocTaskItem::OnItemClicked(CUIWindow*, void*)
{
    if (ObjectiveIdx() != ROOT_TASK_OBJECTIVE)
    {
        SGameTaskObjective* objective = Objective();
        if (objective->GetTaskState() != eTaskStateInProgress)
            return;

        Level().GameTaskManager().SetActiveTask(GameTask(), ObjectiveIdx());
        m_EventsWnd->SetDescriptionMode(true);
    }

    m_EventsWnd->ShowDescription(GameTask(), ObjectiveIdx());
}

CUITaskRootItem::CUITaskRootItem(CUIEventsWnd* w) : inherited(w) { Init(); }

CUITaskRootItem::~CUITaskRootItem() {}

void CUITaskRootItem::Init()
{
    inherited::Init();

    m_taskImage = xr_new<CUIStatic>();
    m_taskImage->SetAutoDelete(true);
    AttachChild(m_taskImage);
    m_captionStatic = xr_new<CUIStatic>();
    m_captionStatic->SetAutoDelete(true);
    AttachChild(m_captionStatic);
    m_remTimeStatic = xr_new<CUIStatic>();
    m_remTimeStatic->SetAutoDelete(true);
    AttachChild(m_remTimeStatic);

    m_switchDescriptionBtn = xr_new<CUI3tButton>();
    m_switchDescriptionBtn->SetAutoDelete(true);
    AttachChild(m_switchDescriptionBtn);
    m_captionTime = xr_new<CUI3tButton>();
    m_captionTime->SetAutoDelete(true);
    AttachChild(m_captionTime);

    m_switchDescriptionBtn->SetWindowName("m_switchDescriptionBtn");
    Register(m_switchDescriptionBtn);
    AddCallback(m_switchDescriptionBtn, BUTTON_DOWN, fastdelegate::MakeDelegate(this, &CUITaskRootItem::OnSwitchDescriptionClicked));

    CUIXmlInit xml_init;
    CUIXml& uiXml = m_EventsWnd->m_ui_task_item_xml;
    xml_init.InitWindow(uiXml, "task_root_item", 0, this);

    xml_init.InitStatic(uiXml, "task_root_item:image", 0, m_taskImage);
    xml_init.InitStatic(uiXml, "task_root_item:caption", 0, m_captionStatic);
    xml_init.InitStatic(uiXml, "task_root_item:caption_time", 0, m_captionTime);
    xml_init.InitStatic(uiXml, "task_root_item:rem_time", 0, m_remTimeStatic);

    xml_init.Init3tButton(uiXml, "task_root_item:switch_description_btn", 0, m_switchDescriptionBtn);
}

void CUITaskRootItem::SetGameTask(CGameTask* gt, u16 obj_idx)
{
    inherited::SetGameTask(gt, obj_idx);

    CStringTable stbl;
    auto& obj = m_GameTask->Objective(m_TaskObjectiveIdx);

    m_taskImage->InitTexture(obj.m_icon_texture_name.c_str());

    if (!CUITextureMaster::ItemExist(obj.m_icon_texture_name))
    {
        Frect r = obj.m_icon_rect;
        // The legacy file form stores x, y, width, and height. Modern
        // CUIStatic::SetTextureRect expects absolute right/bottom values.
        r.x2 += r.x1;
        r.y2 += r.y1;
        m_taskImage->SetTextureRect(r);
    }
    m_taskImage->SetStretchTexture(true);

    m_captionStatic->SetText(stbl.translate(m_GameTask->m_Title).c_str());
    m_captionStatic->AdjustHeightToText();

    xr_string txt = "";
    txt += InventoryUtilities::GetDateAsString(gt->m_ReceiveTime, InventoryUtilities::edpDateToDay, '/', true).c_str();
    txt += " ";
    txt += InventoryUtilities::GetTimeAsString(gt->m_ReceiveTime, InventoryUtilities::etpTimeToMinutes).c_str();

    m_captionTime->SetText(txt.c_str());
    m_captionTime->SetWndPos(m_captionTime->GetWndPos().x, m_captionStatic->GetWndPos().y + m_captionStatic->GetHeight() + 3.0f);

    float h = _max(m_taskImage->GetWndPos().y + m_taskImage->GetHeight(), m_captionTime->GetWndPos().y + m_captionTime->GetHeight());
    h = _max(h, m_switchDescriptionBtn->GetWndPos().y + m_switchDescriptionBtn->GetHeight());
    SetHeight(h);

    m_curr_descr_mode = m_EventsWnd->GetDescriptionMode();
    if (m_curr_descr_mode)
        m_switchDescriptionBtn->InitTexture("ui_icons_newPDA_showtext");
    else
        m_switchDescriptionBtn->InitTexture("ui_icons_newPDA_showmap");

    m_remTimeStatic->Show(GameTask()->Objective(0).GetTaskState() == eTaskStateInProgress && (GameTask()->m_ReceiveTime != GameTask()->m_TimeToComplete));

    if (m_remTimeStatic->IsShown())
    {
        float _height = GetWndSize().y;
        Fvector2 _pos = m_captionTime->GetWndPos();
        _pos.y += m_captionTime->GetWndSize().y;
        _pos.x = m_remTimeStatic->GetWndPos().x;

        m_remTimeStatic->SetWndPos(_pos);

        _height = _max(_height, _pos.y + m_remTimeStatic->GetWndSize().y);
        SetHeight(_height);
    }
}

void CUITaskRootItem::Update()
{
    inherited::Update();

    if (m_curr_descr_mode != m_EventsWnd->GetDescriptionMode())
    {
        m_curr_descr_mode = m_EventsWnd->GetDescriptionMode();
        if (m_curr_descr_mode)
            m_switchDescriptionBtn->InitTexture("ui_icons_newPDA_showtext");
        else
            m_switchDescriptionBtn->InitTexture("ui_icons_newPDA_showmap");
    }

    // The texture shows the current mode. Keep the button in its normal state
    // so it can send the next press and return from text to the map.
    m_switchDescriptionBtn->SetButtonState(CUIButton::BUTTON_NORMAL);

    if (m_remTimeStatic->IsShown())
    {
        string512 buff, buff2;
        InventoryUtilities::GetTimePeriodAsString(buff, sizeof(buff), Level().GetGameTime(), GameTask()->m_TimeToComplete);
        sprintf_s(buff2, "%s %s", CStringTable().translate("ui_st_time_remains").c_str(), buff);
        m_remTimeStatic->SetText(buff2);
    }
}

bool CUITaskRootItem::OnDbClick() { return true; }

void CUITaskRootItem::OnSwitchDescriptionClicked(CUIWindow*, void*)
{
    const bool showMap = !m_EventsWnd->GetDescriptionMode() && m_EventsWnd->IsTaskDescriptionShown(GameTask());
    m_EventsWnd->SetDescriptionMode(showMap);
    OnItemClicked(this, NULL);
}

void CUITaskRootItem::MarkSelected(bool b) {}

CUITaskSubItem::CUITaskSubItem(CUIEventsWnd* w) : inherited(w) { Init(); }

CUITaskSubItem::~CUITaskSubItem() {}

void CUITaskSubItem::Init()
{
    inherited::Init();
    CUIXml& uiXml = m_EventsWnd->m_ui_task_item_xml;

    m_stateStatic = xr_new<CUIStatic>();
    m_stateStatic->SetAutoDelete(true);
    AttachChild(m_stateStatic);
    m_descriptionStatic = xr_new<CUIStatic>();
    m_descriptionStatic->SetAutoDelete(true);
    AttachChild(m_descriptionStatic);
    m_ActiveObjectiveStatic = xr_new<CUIStatic>();
    m_ActiveObjectiveStatic->SetAutoDelete(true);
    AttachChild(m_ActiveObjectiveStatic);
    m_showDescriptionBtn = xr_new<CUI3tButton>();
    m_showDescriptionBtn->SetAutoDelete(true);
    AttachChild(m_showDescriptionBtn);

    m_showDescriptionBtn->SetWindowName("m_showDescriptionBtn");
    Register(m_showDescriptionBtn);

    AddCallback(m_showDescriptionBtn, BUTTON_DOWN, fastdelegate::MakeDelegate(this, &CUITaskSubItem::OnShowDescriptionClicked));

    CUIXmlInit xml_init;
    xml_init.InitWindow(uiXml, "task_sub_item", 0, this);
    xml_init.InitStatic(uiXml, "task_sub_item:state_image", 0, m_stateStatic);
    xml_init.InitStatic(uiXml, "task_sub_item:description", 0, m_descriptionStatic);
    xml_init.InitStatic(uiXml, "task_sub_item:active_objecttive_image", 0, m_ActiveObjectiveStatic);
    xml_init.Init3tButton(uiXml, "task_sub_item:show_descr_btn", 0, m_showDescriptionBtn);

    m_active_color = xml_init.GetColor(uiXml, "task_sub_item:description:text_colors:active", 0, 0x00);
    m_failed_color = xml_init.GetColor(uiXml, "task_sub_item:description:text_colors:failed", 0, 0x00);
    m_accomplished_color = xml_init.GetColor(uiXml, "task_sub_item:description:text_colors:accomplished", 0, 0x00);
    m_skiped_color = xml_init.GetColor(uiXml, "task_sub_item:description:text_colors:skiped", 0, 0x00);
}

void CUITaskSubItem::SetGameTask(CGameTask* gt, u16 obj_idx)
{
    inherited::SetGameTask(gt, obj_idx);

    CStringTable stbl;
    auto& obj = m_GameTask->Objective(m_TaskObjectiveIdx);

    m_descriptionStatic->SetText(stbl.translate(obj.m_Description).c_str());
    m_descriptionStatic->AdjustHeightToText();
    float h = _max(m_ActiveObjectiveStatic->GetWndPos().y + m_ActiveObjectiveStatic->GetHeight(), m_descriptionStatic->GetWndPos().y + m_descriptionStatic->GetHeight());
    SetHeight(h);
    switch (obj.GetTaskState())
    {
        //.		case eTaskUserDefined:
    case eTaskStateInProgress:
        m_stateStatic->InitTexture("ui_icons_PDA_subtask_active");
        m_descriptionStatic->SetTextColor(m_active_color);
        break;
    case eTaskStateFail:
        m_stateStatic->InitTexture("ui_icons_PDA_subtask_failed");
        m_descriptionStatic->SetTextColor(m_failed_color);
        break;
    case eTaskStateCompleted:
        m_stateStatic->InitTexture("ui_icons_PDA_subtask_accomplished");
        m_descriptionStatic->SetTextColor(m_accomplished_color);
        break;
    default: NODEFAULT;
    };
}

void CUITaskSubItem::Update()
{
    inherited::Update();
    CGameTask* activeTask = Level().GameTaskManager().ActiveTask();
    const bool bIsActive = activeTask == m_GameTask && m_GameTask->ActiveObjectiveIdx() == m_TaskObjectiveIdx;
    m_ActiveObjectiveStatic->Show(bIsActive);
    m_showDescriptionBtn->Show(m_EventsWnd->ItemHasDescription(this));
}

bool CUITaskSubItem::OnDbClick()
{
    OnItemClicked(this, nullptr);
    return true;
}

void CUITaskSubItem::OnActiveObjectiveClicked() { OnItemClicked(this, nullptr); }

void CUITaskSubItem::OnShowDescriptionClicked(CUIWindow*, void*)
{
    OnItemClicked(this, nullptr);
}

void CUITaskSubItem::MarkSelected(bool b) { m_showDescriptionBtn->SetButtonState(b ? CUIButton::BUTTON_PUSHED : CUIButton::BUTTON_NORMAL); }
