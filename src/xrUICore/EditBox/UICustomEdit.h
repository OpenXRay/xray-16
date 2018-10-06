#pragma once
#include "xrUICore/Static/UIStatic.h"

namespace text_editor
{
class ENGINE_API line_edit_control;
enum init_mode : u32;
};

class XRUICORE_API CUICustomEdit : public CUIStatic
{
private:
    typedef CUIStatic inherited;

public:
    CUICustomEdit();
    virtual ~CUICustomEdit();

    void Init(u32 max_char_count, bool number_only_mode = false, bool read_mode = false, bool fn_mode = false);

    virtual void InitCustomEdit(Fvector2 pos, Fvector2 size);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnKeyboardHold(int dik);

    virtual void OnFocusLost();
    virtual void Update();
    virtual void Draw();
    virtual void Show(bool status);

    void CaptureFocus(bool bCapture);
    void SetNextFocusCapturer(CUICustomEdit* next_capturer) { m_next_focus_capturer = next_capturer; };
    void ClearText();
    virtual void SetText(LPCSTR str);
    virtual LPCSTR GetText() const;

    virtual void Enable(bool status);

    void SetPasswordMode(bool mode = true);

protected:
    void Register_callbacks();

    void xr_stdcall nothing();
    void xr_stdcall press_escape();
    void xr_stdcall press_commit();
    void xr_stdcall press_tab();

protected:
    typedef fastdelegate::FastDelegate0<void> Callback;

    enum
    {
        EDIT_BUF_SIZE = 256
    };
    text_editor::line_edit_control* m_editor_control;
    text_editor::line_edit_control& ec();
    text_editor::line_edit_control const& ec() const;

    u32 m_last_key_state_time;
    char m_out_str[EDIT_BUF_SIZE];
    float m_dx_cur;

    bool m_bInputFocus;
    bool m_force_update;
    bool m_read_mode;

    CUICustomEdit* m_next_focus_capturer;
};
