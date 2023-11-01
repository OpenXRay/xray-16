////////////////////////////////////////////////////////////////////////////
// Module : line_editor.cpp
// Created : 22.02.2008
// Author : Evgeniy Sokolov
// Description : line editor class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "line_editor.h"

namespace text_editor
{
line_editor::line_editor(size_t str_buffer_size) : m_control(str_buffer_size)
{
}

void line_editor::on_frame() { m_control.on_frame(); }
void line_editor::IR_OnKeyboardPress(int dik) { m_control.on_key_press(dik); }
void line_editor::IR_OnKeyboardHold(int dik) { m_control.on_key_hold(dik); }
void line_editor::IR_OnKeyboardRelease(int dik) { m_control.on_key_release(dik); }
void line_editor::IR_OnTextInput(const char *text) { m_control.on_text_input(text); }

void line_editor::IR_OnActivate()
{
    m_control.on_ir_capture();
    IInputReceiver::IR_OnDeactivate();
}

void line_editor::IR_OnDeactivate()
{
    m_control.on_ir_release();
    IInputReceiver::IR_OnDeactivate();
}
} // namespace text_editor
