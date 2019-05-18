#include "pch.hpp"
#include "UICustomEdit.h"
#include "Lines/UILines.h"
#include "xrEngine/line_edit_control.h"
#include "xrEngine/xr_input.h"

CUICustomEdit::CUICustomEdit()
{
    m_editor_control = new text_editor::line_edit_control((u32)EDIT_BUF_SIZE);
    Init((u32)EDIT_BUF_SIZE);

    TextItemControl()->SetVTextAlignment(valCenter);
    TextItemControl()->SetTextComplexMode(false);
    TextItemControl()->SetColoringMode(false);
    TextItemControl()->SetCutWordsMode(true);
    TextItemControl()->SetUseNewLineMode(false);

    m_out_str[0] = '\0';
    m_dx_cur = 0.0f;
    m_read_mode = false;
    m_force_update = true;
    m_last_key_state_time = 0;
    m_next_focus_capturer = NULL;
}

CUICustomEdit::~CUICustomEdit() { xr_delete(m_editor_control); }
text_editor::line_edit_control& CUICustomEdit::ec()
{
    VERIFY(m_editor_control);
    return *m_editor_control;
}

text_editor::line_edit_control const& CUICustomEdit::ec() const
{
    VERIFY(m_editor_control);
    return *m_editor_control;
}

void CUICustomEdit::Register_callbacks()
{
    ec().assign_callback(SDL_SCANCODE_ESCAPE, text_editor::ks_free, Callback(this, &CUICustomEdit::press_escape));
    ec().assign_callback(SDL_SCANCODE_RETURN, text_editor::ks_free, Callback(this, &CUICustomEdit::press_commit));
    ec().assign_callback(SDL_SCANCODE_KP_ENTER, text_editor::ks_free, Callback(this, &CUICustomEdit::press_commit));
    ec().assign_callback(SDL_SCANCODE_GRAVE, text_editor::ks_free, Callback(this, &CUICustomEdit::nothing));
    ec().assign_callback(SDL_SCANCODE_TAB, text_editor::ks_free, Callback(this, &CUICustomEdit::press_tab));
}

void CUICustomEdit::Init(u32 max_char_count, bool number_only_mode, bool read_mode, bool fn_mode)
{
    if (read_mode)
    {
        m_editor_control->init(max_char_count, text_editor::im_read_only);
        m_editor_control->set_selected_mode(true);
        m_read_mode = true;
    }
    else
    {
        if (number_only_mode)
        {
            m_editor_control->init(max_char_count, text_editor::im_number_only);
        }
        else if (fn_mode)
        {
            m_editor_control->init(max_char_count, text_editor::im_file_name_mode);
        }
        else
        {
            m_editor_control->init(max_char_count);
        }
        m_editor_control->set_selected_mode(false);
        m_read_mode = false;
    }

    Register_callbacks();
    ClearText();

    m_bInputFocus = false;
}

void CUICustomEdit::InitCustomEdit(Fvector2 pos, Fvector2 size)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);
}

void CUICustomEdit::SetPasswordMode(bool mode) { TextItemControl()->SetPasswordMode(mode); }
void CUICustomEdit::OnFocusLost() { inherited::OnFocusLost(); }
void CUICustomEdit::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    //кто-то другой захватил клавиатуру
    if (msg == WINDOW_KEYBOARD_CAPTURE_LOST && m_bInputFocus)
    {
        m_bInputFocus = false;
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    }
}

bool CUICustomEdit::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    //	if (m_bFocusByDbClick)
    {
        if (mouse_action == WINDOW_LBUTTON_DB_CLICK && !m_bInputFocus)
        {
            GetParent()->SetKeyboardCapture(this, true);
            m_bInputFocus = true;
        }
    }

    if (mouse_action == WINDOW_LBUTTON_DOWN && !m_bInputFocus)
    {
        GetParent()->SetKeyboardCapture(this, true);
        m_bInputFocus = true;
    }
    return false;
}

bool CUICustomEdit::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (!m_bInputFocus)
    {
        return false;
    }

    if (keyboard_action == WINDOW_KEY_PRESSED)
    {
        ec().on_key_press(dik);
        return true;
    }

    if (keyboard_action == WINDOW_KEY_RELEASED)
    {
        ec().on_key_release(dik);
        return true;
    }
    return false;
}

bool CUICustomEdit::OnKeyboardHold(int dik)
{
    if (!m_bInputFocus)
    {
        return false;
    }

    ec().on_key_hold(dik);
    return true;
}

void CUICustomEdit::Update()
{
    ec().on_frame();

    if (!ec().get_key_state(text_editor::ks_force))
    {
        m_last_key_state_time = Device.dwTimeGlobal;
    }

    inherited::Update();
}

void CUICustomEdit::Draw()
{
    Fvector2 pos, out;
    GetAbsolutePos(pos);
    CGameFont* font = TextItemControl()->m_pFont;

    if (ec().need_update() || m_force_update)
    {
        float ui_width = GetWidth();

        LPCSTR cursor_str = ec().str_before_cursor();
        u32 cursor_str_size = xr_strlen(cursor_str);

        LPCSTR istr = cursor_str;
        float str_length = font->SizeOf_(istr);
        UI().ClientToScreenScaledWidth(str_length);

        u32 ix = 0;
        while ((str_length > ui_width) && (ix < cursor_str_size))
        {
            istr = cursor_str + ix;
            str_length = font->SizeOf_(istr);
            UI().ClientToScreenScaledWidth(str_length);
            ++ix;
        }
        istr = cursor_str + ix;
        LPCSTR astr = ec().str_edit() + ix;
        u32 str_size = xr_strlen(ec().str_edit());

        u32 jx = 1;
        strncpy_s(m_out_str, sizeof(m_out_str), astr, jx);

        str_length = font->SizeOf_(m_out_str);
        UI().ClientToScreenScaledWidth(str_length);

        while ((str_length < ui_width) && (jx < str_size - ix))
        {
            strncpy_s(m_out_str, sizeof(m_out_str), astr, jx);
            str_length = font->SizeOf_(m_out_str);
            UI().ClientToScreenScaledWidth(str_length);
            ++jx;
        }
        strncpy_s(m_out_str, sizeof(m_out_str), astr, jx);

        TextItemControl()->SetText(m_out_str);

        if (TextItemControl()->IsPasswordMode())
        {
            string256 passText;
            shared_str str(istr);
            int sz = (int)str.size();
            for (int i = 0; i < sz; i++)
                passText[i] = '*';
            passText[sz] = 0;
            m_dx_cur = font->SizeOf_(passText); // cursor_str
        }
        else
            m_dx_cur = font->SizeOf_(istr); // cursor_str

        m_force_update = false;
    }

    inherited::Draw();

    if (m_bInputFocus) // draw cursor here
    {
        out.x = pos.x + 0.0f + TextItemControl()->m_TextOffset.x + TextItemControl()->GetIndentByAlign();
        out.y = pos.y + 2.0f + TextItemControl()->m_TextOffset.y + TextItemControl()->GetVIndentByAlign();
        UI().ClientToScreenScaled(out);

        out.x += m_dx_cur; // cursor_str

        font->Out(out.x, out.y, "_");
    }
    font->OnRender();
}

void CUICustomEdit::Show(bool status)
{
    m_force_update = true;
    inherited::Show(status);
}

void CUICustomEdit::ClearText() { ec().set_edit(""); }
void CUICustomEdit::SetText(LPCSTR str) { ec().set_edit(str); }
LPCSTR CUICustomEdit::GetText() const { return ec().str_edit(); }
void CUICustomEdit::Enable(bool status)
{
    inherited::Enable(status);
    if (!status)
    {
        GetMessageTarget()->SendMessage(this, WINDOW_KEYBOARD_CAPTURE_LOST);
    }
}

// =======================================================

void CUICustomEdit::nothing(){};

void CUICustomEdit::press_escape()
{
    if (xr_strlen(ec().str_edit()) != 0)
    {
        if (!m_read_mode)
        {
            ec().set_edit("");
        }
    }
    else
    {
        m_bInputFocus = false;
        GetParent()->SetKeyboardCapture(this, false);
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_CANCEL, NULL);
    }
}

void CUICustomEdit::press_commit()
{
    m_bInputFocus = false;
    GetParent()->SetKeyboardCapture(this, false);
    GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
}

void CUICustomEdit::press_tab()
{
    if (!m_next_focus_capturer)
        return;

    m_bInputFocus = false;
    GetParent()->SetKeyboardCapture(this, false);
    GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    GetParent()->SetKeyboardCapture(m_next_focus_capturer, true);
    m_next_focus_capturer->CaptureFocus(true);
}

void CUICustomEdit::CaptureFocus(bool bCapture)
{
    if (bCapture)
        GetParent()->SetKeyboardCapture(this, true);

    m_bInputFocus = bCapture;
}
