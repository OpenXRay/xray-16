#include "pch.hpp"

#include "ui_debug.h"
#include "ui_base.h"

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
    ImGui::SetCurrentContext(Device.editor().GetImGuiContext());
}

void CUIDebugger::OnFrame()
{
#ifndef MASTER_GOLD
    if (!get_open_state())
        return;

    if (ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        if (ImGui::BeginMenuBar())
        {
            ImGui::Checkbox("Draw rects", &m_state.drawWndRects);

            ImGui::BeginDisabled(!m_state.drawWndRects);
            ImGui::Checkbox("Coloured rects", &m_state.coloredRects);
            ImGui::EndDisabled();

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
