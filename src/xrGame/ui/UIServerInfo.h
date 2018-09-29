#ifndef UISERVER_INFO_INCLUDED
#define UISERVER_INFO_INCLUDED

#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUIStatic;
class CUIScrollView;
class CUI3tButton;
class CUI3tButton;
class CUITextWnd;

class CUIServerInfo : public CUIDialogWnd, public CUIWndCallback
{
public:
    CUIServerInfo();
    virtual ~CUIServerInfo();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    void Init();
    void SetServerLogo(u8 const* data_ptr, u32 const data_size);
    void SetServerRules(u8 const* data_ptr, u32 const data_size);
    bool HasInfo() { return m_dds_file_created; };
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

private:
    void InitCallbacks();

    void xr_stdcall OnSpectatorBtnClick(CUIWindow* w, void* d);
    void xr_stdcall OnNextBtnClick(CUIWindow* w, void* d);

    static char const* tmp_logo_file_name;

    bool m_dds_file_created;
    CUIStatic* m_caption;
    CUIStatic* m_background;
    CUIScrollView* m_text_desc;
    CUITextWnd* m_text_body;
    CUIStatic* m_image;
    CUI3tButton* m_btn_spectator;
    CUI3tButton* m_btn_next;
}; // class CUIServerInfo

#endif //#ifndef UISERVER_INFO_INCLUDED
