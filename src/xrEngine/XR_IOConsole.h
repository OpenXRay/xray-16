// XR_IOConsole.h: interface for the CConsole class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// refs
class ENGINE_API IConsole_Command;

class ENGINE_API CConsole :
    public pureFrame,
    public IInputReceiver,
    public IEventReceiver,
    public IUserConfigHandler
{
public:
    struct TipString
    {
        shared_str text;
        int HL_start; // Highlight
        int HL_finish;

        TipString()
        {
            text._set("");
            HL_start = 0;
            HL_finish = 0;
        }
        TipString(shared_str const& tips_text, int start_pos, int finish_pos)
        {
            text._set(tips_text);
            HL_start = start_pos;
            HL_finish = finish_pos;
        }
        TipString(pcstr tips_text, int start_pos, int finish_pos)
        {
            text._set(tips_text);
            HL_start = start_pos;
            HL_finish = finish_pos;
        }
        TipString(shared_str const& tips_text)
        {
            text._set(tips_text);
            HL_start = 0;
            HL_finish = 0;
        }
        bool operator==(shared_str const& tips_text) const { return text == tips_text; }
    };

    struct str_pred
    {
        bool operator()(const char* x, const char* y) const { return xr_strcmp(x, y) < 0; }
    };

    using vecCMD     = xr_map<pcstr, IConsole_Command*, str_pred>;
    using vecHistory = xr_vector<shared_str>;
    using vecTips    = xr_vector<shared_str>;
    using vecTipsEx  = xr_vector<TipString>;

    enum
    {
        CONSOLE_BUF_SIZE = 1024
    };
    enum
    {
        VIEW_TIPS_COUNT = 14,
        MAX_TIPS_COUNT = 220
    };

protected:
    bool m_disable_tips;

private:
    EVENT eConsole;

    vecHistory m_cmd_history;
    u32 m_cmd_history_max;
    int m_cmd_history_idx;
    shared_str m_last_cmd;

    vecTips m_temp_tips;
    vecTipsEx m_tips;
    u32 m_tips_mode{};
    shared_str m_cur_cmd;
    int m_select_tip;
    int m_start_tip;
    u32 m_prev_length_str{};

public:
    CConsole(const CConsole&) = delete;
    CConsole(CConsole&&) = delete;

    CConsole& operator=(const CConsole&) = delete;
    CConsole& operator=(CConsole&&) = delete;

public:
    CConsole();
    ~CConsole() override;

    virtual void Initialize();
    virtual void Destroy();

    virtual void OnDeviceInitialize() {}

    void OnFrame() override;
    void OnEvent(EVENT E, u64 P1, u64 P2) override;

    void IR_OnKeyboardPress(int key) override;
    void IR_OnKeyboardRelease(int key) override;
    void IR_OnKeyboardHold(int key) override;
    void IR_OnTextInput(pcstr text) override;

    pcstr GetUserConfigFileName() override { return ConfigFile; }

    string64 ConfigFile;
    bool bVisible{};
    vecCMD Commands;

    void AddCommand(IConsole_Command* cc);
    void RemoveCommand(IConsole_Command* cc);

    void Show();
    void Hide();

    void Execute(pcstr cmd);
    void ExecuteScript(pcstr str);
    void ExecuteCommand(pcstr cmd, bool record_cmd = true);
    void SelectCommand();

    bool GetBool(pcstr cmd) const;
    float GetFloat(pcstr cmd, float& min, float& max) const;
    int GetInteger(pcstr cmd, int& min, int& max) const;
    pcstr GetString(pcstr cmd) const;
    pcstr GetToken(pcstr cmd) const;
    const xr_token* GetXRToken(pcstr cmd) const;
    Fvector GetFVector(pcstr cmd) const;
    Fvector* GetFVectorPtr(pcstr cmd) const;
    IConsole_Command* GetCommand(pcstr cmd) const;

protected:
    char m_edit_string[CONSOLE_BUF_SIZE]{};

    enum Console_mark : char // (int)=char
    {
        no_mark = ' ',
        mark0 = '~',
        mark1 = '!', // error
        mark2 = '@', // console cmd
        mark3 = '#',
        mark4 = '$',
        mark5 = '%',
        mark6 = '^',
        mark7 = '&',
        mark8 = '*',
        mark9 = '-', // green = ok
        mark10 = '+',
        mark11 = '=',
        mark12 = '/'
    };

    bool is_mark(Console_mark type);
    static Fcolor get_mark_color(Console_mark type);

protected:
    void Prev_cmd();
    void Next_cmd();
    void Prev_tip();
    void Next_tip();

    void Begin_tips();
    void End_tips();
    void PageUp_tips();
    void PageDown_tips();

    int InputCallback(ImGuiInputTextCallbackData* data);

protected:
    void add_cmd_history(shared_str const& str);
    void next_cmd_history_idx();
    void prev_cmd_history_idx();
    void reset_cmd_history_idx();

    void next_selected_tip();
    void check_next_selected_tip();
    void prev_selected_tip();
    void check_prev_selected_tip();
    void reset_selected_tip();

    IConsole_Command* find_next_cmd(pcstr in_str, shared_str& out_str);
    bool add_next_cmds(pcstr in_str, vecTipsEx& out_v);
    bool add_internal_cmds(pcstr in_str, vecTipsEx& out_v);

    void update_tips();
    void select_for_filter(pcstr filter_str, const vecTips& in_v, vecTipsEx& out_v);

}; // class CConsole

ENGINE_API extern CConsole* Console;
