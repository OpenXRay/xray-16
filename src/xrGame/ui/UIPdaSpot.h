#pragma once
#ifdef COC_USER_SPOT
#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUIXml;
class CUI3tButton;
class CUIStatic;
class CUIEditBox;
class UIHint;

class CUIPdaSpot : public CUIDialogWnd, public CUIWndCallback
{
    typedef CUIDialogWnd base_class;

    CUIStatic* m_background;
    CUIEditBox* m_editBox;
    CUI3tButton* m_btn_ok;
    CUI3tButton* m_btn_cancel;

    bool m_mainWnd;
    LPCSTR m_levelName;
    Fvector m_position;
    u16 m_spotID;
    shared_str m_spotType;

public:
    CUIPdaSpot();
    ~CUIPdaSpot();

    void Init(u16 spot_id, LPCSTR level_name, Fvector pos, bool main_wnd);
    void InitControls();

    void xr_stdcall OnAdd(CUIWindow* w, void* d);
    void xr_stdcall OnApply(CUIWindow* w, void* d);
    void xr_stdcall OnExit(CUIWindow* w, void* d);
    void Exit();
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
};
#endif
