#include "StdAfx.h"
#include "UIDialogWnd.h"

CUIDialogWnd::CUIDialogWnd()
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

void CUIDialogWnd::OnInputActivate()
{
    SendMessage(this, WINDOW_INPUT_ACTIVATE);
}

bool CUIDialogWnd::IR_process()
{
    if (!IsEnabled())
        return false;

    if (GetHolder()->IgnorePause())
        return true;

    if (Device.Paused() && !WorkInPause())
        return false;

    return true;
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
    if (IsShown())
        GetHolder()->StopDialog(this);
}
