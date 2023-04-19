#include "stdafx.h"

#include "editor_base.h"

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

    InitBackend();
}

ide::~ide()
{
    ShutdownBackend();
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
    convert_path_separators(fName);
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
    if (m_state == visible_state::full)
    {
        if (io.WantTextInput)
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }

    m_render->Frame();
    ImGui::NewFrame();

    switch (m_state)
    {
    case visible_state::full:
        ShowMain();
        [[fallthrough]];

    case visible_state::light:
        if (m_windows.weather)
            ShowWeatherEditor();
        break;
    }

    const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
    const bool double_click = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    if (double_click && !focused)
    {
        SwitchToNextState();
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
#ifndef MASTER_GOLD
            ImGui::MenuItem("Weather Editor", nullptr, &m_windows.weather);
#endif

            if (ImGui::MenuItem("Stats", nullptr, psDeviceFlags.test(rsStatistic)))
                psDeviceFlags.set(rsStatistic, !psDeviceFlags.test(rsStatistic));

            if (ImGui::MenuItem("Close"))
                IR_Release();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About"))
        {
#ifndef MASTER_GOLD
            ImGui::MenuItem("ImGui demo", nullptr, &m_windows.imgui_demo);
            ImGui::MenuItem("ImGui metrics", nullptr, &m_windows.imgui_metrics);
#endif
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (m_windows.imgui_demo)
        ImGui::ShowDemoWindow(&m_windows.imgui_demo);

    if (m_windows.imgui_metrics)
        ImGui::ShowMetricsWindow(&m_windows.imgui_metrics);
}

ImGuiWindowFlags ide::get_default_window_flags() const
{
    if (m_state == visible_state::full)
        return ImGuiWindowFlags_MenuBar;
    return ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoInputs
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoBackground;
}

bool ide::is_shown() const
{
    return m_windows.weather;
}

void ide::SetState(visible_state state)
{
    if (m_state == state)
        return;
    m_state = state;

    switch (m_state)
    {
    case visible_state::hidden:
    case visible_state::light:
        IR_Release();
        break;

    case visible_state::full:
        IR_Capture();
        break;

    default: NODEFAULT;
    }
}

void ide::SwitchToNextState()
{
    switch (m_state)
    {
    case visible_state::hidden:
        SetState(visible_state::full);
        break;

    case visible_state::full:
        if (is_shown())
            SetState(visible_state::light);
        else
            SetState(visible_state::hidden);
        break;

    case visible_state::light:
        SetState(visible_state::hidden);
        break;

    default: NODEFAULT;
    }
}
} // namespace xray::editor
