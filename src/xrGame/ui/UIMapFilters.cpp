#include "StdAfx.h"

#include "UIMapFilters.h"
#include "UIHelper.h"
#include "xrUICore/Buttons/UICheckButton.h"

CUIMapFilters::CUIMapFilters() : CUIWindow("Map locations filters") {}

bool CUIMapFilters::Init(CUIXml& xml)
{
    bool result = false;

    const bool convertPosToOurs = !CUIXmlInit::InitWindow(xml, "filters_wnd", 0, this, false);

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
        if (!filter)
            continue;

        filter->SetMessageTarget(this);
        filter->SetWindowName(filter_section);
        filter->SetCheck(true);

        result = true;
    }

    if (result && convertPosToOurs)
    {
        // Adjust this window rect first
        Frect rect{ type_max<float>, type_max<float>, 0, 0 };

        for (const auto filter : m_filters)
        {
            if (!filter)
                continue;
            rect.lt.min(filter->GetWndPos());
            Frect filterRect;
            filter->GetWndRect(filterRect);
            rect.rb.max(filterRect.rb);
        }

        SetWndRect(rect);

        // Adjust filters positions
        for (const auto filter : m_filters)
        {
            if (!filter)
                continue;
            Fvector2 ourAbsPos;
            GetAbsolutePos(ourAbsPos);
            const Fvector2& pos = filter->GetWndPos();
            filter->SetWndPos({ pos.x - ourAbsPos.x, pos.y - ourAbsPos.y });
        }
    }

    return result;
}

void CUIMapFilters::Reset()
{
    inherited::Reset();
    Activate(false);
}

bool CUIMapFilters::Activate(bool activate)
{
    m_activated = activate;

    auto& focus = UI().Focus();
    if (activate)
    {
        GetMessageTarget()->SetKeyboardCapture(this, true);
        focus.LockToWindow(this);
        focus.SetFocused(m_filters[0]);
    }
    else
    {
        if (GetMessageTarget()->GetKeyboardCapturer() == this)
            GetMessageTarget()->SetKeyboardCapture(nullptr, true);
        if (focus.GetLocker() == this)
            focus.Unlock();
        GetMessageTarget()->SendMessage(GetMessageTarget(), WINDOW_KEYBOARD_CAPTURE_LOST, this);
    }
    return true;
}

bool CUIMapFilters::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (!m_activated)
    {
        if (IsBinded(kPDA_FILTER_TOGGLE, dik, EKeyContext::PDA))
        {
            Activate(true);
            return true;
        }
        return false;
    }

    if (inherited::OnKeyboardAction(dik, keyboard_action))
        return true;

    auto action = GetBindedAction(dik, EKeyContext::UI);
    if (action == kNOTBINDED)
        action = GetBindedAction(dik);

    switch (action)
    {
    case kQUIT:
    case kUI_BACK:
        Activate(false);
        return true;

    case kENTER:
    case kUI_ACCEPT:
        if (keyboard_action == WINDOW_KEY_PRESSED)
        {
            if (const auto filter = GetSelectedFilter())
                filter->OnMouseDown(MOUSE_1);
        }
        return true;

    case kUI_ACTION_1:
    case kUI_ACTION_2:
        return true; // intercept
    }

    return false;
}

void CUIMapFilters::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (msg == BUTTON_CLICKED)
    {
        // This cycle could be implemented through CUIWndCallback,
        // but we don't need it too much here
        for (const auto filter : m_filters)
        {
            if (filter == pWnd)
            {
                GetMessageTarget()->SendMessage(this, PDA_TASK_RELOAD_FILTERS, nullptr);
                return;
            }
        }
    }
    else if (msg == WINDOW_KEYBOARD_CAPTURE_LOST && pWnd == GetMessageTarget())
    {
        if (m_activated)
            Activate(false);
        return;
    }
    else if (msg == WINDOW_FOCUS_LOST && pWnd == this)
    {
        if (m_activated)
            Activate(false);
        return;
    }
    inherited::SendMessage(pWnd, msg, pData);
}

CUICheckButton* CUIMapFilters::GetSelectedFilter() const
{
    auto focused = UI().Focus().GetFocused();
    if (IsChild(focused))
        return static_cast<CUICheckButton*>(focused);
    return nullptr;
}

bool CUIMapFilters::IsFilterEnabled(eSpotsFilter filter) const
{
    return m_filters[filter] && m_filters[filter]->GetCheck();
}

void CUIMapFilters::SetFilterEnabled(eSpotsFilter filter, bool enable) const
{
    if (m_filters[filter])
        m_filters[filter]->SetCheck(enable);
}
