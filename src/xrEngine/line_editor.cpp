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
line_editor::line_editor(u32 str_buffer_size) : m_control(str_buffer_size) {}
line_editor::~line_editor() {}
void line_editor::on_frame() { m_control.on_frame(); }
void line_editor::IR_OnKeyboardPress(int dik) {m_control.on_key_press((SDL_Scancode)dik); }
void line_editor::IR_OnKeyboardHold(int dik) {m_control.on_key_hold((SDL_Scancode)dik); }
void line_editor::IR_OnKeyboardRelease(int dik) {m_control.on_key_release((SDL_Scancode)dik); }
} // namespace text_editor
