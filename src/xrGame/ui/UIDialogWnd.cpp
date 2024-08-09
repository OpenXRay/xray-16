#include "StdAfx.h"
#include "UIDialogWnd.h"

CUIDialogWnd::CUIDialogWnd(pcstr window_name) : CUIWindow(window_name)
{
    m_pParentHolder = NULL;
    m_bWorkInPause = false;
    m_bShowMe = false;
}

CUIDialogWnd::~CUIDialogWnd() {}
void CUIDialogWnd::Show(bool status)
{
    inherited::Show(status);

    if (status)
        ResetAll();
}

bool CUIDialogWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (!IR_process())
        return false;
    if (inherited::OnKeyboardAction(dik, keyboard_action))
        return true;
    return false;
}

bool CUIDialogWnd::OnControllerAction(int axis, float x, float y, EUIMessages controller_action)
{
    if (!IR_process())
        return false;
    if (inherited::OnControllerAction(axis, x, y, controller_action))
        return true;
    return false;
}

bool CUIDialogWnd::IR_process()
{
    if (!IsEnabled())
        return false;

    if (GetHolder() && GetHolder()->IgnorePause())
        return true;

    if (Device.Paused() && !WorkInPause())
        return false;

    return true;
}

void CUIDialogWnd::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (ImGui::CollapsingHeader(CUIDialogWnd::GetDebugType()))
    {
        ImGui::LabelText("Current holder", "%s", m_pParentHolder ? m_pParentHolder->GetDebugType() : "none");
        ImGui::LabelText("Work in pause", m_bWorkInPause ? "true" : "false");
    }
#endif
}

CDialogHolder* CurrentDialogHolder();

void CUIDialogWnd::ShowOrHideDialog(bool bDoHideIndicators)
{
    if (IsShown())
        GetHolder()->StopDialog(this);
    else
        CurrentDialogHolder()->StartDialog(this, bDoHideIndicators);
}

void CUIDialogWnd::ShowDialog(bool bDoHideIndicators)
{
    if (!IsShown())
        CurrentDialogHolder()->StartDialog(this, bDoHideIndicators);
}

void CUIDialogWnd::HideDialog()
{
    if (GetHolder() && IsShown())
        GetHolder()->StopDialog(this);
}
