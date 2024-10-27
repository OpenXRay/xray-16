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

ide::ide() = default;

ide::~ide() = default;

void ide::OnAppStart()
{
    Device.seqFrame.Add(this, -5);
}

void ide::OnAppEnd()
{
    Device.seqFrame.Remove(this);
}

void ide::OnFrame()
{
    ZoneScoped;

    switch (m_state)
    {
    case visible_state::full:
        UpdateMouseData();
        UpdateMouseCursor();
        UpdateTextInput();
        ShowMain();
        [[fallthrough]];

    case visible_state::light:
        if (m_show_weather_editor)
            ShowWeatherEditor();
        for (const auto& tool : m_tools)
            tool->on_tool_frame();
        break;
    }

    const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
    const bool double_click = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    if (double_click && !focused)
    {
        SwitchToNextState();
    }
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
