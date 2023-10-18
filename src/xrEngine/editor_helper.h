#pragma once

namespace xray::imgui
{
inline void ItemHelp(const char* desc, bool use_separate_marker = true, bool on_same_line = true)
{
    if (use_separate_marker)
    {
        if (on_same_line)
            ImGui::SameLine();
        ImGui::TextDisabled("(?)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

inline bool MenuItemWithShortcut(pcstr label, EGameActions shortcut, const char* desc = nullptr, bool selected = false)
{
    cpcstr key_name = GetActionBinding(shortcut);
    const bool result = ImGui::MenuItem(label, key_name, selected);
    if (desc)
        ItemHelp(desc);
    return result;
}

inline bool InputText(pcstr label, shared_str& texture_name)
{
    string_path temp;
    xr_strcpy(temp, texture_name.empty() ? "" : texture_name.c_str());

    if (ImGui::InputText(label, temp, std::size(temp)))
    {
        texture_name = temp;
        return true;
    }

    return false;
}

inline ImGuiKey xr_key_to_imgui_key(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_UNKNOWN:               return ImGuiKey_None;
    case SDL_SCANCODE_A:                     return ImGuiKey_A;
    case SDL_SCANCODE_B:                     return ImGuiKey_B;
    case SDL_SCANCODE_C:                     return ImGuiKey_C;
    case SDL_SCANCODE_D:                     return ImGuiKey_D;
    case SDL_SCANCODE_E:                     return ImGuiKey_E;
    case SDL_SCANCODE_F:                     return ImGuiKey_F;
    case SDL_SCANCODE_G:                     return ImGuiKey_G;
    case SDL_SCANCODE_H:                     return ImGuiKey_H;
    case SDL_SCANCODE_I:                     return ImGuiKey_I;
    case SDL_SCANCODE_J:                     return ImGuiKey_J;
    case SDL_SCANCODE_K:                     return ImGuiKey_K;
    case SDL_SCANCODE_L:                     return ImGuiKey_L;
    case SDL_SCANCODE_M:                     return ImGuiKey_M;
    case SDL_SCANCODE_N:                     return ImGuiKey_N;
    case SDL_SCANCODE_O:                     return ImGuiKey_O;
    case SDL_SCANCODE_P:                     return ImGuiKey_P;
    case SDL_SCANCODE_Q:                     return ImGuiKey_Q;
    case SDL_SCANCODE_R:                     return ImGuiKey_R;
    case SDL_SCANCODE_S:                     return ImGuiKey_S;
    case SDL_SCANCODE_T:                     return ImGuiKey_T;
    case SDL_SCANCODE_U:                     return ImGuiKey_U;
    case SDL_SCANCODE_V:                     return ImGuiKey_V;
    case SDL_SCANCODE_W:                     return ImGuiKey_W;
    case SDL_SCANCODE_X:                     return ImGuiKey_X;
    case SDL_SCANCODE_Y:                     return ImGuiKey_Y;
    case SDL_SCANCODE_Z:                     return ImGuiKey_Z;
    case SDL_SCANCODE_1:                     return ImGuiKey_1;
    case SDL_SCANCODE_2:                     return ImGuiKey_2;
    case SDL_SCANCODE_3:                     return ImGuiKey_3;
    case SDL_SCANCODE_4:                     return ImGuiKey_4;
    case SDL_SCANCODE_5:                     return ImGuiKey_5;
    case SDL_SCANCODE_6:                     return ImGuiKey_6;
    case SDL_SCANCODE_7:                     return ImGuiKey_7;
    case SDL_SCANCODE_8:                     return ImGuiKey_8;
    case SDL_SCANCODE_9:                     return ImGuiKey_9;
    case SDL_SCANCODE_0:                     return ImGuiKey_0;
    case SDL_SCANCODE_RETURN:                return ImGuiKey_Enter;
    case SDL_SCANCODE_ESCAPE:                return ImGuiKey_Escape;
    case SDL_SCANCODE_BACKSPACE:             return ImGuiKey_Backspace;
    case SDL_SCANCODE_TAB:                   return ImGuiKey_Tab;
    case SDL_SCANCODE_SPACE:                 return ImGuiKey_Space;
    case SDL_SCANCODE_MINUS:                 return ImGuiKey_Minus;
    case SDL_SCANCODE_EQUALS:                return ImGuiKey_Equal;
    case SDL_SCANCODE_LEFTBRACKET:           return ImGuiKey_LeftBracket;
    case SDL_SCANCODE_RIGHTBRACKET:          return ImGuiKey_RightBracket;
    case SDL_SCANCODE_BACKSLASH:             return ImGuiKey_Backslash;
    //case SDL_SCANCODE_NONUSHASH:             return ImGuiKey_;
    case SDL_SCANCODE_SEMICOLON:             return ImGuiKey_Semicolon;
    case SDL_SCANCODE_APOSTROPHE:            return ImGuiKey_Apostrophe;
    case SDL_SCANCODE_GRAVE:                 return ImGuiKey_GraveAccent;
    case SDL_SCANCODE_COMMA:                 return ImGuiKey_Comma;
    case SDL_SCANCODE_PERIOD:                return ImGuiKey_Period;
    case SDL_SCANCODE_SLASH:                 return ImGuiKey_Slash;
    case SDL_SCANCODE_CAPSLOCK:              return ImGuiKey_CapsLock;
    case SDL_SCANCODE_F1:                    return ImGuiKey_F1;
    case SDL_SCANCODE_F2:                    return ImGuiKey_F2;
    case SDL_SCANCODE_F3:                    return ImGuiKey_F3;
    case SDL_SCANCODE_F4:                    return ImGuiKey_F4;
    case SDL_SCANCODE_F5:                    return ImGuiKey_F5;
    case SDL_SCANCODE_F6:                    return ImGuiKey_F6;
    case SDL_SCANCODE_F7:                    return ImGuiKey_F7;
    case SDL_SCANCODE_F8:                    return ImGuiKey_F8;
    case SDL_SCANCODE_F9:                    return ImGuiKey_F9;
    case SDL_SCANCODE_F10:                   return ImGuiKey_F10;
    case SDL_SCANCODE_F11:                   return ImGuiKey_F11;
    case SDL_SCANCODE_F12:                   return ImGuiKey_F12;
    case SDL_SCANCODE_PRINTSCREEN:           return ImGuiKey_PrintScreen;
    case SDL_SCANCODE_SCROLLLOCK:            return ImGuiKey_ScrollLock;
    case SDL_SCANCODE_PAUSE:                 return ImGuiKey_Pause;
    case SDL_SCANCODE_INSERT:                return ImGuiKey_Insert;
    case SDL_SCANCODE_HOME:                  return ImGuiKey_Home;
    case SDL_SCANCODE_PAGEUP:                return ImGuiKey_PageUp;
    case SDL_SCANCODE_DELETE:                return ImGuiKey_Delete;
    case SDL_SCANCODE_END:                   return ImGuiKey_End;
    case SDL_SCANCODE_PAGEDOWN:              return ImGuiKey_PageDown;
    case SDL_SCANCODE_RIGHT:                 return ImGuiKey_RightArrow;
    case SDL_SCANCODE_LEFT:                  return ImGuiKey_LeftArrow;
    case SDL_SCANCODE_DOWN:                  return ImGuiKey_DownArrow;
    case SDL_SCANCODE_UP:                    return ImGuiKey_UpArrow;
    case SDL_SCANCODE_NUMLOCKCLEAR:          return ImGuiKey_NumLock;
    case SDL_SCANCODE_KP_DIVIDE:             return ImGuiKey_KeypadDivide;
    case SDL_SCANCODE_KP_MULTIPLY:           return ImGuiKey_KeypadMultiply;
    case SDL_SCANCODE_KP_MINUS:              return ImGuiKey_KeypadSubtract;
    case SDL_SCANCODE_KP_PLUS:               return ImGuiKey_KeypadAdd;
    case SDL_SCANCODE_KP_ENTER:              return ImGuiKey_KeypadEnter;
    case SDL_SCANCODE_KP_1:                  return ImGuiKey_Keypad1;
    case SDL_SCANCODE_KP_2:                  return ImGuiKey_Keypad2;
    case SDL_SCANCODE_KP_3:                  return ImGuiKey_Keypad3;
    case SDL_SCANCODE_KP_4:                  return ImGuiKey_Keypad4;
    case SDL_SCANCODE_KP_5:                  return ImGuiKey_Keypad5;
    case SDL_SCANCODE_KP_6:                  return ImGuiKey_Keypad6;
    case SDL_SCANCODE_KP_7:                  return ImGuiKey_Keypad7;
    case SDL_SCANCODE_KP_8:                  return ImGuiKey_Keypad8;
    case SDL_SCANCODE_KP_9:                  return ImGuiKey_Keypad9;
    case SDL_SCANCODE_KP_0:                  return ImGuiKey_Keypad0;
    case SDL_SCANCODE_KP_PERIOD:             return ImGuiKey_KeypadDecimal;
    //case SDL_SCANCODE_NONUSBACKSLASH:        return ImGuiKey_;
    case SDL_SCANCODE_APPLICATION:           return ImGuiKey_Menu;
    //case SDL_SCANCODE_POWER:                 return ImGuiKey_;
    case SDL_SCANCODE_KP_EQUALS:             return ImGuiKey_KeypadEqual;
    //case SDL_SCANCODE_F13:                   return ImGuiKey_;
    //case SDL_SCANCODE_F14:                   return ImGuiKey_;
    //case SDL_SCANCODE_F15:                   return ImGuiKey_;
    //case SDL_SCANCODE_F16:                   return ImGuiKey_;
    //case SDL_SCANCODE_F17:                   return ImGuiKey_;
    //case SDL_SCANCODE_F18:                   return ImGuiKey_;
    //case SDL_SCANCODE_F19:                   return ImGuiKey_;
    //case SDL_SCANCODE_F20:                   return ImGuiKey_;
    //case SDL_SCANCODE_F21:                   return ImGuiKey_;
    //case SDL_SCANCODE_F22:                   return ImGuiKey_;
    //case SDL_SCANCODE_F23:                   return ImGuiKey_;
    //case SDL_SCANCODE_F24:                   return ImGuiKey_;
    //case SDL_SCANCODE_EXECUTE:               return ImGuiKey_;
    //case SDL_SCANCODE_HELP:                  return ImGuiKey_;
    //case SDL_SCANCODE_MENU:                  return ImGuiKey_;
    //case SDL_SCANCODE_SELECT:                return ImGuiKey_;
    //case SDL_SCANCODE_STOP:                  return ImGuiKey_;
    //case SDL_SCANCODE_AGAIN:                 return ImGuiKey_;
    //case SDL_SCANCODE_UNDO:                  return ImGuiKey_;
    //case SDL_SCANCODE_CUT:                   return ImGuiKey_;
    //case SDL_SCANCODE_COPY:                  return ImGuiKey_;
    //case SDL_SCANCODE_PASTE:                 return ImGuiKey_;
    //case SDL_SCANCODE_FIND:                  return ImGuiKey_;
    //case SDL_SCANCODE_MUTE:                  return ImGuiKey_;
    //case SDL_SCANCODE_VOLUMEUP:              return ImGuiKey_;
    //case SDL_SCANCODE_VOLUMEDOWN:            return ImGuiKey_;
    //case SDL_SCANCODE_KP_COMMA:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_EQUALSAS400:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL1:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL2:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL3:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL4:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL5:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL6:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL7:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL8:        return ImGuiKey_;
    //case SDL_SCANCODE_INTERNATIONAL9:        return ImGuiKey_;
    //case SDL_SCANCODE_LANG1:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG2:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG3:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG4:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG5:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG6:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG7:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG8:                 return ImGuiKey_;
    //case SDL_SCANCODE_LANG9:                 return ImGuiKey_;
    //case SDL_SCANCODE_ALTERASE:              return ImGuiKey_;
    //case SDL_SCANCODE_SYSREQ:                return ImGuiKey_;
    //case SDL_SCANCODE_CANCEL:                return ImGuiKey_;
    //case SDL_SCANCODE_CLEAR:                 return ImGuiKey_;
    //case SDL_SCANCODE_PRIOR:                 return ImGuiKey_;
    //case SDL_SCANCODE_RETURN2:               return ImGuiKey_;
    //case SDL_SCANCODE_SEPARATOR:             return ImGuiKey_;
    //case SDL_SCANCODE_OUT:                   return ImGuiKey_;
    //case SDL_SCANCODE_OPER:                  return ImGuiKey_;
    //case SDL_SCANCODE_CLEARAGAIN:            return ImGuiKey_;
    //case SDL_SCANCODE_CRSEL:                 return ImGuiKey_;
    //case SDL_SCANCODE_EXSEL:                 return ImGuiKey_;
    //case SDL_SCANCODE_KP_00:                 return ImGuiKey_;
    //case SDL_SCANCODE_KP_000:                return ImGuiKey_;
    //case SDL_SCANCODE_THOUSANDSSEPARATOR:    return ImGuiKey_;
    //case SDL_SCANCODE_DECIMALSEPARATOR:      return ImGuiKey_;
    //case SDL_SCANCODE_CURRENCYUNIT:          return ImGuiKey_;
    //case SDL_SCANCODE_CURRENCYSUBUNIT:       return ImGuiKey_;
    //case SDL_SCANCODE_KP_LEFTPAREN:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_RIGHTPAREN:         return ImGuiKey_;
    //case SDL_SCANCODE_KP_LEFTBRACE:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_RIGHTBRACE:         return ImGuiKey_;
    //case SDL_SCANCODE_KP_TAB:                return ImGuiKey_;
    //case SDL_SCANCODE_KP_BACKSPACE:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_A:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_B:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_C:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_D:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_E:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_F:                  return ImGuiKey_;
    //case SDL_SCANCODE_KP_XOR:                return ImGuiKey_;
    //case SDL_SCANCODE_KP_POWER:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_PERCENT:            return ImGuiKey_;
    //case SDL_SCANCODE_KP_LESS:               return ImGuiKey_;
    //case SDL_SCANCODE_KP_GREATER:            return ImGuiKey_;
    //case SDL_SCANCODE_KP_AMPERSAND:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_DBLAMPERSAND:       return ImGuiKey_;
    //case SDL_SCANCODE_KP_VERTICALBAR:        return ImGuiKey_;
    //case SDL_SCANCODE_KP_DBLVERTICALBAR:     return ImGuiKey_;
    //case SDL_SCANCODE_KP_COLON:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_HASH:               return ImGuiKey_;
    //case SDL_SCANCODE_KP_SPACE:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_AT:                 return ImGuiKey_;
    //case SDL_SCANCODE_KP_EXCLAM:             return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMSTORE:           return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMRECALL:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMCLEAR:           return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMADD:             return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMSUBTRACT:        return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMMULTIPLY:        return ImGuiKey_;
    //case SDL_SCANCODE_KP_MEMDIVIDE:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_PLUSMINUS:          return ImGuiKey_;
    //case SDL_SCANCODE_KP_CLEAR:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_CLEARENTRY:         return ImGuiKey_;
    //case SDL_SCANCODE_KP_BINARY:             return ImGuiKey_;
    //case SDL_SCANCODE_KP_OCTAL:              return ImGuiKey_;
    //case SDL_SCANCODE_KP_DECIMAL:            return ImGuiKey_;
    //case SDL_SCANCODE_KP_HEXADECIMAL:        return ImGuiKey_;
    case SDL_SCANCODE_LCTRL:                 return ImGuiKey_LeftCtrl;
    case SDL_SCANCODE_LSHIFT:                return ImGuiKey_LeftShift;
    case SDL_SCANCODE_LALT:                  return ImGuiKey_LeftAlt;
    case SDL_SCANCODE_LGUI:                  return ImGuiKey_LeftSuper;
    case SDL_SCANCODE_RCTRL:                 return ImGuiKey_RightCtrl;
    case SDL_SCANCODE_RSHIFT:                return ImGuiKey_RightShift;
    case SDL_SCANCODE_RALT:                  return ImGuiKey_RightAlt;
    case SDL_SCANCODE_RGUI:                  return ImGuiKey_RightSuper;
    //case SDL_SCANCODE_MODE:                  return ImGuiKey_;
    //case SDL_SCANCODE_AUDIONEXT:             return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOPREV:             return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOSTOP:             return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOPLAY:             return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOMUTE:             return ImGuiKey_;
    //case SDL_SCANCODE_MEDIASELECT:           return ImGuiKey_;
    //case SDL_SCANCODE_WWW:                   return ImGuiKey_;
    //case SDL_SCANCODE_MAIL:                  return ImGuiKey_;
    //case SDL_SCANCODE_CALCULATOR:            return ImGuiKey_;
    //case SDL_SCANCODE_COMPUTER:              return ImGuiKey_;
    //case SDL_SCANCODE_AC_SEARCH:             return ImGuiKey_;
    //case SDL_SCANCODE_AC_HOME:               return ImGuiKey_;
    //case SDL_SCANCODE_AC_BACK:               return ImGuiKey_;
    //case SDL_SCANCODE_AC_FORWARD:            return ImGuiKey_;
    //case SDL_SCANCODE_AC_STOP:               return ImGuiKey_;
    //case SDL_SCANCODE_AC_REFRESH:            return ImGuiKey_;
    //case SDL_SCANCODE_AC_BOOKMARKS:          return ImGuiKey_;
    //case SDL_SCANCODE_BRIGHTNESSDOWN:        return ImGuiKey_;
    //case SDL_SCANCODE_BRIGHTNESSUP:          return ImGuiKey_;
    //case SDL_SCANCODE_DISPLAYSWITCH:         return ImGuiKey_;
    //case SDL_SCANCODE_KBDILLUMTOGGLE:        return ImGuiKey_;
    //case SDL_SCANCODE_KBDILLUMDOWN:          return ImGuiKey_;
    //case SDL_SCANCODE_KBDILLUMUP:            return ImGuiKey_;
    //case SDL_SCANCODE_EJECT:                 return ImGuiKey_;
    //case SDL_SCANCODE_SLEEP:                 return ImGuiKey_;
    //case SDL_SCANCODE_APP1:                  return ImGuiKey_;
    //case SDL_SCANCODE_APP2:                  return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOREWIND:           return ImGuiKey_;
    //case SDL_SCANCODE_AUDIOFASTFORWARD:      return ImGuiKey_;
    //case SDL_SCANCODE_SOFTLEFT:              return ImGuiKey_;
    //case SDL_SCANCODE_SOFTRIGHT:             return ImGuiKey_;
    //case SDL_SCANCODE_CALL:                  return ImGuiKey_;
    //case SDL_SCANCODE_ENDCALL:               return ImGuiKey_;

    case XR_CONTROLLER_BUTTON_A:             return ImGuiKey_GamepadFaceDown;
    case XR_CONTROLLER_BUTTON_B:             return ImGuiKey_GamepadFaceRight;
    case XR_CONTROLLER_BUTTON_X:             return ImGuiKey_GamepadFaceLeft;
    case XR_CONTROLLER_BUTTON_Y:             return ImGuiKey_GamepadFaceUp;
    case XR_CONTROLLER_BUTTON_BACK:          return ImGuiKey_GamepadBack;
    //case XR_CONTROLLER_BUTTON_GUIDE:         return ImGuiKey_;
    case XR_CONTROLLER_BUTTON_START:         return ImGuiKey_GamepadStart;
    case XR_CONTROLLER_BUTTON_LEFTSTICK:     return ImGuiKey_GamepadL3;
    case XR_CONTROLLER_BUTTON_RIGHTSTICK:    return ImGuiKey_GamepadR3;
    case XR_CONTROLLER_BUTTON_LEFTSHOULDER:  return ImGuiKey_GamepadL1;
    case XR_CONTROLLER_BUTTON_RIGHTSHOULDER: return ImGuiKey_GamepadR1;
    case XR_CONTROLLER_BUTTON_DPAD_UP:       return ImGuiKey_GamepadDpadUp;
    case XR_CONTROLLER_BUTTON_DPAD_DOWN:     return ImGuiKey_GamepadDpadDown;
    case XR_CONTROLLER_BUTTON_DPAD_LEFT:     return ImGuiKey_GamepadDpadLeft;
    case XR_CONTROLLER_BUTTON_DPAD_RIGHT:    return ImGuiKey_GamepadDpadRight;
    //case XR_CONTROLLER_BUTTON_MISC1:         return ImGuiKey_;
    //case XR_CONTROLLER_BUTTON_PADDLE1:       return ImGuiKey_;
    //case XR_CONTROLLER_BUTTON_PADDLE2:       return ImGuiKey_;
    //case XR_CONTROLLER_BUTTON_PADDLE3:       return ImGuiKey_;
    //case XR_CONTROLLER_BUTTON_PADDLE4:       return ImGuiKey_;
    //case XR_CONTROLLER_BUTTON_TOUCHPAD:      return ImGuiKey_;
    } // switch key
    return ImGuiKey_None;
}

} // namespace xray::imgui
