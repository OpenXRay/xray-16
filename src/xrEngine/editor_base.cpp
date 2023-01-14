#include "stdafx.h"

#include "editor_base.h"

#include <imgui.h>

namespace xray::editor
{
ide::ide() : m_context(ImGui::CreateContext())
{
    
}

ide::~ide()
{
    ImGui::DestroyContext(m_context);
}

void ide::UpdateWindowProps()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { static_cast<float>(psDeviceMode.Width), static_cast<float>(psDeviceMode.Height) };
}

void ide::UpdateInputAsync()
{
    ImGuiIO& io     = ImGui::GetIO();

    io.DeltaTime    = Device.fTimeDelta;

    if (is_shown())
        return; // Input is updated through IInputReceiver interface

    Ivector2 p;
    pInput->iGetAsyncMousePos(p);
    io.MousePos.x   = static_cast<float>(p.x);
    io.MousePos.y   = static_cast<float>(p.y);

    pInput->iGetAsyncScrollPos(p);
    io.MouseWheel  += static_cast<float>(p.y);
    io.MouseWheelH += static_cast<float>(p.x);

    io.MouseDown[0] = pInput->iGetAsyncKeyState(MOUSE_1);
    io.MouseDown[1] = pInput->iGetAsyncKeyState(MOUSE_2);
    io.MouseDown[2] = pInput->iGetAsyncKeyState(MOUSE_3);
    io.MouseDown[3] = pInput->iGetAsyncKeyState(MOUSE_4);
    io.MouseDown[4] = pInput->iGetAsyncKeyState(MOUSE_5);

    io.KeyCtrl      = pInput->iGetAsyncKeyState(SDL_SCANCODE_LCTRL)  || pInput->iGetAsyncKeyState(SDL_SCANCODE_RCTRL);
    io.KeyShift     = pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT) || pInput->iGetAsyncKeyState(SDL_SCANCODE_RSHIFT);
    io.KeyAlt       = pInput->iGetAsyncKeyState(SDL_SCANCODE_LALT)   || pInput->iGetAsyncKeyState(SDL_SCANCODE_RALT);
    io.KeySuper     = pInput->iGetAsyncKeyState(SDL_SCANCODE_LGUI)   || pInput->iGetAsyncKeyState(SDL_SCANCODE_RGUI);
}

void ide::OnFrame()
{
    UpdateInputAsync();

    ImGui::NewFrame();
    if (is_shown())
    {
        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
    }
    ImGui::EndFrame();
}

void ide::OnRender()
{
    ImGui::Render();
}

void ide::IR_Capture()
{
    m_shown = true;
    IInputReceiver::IR_Capture();
}

void ide::IR_Release()
{
    m_shown = false;
    IInputReceiver::IR_Release();
}

void ide::IR_OnMousePress(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (key)
    {
    case MOUSE_1: io.MouseDown[0] = true; break;
    case MOUSE_2: io.MouseDown[1] = true; break;
    case MOUSE_3: io.MouseDown[2] = true; break;
    case MOUSE_4: io.MouseDown[3] = true; break;
    case MOUSE_5: io.MouseDown[4] = true; break;
    }
}

void ide::IR_OnMouseRelease(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (key)
    {
    case MOUSE_1: io.MouseDown[0] = false; break;
    case MOUSE_2: io.MouseDown[1] = false; break;
    case MOUSE_3: io.MouseDown[2] = false; break;
    case MOUSE_4: io.MouseDown[3] = false; break;
    case MOUSE_5: io.MouseDown[4] = false; break;
    }
}

void ide::IR_OnMouseHold(int /*key*/)
{
    // ImGui handle hold state on its own
}

void ide::IR_OnMouseWheel(int x, int y)
{
    ImGuiIO& io     = ImGui::GetIO();
    io.MouseWheel  += static_cast<float>(y);
    io.MouseWheelH += static_cast<float>(x);
}

void ide::IR_OnMouseMove(int /*x*/, int /*y*/)
{
    // x and y are relative
    // ImGui accepts absolute coordinates
    Ivector2 p;
    pInput->iGetAsyncMousePos(p);

    ImGuiIO& io   = ImGui::GetIO();
    io.MousePos.x = static_cast<float>(p.x);
    io.MousePos.y = static_cast<float>(p.y);
}

void ide::IR_OnKeyboardPress(int key)
{
    ImGuiIO& io     = ImGui::GetIO();
    switch (key)
    {
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        io.KeyCtrl  = true;
        break;

    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        io.KeyShift = true;
        break;

    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
        io.KeyAlt   = true;
        break;

    case SDL_SCANCODE_LGUI:
    case SDL_SCANCODE_RGUI:
        io.KeySuper = true;
        break;

    default:
        VERIFY2(std::size(io.KeysDown) == SDL_NUM_SCANCODES,
            "Either SDL2 or ImGui has changed the total number of keys.\n"
            "Please, update the ide input handling code."
        );
        if (static_cast<size_t>(key) < std::size(io.KeysDown))
            io.KeysDown[key] = true;
    }
}

void ide::IR_OnKeyboardRelease(int key)
{
    ImGuiIO& io = ImGui::GetIO();

    const auto check_opposite_key = [](int opposite_key, bool& state)
    {
        state = !pInput->iGetAsyncKeyState(opposite_key);
    };

    switch (key)
    {
    case SDL_SCANCODE_LCTRL:  check_opposite_key(SDL_SCANCODE_RCTRL,  io.KeyCtrl);  break;
    case SDL_SCANCODE_RCTRL:  check_opposite_key(SDL_SCANCODE_LCTRL,  io.KeyCtrl);  break;
    case SDL_SCANCODE_LSHIFT: check_opposite_key(SDL_SCANCODE_RSHIFT, io.KeyShift); break;
    case SDL_SCANCODE_RSHIFT: check_opposite_key(SDL_SCANCODE_LSHIFT, io.KeyShift); break;
    case SDL_SCANCODE_LALT:   check_opposite_key(SDL_SCANCODE_RALT,   io.KeyAlt);   break;
    case SDL_SCANCODE_RALT:   check_opposite_key(SDL_SCANCODE_LALT,   io.KeyAlt);   break;
    case SDL_SCANCODE_LGUI:   check_opposite_key(SDL_SCANCODE_RGUI,   io.KeySuper); break;
    case SDL_SCANCODE_RGUI:   check_opposite_key(SDL_SCANCODE_LGUI,   io.KeySuper); break;

    default:
        VERIFY2(std::size(io.KeysDown) == SDL_NUM_SCANCODES,
            "Either SDL2 or ImGui has changed the total number of keys.\n"
            "Please, update the ide input handling code."
        );
        if (static_cast<size_t>(key) < std::size(io.KeysDown))
            io.KeysDown[key] = false;
    }
}

void ide::IR_OnKeyboardHold(int /*key*/)
{
    // ImGui handle hold state on its own
}

void ide::IR_OnTextInput(pcstr text)
{

}

void ide::IR_OnControllerPress(int key, float x, float y)
{
    // XXX: implement
}

void ide::IR_OnControllerRelease(int key, float x, float y)
{
    // XXX: implement
}

void ide::IR_OnControllerHold(int /*key*/, float /*x*/, float /*y*/)
{
    // ImGui handle hold state on its own
}

void ide::IR_OnControllerAttitudeChange(Fvector /*change*/)
{
    // XXX: use somewhere?
}
} // namespace xray::editor
