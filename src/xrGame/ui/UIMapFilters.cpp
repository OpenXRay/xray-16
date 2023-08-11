#include "StdAfx.h"

#include "UIMapFilters.h"
#include "UIHelper.h"
#include "xrUICore/Buttons/UICheckButton.h"

CUIMapFilters::CUIMapFilters() : CUIWindow("Map locations filters") {}

bool CUIMapFilters::Init(CUIXml& xml)
{
    if (!CUIXmlInit::InitWindow(xml, "filters_wnd", 0, this, false))
    {
        if (const auto parent = GetParent())
        {
            SetWndPos(parent->GetWndPos());
            SetWndSize(parent->GetWndSize());
        }
    }
    constexpr std::tuple<eSpotsFilter, pcstr> filters[] =
    {
        { Treasures,      "filter_treasures" },
        { QuestNpcs,      "filter_primary_objects" },
        { SecondaryTasks, "filter_secondary_tasks" },
        { PrimaryObjects, "filter_quest_npcs" },
    };

    for (const auto& [filter_id, filter_section] : filters)
    {
        auto& filter = m_filters[filter_id];
        filter = UIHelper::CreateCheck(xml, filter_section, this, false);
        if (filter)
        {
            filter->SetMessageTarget(this);
            filter->SetWindowName(filter_section);
            filter->SetCheck(true);
        }
        m_filters_state[filter_id] = true;
    }

    return true;
}

void CUIMapFilters::Reset()
{
    inherited::Reset();
    SelectFilter(false);
}

bool CUIMapFilters::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (const auto filter = GetSelectedFilter())
    {
        if (keyboard_action != WINDOW_KEY_PRESSED)
            return true; // intercept all

        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_BACK:
            SelectFilter(false);
            return true;

        case kUI_ACCEPT:
            filter->OnMouseDown(MOUSE_1);
            return true;

        case kUI_MOVE_LEFT:
        case kUI_MOVE_DOWN:
            SelectFilter(true, false);
            return true;

        case kUI_MOVE_RIGHT:
        case kUI_MOVE_UP:
            SelectFilter(true, true);
            return true;
        }
    }
    return inherited::OnKeyboardAction(dik, keyboard_action);
}

bool CUIMapFilters::OnControllerAction(int axis, float x, float y, EUIMessages controller_action)
{
    switch (GetBindedAction(axis, EKeyContext::UI))
    {
    default:
        return OnKeyboardAction(axis, controller_action);
    case kUI_MOVE:
        if (GetSelectedFilter())
            return true; // just screw it for now
    }
    return inherited::OnControllerAction(axis, x, y, controller_action);
}

void CUIMapFilters::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    // This cycle could be implemented through CUIWndCallback,
    // but we don't need it too much here
    for (u32 i = 0; i < Filter_Count; ++i)
    {
        if (m_filters[i] == pWnd && msg == BUTTON_CLICKED)
        {
            m_filters_state[i] = m_filters[i]->GetCheck();
            GetMessageTarget()->SendMessage(this, PDA_TASK_RELOAD_FILTERS, nullptr);
            return;
        }
    }
    if (msg == PDA_TASK_SELECT_FILTERS)
    {
        SelectFilter(!GetSelectedFilter());
        return;
    }
    if (msg == WINDOW_KEYBOARD_CAPTURE_LOST && pWnd == GetMessageTarget())
    {
        SelectFilter(false);
        return;
    }
    if (msg == WINDOW_FOCUS_LOST && pWnd == this)
    {
        SelectFilter(false);
        return;
    }
    inherited::SendMessage(pWnd, msg, pData);
}

CUICheckButton* CUIMapFilters::GetSelectedFilter() const
{
    if (m_selected_filter == -1)
        return nullptr;
    return m_filters[m_selected_filter];
}

void CUIMapFilters::SelectFilter(bool select, bool next /*= true*/)
{
    auto& cursor = GetUICursor();

    if (!select)
    {
        m_selected_filter = -1;
        cursor.WarpToWindow(nullptr, pInput->IsCurrentInputTypeController());
    }
    else
    {
        if (next)
        {
            if (m_selected_filter < int(m_filters.size() - 1))
                m_selected_filter++;
            else
                m_selected_filter = 0;
        }
        else // prev
        {
            if (m_selected_filter > 0)
                m_selected_filter--;
            else
                m_selected_filter = int(m_filters.size() - 1);
        }
        cursor.WarpToWindow(m_filters[m_selected_filter]);
        cursor.PauseAutohiding(true);
    }
}

void CUIMapFilters::SetFilterEnabled(eSpotsFilter filter, bool enable)
{
    m_filters_state[filter] = enable;
    if (m_filters[filter])
        m_filters[filter]->SetCheck(enable);
}
