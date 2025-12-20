#ifndef UISERVER_INFO_INCLUDED
#define UISERVER_INFO_INCLUDED

#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUIStatic;
class CUIScrollView;
class CUI3tButton;

class CUIServerInfo final : public CUIDialogWnd, public CUIWndCallback
{
public:
    CUIServerInfo();

    void SetServerLogo(u8 const* data_ptr, u32 data_size);
    void SetServerRules(u8 const* data_ptr, u32 data_size);

    [[nodiscard]]
    bool HasInfo() const { return m_dds_file_created; }

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = nullptr);
    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;

    pcstr GetDebugType() override { return "CUIServerInfo"; }

private:
    void OnSpectatorBtnClick(CUIWindow* w, void* d);
    void OnNextBtnClick(CUIWindow* w, void* d);

    static char const* tmp_logo_file_name;

    bool m_dds_file_created{};
    CUIStatic* m_text_body{};
    CUIStatic* m_image{};
}; // class CUIServerInfo

#endif //#ifndef UISERVER_INFO_INCLUDED
