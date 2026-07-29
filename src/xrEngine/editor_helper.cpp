#include "stdafx.h"

#include "editor_helper.h"

#include "imgui_internal.h"

namespace xray::imgui
{
bool CascadingCollapsingHeader(cpcstr label, ImGuiTreeNodeFlags flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiID seed = 0;
    if (window->IDStack.size() > 1)
        seed = *(window->IDStack.end() - 2);
    else
        seed = window->GetID("CascadingCollapsingHeader");

    ImGuiID id = ImHashStr(label, 0, seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *window->Ctx;
    if (g.DebugHookIdInfoId == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_String, label, nullptr);
#endif
    ImGui::SetNextItemStorageID(id);

    return ImGui::CollapsingHeader(label, flags);
}
} // namespace xray::imgui
