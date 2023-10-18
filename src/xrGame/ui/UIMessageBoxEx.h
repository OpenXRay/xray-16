#pragma once
#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUIMessageBox;

class CUIMessageBoxEx final : public CUIDialogWnd, public CUIWndCallback
{
public:
    CUIMessageBoxEx();
    virtual ~CUIMessageBoxEx();
    void SetText(LPCSTR text);
    LPCSTR GetText();
    virtual bool InitMessageBox(LPCSTR xml_template);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    LPCSTR GetHost();
    LPCSTR GetPassword();

    void SetTextEditURL(LPCSTR text);
    LPCSTR GetTextEditURL();

    CUIWndCallback::void_function func_on_ok;
    CUIWndCallback::void_function func_on_no;
    void OnOKClicked(CUIWindow*, void*);
    void OnNOClicked(CUIWindow*, void*);

    virtual bool NeedCenterCursor() const { return false; }
    CUIMessageBox* m_pMessageBox;

    pcstr GetDebugType() override { return "CUIMessageBoxEx"; }
};
