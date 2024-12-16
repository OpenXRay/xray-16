// XR_IOConsole.cpp: implementation of the CConsole class.
// modify 15.05.2008 sea

#include "stdafx.h"
#include "XR_IOConsole.h"
#include "line_edit_control.h"

#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "xr_input.h"
#include "xr_ioc_cmd.h"

#include <imgui_internal.h>

static u32 const cmd_history_max = 64;

static Fcolor const prompt_font_color  = color_rgba(228, 228, 255, 255);
static Fcolor const tips_font_color    = color_rgba(230, 250, 230, 255);
static Fcolor const cmd_font_color     = color_rgba(138, 138, 245, 255);
static Fcolor const total_font_color   = color_rgba(250, 250, 15, 180);
static Fcolor const default_font_color = color_rgba(250, 250, 250, 250);

static Fcolor const back_color         = color_rgba(20, 20, 20, 200);
static Fcolor const tips_back_color    = color_rgba(20, 20, 20, 200);
static Fcolor const tips_select_color  = color_rgba(90, 90, 140, 230);
static Fcolor const tips_word_color    = color_rgba(5, 100, 56, 200);

ENGINE_API CConsole* Console = NULL;

extern char const* const ioc_prompt;
char const* const ioc_prompt = ">>>";

Fcolor CConsole::get_mark_color(Console_mark type)
{
    Fcolor color = default_font_color;
    switch (type)
    {
    case no_mark: break;
    case mark0: color = color_rgba(255, 255, 0, 255); break;
    case mark1: color = color_rgba(255, 0, 0, 255); break;
    case mark2: color = color_rgba(100, 100, 255, 255); break;
    case mark3: color = color_rgba(0, 222, 205, 155); break;
    case mark4: color = color_rgba(255, 0, 255, 255); break;
    case mark5: color = color_rgba(155, 55, 170, 155); break;
    case mark6: color = color_rgba(25, 200, 50, 255); break;
    case mark7: color = color_rgba(255, 255, 0, 255); break;
    case mark8: color = color_rgba(128, 128, 128, 255); break;
    case mark9: color = color_rgba(0, 255, 0, 255); break;
    case mark10: color = color_rgba(55, 155, 140, 255); break;
    case mark11: color = color_rgba(205, 205, 105, 255); break;
    case mark12: color = color_rgba(128, 128, 250, 255); break;
    }
    return color;
}

bool CConsole::is_mark(Console_mark type)
{
    switch (type)
    {
    case no_mark: break;
    case mark0:
    case mark1:
    case mark2:
    case mark3:
    case mark4:
    case mark5:
    case mark6:
    case mark7:
    case mark8:
    case mark9:
    case mark10:
    case mark11:
    case mark12:
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CConsole::CConsole()
{
    m_cmd_history_max = cmd_history_max;
    m_disable_tips = false;
    xrDebug::SetUserConfigHandler(this);
}

void CConsole::Initialize()
{
    ZoneScoped;

    m_cmd_history.reserve(m_cmd_history_max + 2);
    m_cmd_history.clear();
    reset_cmd_history_idx();

    m_tips.reserve(MAX_TIPS_COUNT + 1);
    m_tips.clear();
    m_temp_tips.reserve(MAX_TIPS_COUNT + 1);
    m_temp_tips.clear();

    reset_selected_tip();

    eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);

    // Commands
    extern void CCC_Register();
    CCC_Register();
}

CConsole::~CConsole()
{
    Destroy();
    xrDebug::SetUserConfigHandler(nullptr);
}

void CConsole::Destroy()
{
    ZoneScoped;

    Commands.clear();
    Engine.Event.Handler_Detach(eConsole, this);
}

void CConsole::AddCommand(IConsole_Command* cc) { Commands[cc->Name()] = cc; }
void CConsole::RemoveCommand(IConsole_Command* cc)
{
    vecCMD_IT it = Commands.find(cc->Name());
    if (Commands.end() != it)
    {
        Commands.erase(it);
    }
}

void CConsole::OnFrame()
{
    ZoneScoped;

    if (Device.dwFrame % 10 == 0)
    {
        update_tips();
    }

    if (!bVisible)
        return;

    if (!Device.editor().IsActiveState())
    {
        Device.editor().UpdateTextInput();

        // Activate console input after hiding the editor
        // XXX: not really great I'd say
        if (pInput->CurrentIR() != this)
            IR_Capture();
    }

    bool bGame = false;
    if ((g_pGameLevel && g_pGameLevel->bReady) ||
        (g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive()))
    {
        bGame = true;
    }
    if (GEnv.isDedicatedServer)
    {
        bGame = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    auto size = viewport->WorkSize;
    if (bGame)
        size.y /= 2;

    ImVec2 pos{};
    const auto padding = ImGui::GetStyle().WindowPadding;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // Console window
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, reinterpret_cast<const ImVec4&>(back_color));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { padding.x, 0.0f }); // we want to disable padding for window bottom

    constexpr ImGuiWindowFlags console_window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav; //| ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Console", nullptr, console_window_flags))
    {
        ImGui::SetCursorPosY(padding.y); // since we have disabled padding

        const size_t amount = LogFile.size();
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

        // Log subwindow
        if (ImGui::BeginChild("Log", { 0, -footer_height_to_reserve }))
        {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(amount));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    const auto& line = LogFile[i];
                    if (!line.c_str())
                        continue;
                    const auto& color = get_mark_color((Console_mark)line[0]);
                    ImGui::PushStyleColor(ImGuiCol_Text, reinterpret_cast<const ImVec4&>(color));
                    ImGui::TextUnformatted(line.c_str(), line.data() + line.size());
                    ImGui::PopStyleColor();
                }
            }
            ImGui::SetScrollHereY();
        }
        ImGui::EndChild();

        // Command input
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(reinterpret_cast<const ImVec4&>(prompt_font_color), "%s", ioc_prompt);

        ImGui::SameLine();
        pos = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, {});
        ImGui::PushStyleColor(ImGuiCol_Text, reinterpret_cast<const ImVec4&>(cmd_font_color));
        ImGui::SetNextItemWidth(-100);

        constexpr ImGuiInputTextFlags input_text_flags =
            ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory |
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;

        constexpr auto callback = [](ImGuiInputTextCallbackData* data)
        {
            return static_cast<CConsole*>(data->UserData)->InputCallback(data);
        };

        if (Device.editor().GetState() != xray::editor::ide::visible_state::full)
            ImGui::SetKeyboardFocusHere(0);

        if (ImGui::InputText("##input", m_edit_string, std::size(m_edit_string), input_text_flags, callback, this))
        {
            if (m_select_tip < 0 || m_select_tip >= static_cast<int>(m_tips.size()))
                ExecuteCommand(m_edit_string);
            else
            {
                shared_str const& str = m_tips[m_select_tip].text;
                if (m_tips_mode == 1)
                {
                    pstr buf;
                    STRCONCAT(buf, str.c_str(), " ");
                    xr_strcpy(m_edit_string, buf);
                }
                else if (m_tips_mode == 2)
                {
                    pstr buf;
                    STRCONCAT(buf, m_cur_cmd.c_str(), " ", str.c_str());
                    xr_strcpy(m_edit_string, buf);
                }
                reset_selected_tip();

                if (ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetItemID()))
                    state->ReloadUserBufAndMoveToEnd();
            }
            m_disable_tips = false;
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::SetItemDefaultFocus();
        ImGui::PopStyleColor(2);

        // Amount of log lines
        ImGui::SameLine();
        ImGui::TextColored(reinterpret_cast<const ImVec4&>(total_font_color), "[%zu]", amount);

        pos.y = ImGui::GetWindowPos().y + ImGui::GetWindowHeight();
    }
    ImGui::End();
    ImGui::PopStyleColor(); // ImGuiCol_WindowBg
    ImGui::PopStyleVar();   // ImGuiStyleVar_WindowPadding

    // Tips
    if (bGame && !m_disable_tips && m_tips.size())
    {
        constexpr ImGuiWindowFlags tips_window_flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, reinterpret_cast<const ImVec4&>(tips_back_color));
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4&)tips_font_color);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowSizeConstraints({ 300.f, 50.f }, { 500.f, size.y * 0.9f });
        if (ImGui::Begin("##tooltip", nullptr, tips_window_flags))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(m_tips.size()));

            // Scroll to focused item once
            static int itm_scroll_to = -1;
            if (itm_scroll_to != m_select_tip && m_select_tip >= 0)
                clipper.IncludeItemByIndex(m_select_tip);

            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    const auto& line = m_tips[i].text;
                    if (line.empty())
                        continue;

                    cpcstr text = line.c_str();
                    pos = ImGui::GetCursorScreenPos();

                    const TipString& ts = m_tips[i];
                    if (ts.HL_start >= 0 && ts.HL_finish >= 0 && ts.HL_start <= ts.HL_finish)
                    {
                        const int str_size = static_cast<int>(ts.text.size());
                        if (ts.HL_start < str_size && ts.HL_finish <= str_size)
                        {
                            const ImVec2 text_size_before = ImGui::CalcTextSize(text, text + ts.HL_start);
                            const ImVec2 text_size_after = ImGui::CalcTextSize(text + ts.HL_start, text + ts.HL_finish);

                            const ImVec2 char_pos = ImVec2(pos.x + text_size_before.x, pos.y);
                            const float text_height = ImGui::GetTextLineHeight();
                            const float rect_width = text_size_after.x;

                            draw_list->AddRectFilled(
                                ImVec2(char_pos.x, char_pos.y),
                                ImVec2(char_pos.x + rect_width, char_pos.y + text_height),
                                tips_word_color.get_windows()
                            );
                        }
                    }

                    ImGui::PushStyleColor(ImGuiCol_Header, reinterpret_cast<const ImVec4&>(tips_select_color));
                    if (ImGui::Selectable(text, i == m_select_tip))
                    {
                        if (m_tips_mode == 1)
                        {
                            pstr buf;
                            STRCONCAT(buf, text, " ");
                            xr_strcpy(m_edit_string, buf);
                        }
                        else if (m_tips_mode == 2)
                        {
                            pstr buf;
                            STRCONCAT(buf, m_cur_cmd.c_str(), " ", text);
                            xr_strcpy(m_edit_string, buf);
                        }
                    }
                    ImGui::PopStyleColor();

                    if (itm_scroll_to != m_select_tip && i == m_select_tip)
                    {
                        ImGui::SetScrollHereY();
                        itm_scroll_to = m_select_tip;
                    }
                }
            } // while (clipper.Step())
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleVar(); // ImGuiStyleVar_WindowBorderSize
}

void CConsole::OnEvent(EVENT E, u64 P1, u64 P2)
{
    pstr command = (pstr)P1;
    ExecuteCommand(command, false);
    xr_free(command);
}

void CConsole::IR_OnKeyboardPress(int key)
{
    switch (GetBindedAction(key))
    {
    case kQUIT:
        if (0 <= m_select_tip && m_select_tip < (int)m_tips.size())
        {
            m_disable_tips = true;
            return;
        }
        [[fallthrough]];

    case kCONSOLE:
    {
        Hide();
        return;
    }
    } // switch (GetBindedAction(key))

    Device.editor().IR_OnKeyboardPress(key);
}

void CConsole::IR_OnKeyboardRelease(int key)
{
    Device.editor().IR_OnKeyboardRelease(key);
}

void CConsole::IR_OnKeyboardHold(int key)
{
    Device.editor().IR_OnKeyboardHold(key);
}

void CConsole::IR_OnTextInput(pcstr text)
{
    Device.editor().IR_OnTextInput(text);
}

void CConsole::ExecuteCommand(pcstr cmd_str, bool record_cmd)
{
    u32 str_size = xr_strlen(cmd_str);
    PSTR edt = (PSTR)xr_alloca((str_size + 1) * sizeof(char));
    PSTR first = (PSTR)xr_alloca((str_size + 1) * sizeof(char));
    PSTR last = (PSTR)xr_alloca((str_size + 1) * sizeof(char));

    xr_strcpy(edt, str_size + 1, cmd_str);
    edt[str_size] = 0;

    reset_cmd_history_idx();
    reset_selected_tip();

    text_editor::remove_spaces(edt);
    if (edt[0] == 0)
    {
        return;
    }
    if (record_cmd)
    {
        char c[2];
        c[0] = mark2;
        c[1] = 0;

        if (m_last_cmd.c_str() == 0 || xr_strcmp(m_last_cmd, edt) != 0)
        {
            Log(c, edt);
            add_cmd_history(edt);
            m_last_cmd = edt;
        }
    }
    text_editor::split_cmd(first, last, edt);

    // search
    vecCMD_IT it = Commands.find(first);
    if (it != Commands.end())
    {
        IConsole_Command* cc = it->second;
        if (cc && cc->bEnabled)
        {
            if (cc->bLowerCaseArgs)
            {
                xr_strlwr(last);
            }
            if (last[0] == 0)
            {
                if (cc->bEmptyArgsHandled)
                {
                    cc->Execute(last);
                }
                else
                {
                    IConsole_Command::TStatus stat;
                    cc->GetStatus(stat);
                    Msg("- %s %s", cc->Name(), stat);
                }
            }
            else
            {
                cc->Execute(last);
                if (record_cmd)
                {
                    cc->add_to_LRU((pcstr)last);
                }
            }
        }
        else
        {
            Log("! Command disabled.");
        }
    }
    else
    {
        Log("! Unknown command: ", first);
    }

    if (record_cmd)
    {
        m_edit_string[0] = '\0';
    }
}

void CConsole::Show()
{
    if (bVisible)
    {
        return;
    }
    bVisible = true;

    m_edit_string[0] = '\0';
    reset_cmd_history_idx();
    reset_selected_tip();
    update_tips();

    if (!Device.editor().IsActiveState())
        IR_Capture();
    Device.seqFrame.Add(this);
}

extern CInput* pInput;

void CConsole::Hide()
{
    if (!bVisible || (g_pGamePersistent && GEnv.isDedicatedServer))
        return;

    // if ( g_pGameLevel ||
    // ( g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive() ))

    bVisible = false;
    reset_selected_tip();
    update_tips();

    Device.seqFrame.Remove(this);
    IR_Release();
}

void CConsole::SelectCommand()
{
    if (m_cmd_history.empty())
    {
        return;
    }
    VERIFY(0 <= m_cmd_history_idx && m_cmd_history_idx < (int)m_cmd_history.size());

    vecHistory::reverse_iterator it_rb = m_cmd_history.rbegin() + m_cmd_history_idx;
    xr_strcpy(m_edit_string, it_rb->c_str());
    reset_selected_tip();
}

void CConsole::Execute(pcstr cmd) { ExecuteCommand(cmd, false); }
void CConsole::ExecuteScript(pcstr str)
{
    u32 str_size = xr_strlen(str);
    PSTR buf = (PSTR)xr_alloca((str_size + 10) * sizeof(char));
    xr_strcpy(buf, str_size + 10, "cfg_load ");
    xr_strcat(buf, str_size + 10, str);
    Execute(buf);
}

// -------------------------------------------------------------------------------------------------

IConsole_Command* CConsole::find_next_cmd(pcstr in_str, shared_str& out_str)
{
    pcstr radmin_cmd_name = "ra ";
    bool b_ra = (in_str == strstr(in_str, radmin_cmd_name));
    u32 offset = (b_ra) ? xr_strlen(radmin_cmd_name) : 0;

    pstr t2;
    STRCONCAT(t2, in_str + offset, " ");

    vecCMD_IT it = Commands.lower_bound(t2);
    if (it != Commands.end())
    {
        IConsole_Command* cc = it->second;
        pcstr name_cmd = cc->Name();
        u32 name_cmd_size = xr_strlen(name_cmd);
        PSTR new_str = (PSTR)xr_alloca((offset + name_cmd_size + 2) * sizeof(char));

        xr_strcpy(new_str, offset + name_cmd_size + 2, (b_ra) ? radmin_cmd_name : "");
        xr_strcat(new_str, offset + name_cmd_size + 2, name_cmd);

        out_str._set((pcstr)new_str);
        return cc;
    }
    return NULL;
}

bool CConsole::add_next_cmds(pcstr in_str, vecTipsEx& out_v)
{
    u32 cur_count = out_v.size();
    if (cur_count >= MAX_TIPS_COUNT)
    {
        return false;
    }

    pstr t2;
    STRCONCAT(t2, in_str, " ");

    shared_str temp;
    IConsole_Command* cc = find_next_cmd(t2, temp);
    if (!cc || temp.size() == 0)
    {
        return false;
    }

    bool res = false;
    for (u32 i = cur_count; i < MAX_TIPS_COUNT * 2; ++i) // fake=protect
    {
        temp._set(cc->Name());
        bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
        if (!dup)
        {
            TipString ts(temp);
            out_v.push_back(ts);
            res = true;
        }
        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            break; // for
        }
        pstr t3;
        STRCONCAT(t3, out_v.back().text.c_str(), " ");
        cc = find_next_cmd(t3, temp);
        if (!cc)
        {
            break; // for
        }
    } // for
    return res;
}

bool CConsole::add_internal_cmds(pcstr in_str, vecTipsEx& out_v)
{
    u32 cur_count = out_v.size();
    if (cur_count >= MAX_TIPS_COUNT)
    {
        return false;
    }
    u32 in_sz = xr_strlen(in_str);

    bool res = false;
    // word in begin
    xr_string name2;
    vecCMD_IT itb = Commands.begin();
    vecCMD_IT ite = Commands.end();
    for (; itb != ite; ++itb)
    {
        pcstr name = itb->first;
        u32 name_sz = xr_strlen(name);
        if (name_sz >= in_sz)
        {
            name2.assign(name, in_sz);
            if (!xr_stricmp(name2.c_str(), in_str))
            {
                shared_str temp;
                temp._set(name);
                bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
                if (!dup)
                {
                    out_v.push_back(TipString(temp, 0, in_sz));
                    res = true;
                }
            }
        }

        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            return res;
        }
    } // for

    // word in internal
    itb = Commands.begin();
    ite = Commands.end();
    for (; itb != ite; ++itb)
    {
        pcstr name = itb->first;
        pcstr fd_str = strstr(name, in_str);
        if (fd_str)
        {
            shared_str temp;
            temp._set(name);
            bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
            if (!dup)
            {
                u32 name_sz = xr_strlen(name);
                int fd_sz = name_sz - xr_strlen(fd_str);
                out_v.push_back(TipString(temp, fd_sz, fd_sz + in_sz));
                res = true;
            }
        }
        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            return res;
        }
    } // for

    return res;
}

void CConsole::update_tips()
{
    m_temp_tips.clear();
    m_tips.clear();

    m_cur_cmd = nullptr;
    if (!bVisible)
    {
        return;
    }

    pcstr cur = m_edit_string;
    u32 cur_length = xr_strlen(cur);

    if (cur_length == 0)
    {
        m_prev_length_str = 0;
        return;
    }

    if (m_prev_length_str != cur_length)
    {
        reset_selected_tip();
    }
    m_prev_length_str = cur_length;

    PSTR first = (PSTR)xr_alloca((cur_length + 1) * sizeof(char));
    PSTR last = (PSTR)xr_alloca((cur_length + 1) * sizeof(char));
    text_editor::split_cmd(first, last, cur);

    u32 first_lenght = xr_strlen(first);

    if ((first_lenght > 2) && (first_lenght + 1 <= cur_length)) // param
    {
        if (cur[first_lenght] == ' ')
        {
            if (m_tips_mode != 2)
            {
                reset_selected_tip();
            }

            vecCMD_IT it = Commands.find(first);
            if (it != Commands.end())
            {
                IConsole_Command* cc = it->second;

                u32 mode = 0;
                if ((first_lenght + 2 <= cur_length) && (cur[first_lenght] == ' ') && (cur[first_lenght + 1] == ' '))
                {
                    mode = 1;
                    last += 1; // fake: next char
                }

                cc->fill_tips(m_temp_tips, mode);
                m_tips_mode = 2;
                m_cur_cmd._set(first);
                select_for_filter(last, m_temp_tips, m_tips);

                if (m_tips.size() == 0)
                {
                    m_tips.push_back(TipString("(empty)"));
                }
                if ((int)m_tips.size() <= m_select_tip)
                {
                    reset_selected_tip();
                }
                return;
            }
        }
    }

    // cmd name
    {
        add_internal_cmds(cur, m_tips);
        // add_next_cmds( cur, m_tips );
        m_tips_mode = 1;
    }

    if (m_tips.size() == 0)
    {
        m_tips_mode = 0;
        reset_selected_tip();
    }
    if ((int)m_tips.size() <= m_select_tip)
    {
        reset_selected_tip();
    }
}

void CConsole::select_for_filter(pcstr filter_str, vecTips& in_v, vecTipsEx& out_v)
{
    out_v.clear();
    u32 in_count = in_v.size();
    if (in_count == 0 || !filter_str)
    {
        return;
    }

    bool all = (xr_strlen(filter_str) == 0);
    const size_t filter_str_len = xr_strlen(filter_str);
    //vecTips::iterator itb = in_v.begin();
    //vecTips::iterator ite = in_v.end();
    //for (; itb != ite; ++itb)
    for (auto& it : in_v)
    {
        shared_str const& str = it;
        if (all)
            out_v.push_back(TipString(str));
        else
        {
            pcstr fd_str = strstr(str.c_str(), filter_str);
            if (fd_str)
            {
                size_t fd_sz = str.size() - xr_strlen(fd_str);
                TipString ts(str, fd_sz, fd_sz + filter_str_len);
                out_v.push_back(ts);
            }
        }
    } // for
}
