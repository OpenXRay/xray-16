#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"

#include <imgui.h>

namespace xray::editor
{
void ide::UpdateInputAsync()
{
    ImGuiIO& io = ImGui::GetIO();

    Ivector2 p;
    pInput->iGetAsyncMousePos(p);
    io.AddMousePosEvent(static_cast<float>(p.x), static_cast<float>(p.y));

    pInput->iGetAsyncScrollPos(p);
    io.AddMouseWheelEvent(static_cast<float>(p.x), static_cast<float>(p.y));

    io.AddMouseButtonEvent(0, IR_GetKeyState(MOUSE_1));
    io.AddMouseButtonEvent(1, IR_GetKeyState(MOUSE_2));
    io.AddMouseButtonEvent(2, IR_GetKeyState(MOUSE_3));
    io.AddMouseButtonEvent(3, IR_GetKeyState(MOUSE_4));
    io.AddMouseButtonEvent(4, IR_GetKeyState(MOUSE_5));

    io.AddKeyEvent(ImGuiMod_Ctrl,  IR_GetKeyState(SDL_SCANCODE_LCTRL)  || IR_GetKeyState(SDL_SCANCODE_RCTRL));
    io.AddKeyEvent(ImGuiMod_Shift, IR_GetKeyState(SDL_SCANCODE_LSHIFT) || IR_GetKeyState(SDL_SCANCODE_RSHIFT));
    io.AddKeyEvent(ImGuiMod_Alt,   IR_GetKeyState(SDL_SCANCODE_LALT)   || IR_GetKeyState(SDL_SCANCODE_RALT));
    io.AddKeyEvent(ImGuiMod_Super, IR_GetKeyState(SDL_SCANCODE_LGUI)   || IR_GetKeyState(SDL_SCANCODE_RGUI));

    for (int i = SDL_SCANCODE_UNKNOWN; i < XR_CONTROLLER_BUTTON_MAX; i++)
    {
        const auto imkey = xr_key_to_imgui_key(i);
        if (imkey == ImGuiKey_None)
            continue;
        io.AddKeyEvent(imkey, IR_GetKeyState(i));
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

void ide::IR_Capture()
{
    m_shown = true;
    IInputReceiver::IR_Capture();
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = true;
}

void ide::IR_Release()
{
    SDL_StopTextInput();
    m_shown = false;
    IInputReceiver::IR_Release();
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = false;
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
    // x and y are relative
    // ImGui accepts absolute coordinates
    Ivector2 p;
    pInput->iGetAsyncMousePos(p);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(p.x), static_cast<float>(p.y));
}

void ide::IR_OnKeyboardPress(int key)
{
    ImGuiIO& io = ImGui::GetIO();

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
