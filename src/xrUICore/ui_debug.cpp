#include "pch.hpp"

#include "ui_debug.h"
#include "ui_base.h"

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
            ImGui::Checkbox("Draw rects", &m_draw_wnd_rects);
            ImGui::EndMenuBar();
        }

        if (ImGui::TreeNode("Windows"))
        {
            for (const auto& window : m_root_windows)
                window->FillDebugInfo();
            ImGui::TreePop();
        }
    }
    ImGui::End();
#endif
}
