#include "StdAfx.h"
#include "UIDialogHolder.h"
#include "ui/UIDialogWnd.h"
#include "UIGameCustom.h"
#include "xrUICore/Cursor/UICursor.h"
#include "Level.h"
#include "Actor.h"
#include "xrEngine/xr_level_controller.h"
#include "xrEngine/CustomHUD.h"

dlgItem::dlgItem(CUIWindow* pWnd)
{
    wnd = pWnd;
    enabled = true;
}

bool dlgItem::operator<(const dlgItem& itm) const { return (int)enabled > (int)itm.enabled; }
bool operator==(const dlgItem& i1, const dlgItem& i2) { return (i1.wnd == i2.wnd) && (i1.enabled == i2.enabled); }
recvItem::recvItem(CUIDialogWnd* r)
{
    m_item = r;
    m_flags.zero();
}
bool operator==(const recvItem& i1, const recvItem& i2) { return i1.m_item == i2.m_item; }

CDialogHolder::CDialogHolder()
{
    m_b_in_update = false;
    RegisterDebuggable();
}

CDialogHolder::~CDialogHolder()
{
    UnregisterDebuggable();
}

void CDialogHolder::StartMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
    R_ASSERT(!pDialog->IsShown());

    AddDialogToRender(pDialog);
    SetMainInputReceiver(pDialog, false);

    if (UseIndicators() && !m_input_receivers.empty()) //Alundaio
    {
        bool b = !!psHUD_Flags.test(HUD_CROSSHAIR_RT);
        m_input_receivers.back().m_flags.set(recvItem::eCrosshair, b);

        b = CurrentGameUI()->GameIndicatorsShown();
        m_input_receivers.back().m_flags.set(recvItem::eIndicators, b);

        if (bDoHideIndicators)
        {
            psHUD_Flags.set(HUD_CROSSHAIR_RT, FALSE);
            CurrentGameUI()->ShowGameIndicators(false);
        }
    }
    pDialog->SetHolder(this);

    if (pDialog->NeedCursor())
    {
        GetUICursor().Show();
        m_become_visible_time = Device.dwTimeContinual;
    }

    if (g_pGameLevel)
    {
        CActor* A = smart_cast<CActor*>(Level().CurrentViewEntity());
        if (A && pDialog->StopAnyMove())
        {
            A->StopAnyMove();
        };
        if (A)
        {
            A->IR_OnKeyboardRelease(kWPN_ZOOM);
            A->IR_OnKeyboardRelease(kWPN_FIRE);
        }
    }
}

void CDialogHolder::StopMenu(CUIDialogWnd* pDialog)
{
    R_ASSERT(pDialog->IsShown());

    if (TopInputReceiver() == pDialog)
    {
        if (UseIndicators() && !m_input_receivers.empty()) //Alundaio
        {
            bool b = !!m_input_receivers.back().m_flags.test(recvItem::eCrosshair);
            psHUD_Flags.set(HUD_CROSSHAIR_RT, b);
            b = !!m_input_receivers.back().m_flags.test(recvItem::eIndicators);
            CurrentGameUI()->ShowGameIndicators(b);
        }

        SetMainInputReceiver(NULL, false);
    }
    else
        SetMainInputReceiver(pDialog, true);

    RemoveDialogToRender(pDialog);
    pDialog->SetHolder(NULL);

    if (!TopInputReceiver() || !TopInputReceiver()->NeedCursor())
        GetUICursor().Hide();
}

void CDialogHolder::AddDialogToRender(CUIWindow* pDialog)
{
    dlgItem itm(pDialog);
    itm.enabled = true;

    bool bAdd =
        (m_dialogsToRender_new.end() == std::find(m_dialogsToRender_new.begin(), m_dialogsToRender_new.end(), itm));
    if (!bAdd)
        return;

    bAdd = (m_dialogsToRender.end() == std::find(m_dialogsToRender.begin(), m_dialogsToRender.end(), itm));
    if (!bAdd)
        return;

    if (m_b_in_update)
        m_dialogsToRender_new.push_back(itm);
    else
        m_dialogsToRender.push_back(itm);

    pDialog->Show(true);
}

void CDialogHolder::RemoveDialogToRender(CUIWindow* pDialog)
{
    dlgItem itm(pDialog);
    itm.enabled = true;
    xr_vector<dlgItem>::iterator it = std::find(m_dialogsToRender.begin(), m_dialogsToRender.end(), itm);

    if (it != m_dialogsToRender.end())
    {
        (*it).wnd->Show(false);
        (*it).wnd->Enable(false);
        (*it).enabled = false;
    }
}

void CDialogHolder::DoRenderDialogs()
{
    ZoneScoped;

    xr_vector<dlgItem>::iterator it = m_dialogsToRender.begin();
    for (; it != m_dialogsToRender.end(); ++it)
    {
        if ((*it).enabled && (*it).wnd->IsShown())
            (*it).wnd->Draw();
    }
}

void CDialogHolder::OnExternalHideIndicators()
{
    xr_vector<recvItem>::iterator it = m_input_receivers.begin();
    xr_vector<recvItem>::iterator it_e = m_input_receivers.end();
    for (; it != it_e; ++it)
    {
        (*it).m_flags.set(recvItem::eIndicators, FALSE);
        (*it).m_flags.set(recvItem::eCrosshair, FALSE);
    }
}

CUIDialogWnd* CDialogHolder::TopInputReceiver()
{
    if (!m_input_receivers.empty())
        return m_input_receivers.back().m_item;
    return NULL;
};

void CDialogHolder::SetMainInputReceiver(CUIDialogWnd* ir, bool _find_remove)
{
    if (TopInputReceiver() == ir)
        return;

    if (!ir || _find_remove)
    {
        if (m_input_receivers.empty())
            return;

        if (!ir)
            m_input_receivers.pop_back();
        else
        {
            VERIFY(ir && _find_remove);

            u32 cnt = m_input_receivers.size();
            for (; cnt > 0; --cnt)
                if (m_input_receivers[cnt - 1].m_item == ir)
                {
                    m_input_receivers[cnt].m_flags.set(
                        recvItem::eCrosshair, m_input_receivers[cnt - 1].m_flags.test(recvItem::eCrosshair));
                    m_input_receivers[cnt].m_flags.set(
                        recvItem::eIndicators, m_input_receivers[cnt - 1].m_flags.test(recvItem::eIndicators));
                    xr_vector<recvItem>::iterator it = m_input_receivers.begin();
                    std::advance(it, cnt - 1);
                    m_input_receivers.erase(it);
                    break;
                }
        }
    }
    else
    {
        m_input_receivers.push_back(recvItem(ir));
    }
};

void CDialogHolder::StartDialog(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
    if (pDialog && pDialog->NeedCenterCursor())
    {
        GetUICursor().SetUICursorPosition({ UI_BASE_WIDTH / 2.0f, UI_BASE_HEIGHT / 2.0f });
    }
    StartMenu(pDialog, bDoHideIndicators);
}

void CDialogHolder::StopDialog(CUIDialogWnd* pDialog) { StopMenu(pDialog); }

void CDialogHolder::StartStopMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
    if (pDialog->IsShown())
        StopDialog(pDialog);
    else
        StartDialog(pDialog, bDoHideIndicators);
}

void CDialogHolder::OnFrame()
{
    ZoneScoped;

    m_b_in_update = true;

    UpdateCursorVisibility();

    CUIDialogWnd* wnd = TopInputReceiver();
    if (wnd && wnd->IsEnabled())
    {
        wnd->Update();
    }
    // else
    {
        xr_vector<dlgItem>::iterator it = m_dialogsToRender.begin();
        for (; it != m_dialogsToRender.end(); ++it)
            if ((*it).enabled && (*it).wnd->IsEnabled())
                (*it).wnd->Update();
    }

    if (m_is_foremost)
        UI().Focus().Update(wnd);

    m_b_in_update = false;
    if (!m_dialogsToRender_new.empty())
    {
        m_dialogsToRender.insert(m_dialogsToRender.end(), m_dialogsToRender_new.begin(), m_dialogsToRender_new.end());
        m_dialogsToRender_new.clear();
    }

    std::sort(m_dialogsToRender.begin(), m_dialogsToRender.end());
    while (!m_dialogsToRender.empty() && (!m_dialogsToRender[m_dialogsToRender.size() - 1].enabled))
        m_dialogsToRender.pop_back();
}

void CDialogHolder::CleanInternals()
{
    while (!m_input_receivers.empty())
        m_input_receivers.pop_back();

    m_dialogsToRender.clear();
    GetUICursor().Hide();
}

void CDialogHolder::UpdateCursorVisibility()
{
    if (m_is_foremost && !GEnv.isDedicatedServer)
    {
        auto& cursor = GetUICursor();
        const bool cursor_is_visible = cursor.IsVisible();
        const bool need_cursor = TopInputReceiver() && TopInputReceiver()->NeedCursor();

        const u32 cur_time = Device.dwTimeContinual;

        // These conditions are optimal, don't reorder.
        if (need_cursor)
        {
            if (!cursor_is_visible)
            {
                cursor.Show();
                m_become_visible_time = cur_time;
            }
        }
        else if (cursor_is_visible)
        {
            if (cur_time - m_become_visible_time > psControllerCursorAutohideTime * 1000.f)
                cursor.Hide();
        }
    }
}

bool CDialogHolder::IR_UIOnKeyboardPress(int dik)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    // mouse click
    if (dik == MOUSE_1 || dik == MOUSE_2 || dik == MOUSE_3)
    {
        Fvector2 cp = GetUICursor().GetCursorPosition();
        EUIMessages action =
            (dik == MOUSE_1) ? WINDOW_LBUTTON_DOWN : (dik == MOUSE_2) ? WINDOW_RBUTTON_DOWN : WINDOW_CBUTTON_DOWN;
        if (TIR->OnMouseAction(cp.x, cp.y, action))
            return true;
    }

    if (TIR->OnKeyboardAction(dik, WINDOW_KEY_PRESSED))
        return true;

    if (UI().GetUICursor().IsVisible() && dik > XR_CONTROLLER_BUTTON_INVALID && dik < XR_CONTROLLER_BUTTON_MAX)
    {
        FocusDirection direction = FocusDirection::Same;
        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_MOVE_LEFT:  direction = FocusDirection::Left; break;
        case kUI_MOVE_RIGHT: direction = FocusDirection::Right; break;
        case kUI_MOVE_UP:    direction = FocusDirection::Up; break;
        case kUI_MOVE_DOWN:  direction = FocusDirection::Down; break;
        }

        if (direction != FocusDirection::Same)
        {
            auto& focus = UI().Focus();
            const auto focused = focus.GetFocused();
            const Fvector2 vec = focused ? focused->GetAbsoluteCenterPos() : UI().GetUICursor().GetCursorPosition();
            const auto [candidate, candidate2] = focus.FindClosestFocusable(vec, direction);

            if (candidate || candidate2)
            {
                focus.SetFocused(candidate ? candidate : candidate2);
            }
            return true;
        }
    }

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
            //				IR->IR_OnKeyboardPress(get_binded_action(dik));
            {
                EGameActions action = GetBindedAction(dik);
                if (action != kQUICK_USE_1 && action != kQUICK_USE_2 && action != kQUICK_USE_3 &&
                    action != kQUICK_USE_4)
                    IR->IR_OnKeyboardPress(action);
            }
            return (false);
        }
    }

    return true;
}

bool CDialogHolder::IR_UIOnKeyboardRelease(int dik)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    // mouse click
    if (dik == MOUSE_1 || dik == MOUSE_2 || dik == MOUSE_3)
    {
        Fvector2 cp = GetUICursor().GetCursorPosition();
        EUIMessages action =
            (dik == MOUSE_1) ? WINDOW_LBUTTON_UP : (dik == MOUSE_2) ? WINDOW_RBUTTON_UP : WINDOW_CBUTTON_UP;
        if (TIR->OnMouseAction(cp.x, cp.y, action))
            return true;
    }

    if (TIR->OnKeyboardAction(dik, WINDOW_KEY_RELEASED))
        return true;

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnKeyboardRelease(GetBindedAction(dik));
            return (false);
        }
    }
    return true;
}

bool CDialogHolder::IR_UIOnTextInput(pcstr text)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    return TIR->OnTextInput(text);
}

bool CDialogHolder::IR_UIOnKeyboardHold(int dik)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    if (TIR->OnKeyboardAction(dik, WINDOW_KEY_HOLD))
        return true;

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnKeyboardHold(GetBindedAction(dik));
            return false;
        }
    }
    return true;
}

bool CDialogHolder::IR_UIOnMouseWheel(float x, float y)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    UpdateCursorVisibility();

    // Vertical scroll is in higher priority
    EUIMessages wheelMessage;
    if (y > 0)
        wheelMessage = WINDOW_MOUSE_WHEEL_UP;
    else if (y < 0)
        wheelMessage = WINDOW_MOUSE_WHEEL_DOWN;
    else if (x > 0)
        wheelMessage = WINDOW_MOUSE_WHEEL_RIGHT;
    else
        wheelMessage = WINDOW_MOUSE_WHEEL_LEFT;

    const Fvector2 pos = GetUICursor().GetCursorPosition();
    TIR->OnMouseAction(pos.x, pos.y, wheelMessage);
    return true;
}

bool CDialogHolder::IR_UIOnMouseMove(int dx, int dy)
{
    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    UpdateCursorVisibility();

    if (GetUICursor().IsVisible())
    {
        GetUICursor().UpdateCursorPosition({ (float)dx, (float)dy });
        Fvector2 cPos = GetUICursor().GetCursorPosition();
        TIR->OnMouseAction(cPos.x, cPos.y, WINDOW_MOUSE_MOVE);
    }
    else if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnMouseMove(dx, dy);
            return false;
        }
    };
    return true;
}

bool CDialogHolder::IR_UIOnControllerPress(int dik, const ControllerAxisState& state)
{
    if (dik > XR_CONTROLLER_BUTTON_INVALID && dik < XR_CONTROLLER_BUTTON_MAX)
    {
        return IR_UIOnKeyboardPress(dik);
    }

    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    if (TIR->OnControllerAction(dik, state, WINDOW_KEY_PRESSED))
        return true;

    // simulate mouse click
    if (GetUICursor().IsVisible())
    {
        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_MOVE:
        {
            return true;
        }
        case kUI_MOVE_SECONDARY:
        {
            if (TIR->StopAnyMove())
                return true;
            break;
        }
        case kUI_CLICK_1:
        {
            Fvector2 cp = GetUICursor().GetCursorPosition();
            TIR->OnMouseAction(cp.x, cp.y, WINDOW_LBUTTON_DOWN);
            return true;
        }
        case kUI_CLICK_2:
        {
            Fvector2 cp = GetUICursor().GetCursorPosition();
            TIR->OnMouseAction(cp.x, cp.y, WINDOW_RBUTTON_DOWN);
            return true;
        }
        } // switch (GetBindedAction(dik, EKeyContext::UI))
    }

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnControllerPress(dik, state);
            return false;
        }
    };
    return true;
}

bool CDialogHolder::IR_UIOnControllerRelease(int dik, const ControllerAxisState& state)
{
    if (dik > XR_CONTROLLER_BUTTON_INVALID && dik < XR_CONTROLLER_BUTTON_MAX)
    {
        return IR_UIOnKeyboardRelease(dik);
    }

    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    if (TIR->OnControllerAction(dik, state, WINDOW_KEY_RELEASED))
        return true;

    // simulate mouse click
    if (GetUICursor().IsVisible())
    {
        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_MOVE:
        {
            return true;
        }
        case kUI_MOVE_SECONDARY:
        {
            if (TIR->StopAnyMove())
                return true;
            break;
        }
        case kUI_CLICK_1:
        {
            Fvector2 cp = GetUICursor().GetCursorPosition();
            TIR->OnMouseAction(cp.x, cp.y, WINDOW_LBUTTON_UP);
            return true;
        }
        case kUI_CLICK_2:
        {
            Fvector2 cp = GetUICursor().GetCursorPosition();
            TIR->OnMouseAction(cp.x, cp.y, WINDOW_RBUTTON_UP);
            return true;
        }
        } // switch (GetBindedAction(dik, EKeyContext::UI))
    }

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnControllerRelease(dik, state);
            return false;
        }
    };
    return true;
}

bool CDialogHolder::IR_UIOnControllerHold(int dik, const ControllerAxisState& state)
{
    if (dik > XR_CONTROLLER_BUTTON_INVALID && dik < XR_CONTROLLER_BUTTON_MAX)
    {
        return IR_UIOnKeyboardHold(dik);
    }

    CUIDialogWnd* TIR = TopInputReceiver();
    if (!TIR)
        return false;
    if (!TIR->IR_process())
        return false;

    if (TIR->OnControllerAction(dik, state, WINDOW_KEY_HOLD))
        return true;

    if (GetUICursor().IsVisible())
    {
        switch (GetBindedAction(dik, EKeyContext::UI))
        {
        case kUI_MOVE:
        {
            if (state.magnitude < 0.9f)
                return true;

            FocusDirection direction;

            if (fis_zero(state.y))
            {
                if (state.x < 0)
                    direction = FocusDirection::Left;
                else
                    direction = FocusDirection::Right;
            }
            else if (state.y < 0)
            {
                if (fis_zero(state.x))
                    direction = FocusDirection::Up;
                else if (state.x < 0)
                    direction = FocusDirection::UpperLeft;
                else
                    direction = FocusDirection::UpperRight;
            }
            else
            {
                if (fis_zero(state.x)) // same x
                    direction = FocusDirection::Down;
                else if (state.x < 0)
                    direction = FocusDirection::LowerLeft;
                else
                    direction = FocusDirection::LowerRight;
            }

            auto& focus = UI().Focus();
            const auto focused = focus.GetFocused();
            const Fvector2 vec = focused ? focused->GetAbsoluteCenterPos() : UI().GetUICursor().GetCursorPosition();
            const auto [candidate, candidate2] = focus.FindClosestFocusable(vec, direction);

            if (candidate || candidate2)
            {
                focus.SetFocused(candidate ? candidate : candidate2);
            }
            return true;
        }
        case kUI_MOVE_SECONDARY:
        {
            if (TIR->StopAnyMove())
            {
                static float intensity = psCursorIntensityMin;

                if (state.magnitude > 0.1f)
                {
                    if (state.magnitude > 0.99f)
                        intensity += psCursorIntensityStep;
                    else
                        intensity -= psCursorIntensityStep;

                    clamp(intensity, psCursorIntensityMin, psCursorIntensityMax);
                }
                else
                {
                    intensity = psCursorIntensityMin;
                }

                const float scale = Device.fTimeDeltaReal * intensity * psControllerStickSensScale * 100.f;

                Fvector2 newPos = state.xy;
                newPos.mul(scale);
                GetUICursor().UpdateCursorPosition(newPos);

                Fvector2 cPos = GetUICursor().GetCursorPosition();
                TIR->OnMouseAction(cPos.x, cPos.y, WINDOW_MOUSE_MOVE);
                return true;
            }
            break;
        }
        case kUI_CLICK_1:
        case kUI_CLICK_2:
        {
            return true;
        }
        } // switch (GetBindedAction(dik, EKeyContext::UI))
    }

    if (!TIR->StopAnyMove() && g_pGameLevel)
    {
        IGameObject* O = Level().CurrentEntity();
        if (O)
        {
            IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(O));
            if (IR)
                IR->IR_OnControllerHold(dik, state);
            return false;
        }
    };
    return true;
}

bool CDialogHolder::FillDebugTree(const CUIDebugState& debugState)
{
#ifndef MASTER_GOLD
    if (m_input_receivers.empty())
        ImGui::BulletText("Input receivers: 0");
    else
    {
        if (ImGui::TreeNode(&m_input_receivers, "Input receivers: %zu", m_input_receivers.size()))
        {
            for (const auto& item : m_input_receivers)
                item.m_item->FillDebugTree(debugState);
            ImGui::TreePop();
        }
    }

    if (m_dialogsToRender.empty())
        ImGui::BulletText("Dialogs to render: 0");
    else
    {
        if (ImGui::TreeNode(&m_dialogsToRender, "Dialogs to render: %zu", m_dialogsToRender.size()))
        {
            for (const auto& item : m_dialogsToRender)
                item.wnd->FillDebugTree(debugState);
            ImGui::TreePop();
        }
    }
#endif
    return true;
}

void CDialogHolder::FillDebugInfo()
{
#ifndef MASTER_GOLD
    if (ImGui::CollapsingHeader(CDialogHolder::GetDebugType()))
    {
        ImGui::DragScalar("Cursor become visible time", ImGuiDataType_U32, &m_become_visible_time);
        ImGui::Checkbox("Foremost", &m_is_foremost);
    }
#endif
}
