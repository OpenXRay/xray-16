#include "stdafx.h"

#include "editor_base.h"

#include <imgui.h>

namespace xray::editor
{
void ide::UpdateWindowProps()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { static_cast<float>(psDeviceMode.Width), static_cast<float>(psDeviceMode.Height) };
}

ide::operator bool()
{
    return false;
}

void ide::OnFrame()
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

    ImGui::NewFrame();



    ImGui::EndFrame();
}

void ide::OnRender()
{
    ImGui::Render();
}

void ide::IR_Capture()
{
    Device.seqFrame.Add(this, REG_PRIORITY_LOW - 10000);
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 10000);
    IInputReceiver::IR_Capture();
}

void ide::IR_Release()
{
    Device.seqFrame.Remove(this);
    Device.seqRender.Remove(this);
    IInputReceiver::IR_Release();
}
} // namespace xray::editor
