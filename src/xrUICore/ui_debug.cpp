#include "pch.hpp"

#include "ui_debug.h"
#include "ui_base.h"
#include "xrEngine/editor_helper.h"

CUIDebuggable::~CUIDebuggable()
{
    if (GEnv.UI)
    {
        if (UI().Debugger().GetSelected() == this)
            UI().Debugger().SetSelected(nullptr);
    }
}

void CUIDebuggable::RegisterDebuggable()
{
    UI().Debugger().Register(this);
}

void CUIDebuggable::UnregisterDebuggable()
{
    UI().Debugger().Unregister(this);
}

void CUIDebugger::Register(CUIDebuggable* debuggable)
{
#ifndef MASTER_GOLD
    m_root_windows.emplace_back(debuggable);
#endif
}

void CUIDebugger::Unregister(CUIDebuggable* debuggable)
{
#ifndef MASTER_GOLD
    const auto it = std::find(m_root_windows.begin(), m_root_windows.end(), debuggable);
    if (it != m_root_windows.end())
        m_root_windows.erase(it);
#endif
}

void CUIDebugger::SetSelected(CUIDebuggable* debuggable)
{
    m_state.selected = debuggable;
    m_state.newSelected = debuggable;
}

CUIDebugger::CUIDebugger()
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
    ImGui::SetCurrentContext(Device.GetImGuiContext());
    reset_settings();
}

void CUIDebugger::on_tool_frame()
{
#ifndef MASTER_GOLD
    if (!get_open_state())
        return;

    using namespace xray;

    if (ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        if (ImGui::BeginMenuBar())
        {
            ImGui::Checkbox("Draw rects", &m_state.settings.drawWndRects);

            if (ImGui::BeginMenu("Options"))
            {
                ImGui::Checkbox("Randomly coloured rects", &m_state.settings.coloredRects);

                ImGui::Text("");
                ImGui::Text("Rect colouring:");

                auto& colors = m_state.settings.colors;

                imgui::ColorEdit4("Normal", colors.normal);
                imgui::ItemHelp("Just a normal window");
                imgui::ColorEdit4("Normal hovered", colors.normalHovered);
                imgui::ItemHelp("Just a normal window hovered by in-game cursor");
                imgui::ColorEdit4("Examined", colors.examined);
                imgui::ItemHelp("A window that is hovered here, in the tree of the UI Debugger");
                imgui::ColorEdit4("Focused", colors.focused);
                imgui::ItemHelp("Window currectly selected in the focus system");

                ImGui::BeginDisabled(m_state.settings.coloredRects);
                imgui::ColorEdit4("Valuable focusable", colors.focusableValuable);
                imgui::ItemHelp("Window that is currently valuable in the focus system");
                imgui::ColorEdit4("Valuable focusable hovered", colors.focusableValuableHovered);
                imgui::ItemHelp("Valuable window hovered by in-game cursor");
                imgui::ColorEdit4("Non valuable focusable", colors.focusableNonValuable);
                imgui::ItemHelp("Window that is currently non-valuable in the focus system");
                imgui::ColorEdit4("Non valuable focusable hovered", colors.focusableNonValuableHovered);
                imgui::ItemHelp("Non-valuable window hovered by in-game cursor");
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("UI tree and properties", 2, flags))
        {
            ImGui::TableSetupColumn("Tree");
            ImGui::TableSetupColumn("Selected element properties", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            for (const auto& window : m_root_windows)
            {
                window->FillDebugTree(m_state);
                if (m_state.selected != m_state.newSelected)
                    m_state.selected = m_state.newSelected;
            }
            ImGui::TableNextColumn();
            if (m_state.selected)
                m_state.selected->FillDebugInfo();

            ImGui::EndTable();
        }
    }
    ImGui::End();
#endif
}

void CUIDebugger::reset_settings()
{
    CUIDebuggerSettings settings
    {
        {
            /*.normal                      =*/ color_rgba(0,   0,   255, 200),
            /*.normalHovered               =*/ color_rgba(0,   0,   255, 255),
            /*.examined                    =*/ color_rgba(0,   255, 255, 255),
            /*.focused                     =*/ color_rgba(255, 215, 0,   255),
            /*.focusableValuable           =*/ color_rgba(0,   255, 0,   200),
            /*.focusableValuableHovered    =*/ color_rgba(0,   255, 0,   255),
            /*.focusableNonValuable        =*/ color_rgba(255, 0,   0,   200),
            /*.focusableNonValuableHovered =*/ color_rgba(255, 0,   0,   255),
        },
        /*.drawWndRects =*/ true,
        /*.coloredRects =*/ false,
    };
    m_state.settings = settings;
}

void CUIDebugger::apply_setting(pcstr line)
{
    auto& settings = m_state.settings;

    int i{};
    u32 color{};

    if (sscanf(line, "ColoredRects=%d", &i) == 1)
        settings.coloredRects = i != 0;
    else if (sscanf(line, "NormalColor=0x%X", &color) == 1)
        settings.colors.normal = color;
    else if (sscanf(line, "NormalHoveredColor=0x%X", &color) == 1)
        settings.colors.normalHovered = color;
    else if (sscanf(line, "ExaminedColor=0x%X", &color) == 1)
        settings.colors.examined = color;
    else if (sscanf(line, "FocusedColor=0x%X", &color) == 1)
        settings.colors.focused = color;
    else if (sscanf(line, "FocusableValuableColor=0x%X", &color) == 1)
        settings.colors.focusableValuable = color;
    else if (sscanf(line, "FocusableValuableHoveredColor=0x%X", &color) == 1)
        settings.colors.focusableValuableHovered = color;
    else if (sscanf(line, "FocusableNonValuableColor=0x%X", &color) == 1)
        settings.colors.focusableNonValuable = color;
    else if (sscanf(line, "FocusableNonValuableHoveredColor=0x%X", &color) == 1)
        settings.colors.focusableNonValuableHovered = color;
}

void CUIDebugger::save_settings(ImGuiTextBuffer* buffer) const
{
    R_ASSERT1_CURE(buffer, return);

    const auto& settings = m_state.settings;
    const auto& colors = settings.colors;

    buffer->appendf("ColoredRects=%d\n", settings.coloredRects);
    buffer->appendf("NormalColor=0x%X\n", colors.normal);
    buffer->appendf("NormalHoveredColor=0x%X\n", colors.normalHovered);
    buffer->appendf("ExaminedColor=0x%X\n", colors.examined);
    buffer->appendf("FocusedColor=0x%X\n", colors.focused);
    buffer->appendf("FocusableValuableColor=0x%X\n", colors.focusableValuable);
    buffer->appendf("FocusableValuableHoveredColor=0x%X\n", colors.focusableValuableHovered);
    buffer->appendf("FocusableNonValuableColor=0x%X\n", colors.focusableNonValuable);
    buffer->appendf("FocusableNonValuableHoveredColor=0x%X\n", colors.focusableNonValuableHovered);
}

size_t CUIDebugger::estimate_settings_size() const
{
    // Additional space for '\n' will be also reserved
    // because std::size also counts '\0'

    constexpr size_t NUMBER_SIZE    = 1; // number serving as bool
    constexpr size_t HEXNUMBER_SIZE = 8; // 8 for the 32-bit value in hexadecimal format

    // Count bytes for each line
    size_t size = 0;

    // "ColoredRects=%d\n"
    size += std::size("ColoredRects=") + NUMBER_SIZE;

    // "NormalColor=0x%X\n"
    size += std::size("NormalColor=0x") + HEXNUMBER_SIZE;

    // "NormalHoveredColor=0x%X\n"
    size += std::size("NormalHoveredColor=0x") + HEXNUMBER_SIZE;

    // "ExaminedColor=0x%X\n"
    size += std::size("ExaminedColor=0x") + HEXNUMBER_SIZE;

    // "FocusedColor=0x%X\n"
    size += std::size("FocusedColor=0x") + HEXNUMBER_SIZE;

    // "FocusableValuableColor=0x%X\n"
    size += std::size("FocusableValuableColor=0x") + HEXNUMBER_SIZE;

    // "FocusableValuableHoveredColor=0x%X\n"
    size += std::size("FocusableValuableHoveredColor=0x") + HEXNUMBER_SIZE;

    // "FocusableNonValuableColor=0x%X\n"
    size += std::size("FocusableNonValuableColor=0x") + HEXNUMBER_SIZE;

    // "FocusableNonValuableHoveredColor=0x%X\n"
    size += std::size("FocusableNonValuableHoveredColor=0x") + HEXNUMBER_SIZE;

    return size;
}
