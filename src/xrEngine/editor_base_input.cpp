#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"
#include "XR_IOConsole.h"

#include <imgui_internal.h>

namespace
{
bool mouse_can_use_global_state()
{
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && defined(IMGUI_ENABLE_VIEWPORTS)
    cpcstr sdl_backend = SDL_GetCurrentVideoDriver();

    // Check and store if we are on a SDL backend that supports global mouse position
    // ("wayland" and "rpi" don't support it, but we chose to use a white-list instead of a black-list)
    for (cpcstr driver : { "windows", "cocoa", "x11", "DIVE", "VMAN" })
    {
        if (strncmp(sdl_backend, driver, strlen(driver)) == 0)
        {
            // We can create multi-viewports on the Platform side (optional)
            return true;
        }
    }
#endif
    return false;
}
}

namespace xray::editor
{
using namespace imgui;

void ide::InitBackend()
{
    ImGuiIO& io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;

    if (mouse_can_use_global_state())
    {
        // We can create multi-viewports on the Platform side (optional)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
#ifndef XR_PLATFORM_APPLE
        m_imgui_backend.mouse_can_report_hovered_viewport = true;
#endif
    }

    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "OpenXRay";
    ini_handler.TypeHash = ImHashStr("OpenXRay");
    ini_handler.UserData = this;

    ini_handler.ClearAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler)
    {
        ide& self = *static_cast<ide*>(handler->UserData);
        for (ide_tool* tool : self.m_tools)
        {
            tool->reset_settings();
        }
    };

    ini_handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, pcstr name) -> void*
    {
        ide& self = *static_cast<ide*>(handler->UserData);
        for (ide_tool* tool : self.m_tools)
        {
            if (xr_strcmp(tool->tool_name(), name) == 0)
            {
                tool->reset_settings(); // Clear existing if recycling previous entry
                return tool;
            }
        }
        return nullptr;
    };

    ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, pcstr line)
    {
        if (!entry)
            return;
        ide_tool& self = *static_cast<ide_tool*>(entry);
        self.apply_setting(line);

    };

    // We don't store separate copy of settings and
    // intended workflow is to apply settings immediately in apply_setting,
    // so this isn't much useful, but who knows
    ini_handler.ApplyAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler)
    {
        ide& self = *static_cast<ide*>(handler->UserData);
        for (ide_tool* tool : self.m_tools)
        {
            tool->apply_settings();
        }
    };

    ini_handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
    {
        ide& self = *static_cast<ide*>(handler->UserData);

        size_t estimated_buffer_size = 0;
        for (const ide_tool* tool : self.m_tools)
        {
            estimated_buffer_size += tool->estimate_settings_size();
        }
        buffer->reserve(estimated_buffer_size);

        for (const ide_tool* tool : self.m_tools)
        {
            buffer->appendf("[%s][%s]\n", handler->TypeName, tool->tool_name());
            tool->save_settings(buffer);
        }
    };
    ImGui::AddSettingsHandler(&ini_handler);
}

void ide::ProcessEvent(const SDL_Event& event)
{
    if (m_state != visible_state::full)
        return;

    auto& bd = m_imgui_backend;

    switch (event.type)
    {
    case SDL_WINDOWEVENT:
    {
        const auto window = SDL_GetWindowFromID(event.window.windowID);
        if (!window)
            break;
        const ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window);
        if (!viewport)
            break;

        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_ENTER:
            bd.mouse_window_id = event.window.windowID;
            bd.mouse_last_leave_frame = 0;
            break;
        case SDL_WINDOWEVENT_LEAVE:
            bd.mouse_last_leave_frame = ImGui::GetFrameCount() + 1;
            break;
        } // switch (event.window.event)
    }
    } // switch (event.type)
}

void ide::UpdateMouseData()
{
    ImGuiIO& io = ImGui::GetIO();
    auto& bd = m_imgui_backend;
    const bool anyMouseButtonPressed = pInput->iAnyMouseButtonDown();

    if (bd.mouse_last_leave_frame && bd.mouse_last_leave_frame >= ImGui::GetFrameCount() && anyMouseButtonPressed)
    {
        bd.mouse_window_id = 0;
        bd.mouse_last_leave_frame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    // Our io.AddMouseViewportEvent() calls will only be valid when not capturing.
    // Technically speaking testing for 'anyMouseButtonPressed' would be more rygorous, but testing for payload reduces noise and potential side-effects.
    if (bd.mouse_can_report_hovered_viewport && ImGui::GetDragDropPayload() == nullptr)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;

    // We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && defined(IMGUI_ENABLE_VIEWPORTS)
    SDL_CaptureMouse(anyMouseButtonPressed ? SDL_TRUE : SDL_FALSE);
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused = focused_window && (Device.m_sdlWnd == focused_window || ImGui::FindViewportByPlatformHandle(focused_window));
#else
    const bool is_app_focused = (SDL_GetWindowFlags(Device.m_sdlWnd) & SDL_WINDOW_INPUT_FOCUS) != 0;
#endif

    if (is_app_focused)
    {
        if (io.WantSetMousePos)
        {
            pInput->iSetMousePos({ (int)io.MousePos.x, (int)io.MousePos.y }, io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
        }
    }

    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        ImGuiID view_id = 0;
        if (SDL_Window* window = SDL_GetWindowFromID(bd.mouse_window_id))
            if (const ImGuiViewport* view = ImGui::FindViewportByPlatformHandle(window))
                view_id = view->ID;
        io.AddMouseViewportEvent(view_id);
    }
}

#ifdef NDEBUG
constexpr
#endif
SDL_SystemCursor get_sdl_cursor(ImGuiMouseCursor cursor)
{
    switch (cursor)
    {
    default:
        VERIFY(false);
        [[fallthrough]];

    case ImGuiMouseCursor_Arrow:        return SDL_SYSTEM_CURSOR_ARROW;
    case ImGuiMouseCursor_TextInput:    return SDL_SYSTEM_CURSOR_IBEAM;
    case ImGuiMouseCursor_ResizeAll:    return SDL_SYSTEM_CURSOR_SIZEALL;
    case ImGuiMouseCursor_ResizeNS:     return SDL_SYSTEM_CURSOR_SIZENS;
    case ImGuiMouseCursor_ResizeEW:     return SDL_SYSTEM_CURSOR_SIZEWE;
    case ImGuiMouseCursor_ResizeNESW:   return SDL_SYSTEM_CURSOR_SIZENESW;
    case ImGuiMouseCursor_ResizeNWSE:   return SDL_SYSTEM_CURSOR_SIZENWSE;
    case ImGuiMouseCursor_Hand:         return SDL_SYSTEM_CURSOR_HAND;
    case ImGuiMouseCursor_NotAllowed:   return SDL_SYSTEM_CURSOR_NO;
    }
}

void ide::UpdateMouseCursor()
{
    const ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

    // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        pInput->ShowCursor(false);
    }
    else
    {
        pInput->SetCursor(get_sdl_cursor(imgui_cursor));
        pInput->ShowCursor(true);
    }
}

void ide::UpdateTextInput(bool force_disable /*= false*/)
{
    if (force_disable)
    {
        if (m_imgui_backend.text_input_enabled)
        {
            pInput->DisableTextInput();
            m_imgui_backend.text_input_enabled = false;
        }
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();

    if (m_imgui_backend.text_input_enabled != io.WantTextInput)
    {
        m_imgui_backend.text_input_enabled = io.WantTextInput;

        if (m_imgui_backend.text_input_enabled)
            pInput->EnableTextInput();
        else
            pInput->DisableTextInput();
    }
}

void ide::OnAppActivate()
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
}

void ide::OnAppDeactivate()
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
}

void ide::IR_OnActivate()
{
    pInput->GrabInput(false);

#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif
}

void ide::IR_OnDeactivate()
{
    UpdateTextInput(true);
    pInput->GrabInput(true);

#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0");
#endif
}

void ide::IR_OnMousePress(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    const int imkey = key - (MOUSE_INVALID + 1);
    io.AddMouseButtonEvent(imkey, true);
}

void ide::IR_OnMouseRelease(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    const int imkey = key - (MOUSE_INVALID + 1);
    io.AddMouseButtonEvent(imkey, false);
}

void ide::IR_OnMouseHold(int /*key*/)
{
    // ImGui handles hold state on its own
}

void ide::IR_OnMouseWheel(float x, float y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(x, y);
}

void ide::IR_OnMouseMove(int /*x*/, int /*y*/)
{
    // x and y are relative to previous mouse position
    // ImGui accepts absolute coordinates (that are relative to window or monitor)
    Ivector2 p;
    pInput->iGetAsyncMousePos(p, ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(p.x), static_cast<float>(p.y));
}

void ide::IR_OnKeyboardPress(int key)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (GetBindedAction(key))
    {
    case kEDITOR:
        SwitchToNextState();
        return;

    case kCONSOLE:
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        {
            if (Console->bVisible)
                Console->Hide();
            else
                Console->Show();
            return;
        }
        break;

    case kSCORES:
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        {
            psDeviceFlags.set(rsStatistic, !psDeviceFlags.test(rsStatistic));
            return;
        }
        break;

    case kQUIT:
        if (io.WantTextInput)
            break; // bypass to ImGui

        // First
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        {
            ImGui::SetWindowFocus(nullptr);
            return;
        }

        // Second
        SetState(visible_state::hidden);
        return;
    }

    switch (key)
    {
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        break;

    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        io.AddKeyEvent(ImGuiMod_Shift, true);
        break;

    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
        io.AddKeyEvent(ImGuiMod_Alt, true);
        break;

    case SDL_SCANCODE_LGUI:
    case SDL_SCANCODE_RGUI:
        io.AddKeyEvent(ImGuiMod_Super, true);
        break;
    } // switch (key)

    const auto imkey = xr_key_to_imgui_key(key);
    if (imkey == ImGuiKey_None)
        return;
    io.AddKeyEvent(imkey, true);
}

void ide::IR_OnKeyboardRelease(int key)
{
    ImGuiIO& io = ImGui::GetIO();

    const auto check = [&, this](ImGuiKey mod, int xr_key)
    {
        if (!IR_GetKeyState(xr_key))
            io.AddKeyEvent(mod, false);
    };

    switch (key)
    {
    case SDL_SCANCODE_LCTRL:  check(ImGuiMod_Ctrl,  SDL_SCANCODE_RCTRL ); break;
    case SDL_SCANCODE_RCTRL:  check(ImGuiMod_Ctrl,  SDL_SCANCODE_LCTRL ); break;
    case SDL_SCANCODE_LSHIFT: check(ImGuiMod_Shift, SDL_SCANCODE_RSHIFT); break;
    case SDL_SCANCODE_RSHIFT: check(ImGuiMod_Shift, SDL_SCANCODE_LSHIFT); break;
    case SDL_SCANCODE_LALT:   check(ImGuiMod_Alt,   SDL_SCANCODE_RALT  ); break;
    case SDL_SCANCODE_RALT:   check(ImGuiMod_Alt,   SDL_SCANCODE_LALT  ); break;
    case SDL_SCANCODE_LGUI:   check(ImGuiMod_Super, SDL_SCANCODE_RGUI  ); break;
    case SDL_SCANCODE_RGUI:   check(ImGuiMod_Super, SDL_SCANCODE_LGUI  ); break;
    } // switch (key)

    const auto imkey = xr_key_to_imgui_key(key);
    if (imkey == ImGuiKey_None)
        return;
    io.AddKeyEvent(imkey, false);
}

void ide::IR_OnKeyboardHold(int /*key*/)
{
    // ImGui handles hold state on its own
}

void ide::IR_OnTextInput(pcstr text)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
        io.AddInputCharactersUTF8(text);
}

void ide::IR_OnControllerPress(int key, float x, float y)
{
    ImGuiIO& io = ImGui::GetIO();

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        io.AddKeyEvent(xr_key_to_imgui_key(key), true);
        return;
    }

    switch (key)
    {
        case XR_CONTROLLER_AXIS_LEFT:
            break;
        case XR_CONTROLLER_AXIS_RIGHT:
            break;
        case XR_CONTROLLER_AXIS_TRIGGER_LEFT:
            io.AddKeyAnalogEvent(ImGuiKey_GamepadL2, true, x);
            break;
        case XR_CONTROLLER_AXIS_TRIGGER_RIGHT:
            io.AddKeyAnalogEvent(ImGuiKey_GamepadR2, true, x);
            break;
    }

/*
#define IM_SATURATE(V) (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)

#define MAP_ANALOG(KEY_NO, AXIS_NO, V0, V1)                                                              \
    do                                                                                                   \
    {                                                                                                    \
        float vn = (float)(SDL_GameControllerGetAxis(nullptr, AXIS_NO) - V0) / (float)(V1 - V0); \
        vn = IM_SATURATE(vn);                                                                            \
        io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn);                                                     \
    } while (false)

    const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.

    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  SDL_CONTROLLER_AXIS_LEFTX,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, SDL_CONTROLLER_AXIS_LEFTX,  +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,    SDL_CONTROLLER_AXIS_LEFTY,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,  SDL_CONTROLLER_AXIS_LEFTY,  +thumb_dead_zone, +32767);

    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  SDL_CONTROLLER_AXIS_RIGHTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, SDL_CONTROLLER_AXIS_RIGHTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,    SDL_CONTROLLER_AXIS_RIGHTY, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,  SDL_CONTROLLER_AXIS_RIGHTY, +thumb_dead_zone, +32767);
#undef MAP_ANALOG
#undef IM_SATURATE*/
}

void ide::IR_OnControllerRelease(int key, float x, float y)
{
    ImGuiIO& io = ImGui::GetIO();

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        io.AddKeyEvent(xr_key_to_imgui_key(key), false);
        return;
    }

    switch (key)
    {
    case XR_CONTROLLER_AXIS_LEFT:
        break;
    case XR_CONTROLLER_AXIS_RIGHT:
        break;
    case XR_CONTROLLER_AXIS_TRIGGER_LEFT:
        io.AddKeyAnalogEvent(ImGuiKey_GamepadL2, false, x);
        break;
    case XR_CONTROLLER_AXIS_TRIGGER_RIGHT:
        io.AddKeyAnalogEvent(ImGuiKey_GamepadR2, false, x);
        break;
    }
}

void ide::IR_OnControllerHold(int /*key*/, float /*x*/, float /*y*/)
{
    // ImGui handles hold state on its own
}

void ide::IR_OnControllerAttitudeChange(Fvector /*change*/)
{
    // XXX: use somewhere?
}
} // namespace xray::editor
