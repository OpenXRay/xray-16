#include "stdafx.h"

#include "editor_base.h"

#include <imgui.h>

namespace xray::editor
{
ide::ide()
{
    ImGui::SetAllocatorFunctions(
        [](size_t size, void* /*user_data*/)
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* /*user_data*/)
        {
            xr_free(ptr);
        }
    );
    m_context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.BackendPlatformName = "imgui_impl_xray";
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

void ide::OnDeviceCreate()
{
    m_render = GEnv.RenderFactory->CreateImGuiRender();
    m_render->OnDeviceCreate(m_context);
}

void ide::OnDeviceDestroy()
{
    m_render->OnDeviceDestroy();
    GEnv.RenderFactory->DestroyImGuiRender(m_render);
    m_render = nullptr;
}

void ide::OnDeviceResetBegin() const
{
    m_render->OnDeviceResetBegin();
}

void ide::OnDeviceResetEnd() const
{
    m_render->OnDeviceResetEnd();
}

void ide::OnAppStart()
{
    ImGuiIO& io = ImGui::GetIO();

    string_path fName;
    FS.update_path(fName, "$app_data_root$", io.IniFilename);
    io.IniFilename = xr_strdup(fName);

    FS.update_path(fName, "$logs$", io.LogFilename);
    io.LogFilename = xr_strdup(fName);

    Device.seqFrame.Add(this, -5);
    Device.seqRender.Add(this, -5);
}

void ide::OnAppEnd()
{
    ImGuiIO& io = ImGui::GetIO();
    xr_free(io.IniFilename);
    xr_free(io.LogFilename);

    Device.seqFrame.Remove(this);
    Device.seqRender.Remove(this);
}

void ide::OnFrame()
{
    const float frametime = m_timer.GetElapsed_sec();
    m_timer.Start();

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = frametime;

    // When shown, input being is updated
    // through IInputReceiver interface
    if (!is_shown())
        UpdateInputAsync();
    else
    {
        if (io.WantTextInput)
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }

    m_render->Frame();
    ImGui::NewFrame();
    if (is_shown())
    {
        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
        ShowMain();
    }
    ImGui::EndFrame();
}

void ide::OnRender()
{
    ImGui::Render();
    m_render->Render(ImGui::GetDrawData());
}

void ide::ShowMain()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Stats", nullptr, psDeviceFlags.test(rsStatistic)))
                psDeviceFlags.set(rsStatistic, !psDeviceFlags.test(rsStatistic));

            if (ImGui::MenuItem("Close"))
            {
                IR_Release();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
} // namespace xray::editor
