#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"

namespace xray::editor
{
ide_tool::ide_tool()
{
    Device.editor().RegisterTool(this);
}

ide_tool::~ide_tool()
{
    Device.editor().UnregisterTool(this);
}

ImGuiWindowFlags ide_tool::get_default_window_flags() const
{
    return Device.editor().get_default_window_flags();
}

void ide::RegisterTool(ide_tool* tool)
{
    m_tools.emplace_back(tool);
}

void ide::UnregisterTool(const ide_tool* tool)
{
    const auto it = std::find(m_tools.begin(), m_tools.end(), tool);
    if (it != m_tools.end())
        m_tools.erase(it);
}

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

void ide::OnFrame()
{
    const float frametime = m_timer.GetElapsed_sec();
    m_timer.Start();

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = frametime;

    m_render->Frame();
    ImGui::NewFrame();

    switch (m_state)
    {
    case visible_state::full:
        UpdateTextInput();
        ShowMain();
        [[fallthrough]];

    case visible_state::light:
        if (m_show_weather_editor)
            ShowWeatherEditor();
        for (const auto& tool : m_tools)
            tool->OnFrame();
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
    static bool show_imgui_demo = false;
    static bool show_imgui_metrics = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (imgui::MenuItemWithShortcut("Stats", kSCORES,
                "Show engine statistics.\n"
                "Key shortcut will only work when no window is in focus",
                psDeviceFlags.test(rsStatistic)))
            {
                psDeviceFlags.set(rsStatistic, !psDeviceFlags.test(rsStatistic));
            }
            if (imgui::MenuItemWithShortcut("Hide", kEDITOR, "Hide main ImGui windows and this menu bar, but leave tools visible (a.k.a. light mode)"))
            {
                SetState(visible_state::light);
            }
            if (imgui::MenuItemWithShortcut("Close", kQUIT, "Close editor and all windows"))
            {
                SetState(visible_state::hidden);
            }
            ImGui::EndMenu();
        }
#ifndef MASTER_GOLD
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Weather Editor", nullptr, &m_show_weather_editor);
            for (const auto& tool : m_tools)
            {
                ImGui::MenuItem(tool->tool_name(), nullptr, &tool->get_open_state());
            }
            ImGui::EndMenu();
        }
#endif
        if (ImGui::BeginMenu("About"))
        {
#ifndef MASTER_GOLD
#   ifdef DEBUG
            ImGui::MenuItem("ImGui demo", nullptr, &show_imgui_demo);
#   endif
            ImGui::MenuItem("ImGui metrics", nullptr, &show_imgui_metrics);
#endif
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (show_imgui_demo)
        ImGui::ShowDemoWindow(&show_imgui_demo);

    if (show_imgui_metrics)
        ImGui::ShowMetricsWindow(&show_imgui_metrics);
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
    for (const auto& tool : m_tools)
    {
        if (tool->get_open_state())
            return true;
    }
    return m_show_weather_editor;
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
