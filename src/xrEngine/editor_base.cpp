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

ide::operator bool() const
{
    return false;
}

void ide::UpdateWindowProps()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { static_cast<float>(psDeviceMode.Width), static_cast<float>(psDeviceMode.Height) };
}

void ide::UpdateInputAsync()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = Device.fTimeDelta;

    Ivector2 p;
    pInput->iGetAsyncMousePos(p);
    io.MousePos.x = static_cast<float>(p.x);
    io.MousePos.y = static_cast<float>(p.y);

    pInput->iGetAsyncScrollPos(p);
    io.MouseWheel += static_cast<float>(p.y);
    io.MouseWheelH += static_cast<float>(p.x);

    io.MouseDown[0] = pInput->iGetAsyncKeyState(MOUSE_1);
    io.MouseDown[1] = pInput->iGetAsyncKeyState(MOUSE_2);
    io.MouseDown[2] = pInput->iGetAsyncKeyState(MOUSE_3);
    io.MouseDown[3] = pInput->iGetAsyncKeyState(MOUSE_4);
    io.MouseDown[4] = pInput->iGetAsyncKeyState(MOUSE_5);
}

void ide::OnFrame()
{
    UpdateInputAsync();

    ImGui::NewFrame();

    //ImGui::ShowDemoWindow();
    //ImGui::ShowMetricsWindow();

    ImGui::EndFrame();
}

void ide::OnRender()
{
    ImGui::Render();
}

void ide::IR_OnMousePress(int btn)
{

}

void ide::IR_OnMouseRelease(int btn)
{

}

void ide::IR_OnMouseHold(int btn)
{

}

void ide::IR_OnMouseWheel(int x, int y)
{

}

void ide::IR_OnMouseMove(int x, int y)
{

}

void ide::IR_OnKeyboardPress(int btn)
{

}

void ide::IR_OnKeyboardRelease(int btn)
{

}

void ide::IR_OnKeyboardHold(int btn)
{

}

void ide::IR_OnTextInput(pcstr text)
{

}

void ide::IR_OnControllerPress(int btn, float x, float y)
{

}

void ide::IR_OnControllerRelease(int btn, float x, float y)
{

}

void ide::IR_OnControllerHold(int btn, float x, float y)
{

}

void ide::IR_OnControllerAttitudeChange(Fvector change)
{

}
} // namespace xray::editor
