////////////////////////////////////////////////////////////////////////////
// Module : line_edit_control.h
// Created : 21.02.2008
// Author : Evgeniy Sokolov
// Description : line edit control class
////////////////////////////////////////////////////////////////////////////

#ifndef LINE_EDIT_CONTROL_H_INCLUDED
#define LINE_EDIT_CONTROL_H_INCLUDED

#include "xr_input.h"

namespace text_editor
{
void remove_spaces(pstr str); // in & out
void split_cmd(pstr first, pstr second, pcstr str);

class base;

enum key_state // Flags32
{
    ks_free = u32(0),
    ks_LShift = u32(1) << 0,
    ks_RShift = u32(1) << 1,
    ks_LCtrl = u32(1) << 2,
    ks_RCtrl = u32(1) << 3,
    ks_LAlt = u32(1) << 4,
    ks_RAlt = u32(1) << 5,
    ks_CapsLock = u32(1) << 6,

    ks_Shift = u32(ks_LShift | ks_RShift),
    ks_Ctrl = u32(ks_LCtrl | ks_RCtrl),
    ks_Alt = u32(ks_LAlt | ks_RAlt),

    ks_force = u32(-1)

}; // enum key_state

enum init_mode : u32
{
    im_standart = 0,
    im_number_only,
    im_read_only,
    im_file_name_mode, // not "/\\:*?\"<>|^()[]%"

    im_count
}; // init_mode

class ENGINE_API line_edit_control
{
    using Base = text_editor::base;
    using Callback = fastdelegate::FastDelegate0<void>;

public:
    line_edit_control(size_t str_buffer_size);
    void init(size_t str_buffer_size, init_mode mode = im_standart);
    ~line_edit_control();

    void clear_states();

    void on_ir_capture();
    void on_ir_release();

    void on_key_press(int dik);
    void on_key_hold(int dik);
    void on_key_release(int dik);
    void on_text_input(const char *text);

    void on_frame();

    void assign_callback(int const dik, key_state state, Callback const& callback);
    void remove_callback(int dik);

    void insert_character(char c);

    bool get_key_state(key_state mask) const { return mask ? !!m_key_state.test(mask) : true; }
    void set_key_state(key_state mask, bool value) { m_key_state.set(mask, value); }
    bool cursor_view() const { return m_cursor_view; }
    bool need_update() const { return m_need_update; }
    pcstr str_edit() const { return m_edit_str; }
    pcstr str_before_cursor() const { return m_buf0; }
    pcstr str_before_mark() const { return m_buf1; }
    pcstr str_mark() const { return m_buf2; }
    pcstr str_after_mark() const { return m_buf3; }
    void set_edit(pcstr str);
    void set_selected_mode(bool status) { m_unselected_mode = !status; }
    bool get_selected_mode() const { return !m_unselected_mode; }

    bool char_is_allowed(char c);

private:
    line_edit_control(line_edit_control const&);
    line_edit_control const& operator=(line_edit_control const&);

    void update_key_states();
    void update_bufs();

    void undo_buf();
    void select_all_buf();
    void flip_insert_mode();

    void copy_to_clipboard();
    void paste_from_clipboard();
    void cut_to_clipboard();

    void move_pos_home();
    void move_pos_end();
    void move_pos_left();
    void move_pos_right();
    void move_pos_left_word();
    void move_pos_right_word();

    void delete_selected_back();
    void delete_selected_forward();
    void delete_word_back();
    void delete_word_forward();
    void SwitchKL();

    void create_key_state(int const dik, key_state state);

    void clear_inserted();
    bool empty_inserted() const;

    void add_inserted_text();

    void delete_selected(bool back);
    void compute_positions();
    void clamp_cur_pos();

private:
    Base* m_actions[CInput::COUNT_KB_BUTTONS];

    char* m_edit_str;
    char* m_undo_buf;
    char* m_inserted;
    char* m_buf0;
    char* m_buf1;
    char* m_buf2;
    char* m_buf3;

    enum
    {
        MIN_BUF_SIZE = 8,
        MAX_BUF_SIZE = 4096
    };
    size_t m_buffer_size;

    size_t m_cur_pos;
    size_t m_inserted_pos;
    size_t m_select_start;
    size_t m_p1;
    size_t m_p2;

    float m_accel;
    float m_cur_time;
    float m_rep_time;
    float m_last_key_time;
    u32 m_last_frame_time;
    u32 m_last_changed_frame;

    Flags32 m_key_state;
    init_mode m_current_mode;

    bool m_hold_mode;
    bool m_insert_mode;
    bool m_repeat_mode;
    bool m_mark;
    bool m_cursor_view;
    bool m_need_update;
    bool m_unselected_mode;
}; // class line_edit_control

} // namespace text_editor

#endif // ##ifndef LINE_EDIT_CONTROL_H_INCLUDED
