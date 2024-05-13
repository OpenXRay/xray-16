#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"

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

    // Clipboard functionality
    io.SetClipboardTextFn = [](void*, const char* text)
    {
        SDL_SetClipboardText(text);
    };
    io.GetClipboardTextFn = [](void* user_data) -> const char*
    {
        auto& bd = *static_cast<ImGuiBackend*>(user_data);

        if (bd.clipboard_text_data)
            SDL_free(bd.clipboard_text_data);

        bd.clipboard_text_data = SDL_GetClipboardText();

        return bd.clipboard_text_data;
    };
    io.ClipboardUserData = &m_imgui_backend;

    auto& bd = m_imgui_backend;

    bd.mouse_cursors[ImGuiMouseCursor_Arrow]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    bd.mouse_cursors[ImGuiMouseCursor_TextInput]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    bd.mouse_cursors[ImGuiMouseCursor_ResizeAll]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    bd.mouse_cursors[ImGuiMouseCursor_ResizeNS]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    bd.mouse_cursors[ImGuiMouseCursor_ResizeEW]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    bd.mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    bd.mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    bd.mouse_cursors[ImGuiMouseCursor_Hand]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    bd.mouse_cursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
}

void ide::ShutdownBackend()
{
    auto& backend = m_imgui_backend;

    if (backend.clipboard_text_data)
    {
        SDL_free(backend.clipboard_text_data);
        backend.clipboard_text_data = nullptr;
    }

    for (auto& cursor : backend.mouse_cursors)
    {
        SDL_DestroyCursor(cursor);
        cursor = nullptr;
    }
    backend.last_cursor = nullptr;
}

void ide::ProcessEvent(const SDL_Event& event)
{
    if (m_state != visible_state::full)
        return;

    auto& bd = m_imgui_backend;
    ImGuiViewport* viewport = nullptr;
    SDL_Window* window = nullptr;
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
    {
        window = SDL_GetWindowFromID(event.window.windowID);
        if (!window)
            break;
        viewport = ImGui::FindViewportByPlatformHandle(window);
        if (!viewport)
            break;
        bd.mouse_window_id = event.window.windowID;
        bd.mouse_last_leave_frame = 0;
        break;
    }
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    {
        window = SDL_GetWindowFromID(event.window.windowID);
        if (!window)
            break;
        viewport = ImGui::FindViewportByPlatformHandle(window);
        if (!viewport)
            break;
        bd.mouse_last_leave_frame = ImGui::GetFrameCount() + 1;
        break;
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

    // We forward mouse input when hovered or captured (via SDL_EVENT_MOUSE_MOTION) or when focused (below)
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

void ide::UpdateMouseCursor()
{
    const ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    auto& bd = m_imgui_backend;
    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_HideCursor();
    }
    else
    {
        // Show OS mouse cursor
        SDL_Cursor* expected_cursor = bd.mouse_cursors[imgui_cursor] ? bd.mouse_cursors[imgui_cursor] : bd.mouse_cursors[ImGuiMouseCursor_Arrow];
        if (bd.last_cursor != expected_cursor)
        {
            SDL_SetCursor(expected_cursor); // SDL function doesn't have an early out (see #6113)
            bd.last_cursor = expected_cursor;
        }
        SDL_ShowCursor();
    }
}

void ide::UpdateTextInput(bool force_disable /*= false*/)
{
    if (force_disable)
    {
        if (m_text_input_enabled)
        {
            pInput->DisableTextInput();
            m_text_input_enabled = false;
        }
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();

    if (m_text_input_enabled != io.WantTextInput)
    {
        m_text_input_enabled = io.WantTextInput;

        if (m_text_input_enabled)
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

void ide::IR_OnMouseWheel(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(x), static_cast<float>(y));
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
        float vn = (float)(SDL_GetGamepadAxis(nullptr, AXIS_NO) - V0) / (float)(V1 - V0); \
        vn = IM_SATURATE(vn);                                                                            \
        io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn);                                                     \
    } while (false)

    const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.

    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  SDL_GAMEPAD_AXIS_LEFTX,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, SDL_GAMEPAD_AXIS_LEFTX,  +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,    SDL_GAMEPAD_AXIS_LEFTY,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,  SDL_GAMEPAD_AXIS_LEFTY,  +thumb_dead_zone, +32767);

    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  SDL_GAMEPAD_AXIS_RIGHTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, SDL_GAMEPAD_AXIS_RIGHTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,    SDL_GAMEPAD_AXIS_RIGHTY, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,  SDL_GAMEPAD_AXIS_RIGHTY, +thumb_dead_zone, +32767);
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
