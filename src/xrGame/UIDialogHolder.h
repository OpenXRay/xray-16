#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/_flags.h"
#include "xrEngine/pure.h"
#include "xrUICore/ui_debug.h"

#include <SDL3/SDL.h>

class CUIDialogWnd;
class CUIWindow;

class dlgItem
{
public:
    dlgItem(CUIWindow* pWnd);
    CUIWindow* wnd;
    bool enabled;
    bool operator<(const dlgItem& itm) const;
};

class recvItem
{
public:
    enum
    {
        eCrosshair = (1 << 0),
        eIndicators = (1 << 1)
    };
    recvItem(CUIDialogWnd*);
    CUIDialogWnd* m_item;
    Flags8 m_flags;
};

class CDialogHolder : public pureFrame, public CUIDebuggable
{
    // dialogs
    xr_vector<recvItem> m_input_receivers;
    xr_vector<dlgItem> m_dialogsToRender;
    xr_vector<dlgItem> m_dialogsToRender_new;
    bool m_b_in_update;

    void StartMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators);
    void StopMenu(CUIDialogWnd* pDialog);

protected:
    void DoRenderDialogs();
    void CleanInternals();

public:
    CDialogHolder();
    ~CDialogHolder() override;

    // dialogs
    void OnExternalHideIndicators();
    CUIDialogWnd* TopInputReceiver();
    void AddDialogToRender(CUIWindow* pDialog);
    void RemoveDialogToRender(CUIWindow* pDialog);
    void SetMainInputReceiver(CUIDialogWnd* ir, bool _find_remove);
    virtual void OnFrame();
    virtual bool UseIndicators() { return true; }
    virtual void StartDialog(CUIDialogWnd* pDialog, bool bDoHideIndicators);
    virtual void StopDialog(CUIDialogWnd* pDialog);
    virtual void StartStopMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators);
    virtual bool IgnorePause() { return false; }

    virtual bool IR_UIOnMouseMove(int dx, int dy);
    virtual bool IR_UIOnMouseWheel(int x, int y);

    virtual bool IR_UIOnKeyboardPress(int dik);
    virtual bool IR_UIOnKeyboardRelease(int dik);
    virtual bool IR_UIOnKeyboardHold(int dik);
    virtual bool IR_UIOnTextInput(pcstr text);

    virtual bool IR_UIOnControllerPress(int dik, float x, float y);
    virtual bool IR_UIOnControllerRelease(int dik, float x, float y);
    virtual bool IR_UIOnControllerHold(int dik, float x, float y);

    pcstr GetDebugType() override { return "CDialogHolder"; }
    bool FillDebugTree(const CUIDebugState& debugState) override;
    void FillDebugInfo() override;
};
