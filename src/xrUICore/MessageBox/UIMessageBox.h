#pragma once
#include "xrUICore/Static/UIStatic.h"

class CUI3tButton;
class CUIEditBox;

class XRUICORE_API CUIMessageBox : public CUIStatic
{
private:
    typedef CUIStatic inherited;

public:
    CUIMessageBox();
    virtual ~CUIMessageBox();

    //разновидности MessageBox
    typedef enum {
        MESSAGEBOX_OK,
        MESSAGEBOX_INFO,
        MESSAGEBOX_YES_NO,
        MESSAGEBOX_YES_NO_CANCEL,
        MESSAGEBOX_DIRECT_IP,
        MESSAGEBOX_PASSWORD,
        MESSAGEBOX_RA_LOGIN,
        MESSAGEBOX_QUIT_WINDOWS,
        MESSAGEBOX_QUIT_GAME,
        MESSAGEBOX_YES_NO_COPY
    } E_MESSAGEBOX_STYLE;

    virtual void InitMessageBox(LPCSTR xml_template);
    void Clear();
    virtual void SetText(LPCSTR str);
    virtual LPCSTR GetText();
    LPCSTR GetHost();
    LPCSTR GetPassword();
    LPCSTR GetUserPassword();
    void SetUserPasswordMode(bool);
    void SetPasswordMode(bool);
    E_MESSAGEBOX_STYLE GetBoxStyle() { return m_eMessageBoxStyle; };
    void SetTextEditURL(LPCSTR text);
    LPCSTR GetTextEditURL();

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    void OnYesOk();

protected:
    xr_string m_ret_val;
    CUI3tButton* m_UIButtonYesOk;
    CUI3tButton* m_UIButtonNo;
    CUI3tButton* m_UIButtonCancel;
    CUI3tButton* m_UIButtonCopy;

    CUIStatic* m_UIStaticPicture;
    CUITextWnd* m_UIStaticText;
    CUITextWnd* m_UIStaticHost;
    CUITextWnd* m_UIStaticPass;
    CUITextWnd* m_UIStaticUserPass;
    CUIEditBox* m_UIEditHost;
    CUIEditBox* m_UIEditPass;
    CUIEditBox* m_UIEditUserPass;
    CUIEditBox* m_UIEditURL;

    E_MESSAGEBOX_STYLE m_eMessageBoxStyle;
};
