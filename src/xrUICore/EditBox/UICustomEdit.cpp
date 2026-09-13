#include "pch.hpp"
#include "UICustomEdit.h"
#include "Lines/UILines.h"
#include "xrCore/Text/Utf8Utils.hpp"
#include "xrEngine/line_edit_control.h"
#include "xrEngine/xr_input.h"

// XXX: replace u32 and int with size_t

CUICustomEdit::CUICustomEdit() : CUIStatic("CUICustomEdit")
{
    m_editor_control = xr_new<text_editor::line_edit_control>(EDIT_BUF_SIZE);
    Init(EDIT_BUF_SIZE);

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

    UI().Focus().RegisterFocusable(this);
}

CUICustomEdit::~CUICustomEdit()
{
    xr_delete(m_editor_control);
    UI().Focus().UnregisterFocusable(this);
}

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

void CUICustomEdit::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    // someone else captured the keyboard
    if (msg == WINDOW_KEYBOARD_CAPTURE_LOST && m_bInputFocus)
    {
        CaptureFocus(false);
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    }
}

bool CUICustomEdit::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    //	if (m_bFocusByDbClick)
    {
        if (mouse_action == WINDOW_LBUTTON_DB_CLICK && !m_bInputFocus)
        {
            CaptureFocus(true);
        }
    }

    if (mouse_action == WINDOW_LBUTTON_DOWN && !m_bInputFocus)
    {
        CaptureFocus(true);
    }
    return false;
}

bool CUICustomEdit::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (!m_bInputFocus)
        return false;

    switch (keyboard_action)
    {
    case WINDOW_KEY_PRESSED:
        ec().on_key_press(dik);
        return true;

    case WINDOW_KEY_HOLD:
        ec().on_key_hold(dik);
        return true;

    case WINDOW_KEY_RELEASED:
        ec().on_key_release(dik);
        return true;
    }

    return false;
}

bool CUICustomEdit::OnTextInput(pcstr text)
{
    if (!m_bInputFocus)
        return false;

    ec().on_text_input(text);
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
        const float ui_width = GetWidth();

        pcstr cursor_str = ec().str_before_cursor();

        // Scroll left: skip whole codepoints until the tail fits in the edit box.
        pcstr istr = cursor_str;
        float str_length = font->SizeOf_(istr);
        UI().ClientToScreenScaledWidth(str_length);
        while (str_length > ui_width && *istr)
        {
            istr = XRay::Utf8::Next(istr);
            str_length = font->SizeOf_(istr);
            UI().ClientToScreenScaledWidth(str_length);
        }

        // The visible part of the full edit string starts at the same codepoint position
        // as the scrolled prefix (istr). Use codepoint distance to avoid landing on a
        // continuation byte if the edit buffer and the prefix buffer ever differ.
        const size_t skipped_codepoints = XRay::Utf8::DistanceCodepoints(cursor_str, istr);
        pcstr astr = XRay::Utf8::Advance(ec().str_edit(), skipped_codepoints);

        // Guard against a misaligned start if the edit buffer ever ends with an incomplete codepoint.
        while (*astr && XRay::Utf8::IsContinuationByte(static_cast<u8>(*astr)))
            ++astr;

        // Grow the visible substring by whole codepoints until it no longer fits.
        const size_t max_out_bytes = sizeof(m_out_str) - 1;
        size_t visible_bytes = 0;
        for (pcstr p = astr; *p; p = XRay::Utf8::Next(p))
        {
            size_t candidate_bytes = XRay::Utf8::Next(p) - astr;
            if (candidate_bytes > max_out_bytes)
            {
                candidate_bytes = max_out_bytes;
                strncpy_s(m_out_str, sizeof(m_out_str), astr, candidate_bytes);
                m_out_str[candidate_bytes] = '\0';

                str_length = font->SizeOf_(m_out_str);
                UI().ClientToScreenScaledWidth(str_length);

                if (visible_bytes == 0 || str_length < ui_width)
                    visible_bytes = candidate_bytes;
                break;
            }

            strncpy_s(m_out_str, sizeof(m_out_str), astr, candidate_bytes);
            m_out_str[candidate_bytes] = '\0';

            str_length = font->SizeOf_(m_out_str);
            UI().ClientToScreenScaledWidth(str_length);

            if (str_length < ui_width)
            {
                visible_bytes = candidate_bytes;
            }
            else if (visible_bytes == 0)
            {
                visible_bytes = candidate_bytes;
                break;
            }
            else
            {
                break;
            }
        }
        strncpy_s(m_out_str, sizeof(m_out_str), astr, visible_bytes);
        m_out_str[visible_bytes] = '\0';

        // Defensive: the visible substring must not end in the middle of a codepoint.
        // Use IsValid() rather than a trailing-continuation check: a completed
        // multi-byte codepoint (e.g. Cyrillic 'т' = D1 82) legitimately ends in a
        // continuation byte and must NOT be trimmed here.
        while (visible_bytes > 0 && !XRay::Utf8::IsValid(m_out_str))
        {
            --visible_bytes;
            m_out_str[visible_bytes] = '\0';
        }

        TextItemControl()->SetText(m_out_str);

        if (TextItemControl()->IsPasswordMode())
        {
            const size_t sz = XRay::Utf8::LengthCodepoints(istr);
            xr_string passText(sz, '*');
            m_dx_cur = font->SizeOf_(passText.c_str()); // cursor_str
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
        CaptureFocus(false);
        GetParent()->SetKeyboardCapture(this, false);
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_CANCEL, NULL);
    }
}

void CUICustomEdit::press_commit()
{
    CaptureFocus(false);
    GetParent()->SetKeyboardCapture(this, false);
    GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
}

void CUICustomEdit::press_tab()
{
    if (!m_next_focus_capturer)
        return;

    CaptureFocus(false);
    GetParent()->SetKeyboardCapture(this, false);
    GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    GetParent()->SetKeyboardCapture(m_next_focus_capturer, true);
    m_next_focus_capturer->CaptureFocus(true);
}

void CUICustomEdit::CaptureFocus(bool bCapture)
{
    if (bCapture)
    {
        GetParent()->SetKeyboardCapture(this, true);
        ec().on_ir_capture();
    }
    else
    {
        ec().on_ir_release();
    }

    m_bInputFocus = bCapture;
}
