////////////////////////////////////////////////////////////////////////////
// Module : line_edit_control.h
// Created : 21.02.2008
// Author : Evgeniy Sokolov
// Description : line edit control class
////////////////////////////////////////////////////////////////////////////

#ifndef LINE_EDIT_CONTROL_H_INCLUDED
#define LINE_EDIT_CONTROL_H_INCLUDED

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
    line_edit_control(u32 str_buffer_size);
    void init(u32 str_buffer_size, init_mode mode = im_standart);
    ~line_edit_control();

    void clear_states();
    void on_key_press(int dik);
    void on_key_hold(int dik);
    void on_key_release(int dik);
    void on_frame();

    void assign_callback(int const dik, key_state state, Callback const& callback);

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

private:
    line_edit_control(line_edit_control const&);
    line_edit_control const& operator=(line_edit_control const&);

    void update_key_states();
    void update_bufs();

    void xr_stdcall undo_buf();
    void xr_stdcall select_all_buf();
    void xr_stdcall flip_insert_mode();

    void xr_stdcall copy_to_clipboard();
    void xr_stdcall paste_from_clipboard();
    void xr_stdcall cut_to_clipboard();

    void xr_stdcall move_pos_home();
    void xr_stdcall move_pos_end();
    void xr_stdcall move_pos_left();
    void xr_stdcall move_pos_right();
    void xr_stdcall move_pos_left_word();
    void xr_stdcall move_pos_right_word();

    void xr_stdcall delete_selected_back();
    void xr_stdcall delete_selected_forward();
    void xr_stdcall delete_word_back();
    void xr_stdcall delete_word_forward();
    void xr_stdcall SwitchKL();

    void assign_char_pairs(init_mode mode);
    void create_key_state(int const dik, key_state state);
    void create_char_pair(int const dik, char c, char c_shift, bool translate = false);

    void clear_inserted();
    bool empty_inserted();

    void add_inserted_text();

    void delete_selected(bool back);
    void compute_positions();
    void clamp_cur_pos();

private:
    Base* m_actions[SDL_NUM_SCANCODES];

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
    int m_buffer_size;

    int m_cur_pos;
    int m_select_start;
    int m_p1;
    int m_p2;

    float m_accel;
    float m_cur_time;
    float m_rep_time;
    float m_last_key_time;
    u32 m_last_frame_time;
    u32 m_last_changed_frame;

    Flags32 m_key_state;

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
