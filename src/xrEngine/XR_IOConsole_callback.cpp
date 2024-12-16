////////////////////////////////////////////////////////////////////////////
// Module : XR_IOConsole_callback.cpp
// Created : 17.05.2008
// Author : Evgeniy Sokolov
// Description : Console`s callback functions class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XR_IOConsole.h"

#include "xr_ioc_cmd.h"

int CConsole::InputCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Shift + Tab doesn't result in the callback call :(
        /*if (ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            constexpr pcstr radmin_cmd_name = "ra ";

            pcstr edt = data->Buf;
            const bool b_ra = edt == strstr(edt, radmin_cmd_name);
            const size_t offset = b_ra ? xr_strlen(radmin_cmd_name) : 0;

            vecCMD_IT it = Commands.lower_bound(edt + offset);
            if (it != Commands.begin())
            {
                --it;
                IConsole_Command& cc = *it->second;
                pcstr name_cmd = cc.Name();
                size_t name_cmd_size = xr_strlen(name_cmd);
                const size_t size = offset + name_cmd_size + 2;
                pstr new_str = static_cast<pstr>(xr_alloca(size * sizeof(char)));

                xr_strcpy(new_str, size, (b_ra) ? radmin_cmd_name : "");
                xr_strcat(new_str, size - offset, name_cmd);
                data->BufTextLen = 0;
                data->InsertChars(0, new_str, new_str + size);
            }
        }
        else*/
        {
            shared_str out_str;

            IConsole_Command* cc = find_next_cmd(m_edit_string, out_str);
            if (cc && out_str.size())
            {
                data->BufTextLen = 0;
                data->InsertChars(0, out_str.c_str(), out_str.c_str() + out_str.size());
            }
        }
        data->ClearSelection();
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        const bool ctrl = ImGui::IsKeyDown(ImGuiMod_Ctrl);
        const bool alt = ImGui::IsKeyDown(ImGuiMod_Alt);

        if (ctrl && alt)
        {
            if (data->EventKey == ImGuiKey_UpArrow)
                Begin_tips();
            else if (data->EventKey == ImGuiKey_DownArrow)
                End_tips();
        }
        else if (alt)
        {
            if (data->EventKey == ImGuiKey_UpArrow)
                PageUp_tips();
            else if (data->EventKey == ImGuiKey_DownArrow)
                PageDown_tips();
        }
        else if (ctrl)
        {
            if (data->EventKey == ImGuiKey_UpArrow)
                Prev_cmd();
            else if (data->EventKey == ImGuiKey_DownArrow)
                Next_cmd();
        }
        else
        {
            if (data->EventKey == ImGuiKey_UpArrow)
                Prev_tip();
            else if (data->EventKey == ImGuiKey_DownArrow)
                Next_tip();
        }
        data->ClearSelection();
        break;
    }
    } // switch (data->EventFlag)
    return 0;
}

void CConsole::Prev_cmd() // SDL_SCANCODE_UP + Ctrl
{
    prev_cmd_history_idx();
    SelectCommand();
}

void CConsole::Next_cmd() // SDL_SCANCODE_DOWN + Ctrl
{
    next_cmd_history_idx();
    SelectCommand();
}

void CConsole::Prev_tip() // SDL_SCANCODE_UP
{
    if (xr_strlen(m_edit_string) == 0)
    {
        prev_cmd_history_idx();
        SelectCommand();
        return;
    }
    prev_selected_tip();
}

void CConsole::Next_tip() // SDL_SCANCODE_DOWN + Ctrl
{
    if (xr_strlen(m_edit_string) == 0)
    {
        next_cmd_history_idx();
        SelectCommand();
        return;
    }
    next_selected_tip();
}

void CConsole::Begin_tips()
{
    m_select_tip = 0;
    m_start_tip = 0;
}

void CConsole::End_tips()
{
    m_select_tip = m_tips.size() - 1;
    m_start_tip = m_select_tip - VIEW_TIPS_COUNT + 1;
    check_next_selected_tip();
}

void CConsole::PageUp_tips()
{
    m_select_tip -= VIEW_TIPS_COUNT;
    check_prev_selected_tip();
}

void CConsole::PageDown_tips()
{
    m_select_tip += VIEW_TIPS_COUNT;
    check_next_selected_tip();
}
