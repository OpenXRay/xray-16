#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "UIDialogHolder.h"

class CDialogHolder;

class CUIDialogWnd : public CUIWindow
{
private:
    typedef CUIWindow inherited;
    CDialogHolder* m_pParentHolder;

protected:
public:
    bool m_bWorkInPause;
    CUIDialogWnd();
    virtual ~CUIDialogWnd();

    virtual void Show(bool status);

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnKeyboardHold(int dik);

    CDialogHolder* GetHolder() { return m_pParentHolder; }
    void SetHolder(CDialogHolder* h) { m_pParentHolder = h; }
    virtual bool StopAnyMove() { return true; }
    virtual bool NeedCursor() const { return true; }
    virtual bool NeedCenterCursor() const { return true; }
    virtual bool WorkInPause() const { return m_bWorkInPause; }
    virtual bool Dispatch(int cmd, int param) { return true; }
    void ShowDialog(bool bDoHideIndicators);
    void HideDialog();

    virtual bool IR_process();
};
