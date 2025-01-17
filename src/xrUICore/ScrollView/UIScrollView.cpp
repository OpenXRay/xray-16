#include "pch.hpp"
#include "UIScrollView.h"
#include "ScrollBar/UIScrollBar.h"
#include "ScrollBar/UIFixedScrollBar.h"
#include "ui_base.h"
#include "Cursor/UICursor.h"
#include "xrEngine/xr_input.h"

CUIScrollView::CUIScrollView() : CUIWindow("CUIScrollView")
{
    SetFixedScrollBar(true);
}

CUIScrollView::CUIScrollView(CUIScrollBar* scroll_bar) : CUIWindow("CUIScrollView")
{
    SetFixedScrollBar(true);

    m_VScrollBar = scroll_bar;
    m_VScrollBar->SetAutoDelete(true);
    AttachChild(m_VScrollBar);
    Register(m_VScrollBar);
    AddCallback(m_VScrollBar, SCROLLBAR_VSCROLL, CUIWndCallback::void_function(this, &CUIScrollView::OnScrollV));
}

CUIScrollView::~CUIScrollView() { Clear(); }
void CUIScrollView::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
    if (CHILD_CHANGED_SIZE == msg && m_pad->IsChild(pWnd))
        m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::ForceUpdate() { m_flags.set(eNeedRecalc, true); }
void CUIScrollView::InitScrollView()
{
    if (!m_pad)
    {
        m_pad = xr_new<CUIWindow>("Scroll view pad");
        m_pad->SetAutoDelete(true);
        AttachChild(m_pad);
    }
    m_pad->SetWndPos(Fvector2().set(0, 0));

    CUIFixedScrollBar* tmp_scroll = smart_cast<CUIFixedScrollBar*>(m_VScrollBar);
    if (tmp_scroll)
    {
        if (!tmp_scroll->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f), false, *m_scrollbar_profile))
        {
            Msg("! Failed to init ScrollView with FixedScrollBar, trying to init with ScrollBar");
            DetachChild(m_VScrollBar);
            m_VScrollBar = nullptr;
        }
    }

    if (!m_VScrollBar)
    {
        m_VScrollBar = xr_new<CUIScrollBar>();
        m_VScrollBar->SetAutoDelete(true);
        AttachChild(m_VScrollBar);
        Register(m_VScrollBar);
        AddCallback(m_VScrollBar, SCROLLBAR_VSCROLL, CUIWndCallback::void_function(this, &CUIScrollView::OnScrollV));
    }

    if (!!m_scrollbar_profile && !tmp_scroll)
    {
        m_VScrollBar->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f),
            GetWndSize().y, false, *m_scrollbar_profile);
    }
    else
    {
        m_VScrollBar->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f),
            GetWndSize().y, false);
    }

    const Fvector2 sc_pos = {m_VScrollBar->GetWndPos().x - m_VScrollBar->GetWndSize().x, m_VScrollBar->GetWndPos().y};
    m_VScrollBar->SetWndPos(sc_pos);
    m_VScrollBar->SetWindowName("scroll_v");
    m_VScrollBar->SetStepSize(_max(1, iFloor(GetHeight() / 10)));
    m_VScrollBar->SetPageSize(iFloor(GetHeight()));
}

void CUIScrollView::SetScrollBarProfile(LPCSTR profile) { m_scrollbar_profile = profile; }
void CUIScrollView::AddWindow(CUIWindow* pWnd, bool auto_delete)
{
    if (auto_delete)
        pWnd->SetAutoDelete(true);

    m_pad->AttachChild(pWnd);
    m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::RemoveWindow(CUIWindow* pWnd)
{
    m_pad->DetachChild(pWnd);
    m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::Clear()
{
    m_pad->DetachAll();
    m_flags.set(eNeedRecalc, true);
    ScrollToBegin();
}

Fvector2 CUIScrollView::GetPadSize()
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    return m_pad->GetWndSize();
}

void CUIScrollView::Update()
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    if (const auto focused = CursorOverWindow() ? UI().Focus().GetFocused() : nullptr)
    {
        const auto scrollItem = focused->GetWindowBeforeParent(m_pad);

        if (scrollItem && GetSelected() != scrollItem)
        {
            const auto prevPos = GetCurrentScrollPos();

            ScrollToWindow(scrollItem);

            if (m_flags.test(eItemsSelectabe))
                scrollItem->OnMouseDown(MOUSE_1);

            if (prevPos != GetCurrentScrollPos())
                UI().GetUICursor().WarpToWindow(focused);
        }
    }

    inherited::Update();
}

void CUIScrollView::RecalcSize()
{
    if (!m_pad)
        return;
    Fvector2 pad_size;
    pad_size.set(0.0f, 0.0f);

    Fvector2 item_pos;
    item_pos.set(m_rightIndent, m_vertInterval + m_upIndent);
    pad_size.y += m_upIndent;
    pad_size.y += m_downIndent;

    if (m_sort_function)
    {
        //. m_pad->GetChildWndList().sort(m_sort_function);
        std::sort(m_pad->GetChildWndList().begin(), m_pad->GetChildWndList().end(), m_sort_function);
    }

    if (GetVertFlip())
    {
        for (auto it = m_pad->GetChildWndList().rbegin();
             m_pad->GetChildWndList().rend() != it; ++it)
        {
            (*it)->SetWndPos(item_pos);
            item_pos.y += (*it)->GetWndSize().y;
            item_pos.y += m_vertInterval;
            pad_size.y += (*it)->GetWndSize().y;
            pad_size.y += m_vertInterval;
            pad_size.x = _max(pad_size.x, (*it)->GetWndSize().x);
        }
    }
    else
    {
        for (auto* it : m_pad->GetChildWndList())
        {
            it->SetWndPos(item_pos);
            item_pos.y += it->GetWndSize().y;
            item_pos.y += m_vertInterval;
            pad_size.y += it->GetWndSize().y;
            pad_size.y += m_vertInterval;
            pad_size.x = _max(pad_size.x, it->GetWndSize().x);
        }
    };

    m_pad->SetWndSize(pad_size);

    if (m_flags.test(eInverseDir))
        m_pad->SetWndPos(Fvector2().set(m_pad->GetWndPos().x, GetHeight() - m_pad->GetHeight()));

    UpdateScroll();

    m_flags.set(eNeedRecalc, false);
    m_visible_rgn.set(-1, -1);
}

void CUIScrollView::UpdateScroll()
{
    const Fvector2 w_pos = m_pad->GetWndPos();
    m_VScrollBar->SetHeight(GetHeight());
    m_VScrollBar->SetRange(0, iFloor(m_pad->GetHeight() * Scroll2ViewV()));

    m_VScrollBar->SetScrollPos(iFloor(-w_pos.y));
}

float CUIScrollView::Scroll2ViewV() const
{
    const float h = m_VScrollBar->GetHeight();
    return (h + GetVertIndent()) / h;
}

void CUIScrollView::SetFixedScrollBar(bool b) { m_flags.set(eFixedScrollBar, b); }
void CUIScrollView::Draw()
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    Frect visible_rect;
    GetAbsoluteRect(visible_rect);
    visible_rect.top += m_upIndent;
    visible_rect.bottom -= m_downIndent;
    UI().PushScissor(visible_rect);

    auto it = m_pad->GetChildWndList().begin();
    //	auto it_e					= m_pad->GetChildWndList().end();

    if (!Empty() && m_visible_rgn.x != -1)
    {
        std::advance(it, m_visible_rgn.x);
        for (int idx = m_visible_rgn.x; idx <= m_visible_rgn.y; ++it, ++idx)
        {
            VERIFY(smart_cast<CUIScrollView*>(*it) == NULL);

            if ((*it)->GetVisible())
                (*it)->Draw();
        }
    }
    else
        for (int idx = 0; it != m_pad->GetChildWndList().end(); ++it, ++idx)
        {
            Frect item_rect;
            (*it)->GetAbsoluteRect(item_rect);
            if (visible_rect.intersected(item_rect))
            {
                if (m_visible_rgn.x == -1) // first visible
                    m_visible_rgn.x = idx;

                m_visible_rgn.y = idx;

                if ((*it)->GetVisible())
                    (*it)->Draw();
            }
            else if (m_visible_rgn.x != -1)
                break;
        }
    UI().PopScissor();

    if (NeedShowScrollBar())
        m_VScrollBar->Draw();
}

bool CUIScrollView::NeedShowScrollBar() const
{
    return m_flags.test(eFixedScrollBar) || GetHeight() < m_pad->GetHeight();
}

void CUIScrollView::OnScrollV(CUIWindow*, void*)
{
    const int s_pos = m_VScrollBar->GetScrollPos();
    const Fvector2 w_pos = m_pad->GetWndPos();
    m_pad->SetWndPos(Fvector2().set(w_pos.x, float(-s_pos)));
    m_visible_rgn.set(-1, -1);
}

bool CUIScrollView::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (inherited::OnMouseAction(x, y, mouse_action))
        return true;
    bool res = false;
    const int prev_pos = m_VScrollBar->GetScrollPos();
    switch (mouse_action)
    {
    case WINDOW_MOUSE_WHEEL_UP:
        m_VScrollBar->TryScrollDec(true);
        res = true;
        break;
    case WINDOW_MOUSE_WHEEL_DOWN:
        m_VScrollBar->TryScrollInc(true);
        res = true;
        break;
    case WINDOW_MOUSE_MOVE:
        if (pInput->iGetAsyncKeyState(MOUSE_1))
        {
            Fvector2 curr_pad_pos = m_pad->GetWndPos();
            curr_pad_pos.y += GetUICursor().GetCursorPositionDelta().y;

            float max_pos = m_pad->GetHeight() - GetHeight();
            max_pos = _max(0.0f, max_pos);
            clamp(curr_pad_pos.y, -max_pos, 0.0f);
            m_pad->SetWndPos(curr_pad_pos);
            UpdateScroll();
            res = true;
        }
        break;
    };
    if (prev_pos != m_VScrollBar->GetScrollPos())
        m_visible_rgn.set(-1, -1);

    return res;
}

bool CUIScrollView::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (inherited::OnKeyboardAction(dik, keyboard_action))
        return true;

    /*if (CursorOverWindow() && keyboard_action == WINDOW_KEY_PRESSED)
    {
        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_MOVE_UP:
            return SelectNext(false, false);
        case kUI_MOVE_DOWN:
            return SelectNext(true, false);
        }
    }*/

    return false;
}

bool CUIScrollView::OnControllerAction(int axis, float x, float y, EUIMessages controller_action)
{
    if (inherited::OnControllerAction(axis, x, y, controller_action))
        return true;

    /*if (CursorOverWindow())
    {
        if (IsBinded(kUI_MOVE, axis, EKeyContext::UI))
        {
            if (fis_zero(x))
            {
                if (y > 0)
                    return SelectNext(true, false);
                return SelectNext(false, false);
            }
        }
    }*/

    return false;
}

int CUIScrollView::GetMinScrollPos() const { return m_VScrollBar->GetMinRange(); }
int CUIScrollView::GetMaxScrollPos() const { return m_VScrollBar->GetMaxRange(); }
int CUIScrollView::GetCurrentScrollPos() const { return m_VScrollBar->GetScrollPos(); }

void CUIScrollView::SetScrollPos(int value)
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    clamp(value, GetMinScrollPos(), GetMaxScrollPos());
    m_VScrollBar->SetScrollPos(value);
    OnScrollV(nullptr, nullptr);
}

void CUIScrollView::ScrollToBegin()
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    m_VScrollBar->SetScrollPos(m_VScrollBar->GetMinRange());
    OnScrollV(nullptr, nullptr);
}

void CUIScrollView::ScrollToEnd()
{
    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    m_VScrollBar->SetScrollPos(m_VScrollBar->GetMaxRange());
    OnScrollV(nullptr, nullptr);
}

void CUIScrollView::ScrollToWindow(CUIWindow* pWnd, float center_y_ratio /*= 0.5f*/)
{
    R_ASSERT2_CURE(pWnd && pWnd->GetParent() == m_pad, "Requested window to scroll to doesn't belong to the scroll view", return);

    if (m_flags.test(eNeedRecalc))
        RecalcSize();

    const float ratio = GetHeight() * center_y_ratio;
    const int pos = iFloor(m_upIndent + pWnd->GetWndPos().y - ratio);

    SetScrollPos(pos);
}

void CUIScrollView::SetRightIndention(float val)
{
    m_rightIndent = val;
    m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::SetLeftIndention(float val)
{
    m_leftIndent = val;
    m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::SetUpIndention(float val)
{
    m_upIndent = val;
    m_flags.set(eNeedRecalc, true);
}

void CUIScrollView::SetDownIndention(float val)
{
    m_downIndent = val;
    m_flags.set(eNeedRecalc, true);
}

u32 CUIScrollView::GetSize() const { return m_pad->GetChildNum(); }

CUIWindow* CUIScrollView::GetItem(u32 idx)
{
    if (m_pad->GetChildWndList().size() <= idx)
        return nullptr;

    auto it = m_pad->GetChildWndList().begin();
    std::advance(it, idx);
    return *it;
}

float CUIScrollView::GetDesiredChildWidth() const
{
    if (NeedShowScrollBar())
        return GetWidth() - m_VScrollBar->GetWidth() - m_rightIndent - m_leftIndent;
    else
        return GetWidth() - m_rightIndent - m_leftIndent;
}

float CUIScrollView::GetHorizIndent() const { return m_rightIndent + m_leftIndent; }
float CUIScrollView::GetVertIndent() const { return m_upIndent + m_downIndent; }

void CUIScrollView::SetSelected(CUIWindow* w)
{
    if (!m_flags.test(eItemsSelectabe))
        return;

    for (auto* it : m_pad->GetChildWndList())
    {
        smart_cast<CUISelectable*>(it)->SetSelected(it == w);
    }
}

bool CUIScrollView::SelectNext(bool next, bool loop)
{
    if (Empty())
        return false;

    auto& focus = UI().Focus();

    bool found = false; // iterator of the current selected item found
    CUIWindow* item = nullptr; // item to be selected

    CUIWindow* currentSelected = nullptr;
    if (!m_flags.test(eItemsSelectabe) && focus.GetFocused())
        currentSelected = focus.GetFocused()->GetWindowBeforeParent(m_pad);

    // Iterate forward or backward
    auto       it    = next ? Items().cbegin() : Items().cend()   - 1;
    const auto ite   = next ? Items().cend()   : Items().cbegin() - 1;

    while (it != ite)
    {
        if (found)
        {
            item = *it;
            break;
        }

        if (m_flags.test(eItemsSelectabe))
        {
            if (smart_cast<CUISelectable*>(*it)->GetSelected())
            {
                found = true;
                currentSelected = *it;
            }
        }
        else if (*it == currentSelected)
            found = true;

        next ? ++it : --it;
    }

    // If no item is selected, always select first
    if (!found)
    {
        item = Items().front();
    }
    // Found current selected, but it is last
    else if (!item && loop)
    {
        item = next ? Items().front() : Items().back();
    }

    if (item)
    {
        // Update CUIScrollView native logic
        ScrollToWindow(item);
        if (m_flags.test(eItemsSelectabe))
            item->OnMouseDown(MOUSE_1);

        // Update focus system:
        // Set item as focused, if possible
        if (focus.IsRegistered(item))
            focus.SetFocused(item);
        else
        {
            // Set any suitable child as focused
            item->ProcessFunctor([&](const CUIWindow* wnd)
            {
                if (focus.IsRegistered(wnd))
                {
                    focus.SetFocused(wnd);
                    return true;
                }
                return false;
            });
        }
        return true;
    }
    return false;
}

CUIWindow* CUIScrollView::GetSelected()
{
    if (!m_flags.test(eItemsSelectabe))
        return nullptr;

    for (auto* it : m_pad->GetChildWndList())
    {
        if (smart_cast<CUISelectable*>(it)->GetSelected())
            return it;
    }

    return nullptr;
}

void CUIScrollView::UpdateChildrenLenght()
{
    const float len = GetDesiredChildWidth();
    for (auto* it : m_pad->GetChildWndList())
    {
        it->SetWidth(len);
    }
}

void CUIScrollView::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (!ImGui::CollapsingHeader(CUIScrollView::GetDebugType()))
        return;

    ImGui::DragFloat("Up indent", &m_upIndent);
    ImGui::DragFloat("Down indent", &m_downIndent);
    ImGui::DragFloat("Left indent", &m_leftIndent);
    ImGui::DragFloat("Right indent", &m_rightIndent);

    ImGui::DragFloat("Vertical interval", &m_vertInterval);

    ImGui::Separator();
    ImGui::Text("Flags:");

    if (ImGui::Button("Recalculate"))
        m_flags.set(eNeedRecalc, true);

    const auto addFlag = [this](pcstr text, u16 flag)
    {
        ImGui::SameLine();
        bool value = m_flags.test(flag);
        if (ImGui::Checkbox(text, &value))
            m_flags.set(flag, value);
    };

    addFlag("Vertical flip", eVertFlip);
    addFlag("Fixed scrollbar", eFixedScrollBar);
    addFlag("Items selectable", eItemsSelectabe);
    addFlag("Inverse direction", eInverseDir);

    ImGui::Separator();
    ImGui::LabelText("Scrollbar profile", "%s", m_scrollbar_profile.empty() ? "" : m_scrollbar_profile.c_str());
    ImGui::Separator();

    ImGui::BeginDisabled();
    ImGui::DragInt2("Visible region", (int*)&m_visible_rgn);
    ImGui::EndDisabled();
#endif
}
